#ifndef	_memory_constants_h_
#define	_memory_constants_h_

//------------------------------------------------
// #define's that you are given:
//------------------------------------------------

// We can read this address in I/O space to figure out how much memory
// is available on the system.
#define	DLX_MEMSIZE_ADDRESS	0xffff0000

// Return values for success and failure of functions
#define MEM_SUCCESS 1
#define MEM_FAIL -1

//--------------------------------------------------------
// Put your constant definitions related to memory here.
// Be sure to prepend any constant names with "MEM_" 
//--------------------------------------------------------
//32 bit virutal address: [PAGE NUM , OFFSET]

#define MEM_L1FIELD_FIRST_BITNUM 12 //100% correct
#define MEM_L2FIELD_FIRST_BITNUM 12 //100% correct
#define MEM_MAX_VIRTUAL_ADDRESS 0xfffff //100% correct
#define MEM_MAX_SIZE 2097152 //100% correct
#define MEM_PTE_READONLY 0x4 //100% correct
#define MEM_PTE_DIRTY 0x2 //100% correct
#define MEM_PTE_VALID 0x1 //100% correct
#define MEM_PAGESIZE (0x1 << MEM_L1FIELD_FIRST_BITNUM) //100% correct
#define MEM_ADDRESS_OFFSET_MASK (MEM_PAGESIZE - 1) //100% correct
#define PTE_TO_ADDRESS ~(MEM_PTE_READONLY | MEM_PTE_DIRTY | MEM_PTE_VALID) //100% correct
#define L1_PT_SIZE ((MEM_MAX_VIRTUAL_ADDRESS + 1) >> MEM_L1FIELD_FIRST_BITNUM) //100% correct


#endif	// _memory_constants_h_
