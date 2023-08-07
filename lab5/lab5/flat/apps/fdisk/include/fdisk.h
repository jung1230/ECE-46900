#ifndef __FDISK_H__
#define __FDISK_H__

typedef unsigned int uint32;

#include "dfs_shared.h" // This gets us structures and #define's from main filesystem driver

#define FDISK_INODE_BLOCK_START 1 // Starts after super block (which is in file system block 0, physical block 1)
#define FDISK_INODE_NUM_BLOCKS 15 // Number of file system blocks to use for inodes

//LAB 5
#define FDISK_NUM_INODES 120
#define FDISK_FBV_BLOCK_START 16

#define FDISK_BOOT_FILESYSTEM_BLOCKNUM 0 // Where the boot record and superblock reside in the filesystem

#ifndef NULL
#define NULL (void *)0x0
#endif

//STUDENT: define additional parameters here, if any
#define DFS_FBV_MAX_NUM_WORDS 1024 * 2 / 4
#define DFS_INODE_MAX_NUM 120
#define DISK_BLOCKSIZE 256
#define PHYS_DISK_SIZE 16777216

#endif
