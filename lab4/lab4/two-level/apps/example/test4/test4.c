#include "usertraps.h"
#include "misc.h"

void recur(int num);

void main (int argc, char *argv[])
{
  int i;

  if (argc != 1) { 
    Exit();
  } 

  recur(50);
}

void recur(int num){
  int arr[100];
  int i;
  for(i = 0; i < 100; i++){
    arr[i] = 1;
  }
  if(num <= 0){
    return;
  }
  //Printf("num: %d\n", num);
  recur(num - 1);
}