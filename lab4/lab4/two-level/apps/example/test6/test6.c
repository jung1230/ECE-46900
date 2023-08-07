#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[])
{
  int i;

  if (argc != 1) { 
    Exit();
  } 

  // Now print a message to show that everything worked
  Printf("Before counting (%d)\n", getpid());
  for(i = 0; i < 10000; i++){

  }
  Printf("After counting (%d)\n", getpid());

}
