#include <msg_queue.h>
#include <context.h>
#include <memory.h>
#include <file.h>
#include <lib.h>
#include <entry.h>



/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/

struct msg_queue_info *alloc_msg_queue_info()
{
	struct msg_queue_info *info;
	info = (struct msg_queue_info *)os_page_alloc(OS_DS_REG);
	
	if(!info){
		return NULL;
	}
	return info;
}

void free_msg_queue_info(struct msg_queue_info *q)
{
	os_page_free(OS_DS_REG, q);
}

struct message *alloc_buffer()
{
	struct message *buff;
	buff = (struct message *)os_page_alloc(OS_DS_REG);
	if(!buff)
		return NULL;
	return buff;	
}

void free_msg_queue_buffer(struct message *b)
{
	os_page_free(OS_DS_REG, b);
}

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/


int do_create_msg_queue(struct exec_context *ctx)
{
	/** 
	 * TODO Implement functionality to
	 * create a message queue
	 **/
	if(!ctx)
		return -EINVAL;

        int fd =3;
        while(fd<MAX_OPEN_FILES && ctx->files[fd])
              fd++;

        if(fd == MAX_OPEN_FILES)
                return -EOTHERS;

	struct file * filep = alloc_file();
        if(!filep)
                return -ENOMEM;

	struct msg_queue_info * info_buff = alloc_msg_queue_info();
	if(!info_buff)
		return -ENOMEM;

//	for(int i=0; i<4; i++){
		
		struct message *msg_buff = alloc_buffer();
 		if(!msg_buff)
			return -ENOMEM;
		info_buff-> msg_buffer = msg_buff;
//	}
	info_buff->msg_num =0;
 	info_buff->member_info_buff->member_count=1;
//	printk("%d\n",info_buff->member_info_buff->member_count);
	  for(int i=0;i<8;i++)
        {       for(int j=0;j<8;j++)
                {	
			if(i==j)
				info_buff->block_matrix[i][j] = -1;
                        else
				info_buff->block_matrix[i][j] = 0;
			
                }
        }
	info_buff->member_info_buff->member_pid[0] = ctx->pid;
//	printk("%d\n",info_buff->member_info_buff->member_pid[0]);
        filep->type = REGULAR;
	filep->ref_count=1;
        filep->fops->read = NULL;
        filep->fops->write = NULL;
        filep->fops->lseek = NULL;
        filep->fops->close = NULL;

	filep->msg_queue = info_buff;

	ctx->files[fd] = filep;

	return fd;
}


int do_msg_queue_rcv(struct exec_context *ctx, struct file *filep, struct message *msg)
{
	/** 
	 * TODO Implement functionality to
	 * recieve a message
	 **/
	if(!(ctx && filep && filep->msg_queue))
		return -EINVAL;
//	printk("Checks OK\n");
        struct msg_queue_info * info = filep->msg_queue;
        int i=0;
	while(i < info->msg_num){
		if(info->msg_buffer[i].to_pid == ctx->pid){
//			printk("Inside While for i=%d\n",i);
			msg->from_pid = info->msg_buffer[i].from_pid;
			msg->to_pid = info->msg_buffer[i].to_pid;

			int k;
			for (k = 0; info->msg_buffer[i].msg_txt[k]!='\0'; k++) {
                                        msg->msg_txt[k] = info->msg_buffer[i].msg_txt[k];
                                }
                        msg->msg_txt[k] = '\0';   
			info->msg_num--;
//			printk("Msg Copied\n");
			int j=i;	
			while(j < info->msg_num){
				
				info->msg_buffer[j].from_pid = info->msg_buffer[j+1].from_pid;
	        	        info->msg_buffer[j].to_pid = info->msg_buffer[j+1].to_pid;

				int k;
				for (k = 0; info->msg_buffer[j+1].msg_txt[k]!='\0'; k++) {
					info->msg_buffer[j].msg_txt[k] = info->msg_buffer[j+1].msg_txt[k];
    				}
				info->msg_buffer[j].msg_txt[k] = '\0';					
				
				j++;
                        }
//			printk("Left Shift\n");
		
			break;
             	}
             
             i++;
        }

	if(info->msg_num!=0 && i==info->msg_num)
		return 0;
	
	return 1;
}


int do_msg_queue_send(struct exec_context *ctx, struct file *filep, struct message *msg)
{
	/** 
	 * TODO Implement functionality to
	 * send a message
	 **/
        if(!(ctx && filep && filep->msg_queue && msg))
                return -EINVAL;
//	printk("Checks Ok\n");

        struct msg_queue_info * info = filep->msg_queue;
	int i;
	if(!(msg->to_pid == BROADCAST_PID)){
		for(i=0; i < info->member_info_buff->member_count;i++){
			//printk("PID at Index %d = %d & To PID = %d\n",i,info->member_info_buff->member_pid[i],msg->to_pid);
			if(info->member_info_buff->member_pid[i]==msg->to_pid)
			{
				break;
			}
		}
		//printk("%d %d\n",i,info->member_info_buff->member_count);
		if(i==info->member_info_buff->member_count)
			return -EINVAL;
		else{
			if(info->block_matrix[ctx->pid-1][info->member_info_buff->member_pid[i]-1]== -1)
				return -EINVAL;
			}
	}
//	printk("Checked for Broadcast\n");
//	for(int j=0;j< info->member_info_buff->member_count;j++)
//		printk("%d ",info->member_info_buff->member_pid[j]);
//	printk("\n");

//	dprintk("MSG NUM updated\n");
	if(msg->to_pid == BROADCAST_PID){
		int count = 0;
		for(int l=0;l<info->member_info_buff->member_count;l++){
			if((info->member_info_buff->member_pid[l] != ctx->pid) && (info->block_matrix[ctx->pid-1][info->member_info_buff->member_pid[l]-1]!=-1)){
			info->msg_buffer[info->msg_num].from_pid = msg->from_pid;
			info->msg_buffer[info->msg_num].to_pid = info->member_info_buff->member_pid[l];
			
//			dprintk("%d %d\n",msg->from_pid,info->member_info_buff->member_pid[l]);

		        int k;
        		for (k = 0; msg->msg_txt[k]!='\0'; k++) {
          			info->msg_buffer[info->msg_num].msg_txt[k] =msg->msg_txt[k];
        		}
        		info->msg_buffer[info->msg_num].msg_txt[k] = '\0';
        		info->msg_num++;
			count++;
			}
		}

                 return count;
	}
	else{

	info->msg_buffer[info->msg_num].from_pid = msg->from_pid;
        info->msg_buffer[info->msg_num].to_pid = msg->to_pid;

	int k;
        for (k = 0; msg->msg_txt[k]!='\0'; k++) {
          info->msg_buffer[info->msg_num].msg_txt[k] =msg->msg_txt[k];
        }
        info->msg_buffer[info->msg_num].msg_txt[k] = '\0';  
	info->msg_num++;
}
//	if(msg->to_pid == BROADCAST_PID)
//		return info->member_info_buff->member_count-1;

        return 1;
}

void do_add_child_to_msg_queue(struct exec_context *child_ctx)
{
	/** 
	 * TODO Implementation of fork handler 
	 **/
        if(!child_ctx)
                return;

        int fd = 3;
        while(fd<MAX_OPEN_FILES){

             if(child_ctx->files[fd] && child_ctx->files[fd]->msg_queue){
		struct msg_queue_member_info * member_info = child_ctx->files[fd]->msg_queue->member_info_buff;
			if(member_info->member_count < MAX_MEMBERS){
				member_info->member_pid[member_info->member_count] = child_ctx->pid;
				member_info->member_count++;
				//dprintk("Ref Count = %d\n",child_ctx->files[fd]->ref_count);
				child_ctx->files[fd]->ref_count++;
 //                             dprintk("Ref Count = %d\n",child_ctx->files[fd]->ref_count);
			    	for(int i=0;i < 8;i++){
						child_ctx->files[fd]->msg_queue->block_matrix[child_ctx->pid-1][i]=0;
						child_ctx->files[fd]->msg_queue->block_matrix[i][child_ctx->pid-1]=0;
				}
				child_ctx->files[fd]->msg_queue->block_matrix[child_ctx->pid-1][child_ctx->pid-1] =-1;
			
			}
	     }
	     
             fd++;
	}
//	printk("%d\n%d\n",child_ctx->files[3]->msg_queue->member_info_buff->member_count,child_ctx->files[3]->msg_queue->member_info_buff->member_pid[1]);
	
/*	 for(int i=0;i<8;i++)
        {       for(int j=0;j<8;j++)
                {
                        printk("%d ",child_ctx->files[fd]->msg_queue->block_matrix[i][j]);
                }
                printk("\n");
	}
*/
}

void do_msg_queue_cleanup(struct exec_context *ctx)
{
	/** 
	 * TODO Implementation of exit handler 
	 **/
        if(!ctx)
                return;

        int fd = 3;
        while(fd<MAX_OPEN_FILES)
	{

             if(ctx->files[fd] && ctx->files[fd]->msg_queue){
         	       do_msg_queue_close(ctx,fd);
		}
             fd++;
        }
}

int do_msg_queue_get_member_info(struct exec_context *ctx, struct file *filep, struct msg_queue_member_info *info)
{
	/** 
	 * TODO Implementation of exit handler 
	 **/
	if(!(ctx && filep && filep->msg_queue))
		return -EINVAL;

	struct msg_queue_member_info *member_info = filep->msg_queue->member_info_buff;
	info->member_count = member_info->member_count;
	int k;
	for(k=0; k < MAX_MEMBERS; k++)
		info->member_pid[k] = member_info->member_pid[k];
	return 0;

}


int do_get_msg_count(struct exec_context *ctx, struct file *filep)
{
	/** 
	 * TODO Implement functionality to
	 * return pending message count to calling process
	 **/
        if(!(ctx && filep && filep->msg_queue))
                return -EINVAL;

        struct msg_queue_info * info = filep->msg_queue;
        int i=0, count=0;

        while(i < info->msg_num){
//		printk("Inside While of Count\n");
//		printk("Msg Buff [%d] to_pid%d\n%s\n",i,info->msg_buffer[i].to_pid,info->msg_buffer[i].msg_txt);
                if(info->msg_buffer[i].to_pid == ctx->pid){ 
			count++;
		}
             i++;
        }
//	printk("%d\n",count);
        return count;
}

int do_msg_queue_block(struct exec_context *ctx, struct file *filep, int pid)
{
	/** 
	 * TODO Implement functionality to
	 * block messages from another process 
	 **/

	if(!(ctx && filep && filep->msg_queue))
		return -EINVAL;

	struct msg_queue_info * info = filep->msg_queue;
        int i;
        for(i=0; i < info->member_info_buff->member_count;i++){
        	if(info->member_info_buff->member_pid[i] == pid)
                	break;
        }
        if(i==info->member_info_buff->member_count)
        	return -EINVAL;
        
	int j;
        for(j=0; j < info->member_info_buff->member_count;j++){
                if(info->member_info_buff->member_pid[j] ==ctx->pid)
                        break;
        }
        if(j==info->member_info_buff->member_count)
                return -EINVAL;
	info->block_matrix[pid-1][ctx->pid-1] = -1;
	
/*	for(int i=0;i<8;i++)
	{	for(int j=0;j<8;j++)
		{
			printk("%d ",info->block_matrix[i][j]);
		}	
		printk("\n");
	}
*/	
	return 0;
}

int do_msg_queue_close(struct exec_context *ctx, int fd)
{
	/** 
	 * TODO Implement functionality to
	 * remove the calling process from the message queue 
	 **/
	if(!(ctx && fd>=0 && fd<MAX_OPEN_FILES))
		return -EINVAL;

	struct file * filep = ctx->files[fd];
	if(!(filep && filep->msg_queue))
		return -EINVAL;
//	printk("HERE IN CLOSE\n");
 
	struct msg_queue_member_info * member_info = filep->msg_queue->member_info_buff;
	struct msg_queue_info * info = filep->msg_queue;

	int i;
	int temp = member_info->member_count;
	for(i =0; i < member_info->member_count; i++){
		if(member_info->member_pid[i]==ctx->pid){
//			printk("Close Loop for i = %d\n",i);
			member_info->member_count--;

                        int j=i;
		        while(j < member_info->member_count){
                                member_info->member_pid[j] = member_info->member_pid[j+1];
	                        j++;
                        }
			member_info->member_pid[member_info->member_count]=0;
 //                      dprintk("Ref Count before= %d\n",ctx->files[fd]->ref_count);
			break;
               }
		
	}
	
	if(i == temp)
		return -EINVAL;
	
/*	  for(int i=0;i<8;i++)
        {       for(int j=0;j<8;j++)
                {
                        printk("%d ",ctx->files[fd]->msg_queue->block_matrix[i][j]);
                }
                printk("\n");
        }
*/
	if(info->member_info_buff->member_count==0){
		free_msg_queue_buffer(filep->msg_queue->msg_buffer);
		free_msg_queue_info(filep->msg_queue);
//		dprintk("%d\n",filep->ref_count);
		do_file_close(filep);
//		if(ctx->files[fd]==NULL)
//		printk("File Pointer Freed\n");
//		printk("Freed Allocated Buffers\n");
	}
	
	return 0;
}
