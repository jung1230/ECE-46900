#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  int inj_NO = 0;               // Used to store number of processes to create
  int inj_H2O = 0;               // Used to store number of processes to create
  int i;
  int j;
  int k;
  int l;
  int m;

  int num_reaction1;
  int num_reaction2;
  int num_reaction3;

  int rem_N2 = 0;               
  int rem_O2 = 0;
  int rem_H2 = 0;
  int rem_HNO3 = 0;
  int rem_NO = 0;
  int rem_H2O = 0;

  int cons_N2 = 0;               
  int cons_O2 = 0;
  int cons_H2 = 0;

  int total_process;

  sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed
  char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  mbox_t mbox_NO_handle;       
  mbox_t mbox_H2O_handle;
  mbox_t mbox_O2_handle;
  mbox_t mbox_N2_handle;
  mbox_t mbox_H2_handle;
  mbox_t mbox_HNO3_handle;   
  char mbox_NO_handle_str[10];
  char mbox_H2O_handle_str[10];
  char mbox_O2_handle_str[10];
  char mbox_N2_handle_str[10];
  char mbox_H2_handle_str[10];
  char mbox_HNO3_handle_str[10];       

  if (argc != 3) {
    Printf("Usage: "); Printf(argv[0]); Printf(" <number of processes to create>\n");
    Exit();
  }

  //create the mailboxes
  if ((mbox_NO_handle = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox!", getpid());
    Exit();
  }
  if ((mbox_H2O_handle = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox!", getpid());
    Exit();
  }
  if ((mbox_O2_handle = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox!", getpid());
    Exit();
  }
  if ((mbox_N2_handle = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox!", getpid());
    Exit();
  }
  if ((mbox_H2_handle = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox!", getpid());
    Exit();
  }
  if ((mbox_HNO3_handle = mbox_create()) == MBOX_FAIL) {
    Printf("makeprocs (%d): ERROR: could not allocate mailbox!", getpid());
    Exit();
  }

  //open the mailboxes
  if (mbox_open(mbox_NO_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_NO_handle);
    Exit();
  }
  if (mbox_open(mbox_H2O_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_H2O_handle);
    Exit();
  }
  if (mbox_open(mbox_O2_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_O2_handle);
    Exit();
  }
  if (mbox_open(mbox_N2_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_N2_handle);
    Exit();
  }
  if (mbox_open(mbox_H2_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_H2_handle);
    Exit();
  }
  if (mbox_open(mbox_HNO3_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_HNO3_handle);
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

  //(inj - remain) / num per reaction
  // inj / num per reaction
  num_reaction1 = inj_NO / 2;
  num_reaction2 = inj_H2O / 2;

  num_reaction3 = 0;
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
    
    //reaction 3 occurs
    num_reaction3++;       
  }

  total_process = num_reaction1 + num_reaction2 + num_reaction3 +inj_NO +inj_H2O; 

  //process semaphore
  if ((s_procs_completed = sem_create(-1 * (total_process - 1))) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //convert semaphore to string to pass in command line args
  ditoa(mbox_NO_handle, mbox_NO_handle_str);
  ditoa(mbox_H2O_handle, mbox_H2O_handle_str);
  ditoa(mbox_N2_handle, mbox_N2_handle_str);
  ditoa(mbox_O2_handle, mbox_O2_handle_str);
  ditoa(mbox_H2_handle, mbox_H2_handle_str);
  ditoa(mbox_HNO3_handle, mbox_HNO3_handle_str);
  ditoa(s_procs_completed, s_procs_completed_str);

  //create injectors
  for(i=0; i < inj_NO; i++){
    process_create("injection_NO.dlx.obj", 0, 0, mbox_NO_handle_str, s_procs_completed_str, NULL); //NO
  }
  for(j=0; j < inj_H2O; j++){
    process_create("injection_H2O.dlx.obj", 0, 0, mbox_H2O_handle_str, s_procs_completed_str, NULL); //H2O 
  }

  //create reactors
  for(k=0; k < num_reaction1; k++){
    process_create("reaction1.dlx.obj", 0, 0, mbox_NO_handle_str, mbox_N2_handle_str, mbox_O2_handle_str, s_procs_completed_str, NULL); //reaction 1
  }
  for(l=0; l < num_reaction2; l++){
    process_create("reaction2.dlx.obj", 0, 0, mbox_H2O_handle_str, mbox_H2_handle_str, mbox_O2_handle_str, s_procs_completed_str, NULL); //reaction 2
  }
  for(m=0; m < num_reaction3; m++){
    process_create("reaction3.dlx.obj", 0, 0, mbox_H2_handle_str, mbox_N2_handle_str, mbox_O2_handle_str, mbox_HNO3_handle_str, s_procs_completed_str, NULL); //reaction 3
  } 

  //wait for processes to finish
  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
    Exit();
  }

  //close all the mailboxes
  if (mbox_close(mbox_NO_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_NO_handle);
    Exit();
  }
  if (mbox_close(mbox_H2O_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_H2O_handle);
    Exit();
  }
  if (mbox_close(mbox_O2_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_O2_handle);
    Exit();
  }
  if (mbox_close(mbox_N2_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_N2_handle);
    Exit();
  }
  if (mbox_close(mbox_H2_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_H2_handle);
    Exit();
  }
  if (mbox_close(mbox_HNO3_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_HNO3_handle);
    Exit();
  }

  Printf("Remain: %d NO, %d N2, %d H2O, %d H2, %d O2, %d HNO3\n", rem_NO, rem_N2, rem_H2O, rem_H2, rem_O2, rem_HNO3);
}
