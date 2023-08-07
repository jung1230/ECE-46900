#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[])
{
  int input_num;
  int i;                               // Loop index variable
  sem_t s_procs_completed;             // Semaphore used to wait until all spawned processes have completed
  char s_procs_completed_str[10];      // Used as command-line argument to pass page_mapped handle to new processes

  if (argc != 2) {
    Exit();
  }
  //Printf("enter makeprocs\n");

  // Convert string from ascii command line argument to integer number
  input_num = dstrtol(argv[1], NULL, 10); // the "10" means base 10

  //test1 
  if(input_num == 1){
    
    process_create("test1.dlx.obj", NULL);
  
  }

  //test2
  if(input_num == 2){
    
    process_create("test2.dlx.obj", NULL);

  }

  //test3
  if(input_num == 3){
    process_create("test3.dlx.obj", NULL);
  }

  //test4
  if(input_num == 4){
    process_create("test4.dlx.obj", NULL);
  }

  //test5
  if(input_num == 5){
    for(i = 0; i < 100; i++){
      //Printf("%d\n", i);
      process_create("test5.dlx.obj", NULL);
    }
  }

  //test5
  if(input_num == 6){
    for(i = 0; i < 30; i++){
      process_create("test6.dlx.obj", NULL);
    }
  }

}