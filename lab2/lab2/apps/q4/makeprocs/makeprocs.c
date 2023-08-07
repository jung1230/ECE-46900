#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  int numprocs = 0;               // Used to store number of processes to create
  int i;                          // Loop index variable
  buffer *buf;               // Used to get address of shared memory page
  uint32 h_mem;                   // Used to hold handle to shared memory page
  sem_t sem;        // Semaphore used to wait until all spawned processes have completed
  char h_mem_str[10];             // Used as command-line argument to pass mem_handle to new processes
  char sem_str[10]; // Used as command-line argument to pass page_mapped handle to new processes
  
  lock_t lockID; 
  lock_t not_full_lock;
  lock_t not_empty_lock;

  cond_t NotFull;
  cond_t NotEmpty;

  //create locks
  lockID = lock_create();
  not_full_lock = lock_create();
  not_empty_lock = lock_create();

  //create cond var
  NotFull = cond_create(not_full_lock);
  NotEmpty = cond_create(not_empty_lock);

  //Printf("makeprocs lockID: %d\n", lockID); //test code to print lock ID

  run_os_tests(); // Use for testing purpose

  if (argc != 2) {
    Printf("Usage: "); Printf(argv[0]); Printf(" <number of processes to create>\n");
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  numprocs = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  //Printf("Creating %d processes\n", numprocs);

  // Allocate space for a shared memory page, which is exactly 64KB
  // Note that it doesn't matter how much memory we actually need: we 
  // always get 64KB
  if ((h_mem = shmget()) == 0) {
    Printf("ERROR: could not allocate shared memory page in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  // Map shared memory page into this process's memory space
  if ((buf = (buffer *)shmat(h_mem)) == NULL) {
    Printf("Could not map the shared page to virtual address in "); Printf(argv[0]); Printf(", exiting..\n");
    Exit();
  }

  buf->numprocs = numprocs; //set the number of processes (how many producers and consumers)
  buf->lock_id = lockID; //add the lock to the shared buffer
  buf->head = 0; //init head to zero
  buf->tail = 0; //init tail to zero
  buf->not_full_id = NotFull;
  buf->not_empty_id = NotEmpty;

  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.  To do this, we will initialize
  // the semaphore to (-1) * (number of signals), where "number of signals"
  // should be equal to the number of processes we're spawning - 1.  Once 
  // each of the processes has signaled, the semaphore should be back to
  // zero and the final sem_wait below will return.
  if ((sem = sem_create(-(numprocs-1))) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  // Setup the command-line arguments for the new process.  We're going to
  // pass the handles to the shared memory page and the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(h_mem, h_mem_str);
  ditoa(sem, sem_str);

  // Now we can create the processes.  Note that you MUST end your call to
  // process_create with a NULL argument so that the operating system
  // knows how many arguments you are sending.
  for(i=0; i<numprocs; i++) {

    //create consumer
    process_create("consumer_spawn.dlx.obj", h_mem_str, sem_str, NULL); 

    //create producer
    process_create("producer_spawn.dlx.obj", h_mem_str, sem_str, NULL);

  }

  // And finally, wait until all spawned processes have finished.
  if (sem_wait(sem) != SYNC_SUCCESS) {
    Printf("Bad semaphore sem (%d) in ", sem); Printf(argv[0]); Printf("\n");
    Exit();
  }
  //Printf("All other processes completed, exiting main process.\n");
}
