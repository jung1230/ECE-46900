//
//	memory.c
//
//	Routines for dealing with memory management.

//static char rcsid[] = "$Id: memory.c,v 1.1 2000/09/20 01:50:19 elm Exp elm $";

#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "memory.h"
#include "queue.h"

// num_pages = size_of_memory / size_of_one_page
static uint32 freemap[L1_PT_SIZE / 32]; //256 pages, each entry holds 32 pages
static uint32 pagestart;
static int nfreepages;
static int freemapmax;

//----------------------------------------------------------------------
//
//	This silliness is required because the compiler believes that
//	it can invert a number by subtracting it from zero and subtracting
//	an additional 1.  This works unless you try to negate 0x80000000,
//	which causes an overflow when subtracted from 0.  Simply
//	trying to do an XOR with 0xffffffff results in the same code
//	being emitted.
//
//----------------------------------------------------------------------
static int negativeone = 0xFFFFFFFF;
static inline uint32 invert (uint32 n) {
  return (n ^ negativeone);
}

//----------------------------------------------------------------------
//
//	MemoryGetSize
//
//	Return the total size of memory in the simulator.  This is
//	available by reading a special location.
//
//----------------------------------------------------------------------
int MemoryGetSize() {
  return (*((int *)DLX_MEMSIZE_ADDRESS));
}


//----------------------------------------------------------------------
//
//	MemoryModuleInit
//
//	Initialize the memory module of the operating system.
//      Basically just need to setup the freemap for pages, and mark
//      the ones in use by the operating system as "VALID", and mark
//      all the rest as not in use.
//
//----------------------------------------------------------------------
void MemoryModuleInit() { //DOUBLE CHECKED
  
  int		i;
  //int		maxpage = MEM_MAX_SIZE / MEM_PAGESIZE;
  int		curpage;
  int freemap_idx;
  int bit_idx;
  int os_idx;

  os_idx = 1;

  //figure out how many pages the OS is using
  pagestart = (lastosaddress + MEM_PAGESIZE - 4) / MEM_PAGESIZE;

  //max pages in freemap = 32 bits * max num pages
  freemapmax = (L1_PT_SIZE+31) / 32;

  //printf("pagestart: %d\n", pagestart);
  //printf("maxpage: %d\n", maxpage);
  //printf("pagesize: %d\n", MEM_PAGESIZE);
  //printf("max mem size: %d\n", MEM_MAX_SIZE);
  //printf("freemapmax: %d\n", freemapmax);
  //printf("lastosaddress: %d\n", lastosaddress);
  //printf("l1ptsize: %d\n", L1_PT_SIZE); 

  //go through and set all freemaps to 0
  for (i = 0; i < freemapmax; i++) {
    freemap[i] = 0;
  }

  //set counter to none in use
  nfreepages = 0;

  //goes through starting after the OS and marks them all as available
  for (curpage = pagestart; curpage < L1_PT_SIZE; curpage++) {
    nfreepages += 1;
    MemorySetFreemap (curpage, 1);
    //printf("setting page %d as available\n", curpage);
  }
}

void MemorySetFreemap (int page, int bit_value) { //DOUBLE CHECKED
  uint32	fremap_idx = page / 32;
  uint32	bit_idx = page % 32;

  freemap[fremap_idx] = (freemap[fremap_idx] & invert(1 << bit_idx)) | (bit_value << bit_idx);
}

//----------------------------------------------------------------------
//
// MemoryTranslateUserToSystem
//
//	Translate a user address (in the process referenced by pcb)
//	into an OS (physical) address.  Return the physical address.
//
//----------------------------------------------------------------------
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr) { //DOUBLE CHECKED
  
  int	pagenum = addr >> MEM_L1FIELD_FIRST_BITNUM;
  int offset = addr & MEM_ADDRESS_OFFSET_MASK;

  return ((pcb->pagetable[pagenum] & PTE_TO_ADDRESS) + offset);
}

//----------------------------------------------------------------------
//
//	MemoryMoveBetweenSpaces
//
//	Copy data between user and system spaces.  This is done page by
//	page by:
//	* Translating the user address into system space.
//	* Copying all of the data in that page
//	* Repeating until all of the data is copied.
//	A positive direction means the copy goes from system to user
//	space; negative direction means the copy goes from user to system
//	space.
//
//	This routine returns the number of bytes copied.  Note that this
//	may be less than the number requested if there were unmapped pages
//	in the user range.  If this happens, the copy stops at the
//	first unmapped address.
//
//----------------------------------------------------------------------
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir) {
  unsigned char *curUser;         // Holds current physical address representing user-space virtual address
  int		bytesCopied = 0;  // Running counter
  int		bytesToCopy;      // Used to compute number of bytes left in page to be copied

  while (n > 0) {
    // Translate current user page to system address.  If this fails, return
    // the number of bytes copied so far.
    curUser = (unsigned char *)MemoryTranslateUserToSystem (pcb, (uint32)user);

    // If we could not translate address, exit now
    if (curUser == (unsigned char *)0) break;

    // Calculate the number of bytes to copy this time.  If we have more bytes
    // to copy than there are left in the current page, we'll have to just copy to the
    // end of the page and then go through the loop again with the next page.
    // In other words, "bytesToCopy" is the minimum of the bytes left on this page 
    // and the total number of bytes left to copy ("n").

    // First, compute number of bytes left in this page.  This is just
    // the total size of a page minus the current offset part of the physical
    // address.  MEM_PAGESIZE should be the size (in bytes) of 1 page of memory.
    // MEM_ADDRESS_OFFSET_MASK should be the bit mask required to get just the
    // "offset" portion of an address.
    bytesToCopy = MEM_PAGESIZE - ((uint32)curUser & MEM_ADDRESS_OFFSET_MASK);
    
    // Now find minimum of bytes in this page vs. total bytes left to copy
    if (bytesToCopy > n) {
      bytesToCopy = n;
    }

    // Perform the copy.
    if (dir >= 0) {
      bcopy (system, curUser, bytesToCopy);
    } else {
      bcopy (curUser, system, bytesToCopy);
    }

    // Keep track of bytes copied and adjust addresses appropriately.
    n -= bytesToCopy;           // Total number of bytes left to copy
    bytesCopied += bytesToCopy; // Total number of bytes copied thus far
    system += bytesToCopy;      // Current address in system space to copy next bytes from/into
    user += bytesToCopy;        // Current virtual address in user space to copy next bytes from/into
  }
  return (bytesCopied);
}

//----------------------------------------------------------------------
//
//	These two routines copy data between user and system spaces.
//	They call a common routine to do the copying; the only difference
//	between the calls is the actual call to do the copying.  Everything
//	else is identical.
//
//----------------------------------------------------------------------
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, from, to, n, 1));
}

int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, to, from, n, -1));
}

//---------------------------------------------------------------------
// MemoryPageFaultHandler is called in traps.c whenever a page fault 
// (better known as a "seg fault" occurs.  If the address that was
// being accessed is on the stack, we need to allocate a new page 
// for the stack.  If it is not on the stack, then this is a legitimate
// seg fault and we should kill the process.  Returns MEM_SUCCESS
// on success. Kill the current process return MEM_FAIL on failure.
//---------------------------------------------------------------------
int MemoryPageFaultHandler(PCB *pcb) {
  
  uint32 page;
  uint32 cur_stack_page;
  uint32 newPage;
  
  //printf("enter page fault handler\n");
  
  //the address with the fault shifted over to get page
  page = pcb->currentSavedFrame[PROCESS_STACK_FAULT] >> MEM_L1FIELD_FIRST_BITNUM;
  //printf("faulty page: %d\n", page);

  //current user stack pointer page shifted over to get page
  cur_stack_page = pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER] >> MEM_L1FIELD_FIRST_BITNUM;
  //printf("cur stack page: %d\n\n", cur_stack_page);

  //check if address is on stack
  //aka if the stack pointer is lower than the address since the stack grows from the top
  if(cur_stack_page <= page){
    //test 4
    //allocate a new page
    newPage = MemoryAllocPage();

    //if none were available, kill the process and return fail
    if(newPage == 0){
      ProcessKill();
      return MEM_FAIL;
    }

    //if it succeeded, add it to the page table and return success
    pcb->pagetable[L1_PT_SIZE - 1 - pcb->npages] = MemorySetupPte(newPage);
    pcb->npages++;
    //printf("page fault, but not seg fault, allocating another page: %d\n\n", newPage);
    return MEM_SUCCESS;
  }
  
  //if it was a seg fault, print error and kill
  printf("FATAL ERROR (%d): segmentation fault at page address %x\n", GetCurrentPid(), pcb->currentSavedFrame[PROCESS_STACK_FAULT]);
  ProcessKill();
  return MEM_FAIL;
}


//---------------------------------------------------------------------
// You may need to implement the following functions and access them from process.c
// Feel free to edit/remove them
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Allocate a page of memory.
//---------------------------------------------------------------------
int MemoryAllocPage(void) { //DOUBLE CHECKED

  int		maxpage = MEM_MAX_SIZE / MEM_PAGESIZE;
  int i;
  static int	freemap_idx = 0;
  int		bitnum;
  uint32	pagenum;
  int found_nonzero;

  //max pages in freemap = 32 bits * max num pages
  freemapmax = (maxpage+31) / 32;

  //printf("freemapmax: %d\n", freemapmax);

  //if there were no free pages, exit function with error (0)
  if (nfreepages == 0) {
    return (0);
  }

  found_nonzero = 0;
  for(i = 0; i < freemapmax; i++){
    if(freemap[i] != 0){
      found_nonzero = i;
      break;
    }
  }

  //go through all the bits and find one that is nonzero
  for(i = 0; i < 32; i++){
    if((freemap[found_nonzero] & (0x1 << i)) != 0){
      break;
    }
  }
  
  pagenum = (found_nonzero * 32) + i;
  nfreepages -= 1;
  MemorySetFreemap (pagenum, 0);

  //printf("(%d) allocated page: %d\n", GetCurrentPid(), pagenum);

  return (pagenum);
}

//---------------------------------------------------------------------
// Set up PTE given page number.
//---------------------------------------------------------------------
uint32 MemorySetupPte (uint32 page) {

  //printf("page: %d\n", page);
  //printf("pte: %x\n\n", (page * MEM_PAGESIZE) | MEM_PTE_VALID);
  return ((page * MEM_PAGESIZE) | MEM_PTE_VALID);
}


//---------------------------------------------------------------------
// Free a memory page
//---------------------------------------------------------------------
void MemoryFreePage(uint32 page) {
  MemorySetFreemap (page, 1);
  nfreepages += 1;
}

