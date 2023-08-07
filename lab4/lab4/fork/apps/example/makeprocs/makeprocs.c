#include "usertraps.h"
#include "misc.h"


void main (int argc, char *argv[])
{
  int child_pid;
  int input_num;
  
  if (argc != 2) {
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  input_num = dstrtol(argv[1], NULL, 10); // the "10" means base 10

  if(input_num == 1){
    child_pid = fork();
    if (child_pid != 0) {
        print_VPTE();
    } else {
        print_VPTE();
    }
  }

  if(input_num == 2){
    
    print_VPTE();
    
    child_pid = fork();
    func(); //generates exception by writing to user stack
    if (child_pid != 0) {
        print_VPTE();
    } else {
        print_VPTE();
    }
  }
}

static void func(){
  return;
}