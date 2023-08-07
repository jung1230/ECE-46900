#ifndef	_memory_h_
#define	_memory_h_

// Put all your #define's in memory_constants.h
#include "memory_constants.h"

extern int lastosaddress; // Defined in an assembly file

//--------------------------------------------------------
// Existing function prototypes:
//--------------------------------------------------------

int MemoryGetSize();
void MemoryModuleInit();
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr);
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir);
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from, unsigned char *to, int n);
int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from, unsigned char *to, int n);
int MemoryPageFaultHandler(PCB *pcb);

//---------------------------------------------------------
// Put your function prototypes here
//---------------------------------------------------------
int MemoryAllocPage(void);
uint32 MemorySetupPte (uint32 page);
void MemorySetFreemap (int p, int b);
uint32 AllocateL2Table(void);
uint32 virAddrToPage(PCB *pcb, uint32 virtualAddr);
void addNewL2toPcbL1(PCB *pcb, uint32 newTableNum, uint32 is_user_stack);
void addEntryToL2Table(PCB *pcb, uint32 pte, uint32 l2_table_idx, uint32 idx);
uint32 checkEntriesInL2Table(PCB *pcb, uint32 tableNum);
void MemoryFreeL2Table(PCB *pcb, uint32 baseAddr);
void MemoryFreePage(uint32 page);

#endif	// _memory_h_
