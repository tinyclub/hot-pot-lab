
/**
 * boot内存分配器
 */
#include <base/string.h>
#include <dim-sum/base.h>
#include <dim-sum/boot_allotter.h>
#include <linux/log2.h>

#include <asm/page.h>
/**
 * boot内存分配起始和结束地址
 */
static unsigned long BootMemoryStart = 0, BootMemoryEnd = 0;

/**
 * boot永久内存分配和临时内存分配结束地址
 */
static unsigned long BootMemoryPtrLow, BootMemoryPtrHigh;

/**
 * boot保留内存管理结构体链表
 */
static struct BootReserveMemBlkHeader *BootReserveMemBlkList = NULL;

/**
 * 初始化Boot保留内存分配
 */
static void InitBootReserveMemory(int ModuleID, int Size, int Align)
{
	unsigned long BootReserveMemAddr   = 0;
	unsigned int  BootReserveMemBlkNum = 0;
	unsigned int  i                    = 0;
	struct BootReserveMemBlkHeader *p  = NULL;

	/* 分配Boot保留内存 */
	BootReserveMemAddr = (unsigned long)AllocPermanentBootMemory(ModuleID, Size, Align);
	if (BootReserveMemAddr == 0)
	{
		return;
	}

	/* 计算Boot保留内存块数目 */
	BootReserveMemBlkNum = (unsigned int)(Size / BOOT_RESERVE_MEM_BLK_SIZE);
	if (BootReserveMemBlkNum == 0)
	{
		return;
	}

	/* 分配Boot保留内存块管理链表内存 */
	BootReserveMemBlkList = (struct BootReserveMemBlkHeader *)AllocPermanentBootMemory(0, 
								BootReserveMemBlkNum * sizeof(struct BootReserveMemBlkHeader), 
								sizeof(struct BootReserveMemBlkHeader));
	if (BootReserveMemBlkList == NULL)
	{
		return;
	}

	/* 记录Boot保留内存块信息 */
	p = BootReserveMemBlkList;
	for (i = 0 ; i < BootReserveMemBlkNum; i++)
	{
		if (i == (BootReserveMemBlkNum - 1))
		{
			p->next = NULL;
		}
		else
		{
			p->next = (struct BootReserveMemBlkHeader *)((unsigned long)p + sizeof(struct BootReserveMemBlkHeader));
		}
		p->ReserveMemBlkAddr = BootReserveMemAddr + (BOOT_RESERVE_MEM_BLK_SIZE * i);
		p->ReserveMemBlkSize = BOOT_RESERVE_MEM_BLK_SIZE;
		p->ReserveMemBlkUsed = 0;
		p = p->next;
	}
	
	return;
}

/**
 * 初始化Boot内存分配器
 */
void InitBootMemory(unsigned long start, unsigned long end)
{
	memset((void *)start, 0, end - start);
	BootMemoryStart		= start;
	BootMemoryEnd		= end;

	BootMemoryPtrLow	= start;
	BootMemoryPtrHigh	= end;

	
	/* 初始化保留内存 */
	InitBootReserveMemory(0, BOOT_RESERVE_MEM_SIZE, BOOT_RESERVE_MEM_BLK_SIZE);
}

/**
 * 在boot内存区中分配保留内存
 */
void* AllocStack(void)
{
	struct BootReserveMemBlkHeader *p  = NULL;

	p = BootReserveMemBlkList;
	while (p != NULL)
	{
		if (p->ReserveMemBlkUsed == 1)
		{
			p = p->next;
			continue;
		}
		else
		{
			p->ReserveMemBlkUsed = 1;
			return (void *)p->ReserveMemBlkAddr;
		}
	}

	return NULL;
}

/**
 * 在boot内存区中释放保留内存
 */
void FreeStack(void* Address)
{
	struct BootReserveMemBlkHeader *p  = NULL;

	p = BootReserveMemBlkList;
	while (p != NULL)
	{
		if (p->ReserveMemBlkAddr != (unsigned long)Address)
		{
			p = p->next;
			continue;
		}
		else
		{
			p->ReserveMemBlkUsed = 0;
			return;
		}
	}

	return;
}

/**
 * 在boot内存区中分配临时的内存
 * 系统初始化完成后即返还给内核
 */
void* AllocTempBootMemory(int ModuleID, int Size, int Align)
{
	unsigned long start;
	struct BootMemoryBlockHeader *BootMemBlkHdr = NULL;

	/* boot内存未初始化 */
	if ((BootMemoryStart == 0)
		&& (BootMemoryEnd == 0))
	{
		BUG();
	}

	/**
	 * 64位系统默认8字节对齐,32位系统默认4字节对齐
	 */
	if (Align == 0)
	{
#ifdef CONFIG_64BIT
		Align = 8;
#else
		Align = 4;
#endif	
	}

	/* 预留boot内存分配所占内存空间 */
	start = BootMemoryPtrHigh - Size;

	/* 按对齐方式，计算boot内存分配实际起始地址 */
	start = start - start % Align;

	/* 移动boot临时内存分配地址指针 */
	BootMemoryPtrHigh = start - sizeof(struct BootMemoryBlockHeader);
	if (BootMemoryPtrHigh < BootMemoryPtrLow)
	{
		BUG();
	}	

	/* 记录boot内存块分配信息 */
	BootMemBlkHdr			= (struct BootMemoryBlockHeader *)(BootMemoryPtrHigh);
	BootMemBlkHdr->ModuleID = ModuleID;
	BootMemBlkHdr->Size 	= Size;
	BootMemBlkHdr->MemStart = start;

	return (void *)start;
}

/**
 * 在boot内存区中分配永久的内存
 * 系统初始化完成后不会返回给内核
 */
void* AllocPermanentBootMemory(int ModuleID, int Size, int Align)
{
	unsigned long start;
	struct BootMemoryBlockHeader *BootMemBlkHdr = NULL;

	/* boot内存未初始化 */
	if ((BootMemoryStart == 0)
		&& (BootMemoryEnd == 0))
	{
		BUG();
	}

	/**
	 * 64位系统默认8字节对齐,32位系统默认4字节对齐
	 */
	if (Align == 0)
	{
#ifdef CONFIG_64BIT
		Align = 8;
#else
		Align = 4;
#endif	
	}

	/* 预留boot内存块头所占内存空间 */
	start = BootMemoryPtrLow + sizeof(struct BootMemoryBlockHeader);

	/* 按对齐方式，计算boot内存分配实际起始地址 */
	start = start + Align;
	start = start - start % Align;

	/* 移动boot永久内存分配地址指针 */
	BootMemoryPtrLow = start + Size;
	if (BootMemoryPtrLow > BootMemoryPtrHigh)
	{
		BUG();
	}	

	/* 记录boot内存块分配信息 */
	BootMemBlkHdr			= (struct BootMemoryBlockHeader *)(start - sizeof(struct BootMemoryBlockHeader));
	BootMemBlkHdr->ModuleID = ModuleID;
	BootMemBlkHdr->Size		= Size;
	BootMemBlkHdr->MemStart	= start;

	return (void *)start;
}

/**
 * 分配boot内存
 */
void* AllocBootMemory(int ModuleID, int Size, int Align, int MemType)
{
	if (MemType == BOOT_PERMANENT_MEM)
	{
		/* 分配boot永久内存 */
		return AllocPermanentBootMemory(ModuleID, Size, Align);
	}
	else if (MemType == BOOT_TEMP_MEM)
	{
		/* 分配boot临时内存 */
		return AllocTempBootMemory(ModuleID, Size, Align);
	}
	else if (MemType == BOOT_RESERVE_MEM)
	{
		/* 分配boot保留内存 */
		return AllocStack();
	}
	else
	{
		/* boot内存分配类型错误 */
		//BUG();
	}

	return NULL;
}

/**
 * 释放Boot内存
 */
void FreeBootMemory(void* Address, int MemType)
{
	/**
	 * boot内存分配分为永久内存分配和临时内存分配
	 * 永久内存分配在系统启动后不释放
	 * 临时内存分配在系统启动后统一释放
	 */
	if (MemType == BOOT_PERMANENT_MEM)
	{
		/* 释放boot永久内存 */
		return;
	}
	else if (MemType == BOOT_TEMP_MEM)
	{
		/* 释放boot临时内存 */
		return;
	}
	else if (MemType == BOOT_RESERVE_MEM)
	{
		/* 释放boot保留内存 */
		FreeStack(Address);
		return;
	}
	else
	{
		/* boot内存释放类型错误 */
		//BUG();
	}

	return;
}

#if 0
/*
 * allocate a large system hash table from bootmem
 * - it is assumed that the hash table must contain an exact power-of-2
 *   quantity of entries
 * - limit is the number of hash buckets, not the total allocation size
 */
void *__init alloc_large_system_hash(const char *tablename,
				     unsigned long bucketsize,
				     unsigned long numentries,
				     int scale,
				     int flags,
				     unsigned int *_hash_shift,
				     unsigned int *_hash_mask,
				     unsigned long limit)
{
	unsigned long log2qty, size;
	void *table = NULL;

	/* 必须指定此参数 */
	if (!numentries) {
		return table;
	}
	/* rounded up to nearest power of 2 in size */
	numentries = 1UL << (long_log2(numentries) + 1);

	if ((limit != 0)
		&& (numentries > limit))
		numentries = limit;

	log2qty = long_log2(numentries);

	do {
		size = bucketsize << log2qty;
		table = AllocPermanentBootMemory(BOOT_MEMORY_MODULE_UNKNOWN, size, 0);
	} while (!table && size > PAGE_SIZE && --log2qty);

	if (!table)
		panic("Failed to allocate %s hash table\n", tablename);

	printk("%s hash table entries: %d (order: %d, %lu bytes)\n",
	       tablename,
	       (1U << log2qty),
	       long_log2(size) - PAGE_SHIFT,
	       size);

	if (_hash_shift)
		*_hash_shift = log2qty;
	if (_hash_mask)
		*_hash_mask = (1 << log2qty) - 1;

	return table;
}
#endif

