#include <dim-sum/mem.h>
#include <base/string.h>


unsigned long dim_sum_mm_start = 0xc0000000 + 32 * 1024 * 1024;
unsigned long dim_sum_mm_size = 64 * 1024 * 1024;

extern void __free_pages_to_buddy(struct page_desc *p_desc, struct mem_zone *zone, 
							       unsigned int order);

struct numa_node ph_numa_node[MAX_NUMA_NODES];

static char *zone_names[ZONE_MAX_NUM] = { "DMA", "NORMAL", "HIGH" };

static struct mem_zone *page_desc_to_zone(struct page_desc *desc)
{
	struct numa_node *node;
	struct mem_zone *zone;
	int i, j;

	for (i = 0; i < MAX_NUMA_NODES; i++) {
		node = &ph_numa_node[i];
		
		for (j = 0; j < ZONE_MAX_NUM; j++) {
			zone = &node->zone[j];

			/* 跳过未使用的zone */
			if (zone->zone_nr_pages == 0)
				continue;

			if ((desc >= zone->zone_mem_map) && 
			    (desc <= (zone->zone_mem_map + zone->zone_nr_pages - 1))) {
				return zone;
			}
		}
	}

	return NULL;
}

struct mem_zone *vaddr_to_zone(unsigned long vaddr)
{
	struct numa_node *node;
	struct mem_zone *zone;
	int i, j;

	for (i = 0; i < MAX_NUMA_NODES; i++) {
		node = &ph_numa_node[i];
		
		for (j = 0; j < ZONE_MAX_NUM; j++) {
			int pfn = (vaddr >> PAGE_SHIFT);
			
			zone = &node->zone[j];

			/* 跳过未使用的zone */
			if (zone->zone_nr_pages == 0)
				continue;

			if ((pfn >= zone->zone_start_pfn) && 
			    (pfn <= (zone->zone_start_pfn + zone->zone_nr_pages - 1))) {
				return zone;
			}
		}
	}

	return NULL;
}

unsigned long page_desc_to_vaddr(struct page_desc *desc)
{
	struct mem_zone *zone;
	unsigned long vaddr = 0, ndesc;

	zone = page_desc_to_zone(desc);
	if (!zone) {
		printk("Bug at %s %d\n", __FUNCTION__, __LINE__);
		dump_stack();
		return vaddr;
	}

	ndesc = ((unsigned long)desc - (unsigned long)zone->zone_mem_map) / 
			sizeof(struct page_desc);

	vaddr = (zone->zone_start_pfn + ndesc) << PAGE_SHIFT;

	return vaddr;
}

struct page_desc *vaddr_to_page_desc(unsigned long vaddr)
{
	struct mem_zone *zone;
	int index;

	zone = vaddr_to_zone(vaddr);
	if (!zone) {
		printk("Bug at %s %d\n", __FUNCTION__, __LINE__);
		dump_stack();
		return NULL;
	}

	index = (vaddr >> PAGE_SHIFT) - zone->zone_start_pfn;

	return &zone->zone_mem_map[index];
}

void dump_all_zones_info(int detail)
{
	struct numa_node *node;
	struct mem_zone *zone;
	int i, j;

	for (i = 0; i < MAX_NUMA_NODES; i++) {
		node = &ph_numa_node[i];

		for (j = 0; j < ZONE_MAX_NUM; j++) {
			unsigned long flags;
			int order;
			
			zone = &node->zone[j];

			if (zone->zone_nr_pages == 0)
				continue;

			spin_lock_irqsave(&zone->zone_lock, flags);

			printk("Dump Zone Info:%s\n", zone->zone_name);
			printk("  start_pfn:%lu, nr_pages:%lu, free_pages:%lu\n",  
		   		    zone->zone_start_pfn, zone->zone_nr_pages, zone->zone_free_pages);

			for (order = 0; order < MAX_ORDER; order++) {
				struct list_head *pp;
				struct page_desc *desc;

				printk("  order:%d, each_nr:%u, nr_free:%lu\n", order, (1<<order), 
					    zone->zone_free_buddys[order].nr_free);

				if (!detail)
					continue;

				list_for_each(pp, &zone->zone_free_buddys[order].free_list) {
					desc = list_entry(pp, struct page_desc, pg_list);
					printk("      page addr:0x%lx, private:%lu\n", page_desc_to_vaddr(desc), 
						    (unsigned long)desc->private);
				}
			}

			spin_unlock_irqrestore(&zone->zone_lock, flags);
		}
	}
}

static int mem_zone_init(struct numa_node *node, int type, 
				    		    unsigned long mem_start, unsigned long mem_size)
{
	struct mem_zone *zone = &node->zone[type];
	unsigned long start_pfn, total_pages, flags;
	int i;

	spin_lock_init(&zone->zone_lock);

	/* 大小为0不做初始化 */
	if (mem_size == 0)
		return -1;

	memset((void *)mem_start, 0, mem_size);

	/* 按页对齐 */
	start_pfn = (mem_start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
	total_pages = ((mem_start + mem_size) - (start_pfn << PAGE_SHIFT)) / PAGE_SIZE;

	spin_lock_irqsave(&zone->zone_lock, flags);

	zone->zone_mem_map = (struct page_desc *)(start_pfn << PAGE_SHIFT);
	zone->zone_pgmap_num = ((total_pages * sizeof(struct page_desc)) + 
					   (PAGE_SIZE - 1)) >> PAGE_SHIFT;

	/* 起始地址需要按2^(MAX_ORDER-1)向后对齐 */
	zone->zone_start_pfn = start_pfn + zone->zone_pgmap_num;
	zone->zone_start_pfn = round_up(zone->zone_start_pfn, (1UL<<(MAX_ORDER - 1)));

	zone->zone_nr_pages = total_pages - (zone->zone_start_pfn - start_pfn);
	zone->node = node;
	zone->zone_name = zone_names[type];

	/* 初始化free_buddys链表 */
	for (i = 0; i < MAX_ORDER; i++) {
		INIT_LIST_HEAD(&zone->zone_free_buddys[i].free_list);
	}

	/* 将所有页释放到buddy中 */
	for (i = 0; i < zone->zone_nr_pages; i++) {
		struct page_desc *desc = zone->zone_mem_map + i;;

		desc->flags = __PG_FREE;
		__free_pages_to_buddy(desc, zone, 0);
	}

	spin_unlock_irqrestore(&zone->zone_lock, flags);

	return 0;
}

static int numa_node_init(int node_id, 
						      unsigned long dma_start, unsigned long dma_size,
						      unsigned long normal_start, unsigned long normal_size,
						      unsigned long high_start, unsigned long high_size)
{
	struct numa_node *node = &ph_numa_node[node_id];

	if (node_id >= MAX_NUMA_NODES)
		return -1;

	memset(node, 0, sizeof(struct numa_node));

	spin_lock_init(&(node->node_lock));

	/* 目前只有一个normal zone */
	node->nr_zones = 1;

	node->dma_addr_start = dma_start;
	node->dma_size = dma_size;

	node->normal_addr_start = normal_start;
	node->normal_size = normal_size;

	node->high_addr_start = high_start;
	node->high_size = high_size;

	/* 初始化各个zone */
	mem_zone_init(node, ZONE_DMA, node->dma_addr_start, node->dma_size);
	mem_zone_init(node, ZONE_NORMAL, node->normal_addr_start, node->normal_size);
	mem_zone_init(node, ZONE_HIGH, node->high_addr_start, node->high_size);

	return 0;
}

void memory_init(void)
{
	unsigned long high_addr;

	high_addr = dim_sum_mm_start + dim_sum_mm_size;

	/* 初始化numa结点，目前只有1个结点 */
	numa_node_init(0, dim_sum_mm_start, 0, dim_sum_mm_start, dim_sum_mm_size, high_addr, 0);

	dump_all_zones_info(0);
}


