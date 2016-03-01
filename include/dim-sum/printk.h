#ifndef _PHOENIX_PRINTK_H
#define _PHOENIX_PRINTK_H

#include <linux/linkage.h>

extern asmlinkage int printk(const char *fmt, ...);

extern asmlinkage int printk_xby(const char *fmt, ...);

#endif
