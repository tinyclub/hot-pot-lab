
/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <linux/kernel.h>
#include <dim-sum/sched.h>

/**********************************************************
 *                       Implements                       *
 **********************************************************/
static void __dump_stack(void)
{
	show_stack(NULL, NULL);
}

#ifdef CONFIG_SMP
asmlinkage __visible void dump_stack(void)
{
}
#else
asmlinkage __visible void dump_stack(void)
{
	__dump_stack();
}
#endif

