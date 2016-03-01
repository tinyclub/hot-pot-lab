/**
 *
 */
#include <dim-sum/base.h>
#include <dim-sum/init.h>
#include <dim-sum/reboot.h>
//#include <dim-sum/mm_types.h>
#include <dim-sum/sort.h>
#include <dim-sum/debug.h>
#include <dim-sum/mem.h>
#include <asm/cputype.h>
#include <asm/procinfo.h>
#include <asm/setup.h>
#include <asm/prom.h>
#include <asm/mach/arch.h>
#include <asm/sections.h>
#include <asm/memory.h>
#include <asm/mach-types.h>
#include "atags.h"

//物理页面总数
unsigned long num_physpages;


unsigned int processor_id;
unsigned int __machine_arch_type;
unsigned int __initdata __atags_pointer;
//unsigned long cr_no_alignment;	/* defined in entry-armv.S */
//unsigned long cr_alignment;	/* defined in entry-armv.S */

#ifdef CONFIG_MULTI_IRQ_HANDLER
extern void (*handle_arch_irq)(struct pt_regs *);
#endif


/**
 * 桩函数，防止 EABI编译器报错
 */
void __aeabi_unwind_cpp_pr0(void)
{
};

void __aeabi_unwind_cpp_pr1(void)
{
};

/**
 * gcc软浮点编译用
 */
asmlinkage void __div0(void)
{
	printk("Division by zero in kernel.\n");
	dump_stack();
	while (1);
}

int mmu_enabled = 0;
void __init early_print(const char *str, ...)
{
	extern void printascii(const char *);
	char buf[256];
	va_list ap;

	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);

#ifdef CONFIG_DEBUG_LL
	if (!mmu_enabled) 
	{
		debug_printstr_nommu(buf);
	}
	else
	{
		debug_printstr_mmu(buf);
	}
#endif
	printk("%s", buf);
}


/* Untouched command line saved by arch-specific code. */
char __initdata boot_command_line[COMMAND_LINE_SIZE];

const struct machine_desc *machine_desc;
enum reboot_mode reboot_mode;

static char __initdata cmd_line[COMMAND_LINE_SIZE];

extern struct mm_struct init_mm;
extern struct meminfo boot_meminfo;
extern void probe_processor(void);
extern void setup_dma_zone(const struct machine_desc *mdesc);
extern void sanity_check_meminfo(void);
extern void v7_coherent_kern_range(unsigned long, unsigned long);

static int __init meminfo_cmp(const void *_a, const void *_b)
{
	const struct membank *a = _a, *b = _b;
	long cmp = bank_pfn_start(a) - bank_pfn_start(b);
	return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}


static inline u32 read_sctlr(void)
{
	u32 u;

	__asm__("mrc p15, 0, %0, c1, c0, 0" : "=r" (u));

	return u;
}


void __init early_trap_init(void)
{

	unsigned long vectors = CONFIG_VECTORS_BASE;

	extern char __stubs_start[], __stubs_end[];
	extern char __vectors_start[], __vectors_end[];


	/*
	 * Copy the vectors, stubs and kuser helpers (in entry-armv.S)
	 * into the vector page, mapped at 0xffff0000, and ensure these
	 * are visible to the instruction stream.
	 */
	memcpy((void *)vectors, __vectors_start, __vectors_end - __vectors_start);
	memcpy((void *)vectors + 0x200, __stubs_start, __stubs_end - __stubs_start);

	//flush_icache_range(vectors, vectors + PAGE_SIZE);
	v7_coherent_kern_range(vectors, vectors + PAGE_SIZE);
	//v7_coherent_kern_range(vectors, vectors + __vectors_end - __vectors_start);
	printk("__vectors_start 0x%p __vectors_end 0x%p\n", __vectors_start, __vectors_end);
	printk("early_trap_init\n");
	printk("sctlr 0x%x\n", read_sctlr());
}



extern int mmu_enabled;
/**
 * 体系结构相关的初始化
 */
void start_arch(void)
{
	const struct machine_desc *mdesc;

	mmu_enabled = 1;
	
	probe_processor();

	/**
	 * 解析boot传递过来的dtb文件
	 * 对335x来说，仍然是通过传统的方式传递参数的。
	 */
	mdesc = setup_machine_fdt(__atags_pointer);
	if (!mdesc)/* 如果没有传递dtb文件，则通过传递方式解析参数 */
		mdesc = setup_machine_tags(__atags_pointer,  machine_arch_type);

	/* 不支持的单板? */
	if (!mdesc)
	{
		debug_printstr_mmu("xby_debug in start_arch, step 2!\n");
		BUG();
	}

	machine_desc = mdesc;

	setup_dma_zone(mdesc);

	if (mdesc->reboot_mode != REBOOT_HARD)
		reboot_mode = mdesc->reboot_mode;

#ifdef xby_debug
	init_mm.start_code = (unsigned long) _text;
	init_mm.end_code   = (unsigned long) _etext;
	init_mm.end_data   = (unsigned long) _edata;
	init_mm.brk	   = (unsigned long) _end;
#endif

	strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);

	//dim-sum就不对参数进行处理了。
	//parse_early_param();

	sort(&boot_meminfo.bank, boot_meminfo.nr_banks, sizeof(boot_meminfo.bank[0]), meminfo_cmp, NULL);
	sanity_check_meminfo();

#ifdef CONFIG_MULTI_IRQ_HANDLER
	handle_arch_irq = mdesc->handle_irq;
#endif

	early_trap_init();

	if (mdesc->init_early)
		mdesc->init_early();
}

void __init time_init(void)
{
	if (machine_desc->init_time) {
		machine_desc->init_time();
	}
}




