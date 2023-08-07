#include "usertraps.h"
#include "misc.h"

#include "fdisk.h"

dfs_superblock sb;
dfs_inode inodes[DFS_INODE_MAX_NUM];
uint32 fbv[DFS_FBV_MAX_NUM_WORDS];

int diskblocksize = 0; // These are global in order to speed things up
int disksize = 0;      // (i.e. fewer traps to OS to get the same number)

int FdiskWriteBlock(uint32 blocknum, dfs_block *b); //You can use your own function. This function 
//calls disk_write_block() to write physical blocks to disk

void main (int argc, char *argv[])
{
	// STUDENT: put your code here. Follow the guidelines below. They are just the main steps. 
	// You need to think of the finer details. You can use bzero() to zero out bytes in memory

  //Initializations and argc check
  int i;
  int j;
  int k;
  dfs_block temp;
  int used_mask;
  int count;

  if(argc != 1){
    Exit();
  }

  //Printf("enter fdisk\n");

  // Need to invalidate filesystem before writing to it to make sure that the OS
  // doesn't wipe out what we do here with the old version in memory
  // You can use dfs_invalidate(); but it will be implemented in Problem 2. You can just do 
  // sb.valid = 0
  sb.valid = 0;

  sb.num_fs_blocks = PHYS_DISK_SIZE / DFS_BLOCKSIZE; //256
  sb.inode_start_block = 4; //0, 2, 3 are not used and 1 is superblock, so inodes start at 4
  sb.start_fbv = FDISK_FBV_BLOCK_START + 3; //4-18 are inodes, so 19 is first fbv
  sb.fs_block_size = DFS_BLOCKSIZE; //1024
  sb.num_inodes = DFS_INODE_MAX_NUM; //120
  //Printf("here5\n");

  // Make sure the disk exists before doing anything else
  if(disk_create() != DISK_SUCCESS){
    //Printf("disk create FAIL\n");
    Exit();
  }
  
  // Write all inodes as not in use and empty (all zeros)
  for(i = 0; i < DFS_INODE_MAX_NUM; i++){
    //Printf("setting up idode %d\n", i);
    //Printf("here4\n");
    inodes[i].inuse = 0;
    inodes[i].file_size = 0;
    for(j = 0; j < 10; j++){
      inodes[i].table[j] = 0;
    }
    //Printf("here3\n");
    inodes[i].translation_table_block_num = 0;  
    //Printf("here1\n");
  }

  //for every block in the fbv
  //Printf("here7\n");
  used_mask = 0x3ffff;
  for(k = 0; k < DFS_FBV_MAX_NUM_WORDS; k++){

    //mark first 18 in use
    if(k == 0){
      fbv[k] = (0 | used_mask);
    }
    else{
      fbv[k] = 0;
    }

  } 

  //Printf("filesystem init\n");
  //for(i = 0; i < DFS_FBV_MAX_NUM_WORDS; i++){
        //Printf("%x ", fbv[i]);
  //}
  //Printf("\n");

  //write the fbv to two physical blocks
  bcopy((char*)fbv, temp.data, DFS_BLOCKSIZE); 
  FdiskWriteBlock(64, &temp); //19 bc 0, 2, 3 not used
  bcopy((char*)fbv + DFS_BLOCKSIZE, temp.data, DFS_BLOCKSIZE); 
  FdiskWriteBlock(68, &temp); //20 bc 0, 2, 3 not used
  
  // Finally, setup superblock as valid filesystem and write superblock and boot record to disk: 
  // boot record is all zeros in the first physical block, and superblock structure goes into the second physical block
  
  //write zeros to block 0, 2, and 3
  bzero(temp.data, DFS_BLOCKSIZE);
 /*  FdiskWriteBlock(0, &temp); 
  FdiskWriteBlock(2, &temp); 
  FdiskWriteBlock(3, &temp); */
  disk_write_block(0, temp.data);
  disk_write_block(2, temp.data);
  disk_write_block(3, temp.data);

  //set superblock as valid and write to block 1
  sb.valid = 1;
  bcopy((char*)&sb, temp.data, sizeof(dfs_superblock)); 
  disk_write_block(1, temp.data);

  //HOW TO WRITE INODES --------------------------------?
  //write iblocks to 4-19
  count = 4;
  for(k = 0; k < DFS_INODE_MAX_NUM; k+=8){
    //Printf("%d\n", k);
    bcopy((char*)inodes + (sizeof(dfs_inode) * 8), temp.data, DFS_BLOCKSIZE); 
    FdiskWriteBlock(count, &temp); //write eight inodes at a time (1024 = 8 inodes = 4 physical blocks)
    count+=4;
  } 
  /* for(i=0; i < 60; i+=2) {
    bcopy((char*)(inodes + (i * sizeof(dfs_inode))), temp.data, 2 * sizeof(dfs_inode));
    FdiskWriteBlock(i + 4, &temp);
  }  */

  Printf("fdisk (%d): Formatted DFS disk for %d bytes.\n", getpid(), disksize);
}

//this takes in a dfs block and writes it across 4 disk blocks
int FdiskWriteBlock(uint32 blocknum, dfs_block *b) {
  // STUDENT: put your code here

  //Printf("writing blocks %d %d %d %d\n", blocknum, blocknum + 1, blocknum + 2, blocknum +3);
  
  disk_write_block(blocknum, b->data);
  disk_write_block(blocknum + 1, (b->data) + 256);
  disk_write_block(blocknum + 2, (b->data) + 512);
  disk_write_block(blocknum + 3, (b->data) + 768);
  

  return DISK_SUCCESS;
  //returns DISK_SUCCESS on success and DISK_FAIL on fail
}
