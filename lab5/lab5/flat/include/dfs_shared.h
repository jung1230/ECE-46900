#ifndef __DFS_SHARED__
#define __DFS_SHARED__

typedef struct dfs_superblock {
  // STUDENT: put superblock internals here
  uint32 valid; //valid or not
  uint32 fs_block_size; //file system block size
  uint32 inode_start_block; //total number of file system blocks
  uint32 start_inode; //starting file system block number for the array of inodes
  uint32 num_inodes; //number of inodes in the inodes array
  uint32 start_fbv; //starting file system block number for the free block vector
  uint32 isopen;
  uint32 num_fs_blocks;
} dfs_superblock;

#define DFS_BLOCKSIZE 1024  // Must be an integer multiple of the disk blocksize
#define FDISK_FBV_START 16

typedef struct dfs_block {
  char data[DFS_BLOCKSIZE];
} dfs_block;

typedef struct dfs_inode {
  // STUDENT: put inode structure internals here
  // IMPORTANT: sizeof(dfs_inode) MUST return 128 in order to fit in enough
  // inodes in the filesystem (and to make your life easier).  To do this, 
  // adjust the maximumm length of the filename until the size of the overall inode 
  // is 128 bytes.
  
  //32 + 32 + 320 + 32 + 8(76) = 1024 bits = 128 bytes
  uint32 inuse; //inuse indicator
  uint32 file_size; //size of file that this inode represents (i.e the maxmimum byte that has been written to this file)
  char filename[76]; //76 is most char we can fit in inode
  uint32 table[10]; //table of direct address translations for the first 10 virtual blocks
  uint32 translation_table_block_num; //block number of a file system block on the disk which holds a table of indirect address translations for the virtual blocks beyond the first 10

} dfs_inode;

#define DFS_MAX_FILESYSTEM_SIZE 0x1000000  // 16MB


#define DFS_FAIL -1
#define DFS_SUCCESS 1



#endif
