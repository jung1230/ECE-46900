#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "disk.h"
#include "dfs.h"
#include "files.h"

//Using DFS_BLOCKSIZE here instead of sb.blockSize because sb is
//not available in ostests.c; STUDENT should use sb.blockSize in
//dfs.c or in any place where it can be read from superblock
//but can use DFS_BLOCKSIZE here for testing purposes

int score = 0;
int total = 0;

void print_grade(int num, int points, int result)
{
  dbprintf('o', "Running test %d\n", num);

  total += points;
  if(result)
  {
    dbprintf('g', "Test %d: Success (%d points)\n", num, points);
    score += points;
  }
  else
    dbprintf('g', "Test %d: Fail (%d points)\n", num, points);
}


//Helper function to test Q2: DFS file system driver
int dfs_write_read_check(int offset, int bytes, int close)
{
  dfs_block block1, block2;
  int i;
  uint32 blocknum;

  if(offset+bytes > DFS_BLOCKSIZE)
  {
    dbprintf('o', "Error in dfs_write_read_check: last byte %d greater than DFS_BLOCKSIZE: %d\n", offset+bytes, DFS_BLOCKSIZE);
    return 0;
  }

  for(i=0; i<offset; i++)
    block1.data[i] = 0xFF;
  for(; i<offset+bytes; i++)
    block1.data[i] = i & 0xFF;
  for(; i<DFS_BLOCKSIZE; i++)
    block1.data[i] = 0xFF;
  for(i=0; i<DFS_BLOCKSIZE; i++)
    block2.data[i] = 0;

  if((blocknum = DfsAllocateBlock()) == DFS_FAIL)
  {
    dbprintf('o', "Error in dfs_write_read_check: DfsAllocateBlock failed\n");
    return 0;
  }

  if((i=DfsWriteBlock(blocknum, &block1)) != DFS_BLOCKSIZE)
  {
    dbprintf('o', "Error in dfs_write_read_check: DfsWriteBlock wrote %d bytes instead of %d\n", i, DFS_BLOCKSIZE);
    return 0;
  }

  if(close)
  {
    if(DfsCloseFileSystem() == DFS_FAIL)
    {
      dbprintf('o', "Error in dfs_write_read_check: DfsCloseFileSystem failed when close was 1\n");
      return 0;
    }
    if(DfsOpenFileSystem() == DFS_FAIL)
    {
      dbprintf('o', "Error in dfs_write_read_check: DfsOpenFileSystem failed when close was 1\n");
      return 0;
    }
 }

  if((i = DfsReadBlock(blocknum, &block2)) != DFS_BLOCKSIZE) 
  {
    dbprintf('o', "Error in dfs_write_read_check: DfsReadBlock read %d bytes instead of %d\n", i, DFS_BLOCKSIZE);
    return 0;
  }

  for(i=0; i<DFS_BLOCKSIZE; i++)
    if(block1.data[i] != block2.data[i])
    {
      dbprintf('o', "Error in dfs_write_read_check: Byte mistmatch b/w write and read, wrote [%d] = 0x%x, read [%d] = 0x%x\n", 
        i, block1.data[i], i, block2.data[i]);
      return 0;
    }

  if(DfsFreeBlock(blocknum) != DFS_SUCCESS)
  {
    dbprintf('o', "Error in dfs_write_read_check: DfsFreeBlock couldn't free the block\n");
    return 0;
  }
  
  return 1;
}

//Helper function to test Q3: Inode-based functions
int dfs_inode_write_read_check(uint32 inode, int start_byte, int num_bytes, int close)
{
  char buf1[12*DFS_BLOCKSIZE], buf2[12*DFS_BLOCKSIZE];
  int i;

  for(i=0; i<num_bytes; i++)
    buf1[i] = i & 0xFF;
  for(i=0; i<num_bytes; i++)
    buf2[i] = 0;

  if((i = DfsInodeWriteBytes(inode, (void*) buf1, start_byte, num_bytes)) != num_bytes)
  {
    dbprintf('o', "Error in dfs_inode_write_read_check: DfsInodeWriteBytes wrote %d bytes instead of %d\n", i, num_bytes);
    return 0;
  }

  if(close)
  {
    if(DfsCloseFileSystem() == DFS_FAIL)
    {
      dbprintf('o', "Error in dfs_inode_write_read_check: DfsCloseFileSystem failed when close was 1\n");
      return 0;
    }
    if(DfsOpenFileSystem() == DFS_FAIL)
    {
      dbprintf('o', "Error in dfs_inode_write_read_check: DfsOpenFileSystem failed when close was 1\n");
      return 0;
    }
  }

  if((i = DfsInodeReadBytes(inode, (void*) buf2, start_byte, num_bytes)) != num_bytes)
  {
    dbprintf('o', "Error in dfs_inode_write_read_check: DfsInodeReadBytes read %d bytes instead of %d\n", i, num_bytes);
    return 0;
  }

  for(i=0; i<num_bytes; i++)
    if(buf1[i] != buf2[i])
    {
      dbprintf('o', "Error in dfs_inode_write_read_check: Byte mistmatch b/w write and read, wrote [%d] = 0x%x, read [%d] = 0x%x\n", 
        i, buf1[i], i, buf2[i]);
      return 0;
    }

  return 1;
}

//Helper function to test Q5: file-based API
int dfs_file_write_seek_read_check(uint32 fd, int num_bytes, int seek_bytes, int where_from)
{
  char buf1[DFS_BLOCKSIZE], buf2[DFS_BLOCKSIZE];
  int i;

  for(i=0; i<num_bytes; i++)
    buf1[i] = i & 0xFF;
  for(i=0; i<num_bytes; i++)
    buf2[i] = 0;

  if((i = FileWrite(fd, (void*) buf1, num_bytes)) != num_bytes)
  {
    dbprintf('o', "Error in dfs_file_write_seek_read_check: FileWrite wrote %d bytes instead of %d\n", i, num_bytes);
    return 0;
  }

  if((i = FileSeek(fd, seek_bytes, where_from)) != FILE_SUCCESS)
  {
    dbprintf('o', "Error in dfs_file_write_seek_read_check: FileSeek returned FILE_FAIL\n");
    return 0;
  }

  if((i = FileRead(fd, (void*) buf2, num_bytes)) != num_bytes)
  {
    dbprintf('o', "Error in dfs_file_write_seek_read_check: FileRead read %d bytes instead of %d\n", i, num_bytes);
    return 0;
  }

  for(i=0; i<num_bytes; i++)
    if(buf1[i] != buf2[i])
    {
      dbprintf('o', "Error in dfs_file_write_seek_read_check: Byte mistmatch b/w write and read, wrote [%d] = 0x%x, read [%d] = 0x%x\n", 
        i, buf1[i], i, buf2[i]);
      return 0;
    }

  return 1;
}

void RunOSTests(int problem) {
  int inodenum, fsblocknum;
  int fd;

  score = total = 0;
  
  switch(problem)
  {
    case 2:
      score = total = 0;
      //Write the first byte
      print_grade(1, 4, dfs_write_read_check(0, 1, 0));

      //Write the last byte
      print_grade(2, 4, dfs_write_read_check(DFS_BLOCKSIZE-1, 1, 1));

      //Write the last (size-1) bytes
      print_grade(3, 4, dfs_write_read_check(1, DFS_BLOCKSIZE-1, 0));

      //Write an entire block
      print_grade(4, 4, dfs_write_read_check(0, DFS_BLOCKSIZE, 1));

      //Write the second half block
      print_grade(5, 4, dfs_write_read_check(DFS_BLOCKSIZE/2, DFS_BLOCKSIZE/2, 0));
    
      break;
    
    case 3:
      //DfsFilenameExists
      print_grade(1, 2, DfsInodeFilenameExists("file.txt") == DFS_FAIL);
      
      //DfsInodeOpen
      print_grade(2, 2, ((inodenum = DfsInodeOpen("file.txt")) != DFS_FAIL));

      //DfsInodeAllocateVirtualBlock and DfsInodeTranslateVirtualToFilesys
      print_grade(3, 3, ((fsblocknum = DfsInodeAllocateVirtualBlock(inodenum, 0)) != DFS_FAIL));
      print_grade(4, 3, fsblocknum == DfsInodeTranslateVirtualToFilesys(inodenum, 0));
     
      //DfsInodeReadBytes and DfsInodeWriteBytes
      //Write first block
      print_grade(5, 3, dfs_inode_write_read_check(inodenum, 0, DFS_BLOCKSIZE, 0));

      //Write second and third blocks
      print_grade(6, 3, dfs_inode_write_read_check(inodenum, DFS_BLOCKSIZE, 2*DFS_BLOCKSIZE, 0));

      //Write first (zero-indexed) byte in first block
      print_grade(7, 3, dfs_inode_write_read_check(inodenum, 1, 1, 1));

      //Write 11 blocks (uses indirect blocks)
      print_grade(8, 5, dfs_inode_write_read_check(inodenum, 0, 11*DFS_BLOCKSIZE, 0));

      //Write until a non-boundary byte
      print_grade(9, 4, dfs_inode_write_read_check(inodenum, 0, DFS_BLOCKSIZE+1, 1));

      //printf("here 101010101010100101\n");

      //DfsInodeDelete
      print_grade(10, 2, DfsInodeDelete(inodenum) == DFS_SUCCESS);

      //printf("here 202002200202\n");
      
      break;

    case 5:
      print_grade(1, 2, (fd = FileOpen("file.txt", "w")) != FILE_FAIL);
      print_grade(2, 2, FileRead(fd, &inodenum, 4) == FILE_FAIL);
      print_grade(3, 2, FileClose(fd) == FILE_SUCCESS);

      fd = FileOpen("file.txt", "rw");
      
      //Write first two bytes, seek from END
      print_grade(4, 4, dfs_file_write_seek_read_check(fd, 2, -2, FILE_SEEK_END));

      //Write DFS_BLOCKSIZE bytes, seek with SET
      print_grade(5, 4, dfs_file_write_seek_read_check(fd, DFS_BLOCKSIZE, 2, FILE_SEEK_SET));

      //Write DFS_BLOCKSIZE bytes, seek from CUR
      print_grade(6, 4, dfs_file_write_seek_read_check(fd, DFS_BLOCKSIZE, -1*DFS_BLOCKSIZE, FILE_SEEK_CUR));

      FileClose(fd);
      print_grade(7, 2, FileDelete("file.txt") == FILE_SUCCESS);
      
      break;

    default:
      //STUDENT: Add your testcases here
  }

  dbprintf('g', "Q%d: %d/%d points\n------------------------------------------------------------------------\n", problem, score, total);
}
