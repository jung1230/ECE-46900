#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  int inj_NO = 0;               // Used to store number of processes to create
  int inj_H2O = 0;               // Used to store number of processes to create
  
  int rem_N2 = 0;               
  int rem_O2 = 0;
  int rem_H2 = 0;
  int rem_HNO3 = 0;
  int rem_NO = 0;
  int rem_H2O = 0;

  int cons_N2 = 0;               
  int cons_O2 = 0;
  int cons_H2 = 0;

  char inj_NO_str[10]; //string to pass number of NO into process command line argument
  char inj_H2O_str[10]; //string to pass number of H2O into process command line argument
  char cons_O2_str[10]; //string to pass number of NO into process command line argument
  char cons_H2_str[10];
  char cons_N2_str[10]; //string to pass number of NO into process command line argument

  //NO semaphore
  sem_t sem_NO;        // Semaphore used to wait until all spawned processes have completed
  char sem_NO_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  //H2O semaphore
  sem_t sem_H2O;        // Semaphore used to wait until all spawned processes have completed
  char sem_H2O_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  //N2 semaphore
  sem_t sem_N2;        // Semaphore used to wait until all spawned processes have completed
  char sem_N2_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  //O2 semaphore
  sem_t sem_O2;        // Semaphore used to wait until all spawned processes have completed
  char sem_O2_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  //H2 semaphore
  sem_t sem_H2;        // Semaphore used to wait until all spawned processes have completed
  char sem_H2_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  //HNO3 semaphore
  sem_t sem_HNO3;        // Semaphore used to wait until all spawned processes have completed
  char sem_HNO3_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed
  char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes


  if (argc != 3) {
    Printf("Usage: "); Printf(argv[0]); Printf(" <number of processes to create>\n");
    Exit();
  }

  // get number of NO from command line arguments
  inj_NO = dstrtol(argv[1], NULL, 10); // the "10" means base 10

  // get number of H2O from command line arguments
  inj_H2O = dstrtol(argv[2], NULL, 10); // the "10" means base 10

  rem_N2 = inj_NO / 2;
  cons_N2 = rem_N2;
  rem_O2 = inj_NO / 2;
  rem_NO = inj_NO % 2;
  rem_H2 = inj_H2O;
  cons_H2 = rem_H2;
  rem_O2 = rem_O2 + (inj_H2O / 2);
  cons_O2 = rem_O2;
  rem_H2O = inj_H2O % 2;
  while(1){
    if(rem_H2 == 0){
      break;
    }
    if(rem_N2 == 0){
      break; 
    }
    if(rem_O2 < 3){
      break; 
    }
    rem_HNO3 = rem_HNO3 + 2;
    rem_H2--;
    rem_N2--;
    rem_O2 = rem_O2 - 3;       
  }


  //create NO semaphore
  if ((sem_NO = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //create H2O semaphore
  if ((sem_H2O = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //create N2 semaphore
  if ((sem_N2 = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //create O2 semaphore
  if ((sem_O2 = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //create H2 semaphore
  if ((sem_H2 = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //create HNO3 semaphore
  if ((sem_HNO3 = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  if ((s_procs_completed = sem_create(-2)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }


  //convert semaphores to strings to pass in command line args
  ditoa(sem_NO, sem_NO_str);
  ditoa(sem_H2O, sem_H2O_str);
  ditoa(sem_N2, sem_N2_str);
  ditoa(sem_O2, sem_O2_str);
  ditoa(sem_H2, sem_H2_str);
  ditoa(sem_HNO3, sem_HNO3_str);
  ditoa(s_procs_completed, s_procs_completed_str);


  //convert number of injection molecules to string to pass in command line args
  ditoa(inj_NO, inj_NO_str);
  ditoa(inj_H2O, inj_H2O_str);

  ditoa(cons_H2, cons_H2_str);
  ditoa(cons_N2, cons_N2_str);
  ditoa(cons_O2, cons_O2_str);


  //create injectors
  process_create("injection.dlx.obj", inj_NO_str, sem_NO_str, "1", NULL); //NO
  process_create("injection.dlx.obj", inj_H2O_str, sem_H2O_str, "2", NULL); //H2O 

  //create reactors
  process_create("reaction.dlx.obj", sem_NO_str, sem_N2_str, sem_O2_str, sem_HNO3_str, inj_NO_str, cons_H2_str, cons_N2_str, s_procs_completed_str, "1", NULL); //reaction 1
  process_create("reaction.dlx.obj", sem_H2O_str, sem_H2_str, sem_O2_str, sem_HNO3_str, inj_H2O_str, cons_H2_str, cons_N2_str, s_procs_completed_str, "2", NULL); //reaction 2
  process_create("reaction.dlx.obj", sem_H2_str, sem_N2_str, sem_O2_str, sem_HNO3_str, cons_H2_str, cons_N2_str, cons_O2_str, s_procs_completed_str, "3", NULL); //reaction 3

  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
    Exit();
  }

  Printf("Remain: %d NO, %d N2, %d H2O, %d H2, %d O2, %d HNO3\n", rem_NO, rem_N2, rem_H2O, rem_H2, rem_O2, rem_HNO3);
}
