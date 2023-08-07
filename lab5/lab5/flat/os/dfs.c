#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

static dfs_inode inodes[120]; // all inodes
static dfs_superblock sb; // superblock
static uint32 fbv[DFS_BLOCKSIZE *2 / 4]; // Free block vector

static uint32 negativeone = 0xFFFFFFFF;
static inline uint32 invert(uint32 n) { return n ^ negativeone; }

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.
static lock_t lock_fbv;
static lock_t lock_inode;

// STUDENT: put your file system level functions below.
// Some skeletons are provided. You can implement additional functions.

///////////////////////////////////////////////////////////////////
// Non-inode functions first
///////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsModuleInit is called at boot time to initialize things and
// open the file system for use.
//-----------------------------------------------------------------

void DfsModuleInit() { //dobule check
    // You essentially set the file system as invalid and then open 
    // using DfsOpenFileSystem().
    //printf("enter dfsmoduleinit\n");
    
    DfsInvalidate();
    DfsOpenFileSystem();

    lock_fbv = LockCreate();
    lock_inode = LockCreate();  

    //printf("exit dfsmoduleinit\n");

}

//-----------------------------------------------------------------
// DfsInavlidate marks the current version of the filesystem in
// memory as invalid.  This is really only useful when formatting
// the disk, to prevent the current memory version from overwriting
// what you already have on the disk when the OS exits.
//-----------------------------------------------------------------

void DfsInvalidate() { //double check
// This is just a one-line function which sets the valid bit of the 
// superblock to 0.
    sb.valid = 0;
}

//-------------------------------------------------------------------
// DfsOpenFileSystem loads the file system metadata from the disk
// into memory.  Returns DFS_SUCCESS on success, and DFS_FAIL on 
// failure.
//-------------------------------------------------------------------

int DfsOpenFileSystem() { //dobule cehck
    
    disk_block temp;
    dfs_block dfs_temp;
    int i;
    int j;
    int first_fbv_block;

    //printf("enter dfsopenfilesystem\n");

    //Basic steps:

    // Check that filesystem is not already open
    if(sb.isopen != 0){
        return DFS_FAIL;
    }

    // Read superblock from disk.  Note this is using the disk read rather 
    // than the DFS read function because the DFS read requires a valid 
    // filesystem in memory already, and the filesystem cannot be valid 
    // until we read the superblock. Also, we don't know the block size 
    // until we read the superblock, either.
    if(DiskReadBlock (1, &temp) == DISK_FAIL){
        //printf("readblock fail\n");
        return DFS_FAIL;
    }

    // Copy the data from the block we just read into the superblock in memory
    bcopy(temp.data, (char*)&sb, sizeof(dfs_superblock));

    //printf("superblock test: %d\n", sb.disk_block_size);

    // All other blocks are sized by virtual block size:

    first_fbv_block = sb.inode_start_block + (sb.num_inodes / 2);

    //read inodes
    for(i = sb.inode_start_block; i < first_fbv_block; i++) {

        //read 2 inodes at a time
        DiskReadBlock(i, &temp); 

        //copy them from buffer into inodes array
        bcopy(temp.data, (char*)inodes + ( (i - sb.inode_start_block) * 2), 2 * sizeof(dfs_inode)); 
    } 
    
    //printf("reading free block vector\n");

    // Read free block vector
    for(i = first_fbv_block; i < first_fbv_block + ((2 * sb.fs_block_size) / sizeof(disk_block)); i++) {
        
        //read 1/8 of fbv from disk (256 out of 2048)
        if(DiskReadBlock(i, &temp) == DISK_FAIL){
            printf("openfs fail 1\n");
            return DFS_FAIL;
        }
        
        //copy from the buffer to the fbv array
        bcopy(temp.data, (char*)fbv + ((i - first_fbv_block) * sizeof(temp)), sizeof(temp));
    }
    
    /* printf("after reading fbv\n");
    for(i = 0; i < (sb.fs_block_size *2 / 4); i++){
        printf("%d ", fbv[i]);
    }
    printf("\n"); */

    // Change superblock to be invalid, write back to disk, then change 
    // it back to be valid in memory

    //invalidate superblock
    DfsInvalidate();

    //copy the superblock to buffer
    bcopy((char*)&sb, temp.data, sizeof(disk_block));

    //write the super block to disk 
    if(DiskWriteBlock(1, &temp) == DFS_FAIL){
        printf("openfs fail 2\n");
        return DFS_FAIL;
    }

    //change it back to valid
    sb.valid = 1;

    //mark it as open
    sb.isopen = 1;

    //printf("exit dfsopenfilesystem\n");

    return DFS_SUCCESS;
}


//-------------------------------------------------------------------
// DfsCloseFileSystem writes the current memory version of the
// filesystem metadata to the disk, and invalidates the memory's 
// version.
//-------------------------------------------------------------------

int DfsCloseFileSystem() { //dobule checked
    int i;
    disk_block temp;
    dfs_block dfs_temp;
    int count;
    int first_fbv_block;

    //printf("enter dfsclosefilesystem\n");

    // Check that filesystem is already closed
    if(sb.isopen == 0){
        return DFS_FAIL;
    }
    
    // Check that filesystem is valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //close the file system
    sb.isopen = 0;

    //printf("here1\n");

    //copy superblock into buffer
    bcopy((char*)&sb, temp.data, sizeof(dfs_superblock));

    //printf("here2\n");
    
    //write superblock to disk
    if(DiskWriteBlock (1, &temp) == DISK_FAIL){
        //printf("close fs fail 1\n");
        return DFS_FAIL;
    }
    //printf("here3\n");

    first_fbv_block = sb.inode_start_block + (sb.num_inodes / 2);
    
    //write inodes
    for(i = sb.inode_start_block; i < first_fbv_block; i++) {
        
        //copy 2 inodes into buffer
        bcopy((char*)inodes + ( (i - sb.inode_start_block) * 2), temp.data, 2 * sizeof(dfs_inode));
        
        //write 2 inodes at a time
        DiskWriteBlock(i, &temp); 
    } 

    //printf("here4\n");

    // write free block vector
    for(i = first_fbv_block; i < first_fbv_block + ((2 * sb.fs_block_size) / sizeof(disk_block)); i++) {

        //copy 256 bytes of fbv into buffer
        bcopy((char*)fbv + ((i - first_fbv_block) * sizeof(temp)), temp.data, sizeof(temp));
        
        //write 1/8 of fbv from disk (256 / 2048)
        DiskWriteBlock(i, &temp); 
    }
    
    //invalidate the superblock
    DfsInvalidate();

    //printf("exit dfsclosefilesystem\n");

    return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsAllocateBlock allocates a DFS block for use. Remember to use 
// locks where necessary.
//-----------------------------------------------------------------

uint32 DfsAllocateBlock() { //double checked
    // Check that file system has been validly loaded into memory
    // Find the first free block using the free block vector (FBV), mark it in use
    // Return handle to block

    int i, j;
    int curbit;
    int gotblock;
    gotblock = -1;

    //printf("enter dfsallocateblock\n");

    /* //printf("fbv in allocblock\n");
    for(i = 0; i < (sb.fs_block_size *2 / 4); i++){
        printf("%x ", fbv[i]);
    }
    printf("\n"); */


    // Check that file system has been validly loaded into memory
    if(sb.valid == 0){
        //printf("fail here 1\n");
        return DFS_FAIL;
    }

    //check that the filesystem has been opened
    if(sb.isopen == 0){
        //printf("fail here 2\n");
        return DFS_FAIL;
    }

    //get lock for fbv
    if(LockHandleAcquire(lock_fbv) != SYNC_SUCCESS){
        //printf("fail here 3\n");
        return DFS_FAIL;
    }

    //go though all the entries in the fbv
    for(i = 0; i < (sb.fs_block_size * 2 / 4); i++) {

        //if we have already got a block, stop looking
        if(gotblock != -1){
            break;
        }

            //go through each bit
            for(j = 0; j < 32; j++) {

                //get curent bit
                curbit = (0x1 << j) & fbv[i];

                //if the current bit is a zero (its available)
                if(curbit == 0) {
                    
                    //flip it to a 1 to claim it
                    fbv[i] |= (0x1 << j);

                    //printf("i: %d\n", i);
                    //printf("j: %d\n", j);

                    //calculate the blocknumber we got and quit the bit loop
                    gotblock = (32 * i + j);

                    //printf("gotblock: %d\n", gotblock);

                    break;
        
                }
            }
    }

    //release lock
    if(LockHandleRelease(lock_fbv) != SYNC_SUCCESS) {
        //printf("fail here 4\n");
        return DFS_FAIL;
    }

    //printf("exit dfsalllcoate blcok with block %d\n", gotblock);

    return gotblock;

}


//-----------------------------------------------------------------
// DfsFreeBlock deallocates a DFS block.
//-----------------------------------------------------------------

int DfsFreeBlock(uint32 blocknum) { //double checked

    int entry_idx;
    int bit_idx;
    int actual_blocknum;

    //printf("enter dfsfreeblock\n");

    //use the file system block to find its fbv location
    actual_blocknum = blocknum; //- (sb.start_fbv + 2);
    entry_idx = actual_blocknum / 32;
    bit_idx = actual_blocknum % 32;

    // check if superblock is valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //check that the filesystem has been opened
    if(sb.isopen == 0){
        return DFS_FAIL;
    }

    //get lock for fbv
    if(LockHandleAcquire(lock_fbv) != SYNC_SUCCESS){
        return DFS_FAIL;
    }

    //flip the bit back to a 0
    fbv[entry_idx] ^= (0x1 << bit_idx);

    //release the lock
    if(LockHandleRelease(lock_fbv) != SYNC_SUCCESS){
        return DFS_FAIL;
    }

    //printf("exit dfsfreeblock\n");

    return DFS_SUCCESS;

}


//-----------------------------------------------------------------
// DfsReadBlock reads an allocated DFS block from the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to read from it.  Returns DFS_FAIL
// on failure, and the number of bytes read on success.  
//-----------------------------------------------------------------

int DfsReadBlock(uint32 blocknum, dfs_block *b) { //dobule cheked

    int entry_idx;
    int bit_idx;
    int actual_blocknum;
    disk_block temp;
    int i;
    int bytes_read;

    actual_blocknum = blocknum; //- (sb.start_fbv + 2); //???????????????
    entry_idx = actual_blocknum / 32;
    bit_idx = actual_blocknum % 32;

    //printf("blocknum: %d\n", blocknum);
    //printf("actual blocknum: %d\n", actual_blocknum);
    //printf("entry idx: %d\n", entry_idx);
    //printf("bit idx: %d\n", bit_idx);
    //printf("fbv[entry]: %x\n", fbv[entry_idx]);

    //printf("enter dfsreadblock\n");

    //if the dfs blocknum was above the max dfs blocknum
    if(blocknum > sb.num_fs_blocks){
        //printf("readblock fail because too big of block\n");
        return DFS_FAIL;
    }

    /* printf("fbv in readblock\n");
    for(i = 0; i < (sb.fs_block_size *2 / 4); i++){
        printf("%x ", fbv[i]);
    }
    printf("\n"); */

    //("here11\n");
    
    //if the fbv entry bit for the block was not set
    if((fbv[entry_idx] & (0x1 << bit_idx)) == 0){
        //printf("HEREHEREHERE\n");
        return DFS_FAIL;
    }

    //printf("here12\n");

    //printf("enter loop\n");
    //printf("disk block size: %d\n", DISK_BLOCKSIZE);
    bytes_read = 0;

    //for however many physical blocks are in a disk block (4 for 1024 and 256)
    for(i = 0; i < (sb.fs_block_size / sizeof(disk_block)); i++){

        //printf("insid eloop\n");
        
        //read fall 4 disk blocks that correspond to the dfs block
        //printf("reading phyiscla block: %d\n", (blocknum * sb.fs_block_size / sb.disk_block_size) + i);
        
        if(DiskReadBlock ((blocknum * (sb.fs_block_size / sizeof(disk_block))) + i, &temp) == DISK_FAIL){
            //printf("here2\n");
            return DFS_FAIL;
        }

        //copy from the buffer into the correct location of the destination block (0, then 256, then 512, then 768)
        bcopy((char *)temp.data, (char *)b->data + (i * sizeof(temp)), sizeof(temp));
    
        //increment bytes read by the size of a disk block
        bytes_read += sizeof(temp);
    }

    //printf("exit dfsreadblock\n");

    //printf("bytes read: %d\n", bytes_read);

    return bytes_read;
}


//-----------------------------------------------------------------
// DfsWriteBlock writes to an allocated DFS block on the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to write to it.  Returns DFS_FAIL
// on failure, and the number of bytes written on success.  
//-----------------------------------------------------------------

int DfsWriteBlock(uint32 blocknum, dfs_block *b){ //double checked
    int entry_idx;
    int bit_idx;
    int actual_blocknum;
    disk_block temp;
    int i;
    int bytes_written;

    //printf("enter dfswriteblock\n");

    actual_blocknum = blocknum;// - (sb.start_fbv + 2);
    entry_idx = actual_blocknum / 32;
    bit_idx = actual_blocknum % 32;
    
    //printf("entry_idx: %d\n", entry_idx);
    //printf("bit idx: %d\n", bit_idx);

    //printf("fbv: %x\n", fbv[entry_idx]);
    /* if((fbv[entry_idx] & (0x1 << bit_idx)) == 0){
        printf("here111111111111111\n");
        return DFS_FAIL;
    } */

    //return fail if superblock invalid
    if(sb.valid == 0){
        //printf("here 66666\n");
        return DFS_FAIL;
    }

    //check that fs is open
    if(sb.isopen == 0){
        //printf("here 777777\n");
        return DFS_FAIL;
    }

    bytes_written = 0;

    //for however many physical blocks are in a disk block (4 for 1024 and 256)
    for(i = 0; i < (sb.fs_block_size / sizeof(disk_block)); i++){

        //copy from the source into the buffer
        bcopy(b->data + (i * sizeof(disk_block)), temp.data, sizeof(disk_block));
        
        //write the buffer to disk
        if(DiskWriteBlock ((blocknum * sb.fs_block_size / sizeof(disk_block)) + i, &temp) == DISK_FAIL){
            //printf("here212222222222222222\n");
            return DFS_FAIL;
        }

        //incremement write counter by disk block size
        bytes_written += sizeof(disk_block);
    }

    //printf("exit dfswriteblock\n");

    return bytes_written;

}


////////////////////////////////////////////////////////////////////////////////
// Inode-based functions
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsInodeFilenameExists looks through all the inuse inodes for 
// the given filename. If the filename is found, return the handle 
// of the inode. If it is not found, return DFS_FAIL.
//-----------------------------------------------------------------

uint32 DfsInodeFilenameExists(char *filename) { //double checked
    int i;
    int found;
    found = -1;

    //printf("filename we are searching for: %s\n", filename);
    //printf("enter dfsinodefilenameexists\n");


    //check for open
    if(sb.isopen == 0){
        return DFS_FAIL;
    }

    //check sb is valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //go through all the inodes
    for(i = 0; i < sb.num_inodes; i++){

        //if the inode is in use
        if(inodes[i].inuse != 0){
            //printf("inode in use\n");
            
            //printf("filename in inodeexists: %s\n", inodes[i].filename);

            //check the filenames and set found if they match
            if(dstrncmp(filename, inodes[i].filename, (sizeof(inodes[i].filename) / sizeof(char))) == 0){
                //printf("found it\n");
                found = i;
                break;
            }
        }
    }

    //if we checked all the inodes and didnt find the filename
    if(found == -1){
        //printf("filenamexists fail here\n");
        return DFS_FAIL;
    }

    //printf("exit dfsinodefilenameexists\n");

    return found;
}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

uint32 DfsInodeOpen(char *filename) { //double checked
    uint32 inode_handle;
    int i;

    //check valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //check open
    if(sb.isopen == 0 ){
        return DFS_FAIL;
    }

    //printf("enter dfsinodeopen\n");

    //check if the filename already exists, return the handle to its inode
    inode_handle = DfsInodeFilenameExists(filename);
    if(inode_handle != DFS_FAIL){
        //printf("inodeopen fail 1\n");
        return inode_handle;
    }

    //get the inode lock
    if(LockHandleAcquire(lock_inode) != SYNC_SUCCESS) {
        //printf("inodeopen fail 2\n");
        return DFS_FAIL;
    }

    
    inode_handle = -1;

    //go through all the inodes
    for(i = 0; i < sb.num_inodes; i++){

        //if one is not in use, take it
        if(inodes[i].inuse == 0){
            inodes[i].inuse = 1;
            inode_handle = i;
            break;
        }
    }

    //release inode lock
    if(LockHandleRelease(lock_inode) != SYNC_SUCCESS) {
        //printf("inodeopen fail 3\n");
        return DFS_FAIL;
    }

    //if we didnt find a free inode, return fail
    if(inode_handle == -1){
        //printf("inodeopen fail 4\n");
        return DFS_FAIL;
    }

    //if we did find one, copy the filename into the inode
    dstrcpy(inodes[inode_handle].filename, (const char*)filename);

    //printf("exit dfsinodeopen success with inode handle: %d\n", inode_handle);

    return inode_handle;

}


//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int DfsInodeDelete(uint32 handle) { //double checked
    
    uint32 addr_table[(sb.fs_block_size / sizeof(uint32))];
    dfs_block temp;
    int i;
    int j;

    //printf("enter dfsinodedelete\n");

    //if the inode was not in use, fail
    if(inodes[handle].inuse == 0){
        return DFS_FAIL;
    }

    //check valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //check open
    if(sb.isopen == 0 ){
        return DFS_FAIL;
    }
    
    //printf(" dfsinodedelete 1\n");

    //go through direct table
    for(j = 0; j < 10; j++){

        //if the table entry exists
        if(inodes[handle].table[j] != 0){

            //free the block that the table entry contains
            if(DfsFreeBlock(inodes[handle].table[j]) == DFS_FAIL) {
                return DFS_FAIL;
            }

            //set the entry back to zero
            inodes[handle].table[j] = 0;
        }
    }
    
    
    //if the translation table exists
    if(inodes[handle].translation_table_block_num != 0){
        
        //read the translation table block from disk
        if(DfsReadBlock(inodes[handle].translation_table_block_num, &temp) == DFS_FAIL) {
            return DFS_FAIL;
        }

        //printf(" dfsinodedelete 2\n");

        //copy the table into a local table so we can parse it
        bcopy(temp.data, (char*)addr_table, sb.fs_block_size);

        //printf(" dfsinodedelete 3\n");

        //printf("fs block size  / sizeof(uint): %d\n", (sb.fs_block_size / sizeof(uint32)));

        //go through indirect table        
        for(i = 0; i < (sb.fs_block_size / sizeof(uint32)); i++){

            //printf("i: %d\n", i);

            //if the table entry exists
            if(addr_table[i] != 0){
                
                //printf("freeing block\n");
                
                //free the block in the entry
                if(DfsFreeBlock(addr_table[i]) == DFS_FAIL) {
                    return DFS_FAIL;
                }
            }
        }

        //printf(" dfsinodedelete 4\n");

        //free the indirect table itself
        if(DfsFreeBlock(inodes[handle].translation_table_block_num) == DFS_FAIL) {
            return DFS_FAIL;
        }

    }

    //printf(" dfsinodedelete 6\n");

    //reset all the inode fields back to zero
    inodes[handle].file_size = 0;
    inodes[handle].translation_table_block_num = 0;
    bzero(inodes[handle].filename, (sizeof(inodes[handle].filename) / sizeof(char)));

    //printf(" dfsinodedelete 7\n");

    //get the lock so we can edit the inuse field
    if(LockHandleAcquire(lock_inode) != SYNC_SUCCESS) {
        return DFS_FAIL;
    }

    //printf(" dfsinodedelete 8\n");

    //mark the inode as free again
    inodes[handle].inuse = 0;

    //printf(" dfsinodedelete 9\n");

    //release the lock
    if(LockHandleRelease(lock_inode) != SYNC_SUCCESS) {
        return DFS_FAIL;
    }

    //printf(" dfsinodedelete 10\n");

    //printf("exit dfsinodedelete\n");

    return DFS_SUCCESS;

}


//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {

    dfs_block temp;
    int cur_byte; 
    int read_start;
    int read_end;
    int bytes_read;
    uint32 temp_handle;
    int was_first;
    int was_last;

    //if the inode isnt in use
    if(inodes[handle].inuse == 0){
        return DFS_FAIL;
    }

    //if the sb isnt valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //check sb is open
    if(sb.isopen == 0){
        return DFS_FAIL;
    }

    //set the current byte to where we will start the read and init bytes read
    cur_byte = start_byte;
    bytes_read = 0;

    //while the current byte is lower than the last byte we will need to read
    while(cur_byte < (start_byte + num_bytes)){
        
        //init the end as the end of current block
        read_end = sb.fs_block_size;
        read_start = 0;
        was_first = 0;
        was_last = 0;
        
        //read a block in
        if (DfsReadBlock(DfsInodeTranslateVirtualToFilesys(handle, cur_byte / sb.fs_block_size), &temp) == DFS_FAIL){
			return DFS_FAIL;
		} 

        //check if it was the first or last read
        if(cur_byte == start_byte){
            was_first = 1;
        }
        if(cur_byte + (read_end - read_start) > (start_byte + num_bytes)){
            was_last = 1;
        }

        //if it was the first read
        if(was_first == 1){

            ///set the relative start in the block
            read_start = cur_byte % sb.fs_block_size;
        }

        //if it was the last read
        if (was_last == 1){ 

            //set the end to the relative position wiuthin the end block
            read_end = ((start_byte + num_bytes) % sb.fs_block_size); 
        }

        //copy over bytes using bounds above
        bcopy(temp.data + read_start, mem + bytes_read, (read_end - read_start));

        //increment the couunters 
        bytes_read += (read_end - read_start); 
        cur_byte += (read_end - read_start); 
        
    }
    return bytes_read;
}


//-----------------------------------------------------------------
// DfsInodeWriteBytes writes num_bytes from the memory pointed to 
// by mem to the file represented by the inode handle, starting at 
// virtual byte start_byte. Note that if you are only writing part 
// of a given file system block, you'll need to read that block 
// from the disk first. Return DFS_FAIL on failure and the number 
// of bytes written on success.
//-----------------------------------------------------------------

int DfsInodeWriteBytes(uint32 handle, void *mem, int start_byte, int num_bytes) { //double checked

    int cur_byte;
    int bytes_written;
	int write_start;
    int write_len; // number of bytes to write
	uint32 alloc_handle; // virtual block number to write into
	dfs_block temp; // place to store the block to write
    int was_first;
    int was_last;

    //printf("\nhandle: %d\n", handle);

    //printf("num bytes: %d\n", num_bytes);

    if(inodes[handle].inuse == 0){
        //printf("fail 1\n");
		return DFS_FAIL;
	}
	if(sb.valid == 0){
        //printf("fail 2\n");
		return DFS_FAIL;
	}
    if(sb.isopen == 0){
        //printf("fail 3\n");
		return DFS_FAIL;
	}
	
    cur_byte = start_byte;
	bytes_written = 0; 

    //while we have not written all of the bytes
	while (bytes_written < num_bytes) {

		write_start = 0; 
        write_len = sb.fs_block_size;
        was_first = 0;
        was_last = 0;
        
        //check if it was the first write
        if(cur_byte == start_byte){
            was_first = 1;
        }

        //check if ti was the last write
        if((cur_byte + write_len) > (start_byte + num_bytes)){
            was_last = 1;
        }

        //first write
		if (was_first == 1) {   
            
            //printf("cur bytes: %d\n", cur_byte);
			
            //set start to the byte relative to the block its in
            write_start = cur_byte % sb.fs_block_size;
            
            //printf("write start: %d\n", write_start);

            //set the length of the write as the distance from this relative location to the end of the block
			write_len = sb.fs_block_size - write_start; 
		}

        //last write
		if (was_last == 1) {   
			
            //if it was the first write also
            if(was_first == 1){

                //set the write length equal, find the end relative to the block and subtract the start relative to the block
                write_len = ((start_byte + num_bytes) % sb.fs_block_size) - write_start;
            }
            //if it was not the first write
            else{

                //set the end equal to its relative position in its block
                write_len = ((start_byte + num_bytes) % sb.fs_block_size); 
            } 
            
        }

        //copy the bytes to write into the buffer
        bcopy(mem + bytes_written, temp.data + write_start, write_len);

        //printf("virtual blocknum: %d\n", cur_byte / sb.fs_block_size);

        //get a new inode
        alloc_handle = DfsInodeAllocateVirtualBlock(handle, cur_byte / sb.fs_block_size);
        
        if (alloc_handle == DFS_FAIL){
            //printf("fail 7\n");
            return DFS_FAIL;
        }

        //write the buffer to the new inode block
        if (DfsWriteBlock(alloc_handle, &temp) == DFS_FAIL) {
            //printf("fail 8\n");
            return DFS_FAIL;
        }


        //increase position and bytes written
        cur_byte += write_len; 
        bytes_written += write_len;
	}

	//update fiel size
	if (inodes[handle].file_size < (start_byte + bytes_written)){
		inodes[handle].file_size = start_byte + bytes_written;
    }

	return bytes_written; 

}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeFilesize(uint32 handle) { //double checked

    int file_size;

    //printf("enter dfsinodefilesize\n");
    
    //if the inode is not in use, return fail
    if(inodes[handle].inuse == 0){
        return DFS_FAIL;
    }

    if(sb.valid == 0){
        return DFS_FAIL;
    }

    if(sb.isopen == 0){
        return DFS_FAIL;
    }

    //get the file size
    file_size = inodes[handle].file_size;

    //printf("exit dfsinodefilesize\n");

    return file_size;
    
}


//-----------------------------------------------------------------
// DfsInodeAllocateVirtualBlock allocates a new filesystem block 
// for the given inode, storing its blocknumber at index 
// virtual_blocknumber in the translation table. If the 
// virtual_blocknumber resides in the indirect address space, and 
// there is not an allocated indirect addressing table, allocate it. 
// Return DFS_FAIL on failure, and the newly allocated file system 
// block number on success.
//-----------------------------------------------------------------

uint32 DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) { //dobule checked

    int table_block;
    int new_alloc_block;
    dfs_block temp;
    uint32 table[sb.fs_block_size / sizeof(uint32)];

    int indirect_index;
    indirect_index = 0;

    if(inodes[handle].inuse == 0){
        return DFS_FAIL;
    }

    if(sb.valid == 0){
        return DFS_FAIL;
    }

    if(sb.isopen == 0){
        return DFS_FAIL;
    }

    //printf("enter dfsinodeallocatevirutalblock\n");

    //if the blocknum is not in the direct table
    if(virtual_blocknum > 9){

        //set the index relative to the direct table (theres 10 in the direct table)
        indirect_index = virtual_blocknum - 10;

        //if the translation table hasnt been allocated
        if(inodes[handle].translation_table_block_num == 0){
            
            //printf("here1\n");
            
            //allocate a block for it
            table_block = DfsAllocateBlock();
            if(table_block == DFS_FAIL){
                return DFS_FAIL;
            }
            
            //save that block number in the inode
            inodes[handle].translation_table_block_num = table_block;

            //clear buffer
            bzero(temp.data, sb.fs_block_size);

            //write buffer to that block to zero it
            if(DfsWriteBlock(inodes[handle].translation_table_block_num, &temp) == DFS_FAIL) {
                return DFS_FAIL;
            }
            
        }

        //printf("handle = %d\n", handle);
        //printf("blocknum being passed to readblock = %d\n", inodes[handle].translation_table_block_num);
        
        //read translation table bc it will be guaranteed to exist now
        if(DfsReadBlock(inodes[handle].translation_table_block_num, &temp) == DFS_FAIL) {
            //printf("readblock fail 1\n");
            return DFS_FAIL;
        }

        //copy the table from the buffer
        bcopy(temp.data, (char*)table, sb.fs_block_size);

        //printf("here2\n");

        //allocate a new block for the table
        new_alloc_block = DfsAllocateBlock();
        if(new_alloc_block == DFS_FAIL){
            return DFS_FAIL;
        }

        //put the blocknum into the table
        table[indirect_index] = new_alloc_block; 

        //put the tableback into buffer
        bcopy((char*)table, temp.data, sb.fs_block_size);
        
        //write the buffer
        if(DfsWriteBlock(inodes[handle].translation_table_block_num, &temp) == DFS_FAIL) {
            return DFS_FAIL;
        } 

        return new_alloc_block;
    }

    //printf("here3\n");

    //if the blocknum was in the direct table, just allocate a block
    new_alloc_block = DfsAllocateBlock();
    if(new_alloc_block == DFS_FAIL){
            return DFS_FAIL;
    }

    //save the new block in the direct table
    inodes[handle].table[virtual_blocknum] = new_alloc_block;

    //printf("exit dfsinodeallocatevirutalblock\n");

    return new_alloc_block;

}



//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) { //double checked

    dfs_block temp;
    uint32 table[sb.fs_block_size / sizeof(uint32)];
    int indirect_index;
    
    //printf("enter dfstranslate\n");

    //check if it is inuse
    if(inodes[handle].inuse == 0){
        return DFS_FAIL;
    }

    //check valid
    if(sb.valid == 0){
        return DFS_FAIL;
    }

    //check open
    if(sb.isopen == 0){
        return DFS_FAIL;
    }

    indirect_index = -1;

    //if the blocknum is not in the direct table
    if(virtual_blocknum > 9){

        //set the index relative to the direct table (theres 10 in the direct table)
        indirect_index = virtual_blocknum - 10;
        
        //read indirect table to buffer
        if(DfsReadBlock(inodes[handle].translation_table_block_num, &temp) == DFS_FAIL){
            return DFS_FAIL;
        }

        //copy table from buffer to array
        bcopy(temp.data, (char *)table, sb.fs_block_size);

        //read and return the entry from the table
        return table[indirect_index];
    }

    //printf("exit dfstranslate\n");

    //if it was in the direct table, just straight up return it
    return inodes[handle].table[virtual_blocknum];

}
