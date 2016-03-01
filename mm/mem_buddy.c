#include <dim-sum/mem.h>


extern struct numa_node ph_numa_node[MAX_NUMA_NODES];
extern struct page_desc *vaddr_to_page_desc(unsigned long vaddr);
extern struct mem_zone *vaddr_to_zone(unsigned long vaddr);
extern unsigned long page_desc_to_vaddr(struct page_desc *desc);

static inline int page_desc_is_buddy(struct page_desc *p_desc, int order)
{
	if ((p_desc->flags & __PG_PRIVATE) && ((unsigned long)p_desc->private == order))
		return 1;

	return 0;
}

void __free_pages_to_buddy(struct page_desc *p_desc, struct mem_zone *zone, 
							       unsigned int order)
{
	unsigned long page_idx;
	struct page_desc *coalesced, *base;
	int order_size = 1 << order;

	base = zone->zone_mem_map;
	page_idx = p_desc - base;

	if (page_idx & (order_size - 1)) {
		printk("BUG at %s %d\n", __FUNCTION__, __LINE__);
		dump_stack();
		return;
	}

	zone->zone_free_pages += order_size;

	while (order < MAX_ORDER-1) {
		struct free_buddy *buddy;
		struct page_desc *tmp;
		int buddy_idx;

		buddy_idx = (page_idx ^ (1 << order));
		tmp = base + buddy_idx;

		if (!page_desc_is_buddy(tmp, order))
			break;
	
		list_del(&tmp->pg_list);
		buddy = zone->zone_free_buddys + order;
		buddy->nr_free--;

		tmp->flags &= ~__PG_PRIVATE;
		tmp->private = 0;
	
		page_idx &= buddy_idx;
		order++;
	}

	coalesced = base + page_idx;

	coalesced->flags |= __PG_PRIVATE;
	coalesced->private = (void *)order;

	list_add(&coalesced->pg_list, &zone->zone_free_buddys[order].free_list);
	zone->zone_free_buddys[order].nr_free++;
}

static struct page_desc *
expand_buddy(struct mem_zone *zone, struct page_desc *p_desc,
				   int low, int high, struct free_buddy *buddy)
{
	unsigned long size = 1 << high;

	while (high > low) {
		buddy--;
		high--;
		size >>= 1;
		//BUG_ON(bad_range(zone, &page[size]));
		list_add(&p_desc[size].pg_list, &buddy->free_list);
		buddy->nr_free++;

		p_desc[size].flags |= __PG_PRIVATE;
		p_desc[size].private = (void *)high;
	}
	return p_desc;
}

static struct page_desc *
__alloc_pages_from_buddy(struct mem_zone *zone, unsigned int order)
{
	struct free_buddy * buddy;
	unsigned int current_order;
	struct page_desc *p_desc;

	if (zone->zone_nr_pages == 0)
		return NULL;

	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		buddy = zone->zone_free_buddys + current_order;
		if (list_empty(&buddy->free_list))
			continue;

		p_desc = list_entry(buddy->free_list.next, struct page_desc, pg_list);
		list_del(&p_desc->pg_list);

		p_desc->flags &= ~__PG_PRIVATE;
		p_desc->private = 0;		

		buddy->nr_free--;
		zone->zone_free_pages -= 1UL << order;

		return expand_buddy(zone, p_desc, order, current_order, buddy);
	}

	return NULL;
}

void *dim_sum_pages_alloc(unsigned int order, unsigned int opt)
{
	struct mem_zone *zone = &ph_numa_node[0].zone[opt];  /* 当前只有一个numa node */
	struct page_desc *p_desc;
	unsigned long flags;

	if (order >= MAX_ORDER)
		return NULL;

	spin_lock_irqsave(&zone->zone_lock, flags);

	p_desc = __alloc_pages_from_buddy(zone, order);
	if (!p_desc) {
		spin_unlock_irqrestore(&zone->zone_lock, flags);
		return NULL;
	}

	p_desc->private = (void *)order;
	p_desc->flags &= ~__PG_FREE;

	spin_unlock_irqrestore(&zone->zone_lock, flags);

	return (void *)page_desc_to_vaddr(p_desc);
}

int dim_sum_pages_free(void *addr)
{
	struct mem_zone *zone;
	struct page_desc *p_desc;
	unsigned long flags;
	int ret = -1;

	if ((unsigned long)addr & ~PAGE_MASK) {
		printk("BUG at %s, %d\n", __FUNCTION__, __LINE__);
		dump_stack();
		return ret;
	}

	zone = vaddr_to_zone((unsigned long)addr);
	if (!zone) {
		printk("BUG at %s, %d\n", __FUNCTION__, __LINE__);
		dump_stack();
		return ret;
	}

	spin_lock_irqsave(&zone->zone_lock, flags);

	p_desc = vaddr_to_page_desc((unsigned long)addr);
	if (p_desc->flags & __PG_FREE) {
		printk("BUG at %s, %d\n", __FUNCTION__, __LINE__);
		dump_stack();
		goto end;
	}

	__free_pages_to_buddy(p_desc, zone, (unsigned int)p_desc->private);
	p_desc->flags |= __PG_FREE;
	ret = 0;

end:
	spin_unlock_irqrestore(&zone->zone_lock, flags);
	return ret;
}


