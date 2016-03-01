
 
/**********************************************************
 * 					  Includers 					      *
 **********************************************************/
#include <dim-sum/sched.h>

#include <asm/memory.h>
#include <asm/errno.h>
#include <asm/current.h>
#include <asm/ptrace.h>

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#ifndef COFNIG_ARM_UNWIND
#define HIGH_MEMORY			0xC8000000
#endif

/**********************************************************
 * 					  Implements					      *
 **********************************************************/
void dump_backtrace_entry(unsigned long where, unsigned long from, unsigned long frame)
{
#ifdef CONFIG_KALLSYMS
	printk("[<%08lx>] (%ps) from [<%08lx>] (%pS)\n", where, (void *)where, from, (void *)from);
#else
	printk("Function entered at [<%08lx>] from [<%08lx>]\n", where, from);
#endif
}

#ifdef COFNIG_ARM_UNWIND
static inline void dump_backtrace(struct pt_regs *regs, struct task_struct *tsk)
{
}
#else
/*
 * Stack pointers should always be within the kernels view of
 * physical memory.  If it is not there, then we can't dump
 * out any information relating to the stack.
 */
static int verify_stack(unsigned long sp)
{
	if ((sp < PAGE_OFFSET)
		|| (sp > (unsigned long)HIGH_MEMORY))
	{
		return -EFAULT;
	}

	return 0;
}

static void dump_backtrace(struct pt_regs *regs, struct task_struct *tsk)
{
	unsigned int fp, mode;
	int ok = 1;

	printk("Backtrace: ");

	if (!tsk)
	{
		tsk = current;
	}
	
	if (regs)
	{
		fp = frame_pointer(regs);
		mode = processor_mode(regs);
	} 
	else if (tsk != current) 
	{
		fp = thread_saved_fp(tsk);
		mode = 0x10;
	} 
	else 
	{
		asm("mov %0, fp" : "=r" (fp) : : "cc");
		mode = 0x10;
	}

	if (!fp) 
	{
		printk("no frame pointer");
		ok = 0;
	} 
	else if (verify_stack(fp)) 
	{
		printk("invalid frame pointer 0x%08x", fp);
		ok = 0;
	} 
	else if (fp < (unsigned long)end_of_stack(tsk))
	{
		printk("frame pointer underflow");
	}
	
	printk("\n");

	if (ok)
		c_backtrace(fp, mode);
}
#endif

void show_stack(struct task_struct *tsk, unsigned long *sp)
{
	dump_backtrace(NULL, tsk);
	barrier();
}

void dabt_exception_handler(void)
{
	dump_stack();
	while (1);
}


