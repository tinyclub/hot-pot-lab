#include <base/string.h>
#include <dim-sum/mem.h>


extern struct numa_node ph_numa_node[MAX_NUMA_NODES];
extern void dump_all_zones_info(int detail);
extern void dump_tiny_info(void);
extern unsigned long stats_tiny_free_units;

int sh_showmem_cmd(int argc, char **args)
{
	struct numa_node *node;
	struct mem_zone *zone;
	unsigned long total_size = 0, free_size = 0;
	int i, j;

	if (argc > 2) {
		printk("Usage: showmem/showmem zone|tiny\n");
		return -1;
	}

	for (i = 0; i < MAX_NUMA_NODES; i++) {
		node = &ph_numa_node[i];
		
		for (j = 0; j < ZONE_MAX_NUM; j++) {
			zone = &node->zone[j];

			if (zone->zone_nr_pages == 0)
				continue;

			total_size += zone->zone_nr_pages;
			free_size += zone->zone_free_pages;
		}
	}

	total_size = total_size << PAGE_SHIFT;
	free_size = free_size << PAGE_SHIFT;

	/* 加上tiny中空闲单元的内存 */
	free_size += stats_tiny_free_units * TINY_UNIT;

	printk("Total Memory: %lu M\n", total_size/(1024*1024));
	printk("Free  Memory: %lu M\n\n", free_size/(1024*1024));

	if (argc == 1)
		return 0;

	if (!strcmp(args[1], "zone")) {
		dump_all_zones_info(1);
	} else if (!strcmp(args[1], "tiny")) {
		dump_tiny_info();
	} else {
		printk("Usage: showmem/showmem zone|tiny\n");
		return -1;
	}

	return 0;
}

