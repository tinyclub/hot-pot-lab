/**
 * 在boot阶段调试时，向串口等设备输出字符串
 */
#include <linux/linkage.h>
#include <linux/types.h>


#include <dim-sum/of.h>
#include <dim-sum/of_fdt.h>
#include <dim-sum/debug.h>
#include <dim-sum/printk.h>
#include <dim-sum/delay.h>

/**
 * 调试串口的物理地址和虚拟地址
 * 虚拟地址是指内核在boot阶段，还没有调用page_init前的虚拟地址。
 * 在调用page_init后，可以直接调用printk打印。
 */
//#define OMAP3_UART3_BASE	0x49020000
//#define DEBUG_PHY_AM33XX_UART1_BASE	0x49020000
//#define DEBUG_VIRT_AM33XX_UART1_BASE	0xFB020000
#define DEBUG_PHY_AM33XX_UART1_BASE   0x10009000
#define DEBUG_VIRT_AM33XX_UART1_BASE  0xC5009000

//#define DEBUG_PHY_AM33XX_UART1_BASE	0x44E09000
//#define DEBUG_VIRT_AM33XX_UART1_BASE	0xF9E09000
#define DEBUG_OMAP_PORT_SHIFT		2
#define DEBUG_UART_OMAP_MDR1		0x08	/* Mode definition register */
#define DEBUG_MDR1_MODE_MASK			0x07
#define DEBUG_UART_LSR	5	/* In:  Line Status Register */
#define DEBUG_UART_LSR_THRE		(0x20) /* Transmit-hold-register empty */
#define DEBUG_UART_LSR_TEMT		(0x40)
#define DEBUG_BOTH_EMPTY (DEBUG_UART_LSR_THRE | DEBUG_UART_LSR_TEMT)

#define DEBUG_UART_RX		0	/* In:  Receive buffer */
#define DEBUG_UART_TX		0	/* Out: Transmit buffer */


static asmlinkage void debug_putc_nommu(int c)
{
	volatile u8 *uart_base;
	int uart_shift;
	
	uart_base = (u8 *)DEBUG_PHY_AM33XX_UART1_BASE;
	uart_shift = DEBUG_OMAP_PORT_SHIFT;
	if (!uart_base)
		return;

	/* Check for UART 16x mode */
	if ((uart_base[DEBUG_UART_OMAP_MDR1 << uart_shift] & DEBUG_MDR1_MODE_MASK) != 0)
		return;

	while (!(uart_base[DEBUG_UART_LSR << uart_shift] & DEBUG_UART_LSR_THRE))
		barrier();
	uart_base[DEBUG_UART_TX << uart_shift] = c;
}

/**
 * 在未打开MMU时，调用此函数打印字符串
 */
asmlinkage void debug_printstr_nommu(const char *ptr)
{
	char c;

	while ((c = *ptr++) != '\0') {
		if (c == '\n')
			debug_putc_nommu('\r');
		debug_putc_nommu(c);
	}

	//flush();
}

asmlinkage void debug_printstr_nommu_devtree_magic(void)
{
	struct boot_param_header *devtree = (void *)0x80001000;
	
	early_print("xby_debug, devtree->magic is fail, %x, %x\n", devtree->magic, OF_DT_HEADER);
}

static inline void debug_wait_for_xmitr(void)
{
	volatile u8 *uart_base;
	int uart_shift;
	unsigned int status, tmout = 10000;
	uart_base = (u8 *)DEBUG_VIRT_AM33XX_UART1_BASE;
	uart_shift = DEBUG_OMAP_PORT_SHIFT;
	if (!uart_base)
		return;
	
	do {
		status = uart_base[DEBUG_UART_LSR << uart_shift];
		if (--tmout == 0)
			break;
		ndelay(1);
	} while ((status & DEBUG_BOTH_EMPTY) != DEBUG_BOTH_EMPTY);
}


static asmlinkage void debug_putc_mmu(int c)
{
#if 1
	volatile u8 *uart_base;
	int uart_shift;
	
	uart_base = (u8 *)DEBUG_VIRT_AM33XX_UART1_BASE;
	uart_shift = DEBUG_OMAP_PORT_SHIFT;
	if (!uart_base)
		return;

	/* Check for UART 16x mode */
	if ((uart_base[DEBUG_UART_OMAP_MDR1 << uart_shift] & DEBUG_MDR1_MODE_MASK) != 0)
		return;
	

	//while (!(uart_base[DEBUG_UART_LSR << uart_shift] & DEBUG_UART_LSR_THRE))
	//while ((uart_base[DEBUG_UART_LSR << uart_shift] & (0x20 | 0x40)) != (0x20 | 0x40))
	//	barrier();
	debug_wait_for_xmitr();
	uart_base[DEBUG_UART_TX << uart_shift] = c;
	debug_wait_for_xmitr();
#else
	volatile u8 *uart_base;
	int uart_shift;

	uart_base = (u8 *)DEBUG_VIRT_AM33XX_UART1_BASE;
	uart_shift = DEBUG_OMAP_PORT_SHIFT;
	if (!uart_base)
		return;

	/* Check for UART 16x mode */
	if ((uart_base[DEBUG_UART_OMAP_MDR1 << uart_shift] & DEBUG_MDR1_MODE_MASK) != 0)
		return;

	while (!(uart_base[DEBUG_UART_LSR << uart_shift] & DEBUG_UART_LSR_THRE))
		barrier();
	uart_base[DEBUG_UART_TX << uart_shift] = c;

#endif
}

/**
 * 在打开MMU时，调用此函数打印字符串
 */	
asmlinkage void debug_printstr_mmu(const char *ptr)
{
	char c;

	while ((c = *ptr++) != '\0') {
		if (c == '\n')
			debug_putc_mmu('\r');
		debug_putc_mmu(c);
	}
	debug_wait_for_xmitr();
	//flush();
}
