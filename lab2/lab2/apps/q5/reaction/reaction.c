#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  sem_t sem1; 
  sem_t sem2; 
  sem_t sem3; 
  sem_t sem4;
  sem_t s_procs_completed;
  int react_num;
  int inject_recv_12 = 0;
  int inject_recv_3H2 = 0;
  int inject_recv_3N2 = 0;
  int inject_recv_3O2 = 0;
  int cnt1, cnt2, cnt3;

  if (argc != 10) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  sem1 = dstrtol(argv[1], NULL, 10); //injection semaphore
  sem2 = dstrtol(argv[2], NULL, 10); //reaction molecule 1
  sem3 = dstrtol(argv[3], NULL, 10); //reaction molecule 2
  sem4 = dstrtol(argv[4], NULL, 10);
  s_procs_completed = dstrtol(argv[8], NULL, 10);
  react_num = dstrtol(argv[9], NULL, 10); //reaction number so that we know which molecules are being injected/produced
  
  cnt1 = dstrtol(argv[5], NULL, 10);
  cnt2 = dstrtol(argv[6], NULL, 10);
  cnt3 = dstrtol(argv[7], NULL, 10);


  //reaction 1 or 2
  if(react_num != 3){
    
    while(cnt1 > 1){

      //wait and print success when wait returns (got a molecule from the sempaphore)
      //Printf("\n\nTEST2\n\n");
      if(sem_wait(sem1) == SYNC_SUCCESS){
        //Printf("\n\nTEST3\n\n");
        //increment injection counter
        inject_recv_12++;
        

        //if we have 2 injected molecules and need to process a reaction
        if(inject_recv_12 == 2){

          //if this is reaction 1, signal sem2 and sem3 each one time
          if(react_num == 1){ 
            sem_signal(sem2);
            sem_signal(sem3);
            Printf("Reaction: 1 PID: %d\n", Getpid());
            cnt1 -= 2;
          }
          else{ //if this is reaction 2, signal sem2 twice and sem3 once
            sem_signal(sem2);
            sem_signal(sem2);
            sem_signal(sem3);
            Printf("Reaction: 2 PID: %d\n", Getpid());
            cnt1 -= 2;
          }

          //reset injection counter
          inject_recv_12 = 0;
        }
      }
       //2 NO are consumed every time
       
    }
    
  }
  else{ //reaction 3
    
    while((cnt1 > 0) && (cnt2 > 0) && (cnt3 > 2)){

      //wait and print success when wait returns (got a molecule from the sempaphore)
      //Printf("\n\nTEST7\n\n");
      if(sem_wait(sem1) == SYNC_SUCCESS){
        //Printf("\n\nTEST8\n\n");
        //Printf("got a molecule from the semaphore (PID: %d)\n", Getpid());
        //increment injection counter
        inject_recv_3H2++;
        
      }
      //Printf("\n\nTEST14\n\n");
      if(sem_wait(sem2) == SYNC_SUCCESS){
        //Printf("\n\nTEST20\n\n");
        //increment injection counter
        inject_recv_3N2++;
        
      }
      //Printf("\n\nTEST15\n\n");
      if(sem_wait(sem3) == SYNC_SUCCESS){

        //increment injection counter
        inject_recv_3O2++;
      }
      //Printf("\n\nTEST16\n\n");
      if(sem_wait(sem3) == SYNC_SUCCESS){

        //increment injection counter
        inject_recv_3O2++;
        
      }
      //Printf("\n\nTEST17\n\n");
      if(sem_wait(sem3) == SYNC_SUCCESS){

        //increment injection counter
        inject_recv_3O2++;
        
      }
      //Printf("\n\nTEST9\n\n");
      if((inject_recv_3H2 == 1) && (inject_recv_3N2 == 1) && (inject_recv_3O2 == 3)){
        //Printf("\n\nTEST10\n\n");
        sem_signal(sem4);
        //Printf("\n\nTEST11\n\n");
        inject_recv_3H2 = 0;
        inject_recv_3N2 = 0;
        inject_recv_3O2 = 0;
        Printf("Reaction: 3 PID: %d\n", Getpid());
        cnt3-=3;
        cnt2--;
        cnt1--;
      }
    }
    
  }

  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

}
