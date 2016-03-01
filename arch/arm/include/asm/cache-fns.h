#ifndef _ASMARM_CACHEFNS_H
#define _ASMARM_CACHEFNS_H

struct cpu_cache_fns {
	void (*flush_icache_all)(void);
	void (*flush_kern_all)(void);
	void (*flush_kern_louis)(void);
	void (*flush_user_all)(void);
	void (*flush_user_range)(unsigned long, unsigned long, unsigned int);

	void (*coherent_kern_range)(unsigned long, unsigned long);
	int  (*coherent_user_range)(unsigned long, unsigned long);
	void (*flush_kern_dcache_area)(void *, size_t);

	void (*dma_map_area)(const void *, size_t, int);
	void (*dma_unmap_area)(const void *, size_t, int);

	void (*dma_flush_range)(const void *, const void *);
};


/*
struct outer_cache_fns {
	void (*inv_range)(unsigned long, unsigned long);
	void (*clean_range)(unsigned long, unsigned long);
	void (*flush_range)(unsigned long, unsigned long);
	void (*flush_all)(void);
	void (*inv_all)(void);
	void (*disable)(void);
#ifdef CONFIG_OUTER_CACHE_SYNC
	void (*sync)(void);
#endif
	void (*set_debug)(unsigned long);
	void (*resume)(void);
};
*/

#endif /* _ASMARM_CACHEFNS_H */

