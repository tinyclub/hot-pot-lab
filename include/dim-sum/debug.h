#ifndef _PHOENIX_DEBUG_H
#define _PHOENIX_DEBUG_H

#include <linux/linkage.h>

extern asmlinkage void debug_printstr_nommu_devtree_magic(void);
extern asmlinkage void debug_printstr_mmu(const char *ptr);
extern asmlinkage void debug_printstr_nommu(const char *ptr);
extern void early_print(const char *str, ...);
#endif