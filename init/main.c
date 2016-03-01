#include <dim-sum/threads.h>
#include <dim-sum/irqflags.h>
#include <dim-sum/smp.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <dim-sum/delay.h>
#include <dim-sum/irq.h>
#include <dim-sum/init.h>
#include <dim-sum/debug.h>
#include <dim-sum/usr_app_entry.h>
#include <dim-sum/thread_info.h>
#include <dim-sum/idle.h>
#include <dim-sum/sched.h>
#include <dim-sum/mem.h>
#include <dim-sum/timer.h>
#include <fs/fs.h>
#include <dim-sum/syscall.h>
#include <dim-sum/boot_allotter.h>

/* 内核初始化过程中，每个核的SP指针 */
unsigned long kernelsp[NR_CPUS];


asmlinkage void work_notifysig(void)
{
}

asmlinkage void do_notify_resume(void)
{
}


asmlinkage void do_syscall_trace(struct pt_regs *regs, int entryexit)
{
}

asmlinkage void schedule_tail(struct task_struct *prev)
{
}

asmlinkage void resume_kernel(void)
{
}

void panic(const char * fmt, ...)
{
	printk(fmt);
	while (1);
}


dev_t ROOT_DEV;

unsigned long loops_per_jiffy = 3590144;



/**
 * 从核初始化
 */
asmlinkage __cpuinit void start_slave(void)
{

}

extern unsigned long num_physpages;
static void __init boot_cpu_init(void)
{
	int cpu = smp_processor_id();
	/* 主核总是可用的 */
	set_cpu_online(cpu, true);
	set_cpu_active(cpu, true);
	set_cpu_present(cpu, true);
	set_cpu_possible(cpu, true);
}


#if 0
extern initcall_t __initcall_start[], __initcall_end[];
static int run_initcall(void) 
{
	initcall_t *call;
	for (call = __initcall_start; call < __initcall_end; call++)
	{
		(*call)();
	}

	return 0;
}
#endif

extern void __init_klibc(void);


#define __makedev(__ma, __mi) \
	((dev_t)((((__ma) & 0xfffU) << 8)| \
		 ((__mi) & 0xffU)|(((__mi) & 0xfff00U) << 12)))
static dev_t _makedev(int __ma, int __mi)
{
	return __makedev(__ma, __mi);
}
#define makedev(__ma, __mi) _makedev(__ma, __mi)

/**
 * 在开中断的情况下运行，此时可以睡眠。
 */
static __maybe_unused void rest_init(void)
{
	int ret;
	//目前主要是进行文件系统的初始化。
	sys_setup();
	__init_klibc();

#if 1
	ret = mkdir("/dev", 0);
	//creat("/--linux-.---", 0);
	//ret = creat("/dev/--linux-.---", 0);
	ret = mknod("/dev/ttyS0", 0, makedev(4, 0));
#endif
}

extern void blk_dev_init(void);
extern asmlinkage int sys_setup(void);

extern void omap_serial_init(void);
extern void omap_gpio_init(void);

void __init serial_init(void)
{
	omap_serial_init();
}

#define FS
static int keventd(void *data)
{
	return 0;
}

extern void usrAppInit(void);
void TaskEntry(void)
{
		SysTaskCreate(keventd,
			NULL,
			"keventd",
			5,
			NULL,
			0
		);
#ifdef FS
			rest_init();
#endif

		usrAppInit();
}

extern void createSysTask(void);
extern void buffer_init_early(void);
extern void buffer_init_tail(void);
extern void tty_init(void);
extern int mmc_init(void);


/**
 * 主核初始化
 */
asmlinkage void __init start_master(void)
{
	debug_printstr_mmu("xby_debug, start cpu 0\n");

	local_irq_disable();

	//tick_init();
	
	/* 为当前CPU设置其活动掩码 */
	boot_cpu_init();
	/* 体系结构特定的初始化过程 */
	start_arch();

	//初始化boot内存分配器
	InitBootMemory(0xc0000000 + 8 * 1024 * 1024, 0xc0000000 + 24 * 1024 * 1024);
	
	init_IRQ();
	//体系结构和驱动的一些初始化过程
	//run_initcall();

	//为VFS分配一些大的内存块，用于哈希表。此过程必须在boot阶段分配。
	//vfs_caches_init_early();

#ifdef FS
	blk_dev_init();
	inode_init();
	file_table_init();
	name_cache_init();
	buffer_init_early();
#endif
	memory_init();
#ifdef FS
	buffer_init_tail();
	tty_init();
#endif
	sched_init();
	time_init();
	setup_timer();
	serial_init();
	mmc_init();
	omap_gpio_init();
	//初始化文件系统
	//vfs_caches_init(num_physpages);
	local_irq_enable();

	//创建系统任务，以及用户任务入口
	TaskEntry();
	cpu_idle();

	//不可能运行到这里来
	BUG();
}

