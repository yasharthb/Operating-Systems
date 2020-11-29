#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/

void free_file_object(struct file *filep)
{
	if(filep)
	{
		os_page_free(OS_DS_REG ,filep);
		stats->file_objects--;
	}
}

struct file *alloc_file()
{
	struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
	file->fops = (struct fileops *) (file + sizeof(struct file)); 
	bzero((char *)file->fops, sizeof(struct fileops));
	file->ref_count = 1;
	file->offp = 0;
	stats->file_objects++;
	return file; 
}

void *alloc_memory_buffer()
{
	return os_page_alloc(OS_DS_REG); 
}

void free_memory_buffer(void *ptr)
{
	os_page_free(OS_DS_REG, ptr);
}

/* STDIN,STDOUT and STDERR Handlers */

/* read call corresponding to stdin */

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
	kbd_read(buff);
	return 1;
}

/* write call corresponding to stdout */

static int do_write_console(struct file* filep, char * buff, u32 count)
{
	struct exec_context *current = get_current_ctx();
	return do_write(current, (u64)buff, (u64)count);
}

long std_close(struct file *filep)
{
	filep->ref_count--;
	if(!filep->ref_count)
		free_file_object(filep);
	return 0;
}
struct file *create_standard_IO(int type)
{
	struct file *filep = alloc_file();
	filep->type = type;
	if(type == STDIN)
		filep->mode = O_READ;
	else
		filep->mode = O_WRITE;
	if(type == STDIN){
		filep->fops->read = do_read_kbd;
	}else{
		filep->fops->write = do_write_console;
	}
	filep->fops->close = std_close;
	return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
	int fd = type;
	struct file *filep = ctx->files[type];
	if(!filep){
		filep = create_standard_IO(type);
	}else{
		filep->ref_count++;
		fd = 3;
		while(ctx->files[fd])
			fd++; 
	}
	ctx->files[fd] = filep;
	return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

/* File exit handler */
void do_file_exit(struct exec_context *ctx)
{
	/*TODO the process is exiting. Adjust the refcount
	of files*/
	
	if(!ctx)
		return;
	
	int fd=0;
	while(fd < MAX_OPEN_FILES)
	{	
		if(ctx->files[fd]){
			long temp = do_file_close(ctx->files[fd]);
			if(temp == 0)
			  ctx->files[fd]=NULL;
		}
		fd++;
	}
	
}

/*Regular file handlers to be written as part of the assignmemnt*/


static int do_read_regular(struct file *filep, char * buff, u32 count)
{
	/** 
	*  TODO Implementation of File Read, 
	*  You should be reading the content from File using file system read function call and fill the buf
	*  Validate the permission, file existence, Max length etc
	*  Incase of Error return valid Error code 
	**/
	
	if(!(filep && buff && filep->inode))
		return -EINVAL;
	if(!(filep->mode & O_READ))
		return -EACCES;

	int rdata = flat_read(filep->inode,buff,count,&(filep->offp));
	
	if( rdata < 0)
		return -EINVAL;
	
	filep->offp += rdata;
	
	return rdata;

}

/*write call corresponding to regular file */

static int do_write_regular(struct file *filep, char * buff, u32 count)
{
	/** 
	*   TODO Implementation of File write, 
	*   You should be writing the content from buff to File by using File system write function
	*   Validate the permission, file existence, Max length etc
	*   Incase of Error return valid Error code 
	* **/

        if(!(filep && buff && filep->inode))
		return -EINVAL;
        if(!(filep->mode & O_WRITE))
                return -EACCES;

        int rdata = flat_write(filep->inode, buff, count, &(filep->offp));
        if(rdata<0)
		return -EINVAL;
	
	filep->offp += rdata;

	return rdata;
        
}

long do_file_close(struct file *filep)
{
	/** TODO Implementation of file close  
	*   Adjust the ref_count, free file object if needed
	*   Incase of Error return valid Error code 
	*/

	if(!((filep) && (filep->ref_count > 0)))
		return -EINVAL;
	
	filep->ref_count--;

	if((filep->ref_count) == 0)
	{	
		free_file_object(filep);
		filep=NULL;
	}
	return 0;
}

static long do_lseek_regular(struct file *filep, long offset, int whence)
{
	/** 
	*   TODO Implementation of lseek 
	*   Set, Adjust the ofset based on the whence
	*   Incase of Error return valid Error code 
	* */
//	dprintk("Inside Lseek\n");
	if(!(filep && filep->inode 
	   && (whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END))){
		return -EINVAL;
	}
	
//	dprintk("Input Alright");

	long final_offset = offset;

	switch(whence){
		case SEEK_SET: final_offset +=0;
			       break;
		case SEEK_CUR: final_offset += filep->offp;
			       break;
		case SEEK_END: final_offset += filep->inode->file_size;
                               break;
		default: return -EINVAL;
	}
	
//	dprintk("Offset Generated");

	if(!((final_offset >= 0) && (final_offset <=((long)filep->inode->e_pos - (long)filep->inode->s_pos))))
		return -EINVAL;
	
	filep->offp = final_offset;
//	dprintk("Offp updated");
	return filep->offp;
}

extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{

	/**  
	*  TODO Implementation of file open, 
	*  You should be creating file(use the alloc_file function to creat file), 
	*  To create or Get inode use File system function calls, 
	*  Handle mode and flags 
	*  Validate file existence, Max File count is 16, Max Size is 4KB, etc
	*  Incase of Error return valid Error code 
	* */

	if(!(ctx && filename))
		return -EINVAL;

	struct inode *inode = lookup_inode(filename);
	
	if(!inode){
		if(!(flags & O_CREAT))
			return -EINVAL;
		
		inode = create_inode(filename,mode);

	}
	else{
		if(!(((flags & O_READ) <= (inode->mode & O_READ)) 
	          && ((flags & O_WRITE) <= (inode->mode & O_WRITE))
		  && ((flags & O_EXEC) <= (inode->mode & O_EXEC)))){
			return -EACCES;
		}
	    }
    
        struct file * filep = alloc_file();
	if(!filep)
		return -ENOMEM;

        filep->type = REGULAR;
        filep->mode = flags;
        filep->inode = inode;

        filep->fops->read = do_read_regular;
        filep->fops->write = do_write_regular;
        filep->fops->lseek = do_lseek_regular;
        filep->fops->close = do_file_close;
        filep->pipe = NULL;

        int fd =3;
        while(fd<MAX_OPEN_FILES && ctx->files[fd])
              fd++;

        if(fd == MAX_OPEN_FILES)
                return -EOTHERS;

        ctx->files[fd] = filep;
	//dprintk("File already exists\n");
        return fd;
}

int fd_dup(struct exec_context *current, int oldfd)
{
     /** TODO Implementation of dup 
      *  Read the man page of dup and implement accordingly 
      *  return the file descriptor,
      *  Incase of Error return valid Error code 
      * */
    // int ret_fd = -EINVAL; 
    // return ret_fd;

    if( current == NULL || oldfd < 0 || oldfd >= MAX_OPEN_FILES ){
      return (-EINVAL);
    }



    if( current->files[oldfd] == NULL ){
      return (-EINVAL);
    }

    int newfd = 0;

    while( newfd < MAX_OPEN_FILES && current->files[newfd] != NULL ){
      newfd++;
    }

    if( newfd < 0 || newfd >= MAX_OPEN_FILES ){
      return (-EOTHERS);//check here
    }

    current->files[newfd] = current->files[oldfd];

    current->files[oldfd]->ref_count++;       //check here

    return newfd;
}

/**
 * Implementation dup 2 system call;
 */
int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
	/** 
	*  TODO Implementation of the dup2 
	*  Incase of Error return valid Error code 
	**/
	
	if(!(current 
	   && oldfd >= 0 && oldfd < MAX_OPEN_FILES && current->files[oldfd] 
	   && newfd >= 0 && newfd < MAX_OPEN_FILES)) {
		return -EINVAL;
	}
	
	if(current->files[newfd]){
		if(do_file_close(current->files[newfd]))
			return -EOTHERS;
	}
	
	current->files[newfd] = current->files[oldfd];
	current->files[oldfd]->ref_count ++;
	
	return newfd;
}

int do_sendfile(struct exec_context *ctx, int outfd, int infd, long *offset, int count) {
	/** 
	*  TODO Implementation of the sendfile 
	*  Incase of Error return valid Error code 
	**/
//	dprintk("Inside Do sendfile\n");
	if(!(ctx
	      && infd > 0 && infd < MAX_OPEN_FILES && ctx->files[infd] 
	      && outfd > 0 && outfd < MAX_OPEN_FILES && ctx->files[outfd])){
		return -EINVAL;
	}
//	dprintk("Valid Input\n");
	struct file *infilep = ctx->files[infd];
	struct file *outfilep = ctx->files[outfd];

	if(!((infilep->mode & O_READ) && (outfilep->mode & O_WRITE)))
		return -EACCES;
//      dprintk("Permissions alright\n");
	
        char *buff = alloc_memory_buffer();
	if(!buff)
		return -ENOMEM;
//	dprintk("BUffer Allocated\n");
	int rdata;

	if(offset){
//		dprintk("Inside Offset NOn NUll\n");
		if(!infilep->inode)
			return -EINVAL;

		rdata = flat_read(infilep->inode, buff, count,(int *)offset);

        	if( rdata < 0)
                	return -EINVAL;

         	*offset += rdata;
	}
	else{
//		dprintk("Inside Offset NUll\n");
		rdata = do_read_regular(infilep, buff, count);
		if(rdata < 0)
			return -EOTHERS;
	}

//	dprintk("Read Successful\n");

	int wdata = do_write_regular(outfilep, buff, rdata);
	
	if(wdata < 0)
		return -EOTHERS;

//	dprintk("Write Successful\n");
	
	free_memory_buffer(buff);

	return rdata;
}

