/**
 * ARM体系结构相关的内存初始化模块
 */
#include <dim-sum/base.h>
#include <base/string.h>

#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <asm/cachetype.h>
//#include <asm/pgtable.h>

struct meminfo boot_meminfo;

void __init setup_dma_zone(const struct machine_desc *mdesc)
{
#ifdef CONFIG_ZONE_DMA
	if (mdesc->dma_zone_size) {
		arm_dma_zone_size = mdesc->dma_zone_size;
		arm_dma_limit = PHYS_OFFSET + arm_dma_zone_size - 1;
	} else
		arm_dma_limit = 0xffffffff;
#endif
}

#ifdef xby_debug
static void * __initdata vmalloc_min =
	(void *)(VMALLOC_END - (240 << 20) - VMALLOC_OFFSET);
#endif

phys_addr_t __initdata arm_lowmem_limit = 0;

struct meminfo meminfo;
void * high_memory;

#ifdef xby_debug
void __init sanity_check_meminfo(void)
{
	phys_addr_t memblock_limit = 0;
	int i, j, highmem = 0;
	phys_addr_t vmalloc_limit = __pa(vmalloc_min - 1) + 1;


	for (i = 0, j = 0; i < meminfo.nr_banks; i++) {
		struct membank *bank = &meminfo.bank[j];
		phys_addr_t size_limit;

		*bank = meminfo.bank[i];
		size_limit = bank->size;

		if (bank->start >= vmalloc_limit)
			highmem = 1;
		else
			size_limit = vmalloc_limit - bank->start;

		bank->highmem = highmem;

#ifdef CONFIG_HIGHMEM
		/*
		 * Split those memory banks which are partially overlapping
		 * the vmalloc area greatly simplifying things later.
		 */
		if (!highmem && bank->size > size_limit) {
			if (meminfo.nr_banks >= NR_BANKS) {
				printk(KERN_CRIT "NR_BANKS too low, "
						 "ignoring high memory\n");
			} else {
				memmove(bank + 1, bank,
					(meminfo.nr_banks - i) * sizeof(*bank));
				meminfo.nr_banks++;
				i++;
				bank[1].size -= size_limit;
				bank[1].start = vmalloc_limit;
				bank[1].highmem = highmem = 1;
				j++;
			}
			bank->size = size_limit;
		}
#else
		/*
		 * Highmem banks not allowed with !CONFIG_HIGHMEM.
		 */
		if (highmem) {
			printk(KERN_NOTICE "Ignoring RAM at %.8llx-%.8llx "
			       "(!CONFIG_HIGHMEM).\n",
			       (unsigned long long)bank->start,
			       (unsigned long long)bank->start + bank->size - 1);
			continue;
		}

		/*
		 * Check whether this memory bank would partially overlap
		 * the vmalloc area.
		 */
		if (bank->size > size_limit) {
			printk(KERN_NOTICE "Truncating RAM at %.8llx-%.8llx "
			       "to -%.8llx (vmalloc region overlap).\n",
			       (unsigned long long)bank->start,
			       (unsigned long long)bank->start + bank->size - 1,
			       (unsigned long long)bank->start + size_limit - 1);
			bank->size = size_limit;
		}
#endif
		if (!bank->highmem) {
			phys_addr_t bank_end = bank->start + bank->size;

			if (bank_end > arm_lowmem_limit)
				arm_lowmem_limit = bank_end;

			/*
			 * Find the first non-section-aligned page, and point
			 * memblock_limit at it. This relies on rounding the
			 * limit down to be section-aligned, which happens at
			 * the end of this function.
			 *
			 * With this algorithm, the start or end of almost any
			 * bank can be non-section-aligned. The only exception
			 * is that the start of the bank 0 must be section-
			 * aligned, since otherwise memory would need to be
			 * allocated when mapping the start of bank 0, which
			 * occurs before any free memory is mapped.
			 */
			if (!memblock_limit) {
				if (!IS_ALIGNED(bank->start, SECTION_SIZE))
					memblock_limit = bank->start;
				else if (!IS_ALIGNED(bank_end, SECTION_SIZE))
					memblock_limit = bank_end;
			}
		}
		j++;
	}
#if 1
#ifdef CONFIG_HIGHMEM
	if (highmem) {
		const char *reason = NULL;

		if (cache_is_vipt_aliasing()) {
			/*
			 * Interactions between kmap and other mappings
			 * make highmem support with aliasing VIPT caches
			 * rather difficult.
			 */
			reason = "with VIPT aliasing cache";
		}
		if (reason) {
			printk(KERN_CRIT "HIGHMEM is not supported %s, ignoring high memory\n",
				reason);
			while (j > 0 && meminfo.bank[j - 1].highmem)
				j--;
		}
	}
#endif
	meminfo.nr_banks = j;
	high_memory = __va(arm_lowmem_limit - 1) + 1;

	/*
	 * Round the memblock limit down to a section size.  This
	 * helps to ensure that we will allocate memory from the
	 * last full section, which should be mapped.
	 */
	if (memblock_limit)
		memblock_limit = round_down(memblock_limit, SECTION_SIZE);
	if (!memblock_limit)
		memblock_limit = arm_lowmem_limit;

	//memblock_set_current_limit(memblock_limit);
#endif
}
#else
void __init sanity_check_meminfo(void)
{
}
#endif

