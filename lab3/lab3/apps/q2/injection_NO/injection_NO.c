#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  mbox_t mbox_NO_handle; 
  sem_t s_procs_completed;

  char content[10] = "NO";

  if (argc != 3) { 
    Printf("ERROR: wrong number of arguments to injection_NO\n"); 
    Exit();
  }
  
  mbox_NO_handle = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);

  //open the mailbox
  if(mbox_open(mbox_NO_handle) == MBOX_FAIL) {
    Printf("spawn_me (%d): Could not open the mailbox!\n", getpid());
    Exit();
  }

  //send the injection molecule
  if(mbox_send(mbox_NO_handle, sizeof(content), (void *)content) == MBOX_FAIL) {
      Printf("Could not send message to mailbox %d in %s (%d)\n", mbox_NO_handle, argv[0], getpid());
      Exit();
  }

  if (mbox_close(mbox_NO_handle) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_NO_handle);
    Exit();
  }

  //signal that the process finished
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  
  Printf("Injection: NO PID: %d\n", getpid());

}
