#ifndef __PHOENIX_MEM_H__
#define __PHOENIX_MEM_H__

#include <dim-sum/spinlock.h>
#include <asm/page.h>
#include <uapi/dim-sum/mem.h>


enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

#define VERIFY_READ	0
#define VERIFY_WRITE	1

#define MAX_NUMA_NODES	1

#define	__PG_FREE			(1<<0)
#define	__PG_DMA			(1<<1)
#define	__PG_NORMAL		(1<<2)
#define	__PG_HIGH			(1<<3)
#define	__PG_TINY			(1<<4)
#define	__PG_PRIVATE		(1<<5)
#define	__PG_TINY_FREE		(1<<6)

#define MAX_ORDER	9   /* 最大支持连续内存分配为1M */

typedef signed int tinyidx_t;

struct tiny_block {
	tinyidx_t units;
};
typedef struct tiny_block tiny_t;

#define TINY_UNIT sizeof(tiny_t)


struct page_desc
{
	unsigned long		flags;			/* 页面标志 */
	struct list_head	pg_list;
	void				*private;

	/* 以下几项针对tiny小内存分配 */
	tinyidx_t			free_units;		/* 剩余的单元数 */
	tiny_t			*free_tiny;		/* 指向第一个空闲的tiny */
	struct list_head	tiny_list;			/* 连接所有的空闲tiny页数 */
};

struct kmem_cache {
	unsigned int		size;
	unsigned int		align;
	unsigned int		opt;
	const char		*name;
};

struct free_buddy {
	struct list_head	free_list;
	unsigned long		nr_free;
};

struct mem_zone
{
	spinlock_t		zone_lock;

	struct page_desc	*zone_mem_map;
	unsigned long 	zone_pgmap_num;

	unsigned long		zone_start_pfn;
	unsigned long		zone_nr_pages;
	unsigned long		zone_free_pages;

	struct free_buddy	zone_free_buddys[MAX_ORDER];

	struct numa_node *node;
	char				*zone_name;	
};

struct numa_node
{
	spinlock_t 		node_lock;

	struct mem_zone	zone[ZONE_MAX_NUM];
	int				nr_zones;

	unsigned long		dma_addr_start;
	unsigned long		dma_size;

	unsigned long		normal_addr_start;
	unsigned long		normal_size;

	unsigned long		high_addr_start;
	unsigned long		high_size;
};

void memory_init(void);

#endif

