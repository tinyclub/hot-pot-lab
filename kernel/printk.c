/**
 * ÄÚºË´òÓ¡Ä£¿é
 */
#include <stdarg.h>

#include <dim-sum/console.h>
#include <dim-sum/debug.h>
#include <dim-sum/irqflags.h>
#include <linux/kernel.h>

void register_console(struct console *newcon)
{
}


extern int vsprintf(char *buf, const char *fmt, va_list args);

char printbuffer[1024];
asmlinkage int printk(const char *fmt, ...)
{
	va_list args;
	unsigned long flags;
	int len;

	local_irq_save(flags);
	
	va_start(args, fmt);
	len = vsprintf(printbuffer, fmt, args);
	va_end(args);

	debug_printstr_mmu(printbuffer);

	local_irq_restore(flags);

	return len;
}

asmlinkage int printk_xby(const char *fmt, ...)
{
#if 1
	va_list args;
	unsigned long flags;
	int len;

	local_irq_save(flags);
	
	va_start(args, fmt);
	len = vsprintf(printbuffer, fmt, args);
	va_end(args);

	debug_printstr_mmu(printbuffer);

	local_irq_restore(flags);

	return len;
#else
	return 0;
#endif
}

