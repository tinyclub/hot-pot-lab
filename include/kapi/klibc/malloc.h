/*
 * malloc.h
 *
 * Apparently people haven't caught on to use <stdlib.h>, which is the
 * standard place for this crap since the 1980's...
 */

#ifndef _MALLOC_H
#define _MALLOC_H

#include <klibc/extern.h>
#include <klibc/compiler.h>
#include <stddef.h>
#include <dim-sum/mem.h>

//xby_debug
#if 0
__extern void free(void *);

__extern __mallocfunc void *malloc(size_t);
#else
extern int dim_sum_mem_free(void *addr);
static inline void free(void *ptr)
{
	dim_sum_mem_free(ptr);
}

extern void *dim_sum_mem_alloc(int nbytes, unsigned int opt);
static inline __mallocfunc void *malloc(size_t size)
{
	return dim_sum_mem_alloc(size, MEM_NORMAL);
}
#endif
__extern __mallocfunc void *zalloc(size_t);
__extern __mallocfunc void *calloc(size_t, size_t);
__extern __mallocfunc void *realloc(void *, size_t);

#endif				/* _MALLOC_H */
