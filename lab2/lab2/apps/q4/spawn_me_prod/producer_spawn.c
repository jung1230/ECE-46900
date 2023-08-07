#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  buffer *buf;        // Used to access missile codes in shared memory page
  uint32 h_mem;            // Handle to the shared memory page
  sem_t sem; // Semaphore to signal the original process that we're done
  char h_mem_str[10];             // Used as command-line argument to pass mem_handle to new processes
  char sem_str[10]; // Used as command-line argument to pass page_mapped handle to new processes
  char hello_world[11] = "Hello World";
  int hello_index; //index within string to send
  char send_char; //char being sent ("produce an item")
  int inserted; //0 = false, 1 = true

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

  //convert command line arguments for shared memory
  ditoa(h_mem, h_mem_str);
  ditoa(sem, sem_str);

  //start at the first letter of the string to send
  hello_index = 0; 

  while (hello_index < 11){ //while we have not sent all the letters

    //produce an item
    send_char = hello_world[hello_index]; 

    //inserted = false
    inserted = 0; 

    while(inserted == 0){ //while not inserted
      
      //lock_acq
      lock_acquire(buf->lock_id);

      if(buf->tail == (buf->head + 1) % BUFFERSIZE){ //if buffer is full

        //lock_rel
        lock_release(buf->lock_id);

        //wait
        //Printf("producer entering wait\n");
        cond_wait(buf->not_full_id);
        //Printf("producer exiting wait\n");

        lock_acquire(buf->lock_id);
      }

      //insert item
      buf->content[buf->tail] = send_char; //put the character into the buffer
      inserted = 1; //set inserted to true
      Printf("Producer %d inserted: %c\n", Getpid(), buf->content[buf->tail]);
      buf->tail++; //increase the buffer head
      if(buf->tail >= BUFFERSIZE){ //if the head has gone past the end of the buffer
        buf->tail = 0; //wrap it back around to the front
      }

      //signal
      //Printf("producer signaling\n");
      cond_signal(buf->not_empty_id);

      //release lock
      lock_release(buf->lock_id);

      //increase index in string
      hello_index++; 
    }
  }
  //-------------------------------------------------------------------------------

  // Signal the semaphore to tell the original process that we're done
  //Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(sem) != SYNC_SUCCESS) {
    Printf("Bad semaphore sem (%d) in ", sem); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
