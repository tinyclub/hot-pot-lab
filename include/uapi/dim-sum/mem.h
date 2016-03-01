#ifndef __UAPI_PHOENIX_MEM_H__
#define __UAPI_PHOENIX_MEM_H__

#include <asm/page.h>

/* Pure 2^n version of get_order */
static __inline__ int get_order(unsigned long size)
{
	int order;

	size = (size-1) >> (PAGE_SHIFT-1);
	order = -1;
	do {
		size >>= 1;
		order++;
	} while (size);
	return order;
}

enum{
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_HIGH,
	ZONE_MAX_NUM
};


#define MEM_NORMAL		ZONE_NORMAL
#define MEM_DMA		ZONE_DMA
#define MEM_HIGH		ZONE_HIGH

void *dim_sum_pages_alloc(unsigned int order, unsigned int opt);
int dim_sum_pages_free(void *addr);

void *dim_sum_mem_alloc(int size, unsigned int opt);
int dim_sum_mem_free(void *addr);

struct kmem_cache *dim_sum_memcache_create(char *name, int size, unsigned int opt);
int dim_sum_memcache_destroy(struct kmem_cache *p_cache);
void *dim_sum_memcache_alloc(struct kmem_cache *p_cache);
int dim_sum_memcache_free(struct kmem_cache *p_cache, void *p_obj);

#endif

