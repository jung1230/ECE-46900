#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

//array of all file handles
static file_descriptor fd_list[FILE_MAX_OPEN_FILES];

//lock for files
static lock_t file_lock;

static int lock_initialized;

// STUDENT: put your file-level functions here
uint32 FileOpen(char *filename, char *mode){ //double chedked
    char *r;
    char *w;
    char *rw;
    int i;
    int filenum;
    uint32 alloc_handle;

    //printf("enter fileopen\n");

    //if the lock hasnt been initialized, create it
    if(lock_initialized != 999){
        file_lock = LockCreate();
        lock_initialized = 999;
    }

    //get the lock
    if(LockHandleAcquire(file_lock) != SYNC_SUCCESS) {
        return FILE_FAIL;
    }

    //go through the files
    for(i = 0; i < FILE_MAX_OPEN_FILES; i++){
        
        //if the file was in use
        if(fd_list[i].mode != 0){

            //if the filenames match
            if(dstrncmp(fd_list[i].filename, filename, (sizeof(fd_list[i].filename) / sizeof(char))) == 0) {
                
                //return fail because it was already in use
                return FILE_FAIL;
            }
        }
    }

    //now we go back through and find an available file 
    for(filenum = 0; filenum < FILE_MAX_OPEN_FILES; filenum++){
        
        //if the file mode is 0 (not in use)
        if(fd_list[filenum].mode == 0) {

            //copy the filename into it
            dstrcpy(fd_list[filenum].filename, filename);

            //get an inode for the file
            alloc_handle = DfsInodeOpen(filename);

            //if the inode open failed, return fail
            if(alloc_handle == DFS_FAIL){
                return FILE_FAIL;
            }
            
            //if the handle successed, put it in the file array
            fd_list[filenum].inode_handle = alloc_handle;

            //if it was rw, set mode to 3, w to 2, r to 1
            if((mode[0] == 'r') && (mode[1] == 'w')){
                fd_list[filenum].mode = 3;
            }
            else if(mode[0] == 'r'){
                fd_list[filenum].mode = 1;
            }
            else if(mode[0] == 'w'){
                fd_list[filenum].mode = 2;
            }

            //release the lock
            if(LockHandleRelease(file_lock) != SYNC_SUCCESS) {
                return FILE_FAIL;
            }

            //return file handle
            return filenum;
        }
    }

    //release the lock
    if(LockHandleRelease(file_lock) != SYNC_SUCCESS) {
        return FILE_FAIL;
    }

    //printf("exit fileopen\n");

    return FILE_FAIL;
}

int FileClose(int handle){ //douvle checked
    //printf("enter fileclose\n");
    
    //release the lock
    if(LockHandleAcquire(file_lock) != SYNC_SUCCESS) {
        return FILE_FAIL;
    }
    
    fd_list[handle].mode = 0;

    //release the lock
    if(LockHandleRelease(file_lock) != SYNC_SUCCESS) {
        return FILE_FAIL;
    }

    return FILE_SUCCESS;
}

int FileRead(int handle, void *mem, int num_bytes){

    int bytes_to_read;
    int bytes_read;
    int filesize;
    //printf("enter fileread\n");

    
    //if it was not r or rw, return fail
    if((fd_list[handle].mode != 1) && (fd_list[handle].mode != 3)) {
        return FILE_FAIL;
    }

    //if they wanted to read too many bytes, return fail
    if(num_bytes > 4096) {
        return FILE_FAIL;
    }

    //return fail if reached eof
    if(fd_list[handle].eof) {
        return FILE_FAIL;
    }

    filesize = DfsInodeFilesize(fd_list[handle].inode_handle);
    //if we will reach eof
    if(filesize < (num_bytes + fd_list[handle].position)){
        
        //set eof bit and change bytes read to the remaining number of bytes
        fd_list[handle].eof = 1;
        bytes_to_read = filesize - fd_list[handle].position;
    }
    //if we didnt reach eof
    else{
        
        //set bytes read to passed in number
        bytes_to_read = num_bytes;
    }

    //actually read bytes
    bytes_read = DfsInodeReadBytes(fd_list[handle].inode_handle, mem, fd_list[handle].position, bytes_to_read);
    if(bytes_read == DFS_FAIL) {
        return FILE_FAIL;
    }   
    
    //move position by the number of bytes read
    fd_list[handle].position += bytes_read;

    //if we didnt reach eof, return bytes read
    if(bytes_read == num_bytes){
        return bytes_read;
    }

    //if we reached eof, return fail
    return FILE_FAIL;
}

int FileWrite(int handle, void *mem, int num_bytes){

    int bytes_written;
    //printf("enter filewrite\n");
    
    //if it was not w or rw, return fail
    //printf("mode: %d\n", fd_list[handle].mode);
    if((fd_list[handle].mode != 2) && (fd_list[handle].mode != 3)) {
        //printf("filewrite fail 1\n");
        return FILE_FAIL;
    }

    //if they wanted to write too many bytes, return fail
    if(num_bytes > 4096) {
        //printf("filewrite fail 2\n");
        return FILE_FAIL;
    }

    //write data
    bytes_written = DfsInodeWriteBytes(fd_list[handle].inode_handle, mem, fd_list[handle].position, num_bytes);
    if(bytes_written == DFS_FAIL) {
        //printf("filewrite fail 3\n");
        return FILE_FAIL;
    }

    //if we wrote to eof, set eof flag
    if(fd_list[handle].position + bytes_written >= DfsInodeFilesize(fd_list[handle].inode_handle)) {
        fd_list[handle].eof = 1;
    }

    //increase position by bytes written
    fd_list[handle].position += bytes_written;

    return bytes_written;
}

int FileSeek(int handle, int num_bytes, int from_where){

    int fsize; 
    int cur_desired_pos;

    //printf("enter fileseek\n");

    //clear eof 
    fd_list[handle].eof = 0;

    //get filesize
    fsize = DfsInodeFilesize(fd_list[handle].inode_handle);

    //if it was CUR
    if(from_where == FILE_SEEK_CUR){

        //set the desired position
        cur_desired_pos = num_bytes + fd_list[handle].position;
    }

    //if it was SET
    if(from_where == FILE_SEEK_SET){
        
        //set the desired position
        cur_desired_pos = num_bytes;
    }

    //if it was END
    if(from_where == FILE_SEEK_END){
        
        //set the desired position
        cur_desired_pos = fsize + num_bytes;
    }

    //check to make sure that the position is good
    if((cur_desired_pos > fsize) || (cur_desired_pos < 0)){
        return FILE_FAIL;
    }

    //check for eof
    if(cur_desired_pos == fsize){
        fd_list[handle].eof = 1;
    }

    //set the position
    fd_list[handle].position = cur_desired_pos;

    return FILE_SUCCESS;

}

int FileDelete(char *filename){
    uint32 handle;
    int delete;

    //printf("enter filedelete\n");

    //get the handle for the filename
    //printf("filename: %s\n", filename);
    handle = DfsInodeFilenameExists(filename);
    if(handle == DFS_FAIL) {
        //printf("exit filedelete1\n");
        return FILE_FAIL;
    }
    
    //delete the handle
    delete = DfsInodeDelete(handle);
    if(delete == DFS_FAIL) {
        //printf("exit filedelete2\n");
        return FILE_FAIL;
    }

    //printf("exit filedelete\n");

    return FILE_SUCCESS;
}