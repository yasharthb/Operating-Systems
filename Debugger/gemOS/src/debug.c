#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>


/*****************************HELPERS******************************************/

/* 
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info)); 
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}

/*
 * frees a debug_info struct 
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}

/*
 * allocates memory to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers)); 
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/* 
 * allocate a node for breakpoint list 
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));
	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait_and_continue
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	child_ctx->dbg = NULL;	
	child_ctx->state = WAITING;	
}


/******************************************************************************/

/* This is the int 0x3 handler
 * Hit from the childs context
 */
long int3_handler(struct exec_context *ctx)
{
	// Your code

        if(!ctx)
                return -1;

        struct exec_context * debuger_ctx = get_ctx_by_pid(ctx->ppid);

	ctx->regs.entry_rip -= (char)1;

	struct breakpoint_info * curr = debuger_ctx->dbg->head;

        while(curr){

                if(curr->addr == (u64)(ctx->regs.entry_rip - (char)1)){
		
			*(char *)(ctx->regs.entry_rip) = curr->next_inst;
		
			if(curr->status == 1)
				*(char *)(ctx->regs.entry_rip-1) = 0xCC;

			debuger_ctx->state =WAITING;
			ctx->state = READY;

			schedule(ctx);

			return 0;

                }

                curr = curr -> next;
        }	

        debuger_ctx->state = READY;
        ctx->state = WAITING;

	*(char *)(ctx->regs.entry_rip) = 0x55;	
	*(char *)(ctx->regs.entry_rip + 1) = 0xCC;

/*----------------*/

	curr = debuger_ctx->dbg->head;

        while(curr){

                if(curr->addr == (u64)(ctx->regs.entry_rip)){
			break;
                }

                curr = curr -> next;
        }

	if(!curr)
		return -1;

	int count =0;
        struct user_regs regs = ctx->regs;
        u64 rbp_i = regs.rbp;

        curr->backtrace[count++] = regs.entry_rip;
        curr->backtrace[count++] = *(u64 *)(regs.entry_rsp);;

        while(count < MAX_BACKTRACE && *(u64 *)(rbp_i+8) != END_ADDR)
        {
                curr->backtrace[count++] = *(u64 *)(rbp_i+8);          
		rbp_i = *(u64 *)(rbp_i);

        }

	curr->backtrace_count = count;

/*--------------------*/

	debuger_ctx->regs.rax = ctx->regs.entry_rip;
	ctx->regs.rax =0x0;

        schedule(debuger_ctx);
	
	return 0;
}

/*
 * Exit handler.
 * Called on exit of Debugger and Debuggee 
 */
void debugger_on_exit(struct exec_context *ctx)
{
	// Your code
	//printk("On Exit \n");
	if(!ctx)
		return;
	if(!ctx->dbg){

		struct exec_context * debuger_ctx = get_ctx_by_pid(ctx->ppid);
        	debuger_ctx->state = READY;
		debuger_ctx->regs.rax = 0x0;
		return;
	}

        struct breakpoint_info * curr = ctx->dbg->head;
	struct breakpoint_info * next;
        while(curr){
                next = curr -> next;
		free_breakpoint_info(curr);
		curr = next;
        }

	free_debug_info(ctx->dbg);	
}

/*
 * called from debuggers context
 * initializes debugger state
 */
int do_become_debugger(struct exec_context *ctx)
{
	// Your code
	if(!ctx)
		return -1;
	
        if(!ctx->dbg)
                ctx->dbg = alloc_debug_info();

        if(!ctx->dbg)
                return -1;

	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr)
{
	// Your code

	if(!(ctx && addr))
		return -1;

	if(!ctx->dbg)
		return -1;
	
	struct breakpoint_info * curr = ctx->dbg->head, * prev;
	int num = 1, count = 0;
	
	if(!curr){
           
	   curr = alloc_breakpoint_info();
            
 	   if(!curr)
                return -1;

           curr -> num = num;
           curr -> status = 1;
           curr -> addr = (u64)addr;
	   curr -> next_inst = *((char *)(addr+1));
           curr -> next = NULL;
	   *(char *)addr = 0xCC;

	   ctx->dbg->head = curr;
           return 0;
	}	

	while(curr){

		num  = curr->num;
		count++;
		
		if(curr->addr == (u64)addr){
			curr->status =1;
			return 0;
		}
		prev = curr;
		curr = curr -> next;
	}

	if(MAX_BREAKPOINTS == count)
		return -1;

	curr = alloc_breakpoint_info();
	if(!curr)
		return -1;

	curr -> num = ++num;
	curr -> status = 1;
	curr -> addr = (u64)addr;
        curr -> next_inst = *((char *)(addr+ 1));
	curr -> next = NULL;
	prev->next = curr;
	*(char *)addr = 0xCC;
	
	return 0;
}

/*
 * called from debuggers context
 */
int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	// Your code
        if(!(ctx && ctx->dbg && addr))
                return -1;

        struct breakpoint_info * curr = ctx->dbg->head,* prev;

	if (curr && (curr->addr == (u64)addr)){
		*(char *)(curr->addr) =0x55;
		*(char *)(curr->addr+1) = curr->next_inst; 
		ctx->dbg->head = curr->next; 
		free_breakpoint_info(curr); 
       		return 0; 
   	     } 
   
	while (curr && (curr->addr != (u64)addr)) 
	{ 	
       		prev = curr; 
       		curr = curr->next; 
   	 } 
   	
   	if (!curr)
		 return -1; 
   
  	 prev->next = curr->next;
        *(char *)(curr->addr) =0x55;
	*(char *)(curr->addr+1) = curr->next_inst;

  
 	 free_breakpoint_info(curr); 

         return 0;

}

/*
 * called from debuggers context
 */
int do_enable_breakpoint(struct exec_context *ctx, void *addr)
{
	// Your code
        if(!(ctx && addr))
                return -1;

        if(!ctx->dbg)
                return -1;

        struct breakpoint_info * curr = ctx->dbg->head;

        while(curr){

                if(curr->addr == (u64)addr){
			*(char *)addr =0xCC;
                        curr->status = 1;
                        return 0;
                }

                curr = curr -> next;
        }

        return -1;
}

/*
 * called from debuggers context
 */
int do_disable_breakpoint(struct exec_context *ctx, void *addr)
{
	// Your code
        if(!(ctx && addr))
                return -1;

        if(!ctx->dbg)
                return -1;

        struct breakpoint_info * curr = ctx->dbg->head;

        while(curr){

                if(curr->addr == (u64)addr){
			*(char *)addr = 0x55;  
                        curr->status = 0;
                        return 0;
                }

                curr = curr -> next;
        }

        return -1;
}

/*
 * called from debuggers context
 */ 
int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *bp)
{
	// Your code
	if(!(ctx && ctx->dbg && bp))
		return -1;
	
	struct breakpoint_info * curr = ctx->dbg->head;
	int count = 0;
        while(curr){
		bp[count].num = curr->num;
		bp[count].addr = curr->addr;
		bp[count].status = curr->status;
		count++;
                curr = curr->next;

		if(count == MAX_BREAKPOINTS)
			return -1;
        }

	return count;
}

/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	// Your code
	if(!(ctx && regs))
		return -1;
        int pid;
        for(pid =0;pid <MAX_PROCESSES;pid++)
         {
                if(ctx->pid == get_ctx_by_pid(pid)->ppid)
                        break;
         }

        if(pid == MAX_PROCESSES)
                return -1;

        struct exec_context * debugee_ctx = get_ctx_by_pid(pid);

	struct user_regs reg = debugee_ctx->regs;
 
	regs->r15 = reg.r15;
        regs->r14 = reg.r14;
        regs->r13 = reg.r13;
        regs->r12 = reg.r12;
        regs->r11 = reg.r11;
        regs->r10 = reg.r10;
        regs->r9 = reg.r9;
        regs->r8 = reg.r8;
        regs->rbp = reg.rbp;
        regs->rdi = reg.rdi;
        regs->rsi = reg.rsi;
        regs->rdx = reg.rdx;
        regs->rcx = reg.rcx;        
        regs->rbx = reg.rbx;
        regs->rax = reg.rax;
        regs->entry_rip = reg.entry_rip;
        regs->entry_cs = reg.entry_cs;
        regs->entry_rflags = reg.entry_rflags;
        regs->entry_rsp = reg.entry_rsp;
        regs->entry_ss = reg.entry_ss;

	return 0;
}

/* 
 * Called from debuggers context
*/
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{
	// Your code
	if(!(ctx && bt_buf))
		return -1;

        int pid, count =0;
        for(pid =0;pid <MAX_PROCESSES;pid++)
         {
                if(ctx->pid == get_ctx_by_pid(pid)->ppid)
                        break;
         }

        if(pid == MAX_PROCESSES)
                return -1;

        struct exec_context * debugee_ctx = get_ctx_by_pid(pid);

       struct breakpoint_info * curr = ctx->dbg->head;

        while(curr){

                if(curr->addr == (u64)(debugee_ctx->regs.entry_rip)){
                        break;
                }

                curr = curr -> next;
        }

	if(!curr)
		return -1;

        while(count < curr->backtrace_count)
        {
                *(u64 *)(bt_buf+8*count) = curr->backtrace[count];
		count++;

        }

	return curr->backtrace_count;
}


/*
 * When the debugger calls wait
 * it must move to WAITING state 
 * and its child must move to READY state
 */

s64 do_wait_and_continue(struct exec_context *ctx)
{
	// Your code	
	if(!(ctx && ctx->dbg))
		return -1;

	int pid,flag = 0 ;
	for(pid =0;pid <MAX_PROCESSES;pid++)
	 {
		if(ctx->pid == get_ctx_by_pid(pid)->ppid)
			break;
	 }

	if(pid == MAX_PROCESSES)
		return -1;

	struct exec_context * debugee_ctx = get_ctx_by_pid(pid);
	
	if(!debugee_ctx || debugee_ctx->state == 0)
		return CHILD_EXIT;
	
	debugee_ctx->state = READY;
	ctx->state = WAITING;

	schedule(debugee_ctx);
					
	return 0;
}
