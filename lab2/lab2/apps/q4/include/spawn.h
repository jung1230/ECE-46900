#ifndef __USERPROG__
#define __USERPROG__

typedef struct buffer {
  int head;
  int tail;
  int numprocs;
  char content[BUFFERSIZE];
  lock_t lock_id;
  cond_t not_full_id;
  cond_t not_empty_id;
} buffer;

#define FILENAME_TO_RUN "consumer_spawn.dlx.obj"

#endif
