/**
 * boot内存分配器
 */
#ifndef _PHOENIX_BOOT_ALLOTTER_H
#define _PHOENIX_BOOT_ALLOTTER_H

#define HASH_HIGHMEM	0x00000001	/* Consider highmem? */
#define HASH_EARLY	0x00000002	/* Allocating during early boot? */

/**
 * 要分配boot内存的模块标识，内存安全考虑
 * 不同的模块，可以在不同的内存区域中分配boot内存
 * 可由各模块负责人任意定义模块标识
 */
// 不区分的模块，默认用此标识
#define	BOOT_MEMORY_MODULE_UNKNOWN	0
// IO模块
#define	BOOT_MEMORY_MODULE_IO		1
// 调度模块
#define	BOOT_MEMORY_MODULE_SCHED	2

/**
 * boot内存分配分为永久内存分配和临时内存分配
 * 永久内存分配在系统启动后不释放
 * 临时内存分配在系统启动后统一释放
 */
// boot永久内存分配标识
#define BOOT_PERMANENT_MEM		1
// boot临时内存分配标识
#define BOOT_TEMP_MEM		2
// boot保留内存分配标识
#define BOOT_RESERVE_MEM	3

/**
 * 分配出去的boot块管理结构
 */
struct BootMemoryBlockHeader
{
	int 			ModuleID;	/* boot内存分配块ID */
	int				Size;		/* boot内存分配大小*/
	unsigned long	MemStart;	/* boot内存分配起始地址 */
};

// 保留内存大小
#define BOOT_RESERVE_MEM_SIZE		(1024 * 1024)

// 保留内存块大小
#define BOOT_RESERVE_MEM_BLK_SIZE	(8 * 1024)

/**
 * boot保留内存管理结构
 */
struct BootReserveMemBlkHeader
{
	struct BootReserveMemBlkHeader *next;	/* 指向下一个boot保留内存块管理结构体 */
	unsigned long ReserveMemBlkAddr;		/* 保留内存块起始地址 */
	unsigned int  ReserveMemBlkSize;		/* 保留内存块大小 */
	unsigned int  ReserveMemBlkUsed;		/* 是否使用保留内存块 */
};

void InitBootMemory(unsigned long start, unsigned long end);
void* AllocTempBootMemory(int ModuleID, int Size, int Align);
void* AllocPermanentBootMemory(int ModuleID, int Size, int Align);
void* AllocStack(void);
void  FreeStack(void * Address);

/**
 * 分配boot内存
 */
void* AllocBootMemory(int ModuleID, int Size, int Align, int MemType);

/**
 * 释放boot内存
 */
void FreeBootMemory(void* Address, int MemType);

#endif

