#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "synch.h"
#include "queue.h"
#include "mbox.h"

// arrays for mboxes and mbox messages

static mbox mboxes[MBOX_NUM_MBOXES]; 
static mbox_message messages[MBOX_NUM_BUFFERS];

//-------------------------------------------------------
//
// void MboxModuleInit();
//
// Initialize all mailboxes.  This process does not need
// to worry about synchronization as it is called at boot
// time.  Only initialize necessary items here: you can
// initialize others in MboxCreate.  In other words,
// don't waste system resources like locks and semaphores
// on unused mailboxes.
//
//-------------------------------------------------------

void MboxModuleInit() {

  int i; // Loop Index variable
  
  //loop through all mailboxes
  for(i=0; i<MBOX_NUM_MBOXES; i++) {
  
    //mark the current mailbox as not in use
    mboxes[i].inuse = 0;
  }

  //initialize all messages as available
  for(i=0; i<MBOX_NUM_BUFFERS; i++) {
    messages[i].inuse = 0;
  }

}

//-------------------------------------------------------
//
// mbox_t MboxCreate();
//
// Allocate an available mailbox structure for use.
//
// Returns the mailbox handle on success
// Returns MBOX_FAIL on error.
//
//-------------------------------------------------------
mbox_t MboxCreate() {

  int j;

  //initialize lock and cond variables for mailbox being created
  lock_t mbox_lock;
  cond_t mbox_cond1;
  cond_t mbox_cond2;

  //create mailbox
  mbox_t mailbox_handle;

  //interrupt variable
  uint32 intrval;

  //create locks
  mbox_lock = LockCreate();

  //create condition variables
  mbox_cond1 = CondCreate(mbox_lock);
  mbox_cond2 = CondCreate(mbox_lock);

  //disable interrupts
  intrval = DisableIntrs();

  //go through all the mailboxes
  for(mailbox_handle=0; mailbox_handle<MBOX_NUM_MBOXES; mailbox_handle++) {
    
    //if the current one is available
    if(mboxes[mailbox_handle].inuse==0) {
      mboxes[mailbox_handle].inuse = 1; //mark it as in use now (take it)
      break; //quit the for loop
    }
  }

  //restore interrupts
  RestoreIntrs(intrval);

  //if all the mailboxes are being used, cant make a new one
  if(mailbox_handle>=MBOX_NUM_MBOXES){
    printf("Error: all of the mailboxes are being used, cant get a new one\n");
    return MBOX_FAIL;
  }

  //if the structure for the mailbox hasnt been initialized
  if(!(&mboxes[mailbox_handle])) return MBOX_FAIL;

  //initialize the queue for the mbox
  if (AQueueInit (&(mboxes[mailbox_handle]).msgs_queue) != QUEUE_SUCCESS) {
    printf("Error: couldn't initialize queue for the mailbox being created\n");
    return MBOX_FAIL;
  }

  //set the lock and conds of the mailbox
  (&mboxes[mailbox_handle])->lock_id = mbox_lock;
  (&mboxes[mailbox_handle])->send_cond = mbox_cond1;
  (&mboxes[mailbox_handle])->recv_cond = mbox_cond2;
  (&mboxes[mailbox_handle])->num_waiting = 0;
  
  //initialize all spots in the waiting array to -1 (nobody waiting at that spot)
  for(j = 0; j < 32; j++){
    mboxes[mailbox_handle].waiting[j] = -1;
  }

  return mailbox_handle;
}

//-------------------------------------------------------
//
// void MboxOpen(mbox_t);
//
// Open the mailbox for use by the current process.  Note
// that it is assumed that the internal lock/mutex handle
// of the mailbox and the inuse flag will not be changed
// during execution.  This allows us to get the a valid
// lock handle without a need for synchronization.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxOpen(mbox_t handle) {

  int i;
  int opened;

  //get the lock
  LockHandleAcquire(mboxes[handle].lock_id);

  //if there is no room on the waiting queue, return fail
  if(mboxes[handle].num_waiting >= 32){
    printf("Error: no room on mailbox waiting queue, cant open it\n");
    return MBOX_FAIL;
  }

  opened = 0;

  //go through all the spots in the mailbox waiting queue
  for(i = 0; i < 32; i++){

    //if a spot is open, put the current pid in it and break
    if(mboxes[handle].waiting[i] == -1){
      mboxes[handle].waiting[i] = GetCurrentPid();
      opened = 1;
      break;
    }
  }

  //if we nexver opened a mailbox
  if(opened == 0){
    printf("ERROR: Failed to open mailbox\n");
    LockHandleRelease(mboxes[handle].lock_id);
    return MBOX_FAIL;
  }

  //increment the number of waiting processes on the wait array
  mboxes[handle].num_waiting++;

  //release the lock
  LockHandleRelease(mboxes[handle].lock_id);

  return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxClose(mbox_t);
//
// Close the mailbox for use to the current process.
// If the number of processes using the given mailbox
// is zero, then disable the mailbox structure and
// return it to the set of available mboxes.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxClose(mbox_t handle) {

  int i;
  int closed;

  //get the lock
  LockHandleAcquire(mboxes[handle].lock_id);

  //go through the waiting array of the mailbox
  for(i = 0; i < 32; i++){

    //if we find the current pid on the waiting array, remove itself and break
    if(mboxes[handle].waiting[i] == GetCurrentPid()){
      mboxes[handle].waiting[i] = -1;
      closed = 1;
      break;
    }
  }

  if(closed == 0){
    printf("ERROR: Failed to close mailbox\n");
    LockHandleRelease(mboxes[handle].lock_id);
    return MBOX_FAIL;
  }

  //decrease the number of waiting processes on the mailbox waiting array
  mboxes[handle].num_waiting--;

  //if there is nothing using this mailbox, free it for use again
  if(mboxes[handle].num_waiting <= 0){
    mboxes[handle].inuse = 0;
  }

  //release the lock
  LockHandleRelease(mboxes[handle].lock_id);

  return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxSend(mbox_t handle,int length, void* message);
//
// Send a message (pointed to by "message") of length
// "length" bytes to the specified mailbox.  Messages of
// length 0 are allowed.  The call
// blocks when there is not enough space in the mailbox.
// Messages cannot be longer than MBOX_MAX_MESSAGE_LENGTH.
// Note that the calling process must have opened the
// mailbox via MboxOpen.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxSend(mbox_t handle, int length, void* message) {

  int i;
  int had_opened;
  int got_avail_msg;
  uint32 intrval;
  int new_msg;
  Link *new_link;

  //get the lock
  LockHandleAcquire(mboxes[handle].lock_id);

  had_opened = 0;

  //loop through waiting array and break if we find the current process
  for(i = 0; i < 32; i++){
    
    //if we find the current pid waiting, break early
    if(mboxes[handle].waiting[i] == GetCurrentPid()){
      had_opened = 1;
      break;
    }
  }

  //if we got all the way to the end without finding the current pid, fail
  if(had_opened == 0){
    printf("MboxSend FAIL: process wasnt in waiting array (it didnt have the mailbox open)\n");
    return MBOX_FAIL;
  }

  //check the mesage length and return fail if its too big
  if(length > MBOX_MAX_MESSAGE_LENGTH){
    printf("MboxSend FAIL: message too big\n");
    return MBOX_FAIL;
  }
  
  //if the mailbox is full, wait for it to not be full
  if(AQueueLength(&(mboxes[handle]).msgs_queue) >= MBOX_MAX_BUFFERS_PER_MBOX){
    CondHandleWait(mboxes[handle].send_cond);
  }

  //disable interrupts
  intrval = DisableIntrs();

  got_avail_msg = 0;

  //go through all the messages
  for(new_msg=0; new_msg<MBOX_NUM_BUFFERS; new_msg++) {
    
    //if the current one is available
    if(messages[new_msg].inuse==0) {
      messages[new_msg].inuse = 1; //mark it as in use now (take it)
      got_avail_msg = 1;
      break; //quit the for loop
    }
  }

  //no free mbox message structs available
  if(got_avail_msg == 0){
    printf("MboxSend FAIL: no free message structs available\n");
    return MBOX_FAIL;
  }

  //store message in struct
  bcopy(message, (&messages[new_msg])->buf, length);
  messages[new_msg].msg_len = length;

  //restore interrupts
  RestoreIntrs(intrval);

  //create a new queue link
  if ((new_link = AQueueAllocLink((void *)(&messages[new_msg]))) == NULL) {
    return MBOX_FAIL;
  }

  //put link into the wait queue
  if (AQueueInsertLast (&(mboxes[handle]).msgs_queue, new_link) != QUEUE_SUCCESS) {
    return MBOX_FAIL;
  }
  
  //signal not empty
  CondHandleSignal(mboxes[handle].recv_cond);

  //release lock
  LockHandleRelease(mboxes[handle].lock_id);

  return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxRecv(mbox_t handle, int maxlength, void* message);
//
// Receive a message from the specified mailbox.  The call
// blocks when there is no message in the buffer.  Maxlength
// should indicate the maximum number of bytes that can be
// copied from the buffer into the address of "message".
// An error occurs if the message is larger than maxlength.
// Note that the calling process must have opened the mailbox
// via MboxOpen.
//
// Returns MBOX_FAIL on failure.
// Returns number of bytes written into message on success.
//
//-------------------------------------------------------

int MboxRecv(mbox_t handle, int maxlength, void* message) {

  int i;
  int had_opened;
  Link *link_removed;
  mbox_message *recv_msg;

  //acqruire lock
  LockHandleAcquire(mboxes[handle].lock_id);

  had_opened = 0;

  //loop through waiting array and break if we find the current process
  for(i = 0; i < 32; i++){
    
    //if we find the current pid waiting, break early
    if(mboxes[handle].waiting[i] == GetCurrentPid()){
      had_opened = 1;
      break;
    }
  }

  //if we got all the way to the end without finding the current pid, fail
  if(had_opened == 0){
    printf("MboxRecv FAIL: process wasnt in waiting array (it didnt have the mailbox open)\n");
    return MBOX_FAIL;
  }

  //check if the message is too long
  if(maxlength > MBOX_MAX_MESSAGE_LENGTH){
    printf("MboxREcv FAIL: message too big\n");
    return MBOX_FAIL;
  }

  //if the mailbox is empty, wait for it to get a message
  if(AQueueEmpty(&(mboxes[handle]).msgs_queue)){
    CondHandleWait(mboxes[handle].recv_cond);
  }

  //if the queue is not empty (there is a message)
  if(!AQueueEmpty(&(mboxes[handle]).msgs_queue)) { 
    
    //get the link from the queue
    link_removed = AQueueFirst(&(mboxes[handle]).msgs_queue);
    
    //remove the message from the link
    recv_msg = (mbox_message *)AQueueObject(link_removed);
    bcopy(recv_msg->buf, message, sizeof(recv_msg->buf));
    
    //remove the link from the queue
    if (AQueueRemove(&link_removed) != QUEUE_SUCCESS) { 
      printf("FATAL ERROR: could not remove link from queue in MboxRecv!\n");
      exitsim();
    }

  }

  //signal not full because we just consumed a message
  CondHandleSignal(mboxes[handle].send_cond); 

  //release lock
  LockHandleRelease(mboxes[handle].lock_id);

  //return sizeof(message)
  return sizeof(recv_msg->buf);
}

//--------------------------------------------------------------------------------
//
// int MboxCloseAllByPid(int pid);
//
// Scans through all mailboxes and removes this pid from their "open procs" list.
// If this was the only open process, then it makes the mailbox available.  Call
// this function in ProcessFreeResources in process.c.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//--------------------------------------------------------------------------------
int MboxCloseAllByPid(int pid) {

  int i;
  int j;
  int handle;

  //go through all the mailboxes
  for(j = 0; j < MBOX_NUM_MBOXES; j++){
    
    handle = j;

    //get the lock
    LockHandleAcquire(mboxes[handle].lock_id);

    //go through the waiting array of the mailbox
    for(i = 0; i < 32; i++){

      //if we find the current pid on the waiting array, remove itself and break
      if(mboxes[handle].waiting[i] == pid){    
        mboxes[handle].waiting[i] = -1;
        mboxes[handle].num_waiting--;
        break;
      }
    }

    //if there is nothing using this mailbox, free it for use again
    if(mboxes[handle].num_waiting == 0){
      mboxes[handle].inuse = 0;
    }

    //release the lock
    LockHandleRelease(mboxes[handle].lock_id);
  }

  return MBOX_SUCCESS;
}
