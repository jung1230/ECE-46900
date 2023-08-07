#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  sem_t sem; // Semaphore to signal the original process that we're done
  int num_mol; //number of injection molecules
  char sem_str[10]; // Used as command-line argument to pass page_mapped handle to new processes
  int react_num;

  if (argc != 4) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  sem = dstrtol(argv[2], NULL, 10);
  react_num = dstrtol(argv[3], NULL, 10);

  //convert number of injection molecule from a command line string to an int again
  num_mol = dstrtol(argv[1], NULL, 10);

  //convert command line arguments for shared memory
  ditoa(sem, sem_str);

  //while there are molecules to inject
  while(num_mol > 0){

    //signal that there is a new molecule injected to the semaphore (sem++)
    sem_signal(sem);

    if(react_num == 1){
      Printf("Injection: NO PID: %d\n", Getpid());
    }
    else{
      Printf("Injection: H2O PID: %d\n", Getpid());
    }

    //subtract the molecules that were just injected
    num_mol--;
  }

}
