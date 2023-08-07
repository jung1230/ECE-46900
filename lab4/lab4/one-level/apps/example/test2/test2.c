#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[])
{
  int arr[10000000];
  int i;

  if (argc != 1) { 
    Exit();
  } 

  // Now print a message to show that everything worked
  for(i = 0; i < 10000000; i++){
    arr[i] = 1;
  }
}
