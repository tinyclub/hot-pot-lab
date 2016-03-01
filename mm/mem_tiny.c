/*
 * TINY主要用于分配小于一个页面的小内存
 */

#include <dim-sum/mem.h>


#define TINY_MINALIGN sizeof(unsigned long)

#define TINY_UNITS(size) (((size) + TINY_UNIT - 1)/TINY_UNIT)

#define TINY_BREAK1 256
#define TINY_BREAK2 1024

unsigned long stats_tiny_used_units = 0;    //已使用的tiny数量
unsigned long stats_tiny_free_units = 0;    //剩余的tiny数量(从buddy中分配内存页未使用完的)
unsigned long stats_tiny_pages = 0;          //总共从buddy中分配了多少页用于tiny

static LIST_HEAD(free_tiny_small);
static LIST_HEAD(free_tiny_medium);
static LIST_HEAD(free_tiny_large);

static DEFINE_SPINLOCK(tiny_lock);

extern struct page_desc *vaddr_to_page_desc(unsigned long vaddr);
extern unsigned long page_desc_to_vaddr(struct page_desc *desc);

static void set_tiny(tiny_t *s, tinyidx_t size, tiny_t *next)
{
	tiny_t *base = (tiny_t *)((unsigned long)s & PAGE_MASK);
	tinyidx_t offset = next - base;

	if (size > 1) {
		s[0].units = size;
		s[1].units = offset;
	} else
		s[0].units = -offset;
}

static tinyidx_t tiny_units(tiny_t *s)
{
	if (s->units > 0)
		return s->units;
	return 1;
}

static tiny_t *tiny_next(tiny_t *s)
{
	tiny_t *base = (tiny_t *)((unsigned long)s & PAGE_MASK);
	tinyidx_t next;

	if (s[0].units < 0)
		next = -s[0].units;
	else
		next = s[1].units;
	return base+next;
}

static int tiny_last(tiny_t *s)
{
	return !((unsigned long)tiny_next(s) & ~PAGE_MASK);
}

static void *tiny_page_alloc(struct page_desc *p_desc, size_t size, int align)
{
	tiny_t *prev, *cur, *aligned = NULL;
	int delta = 0, units = TINY_UNITS(size);

	for (prev = NULL, cur = p_desc->free_tiny; ; prev = cur, cur = tiny_next(cur)) {
		tinyidx_t avail = tiny_units(cur);

		if (align) {
			aligned = (tiny_t *)ALIGN((unsigned long)cur, align);
			delta = aligned - cur;
		}
		if (avail >= units + delta) {
			tiny_t *next;

			if (delta) {
				next = tiny_next(cur);
				set_tiny(aligned, avail - delta, next);
				set_tiny(cur, delta, aligned);
				prev = cur;
				cur = aligned;
				avail = tiny_units(cur);
			}

			next = tiny_next(cur);
			if (avail == units) {
				if (prev)
					set_tiny(prev, tiny_units(prev), next);
				else
					p_desc->free_tiny= next;
			} else {
				if (prev)
					set_tiny(prev, tiny_units(prev), cur + units);
				else
					p_desc->free_tiny = cur + units;
				set_tiny(cur + units, avail - units, next);
			}

			p_desc->free_units -= units;
			if (!p_desc->free_units) {
				list_del(&p_desc->tiny_list);
				p_desc->flags &= ~__PG_TINY_FREE;
			}

			stats_tiny_used_units += units;
			stats_tiny_free_units -= units;

			return cur;
		}
		if (tiny_last(cur))
			return NULL;
	}
}

static void *mem_tiny_alloc(size_t size, unsigned int opt, int align)
{
	struct page_desc *p_desc;
	struct list_head *prev;
	struct list_head *tiny_list;
	tiny_t *b = NULL;
	unsigned long flags;

	if (size < TINY_BREAK1)
		tiny_list = &free_tiny_small;
	else if (size < TINY_BREAK2)
		tiny_list = &free_tiny_medium;
	else
		tiny_list = &free_tiny_large;

	spin_lock_irqsave(&tiny_lock, flags);

	list_for_each_entry(p_desc, tiny_list, tiny_list) {
		if (p_desc->free_units < TINY_UNITS(size))
			continue;

		prev = p_desc->tiny_list.prev;
		b = tiny_page_alloc(p_desc, size, align);
		if (!b)
			continue;

		if (prev != tiny_list->prev &&
				tiny_list->next != prev->next)
			list_move_tail(tiny_list, prev->next);
		break;
	}
	spin_unlock_irqrestore(&tiny_lock, flags);

	if (!b) {
		b = dim_sum_pages_alloc(0, opt);
		if (!b)
			return NULL;
		p_desc = vaddr_to_page_desc((unsigned long)b);
		p_desc->flags |= __PG_TINY;

		stats_tiny_pages++;

		spin_lock_irqsave(&tiny_lock, flags);
		p_desc->free_units = TINY_UNITS(PAGE_SIZE);
		p_desc->free_tiny = b;
		INIT_LIST_HEAD(&p_desc->tiny_list);
		set_tiny(b, TINY_UNITS(PAGE_SIZE), b + TINY_UNITS(PAGE_SIZE));

		list_add(&p_desc->tiny_list, tiny_list);
		p_desc->flags |= __PG_TINY_FREE;

		stats_tiny_free_units += TINY_UNITS(PAGE_SIZE);
		
		b = tiny_page_alloc(p_desc, size, align);
		spin_unlock_irqrestore(&tiny_lock, flags);
	}

	return b;
}

static int mem_tiny_free(void *block, int size)
{
	struct page_desc *p_desc;
	tiny_t *prev, *next, *b = (tiny_t *)block;
	tinyidx_t units;
	unsigned long flags;

	if (!block)
		return -1;

	p_desc = vaddr_to_page_desc((unsigned long)block);
	if (!p_desc)
		return -1;

	units = TINY_UNITS(size);

	spin_lock_irqsave(&tiny_lock, flags);

	stats_tiny_used_units -= units;
	stats_tiny_free_units += units;

	if (p_desc->free_units + units == TINY_UNITS(PAGE_SIZE)) {
		if (p_desc->flags & __PG_TINY_FREE) {
			list_del(&p_desc->tiny_list);
			p_desc->flags &= ~__PG_TINY_FREE;

			stats_tiny_free_units -= TINY_UNITS(PAGE_SIZE);
		}
		spin_unlock_irqrestore(&tiny_lock, flags);

		p_desc->flags &= ~__PG_TINY;
		stats_tiny_pages--;
		dim_sum_pages_free((void *)page_desc_to_vaddr(p_desc));

		return 0;
	}

	if (!(p_desc->flags & __PG_TINY_FREE)) {
		p_desc->free_units = units;
		p_desc->free_tiny= b;
		set_tiny(b, units,
			(void *)((unsigned long)(b +
					TINY_UNITS(PAGE_SIZE)) & PAGE_MASK));
		list_add(&p_desc->tiny_list, &free_tiny_small);
		p_desc->flags|= __PG_TINY_FREE;
		goto out;
	}

	p_desc->free_units += units;

	if (b < p_desc->free_tiny) {
		if (b + units == p_desc->free_tiny) {
			units += tiny_units(p_desc->free_tiny);
			p_desc->free_tiny = tiny_next(p_desc->free_tiny);
		}

		set_tiny(b, units, p_desc->free_tiny);
		p_desc->free_tiny = b;
	} else {
		prev = p_desc->free_tiny;
		next = tiny_next(prev);
		while (b > next) {
			prev = next;
			next = tiny_next(prev);
		}

		if (!tiny_last(prev) && b + units == next) {
			units += tiny_units(next);
			set_tiny(b, units, tiny_next(next));
		} else
			set_tiny(b, units, next);

		if (prev + tiny_units(prev) == b) {
			units = tiny_units(b) + tiny_units(prev);
			set_tiny(prev, units, tiny_next(b));
		} else
			set_tiny(prev, tiny_units(prev), b);
	}

out:
	spin_unlock_irqrestore(&tiny_lock, flags);
	return 0;
}

static void __dump_tiny_info(struct list_head *tiny_list_head)
{
	struct page_desc *p_desc;
	unsigned long flags;

	spin_lock_irqsave(&tiny_lock, flags);

	list_for_each_entry(p_desc, tiny_list_head, tiny_list) {
		printk("    page addr:0x%lx, free_units:%u\n", page_desc_to_vaddr(p_desc), 
											     p_desc->free_units);
	}

	spin_unlock_irqrestore(&tiny_lock, flags);
}

void dump_tiny_info(void)
{
	printk("Dump Tiny Info:\n");
	printk("    used_units: %lu\n", stats_tiny_used_units);
	printk("    free_units: %lu\n", stats_tiny_free_units);
	printk("    tiny_pages: %lu\n", stats_tiny_pages);
	printk("    unit_size: %u\n", TINY_UNIT);

	printk("\nsmall tiny(<256):\n");
	__dump_tiny_info(&free_tiny_small);

	printk("\nmedium tiny(<1024):\n");
	__dump_tiny_info(&free_tiny_medium);

	printk("\nlarge tiny(<4096):\n");
	__dump_tiny_info(&free_tiny_large);
}

void *dim_sum_mem_alloc(int size, unsigned int opt)
{
	unsigned int *mem;
	int align = TINY_MINALIGN;
	void *ret = NULL;

	if (!size)
		return ret;
	
	/* 申请的大小需要为TINY_UNIT的整数倍 */
	size = ((size + TINY_UNIT - 1) / TINY_UNIT) * TINY_UNIT; 

	/* 申请大小不足1页，从tiny中分配 */
	if (size < PAGE_SIZE - align) {
		mem = mem_tiny_alloc(size + align, opt, align);
		if (!mem)
			return NULL;

		/* 将真实大小保存在头四字节中 */
		*mem = size;
		ret = (void *)mem + align;
	} else {
		/* 申请大小超过1页，直接分配整数个页面 */
		ret = dim_sum_pages_alloc(get_order(size), opt);
	}

	return ret;
}

int dim_sum_mem_free(void *addr)
{
	struct page_desc *p_desc;
	int ret = -1;

	if (!addr)
		return ret;

	p_desc = vaddr_to_page_desc((unsigned long)addr);
	if (!p_desc)
		return ret;

	/* 页面为tiny页，走tiny的释放流程 */
	if (p_desc->flags & __PG_TINY) {
		int align = TINY_MINALIGN;
		unsigned int *mem = (unsigned int *)(addr - align);

		/* mem为真实起始地址 */
		ret = mem_tiny_free(mem, *mem + align);
	} else {
		/* 非tiny页，直接释放页 */
		ret = dim_sum_pages_free(addr);
	}

	return ret;
}

struct kmem_cache *dim_sum_memcache_create(char *name, int size, unsigned int opt)
{
	struct kmem_cache *cache;

	if ((size == 0) || (opt > MEM_HIGH))
		return NULL;

	cache = mem_tiny_alloc(sizeof(struct kmem_cache),
						   MEM_NORMAL, TINY_MINALIGN);

	if (cache) {
		cache->size = size;
		cache->align = TINY_MINALIGN;
		cache->opt= opt;
		cache->name = name;
	}

	return cache;
}

int dim_sum_memcache_destroy(struct kmem_cache *p_cache)
{
	return mem_tiny_free(p_cache, sizeof(struct kmem_cache));
}

void *dim_sum_memcache_alloc(struct kmem_cache *p_cache)
{
	void *ret;

	if (p_cache->size < PAGE_SIZE) {
		ret = mem_tiny_alloc(p_cache->size, p_cache->opt, p_cache->align);
	} else {
		ret = dim_sum_pages_alloc(get_order(p_cache->size), p_cache->opt);
	}

	return ret;
}

int dim_sum_memcache_free(struct kmem_cache *p_cache, void *p_obj)
{
	int ret;

	if (p_cache->size < PAGE_SIZE) {
		ret = mem_tiny_free(p_obj, p_cache->size);
	} else {
		ret = dim_sum_pages_free(p_obj);
	}

	return ret;
}


