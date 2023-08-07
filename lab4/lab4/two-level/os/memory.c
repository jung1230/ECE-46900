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
static uint32 freemap[MAX_PAGES / 32]; //256 pages, each entry holds 32 pages
static uint32 pagestart;
static int nfreepages;
static int freemapmax;

//L2 PAGING
static uint32 l2_pagetables[64][16]; //64 l2 tables, each with 16 entries

//reference counter for fork
uint32 reference_counter[256];

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
void MemoryModuleInit() { //DOUBLE CHECKED L2
  
  int		i;
  int		curpage;
  int os_idx;

  os_idx = 1;

  //figure out how many pages the OS is using
  pagestart = (lastosaddress + MEM_PAGESIZE - 4) / MEM_PAGESIZE;

  //max pages in freemap = 32 bits * max num pages
  freemapmax = (MAX_PAGES+31) / 32;

  //go through and set all freemaps to 0
  for (i = 0; i < freemapmax; i++) {
    freemap[i] = 0;
  }

  for (i = 0; i < 256; i++) {
    reference_counter[i] = 0;
  }

  //set counter to none in use
  nfreepages = 0;

  //goes through starting after the OS and marks them all as available
  for (curpage = pagestart; curpage < MAX_PAGES; curpage++) {
    nfreepages += 1;
    MemorySetFreemap (curpage, 1);
  }

  //printf("os used %d pages\n", pagestart);
  //printf("freemap has %d pages\n", freemapmax);
  //printf("lastosaddress: %d\n", lastosaddress);
}

void MemorySetFreemap (int page, int bit_value) { //DOUBLE CHECKED L2
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
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr) { //doulvbe checked l2
  
  int l1_idx;
  int l2_pagenum;
  int offset;
  uint32 pte;
  uint32 *l2_physical_addr;

  l1_idx = addr >> MEM_L1FIELD_FIRST_BITNUM; //0-16
  l2_pagenum = (addr >> MEM_L2FIELD_FIRST_BITNUM) & 0xf; //0-16
  offset = addr & MEM_ADDRESS_OFFSET_MASK;

  l2_physical_addr = (uint32 *)pcb->pagetable[l1_idx]; //base physical address of l2 page table

  pte = (uint32)l2_physical_addr[l2_pagenum]; //index l2 page table to get pte HERE IS IUSSUE

  /* printf("virutal addr: %x\n", addr);
  printf("l1_idx: %d\n", l1_idx);
  printf("l2_pagenum: %d\n", l2_pagenum);
  printf("offset: %x\n", offset);
  printf("offset mask: %x\n", MEM_ADDRESS_OFFSET_MASK);
  printf("pte: %x\n", pte);
  printf("physical address: %x\n\n", (pte & PTE_TO_ADDRESS) + offset);  */

  return (pte & PTE_TO_ADDRESS) + offset;
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
int MemoryPageFaultHandler(PCB *pcb) { //COME BACK
  
  uint32 fault_addr;
  uint32 stack_addr;
  uint32 newPage;
  uint32 newTable;
  uint32 fault_pagenum;
  uint32 stack_pagenum;

  
  //printf("enter page fault handler\n");
  
  //GET FAULT PAGE NUM
  fault_addr = pcb->currentSavedFrame[PROCESS_STACK_FAULT]; //virtual address of fault
  //printf("fault addr: %x\n", fault_addr);
  fault_pagenum = virAddrToPage(pcb, fault_addr);

  //printf("fault page: %d\n", fault_pagenum);


  //GET STACK PAGE NUM
  stack_addr = pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER]; //virtual address of stack
  
  stack_pagenum = virAddrToPage(pcb, stack_addr);

  //printf("stack page: %d\n\n", stack_pagenum);

  //if we need a new page and there is still room in current l2 table
  if((stack_pagenum <= fault_pagenum)){

    //printf("nfreepages: %d\n", nfreepages);

    //allocate a new page
    newPage = MemoryAllocPage();

    //if none were available, kill the process and return fail
    if(newPage == 0){
      //printf("exit fault handler 1\n");
      ProcessKill();
      return MEM_FAIL;
    }

    //if it succeeded, add it to the page table and return success
    //printf("%d\n", stack_addr >> MEM_L1FIELD_FIRST_BITNUM);
    addEntryToL2Table(pcb, MemorySetupPte(newPage), stack_addr >> MEM_L1FIELD_FIRST_BITNUM, pcb->npages); 
    //addEntryToL2Table(pcb, MemorySetupPte(newPage), pcb->pagetable[stack_addr >> MEM_L1FIELD_FIRST_BITNUM], pcb->npages); 
    pcb->npages++;
    //printf("npages: %d\n", pcb->npages);
    
    //printf("page fault, but not seg fault, allocating another page: %d\n\n", newPage);
    return MEM_SUCCESS;
  }

  //if we need a new page, and there is no more room in this l2 table
  /* if((stack_pagenum <= fault_pagenum) && (checkEntriesInL2Table(pcb, stack_addr >> MEM_L1FIELD_FIRST_BITNUM) == 16)){

    //allocate a new page and a new l2 table
    newPage = MemoryAllocPage();
    newTable = AllocateL2Table();
    if(newPage == 0){
      printf("exit fault handler 2\n");
      ProcessKill();
      return MEM_FAIL;
    }
    if(newTable == -1){
      printf("exit fault handler 3\n");
      ProcessKill();
      return MEM_FAIL;
    }
    addNewL2toPcbL1(pcb, newTable, 0);
    addEntryToL2Table(pcb, MemorySetupPte(newPage), newTable, 0);
    pcb->npages++;
    printf("page fault, but not seg fault, allocating another page: %d\n\n", newPage);
  } */
  
  //if it was a seg fault, print error and kill
  printf("FATAL ERROR (%d): segmentation fault at page address %x\n", GetCurrentPid(), pcb->currentSavedFrame[PROCESS_STACK_FAULT]);
  ProcessKill();
  
  return MEM_FAIL;
}

void addNewL2toPcbL1(PCB *pcb, uint32 newTableNum, uint32 is_user_stack){ //CERTIFIED
  //loop through L1 table in pcb, when we find an available spot, we will put in the new table base address
  int i;

  if(is_user_stack == 1){
    if(pcb->pagetable[15] == 0){
      pcb->pagetable[15] = (uint32)&(l2_pagetables[newTableNum][0]);
    }
    return;
  }

  for(i = 0; i < 16; i++){
    if(pcb->pagetable[i] == 0){
      pcb->pagetable[i] = (uint32)&(l2_pagetables[newTableNum][0]);
      break;
    }
  }
  //printf("actual adr of new table (%d): %x\n", i, (uint32)l2_pagetables[0]);
}

//---------------------------------------------------------------------
// You may need to implement the following functions and access them from process.c
// Feel free to edit/remove them
//---------------------------------------------------------------------
void addEntryToL2Table(PCB *pcb, uint32 pte, uint32 l1_idx, uint32 idx){ //confirmed
  uint32 *l2_table;
  int i;
  //l2_table = (uint32 *)l2_pagetables[l2_table_num];
  l2_table = (uint32 *)pcb->pagetable[l1_idx];
  l2_table[idx] = pte | MEM_PTE_VALID;
}

//---------------------------------------------------------------------
// Allocate a page of memory.
//---------------------------------------------------------------------
int MemoryAllocPage(void) { //DOUBLE CHECKED

  int		maxpage = MEM_MAX_SIZE / MEM_PAGESIZE;
  int i;
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
  
  //printf("found nonzero: %d\n", found_nonzero);
  //printf("i: %d\n", i);

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

void MemoryFreeL2Table(PCB *pcb, uint32 baseAddr){
  uint32 *arrStart;
  int i;
  //printf("baseaddr: %d\n", baseAddr);
  arrStart = (uint32 *)baseAddr;
  for(i = 0; i < 16; i++){
    if(arrStart[i] != 0){
      //printf("freeing page %d\n", virAddrToPage(pcb, arrStart[i]));
      MemoryFreePage(arrStart[i] / MEM_PAGESIZE);
      arrStart[i] = 0;
    }
  } 
}

//takes in a table number (index within an L1 table) and returns the number of entries in that L2 table
uint32 checkEntriesInL2Table(PCB *pcb, uint32 tableNum){
  int i;
  uint32 *l2_base_addr;
  uint32 count;
  l2_base_addr = (uint32 *)pcb->pagetable[tableNum]; //get base address of l2 table
  for(i = 0; i < 16; i++){
    if(l2_base_addr[i] != 0){
      count++;
    }
  }
  return count;
}

//this takes in a virtual address and a pcb and returns the page number of that address
uint32 virAddrToPage(PCB *pcb, uint32 virtualAddr){
  uint32 page;
  uint32 l1_table_num;
  uint32 l2_entry_num;
  uint32 *l2_physical_addr;
  uint32 pte;

  //printf("fault addr: %x\n", virtualAddr);

  l1_table_num = virtualAddr >> MEM_L1FIELD_FIRST_BITNUM; //index of l2 table in l1 table
  l2_entry_num = (virtualAddr >> MEM_L2FIELD_FIRST_BITNUM) & 0xf; //index of page within l2 table

  //printf("l1 table num: %x\n", l1_table_num);
  //printf("l2 entry num: %x\n", l2_entry_num);

  l2_physical_addr = (uint32 *)pcb->pagetable[l1_table_num]; //base address of l2 page table

  //printf("l2 phys addr: %x\n", l2_physical_addr);

  pte = (uint32)l2_physical_addr[l2_entry_num]; //use l2 index to get pte from l2 table base address

  //printf("pte: %x\n", pte);

  page = (pte & PTE_TO_ADDRESS) / MEM_PAGESIZE; //divide this page table entry by pagesize to get a pagenum

  return page;
}

//allocates a new L2 table from the global pool
uint32 AllocateL2Table(void){
  int i;
  int j;
  int count;
  uint32 newTable;
  count = 0;
  newTable = -1;

  //for all l2 tables
  for(i = 0; i < 64; i++){
    
    count = 0;
    
    //go through all 16 entries in the l2 table
    for(j = 0; j < 16; j++){

      //if we find an entry that is taken, break
      if(l2_pagetables[i][j] != 0){
        break;
      }
      
      //if the entry wasnt taken, increase count
      else{
        count++;
      }
    }

    //if all 16 were available, the table was available
    if(count == 16){
      newTable = i;
      break;
    }
  }

  //printf("table allocated exit\n");
  return newTable;
}

