#include <linux/linkage.h>
#include <dim-sum/init.h>
//#include <dim-sum/mm_types.h>
#include <asm/ptrace.h>
//#include <asm/page.h>
#include <dim-sum/printk.h>

#if 0
static void v6_clear_user_highpage_nonaliasing(struct page *page, unsigned long vaddr)
{
	/* to do */
}

static void v6_copy_user_highpage_nonaliasing(struct page *to,
	struct page *from, unsigned long vaddr, struct vm_area_struct *vma)
{
	/* to do */
}

struct cpu_user_fns v6_user_fns __initdata = {
	.cpu_clear_user_highpage = v6_clear_user_highpage_nonaliasing,
	.cpu_copy_user_highpage	= v6_copy_user_highpage_nonaliasing,
};
#else
int v6_user_fns = 0;
#endif

/*
 * Dispatch a data abort to the relevant handler.
 */
asmlinkage void 
do_DataAbort(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	/* to do */
	printk("xby_debug in do_DataAbort\n");
}

asmlinkage void 
do_PrefetchAbort(unsigned long addr, unsigned int ifsr, struct pt_regs *regs)
{
	/* to do */
	printk("xby_debug in do_PrefetchAbort\n");
}
