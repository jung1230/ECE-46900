#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  buffer *buf;        // Used to access missile codes in shared memory page
  uint32 h_mem;            // Handle to the shared memory page
  sem_t sem; // Semaphore to signal the original process that we're done
  int recv; //counter for the number of chars received

   if (argc != 3) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  }  

  // Convert the command-line strings into integers for use as handles
  h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  sem = dstrtol(argv[2], NULL, 10);

  // Map shared memory page into this process's memory space
  if ((buf = (buffer *)shmat(h_mem)) == NULL) {
    Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  
  //init number of received chars to 0
  recv = 0;

  while(recv != strlen("Hello World")){ //while we havent received all the chars in the string
    
    //acquire lock
    lock_acquire(buf->lock_id);

    //if buffer is empty
    if(buf->head == buf->tail){

      //get lock
      lock_release(buf->lock_id);

      //wait not empty
      cond_wait(buf->not_empty_id);
      
      //get lock
      lock_acquire(buf->lock_id);

    }

    //remove item
    Printf("Consumer %d removed: %c\n", Getpid(), buf->content[buf->head]);
    buf->head++;
    if(buf->head >= BUFFERSIZE){
      buf->head = 0;
    }
    recv++;

    //signal not full
    cond_signal(buf->not_full_id);

    //release lock
    lock_release(buf->lock_id);
  }

  // Signal the semaphore to tell the original process that we're done
  //Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(sem) != SYNC_SUCCESS) {
    Printf("Bad semaphore sem (%d) in ", sem); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
