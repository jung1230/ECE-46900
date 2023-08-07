#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  mbox_t mbox_handle_H2O;
  mbox_t mbox_handle_H2;
  mbox_t mbox_handle_O2;
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  char recv_buf[10];
  char content_h2[10] = "H2";
  char content_o2[10] = "O2";

  if (argc != 5) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 


  mbox_handle_H2O = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  mbox_handle_H2 = dstrtol(argv[2], NULL, 10);
  mbox_handle_O2 = dstrtol(argv[3], NULL, 10);
  s_procs_completed = dstrtol(argv[4], NULL, 10);

  //open the mailboxes needed for reacction 1
  if(mbox_open(mbox_handle_H2O) == MBOX_FAIL) {
    Printf("spawn_me (%d): Could not open the mailbox!\n", getpid());
    Exit();
  }
  if(mbox_open(mbox_handle_H2) == MBOX_FAIL) {
    Printf("spawn_me (%d): Could not open the mailbox!\n", getpid());
    Exit();
  }
  if(mbox_open(mbox_handle_O2) == MBOX_FAIL) {
    Printf("spawn_me (%d): Could not open the mailbox!\n", getpid());
    Exit();
  }

  //get the two NO from mailbox
  if (mbox_recv(mbox_handle_H2O, 10, (void *)recv_buf) == MBOX_FAIL) {
    Printf("spawn_me (%d): Could not map the virtual address to the memory!\n", getpid());
    Exit();
  }
  if (mbox_recv(mbox_handle_H2O, 10, (void *)recv_buf) == MBOX_FAIL) {
    Printf("spawn_me (%d): Could not map the virtual address to the memory!\n", getpid());
    Exit();
  }

  //since we got the two NO, send N2 and O2
  if(mbox_send(mbox_handle_H2, sizeof(content_h2), (void *)content_h2) == MBOX_FAIL) {
      Printf("Could not send message to mailbox %d in %s (%d)\n", mbox_handle_H2, argv[0], getpid());
      Exit();
  }
  if(mbox_send(mbox_handle_H2, sizeof(content_h2), (void *)content_h2) == MBOX_FAIL) {
      Printf("Could not send message to mailbox %d in %s (%d)\n", mbox_handle_H2, argv[0], getpid());
      Exit();
  }
  if(mbox_send(mbox_handle_O2, sizeof(content_o2), (void *)content_o2) == MBOX_FAIL) {
      Printf("Could not send message to mailbox %d in %s (%d)\n", mbox_handle_O2, argv[0], getpid());
      Exit();
  }

  if (mbox_close(mbox_handle_H2O) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_handle_H2O);
    Exit();
  }
  if (mbox_close(mbox_handle_H2) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_handle_H2);
    Exit();
  }
  if (mbox_close(mbox_handle_O2) == MBOX_FAIL) {
    Printf("makeprocs (%d): Could not open mailbox %d!\n", getpid(), mbox_handle_O2);
    Exit();
  }

  //signal that the process finished
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  

  Printf("Reaction: 2 PID: %d\n", getpid());
}
