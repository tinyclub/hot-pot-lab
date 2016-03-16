#if 1
#include <linux/types.h>
#include <linux/serial_reg.h>

#include <mach/serial.h>

#define MDR1_MODE_MASK	0x07

volatile u8 *uart_base;
int uart_shift;

#define OMAP2_UART3_BASE 0x4806e000
#define OMAP_PORT_SHIFT 2
#define OMAP3UART3 33
#define OMAP_UART_INFO_OFS 0x3ffc


#define machine_is_omap3_beagle() (1)

#define _DEBUG_LL_ENTRY(mach, dbg_uart, dbg_shft, dbg_id)               \
        if (machine_is_##mach()) {                                      \
                uart_base = (volatile u8 *)(dbg_uart);                  \
                uart_shift = (dbg_shft);                                \
                port = (dbg_id);                                        \
                set_omap_uart_info(port);                               \
                break;                                                  \
        }

#define DEBUG_LL_OMAP3(p, mach)                                         \
        _DEBUG_LL_ENTRY(mach, OMAP3_UART##p##_BASE, OMAP_PORT_SHIFT,    \
                OMAP3UART##p)

static void set_omap_uart_info(unsigned char port)
{
        /*
         * Get address of some.bss variable and round it down
         * a la CONFIG_AUTO_ZRELADDR.
         */
        u32 ram_start = (u32)&uart_shift & 0xf8000000;
        u32 *uart_info = (u32 *)(ram_start + OMAP_UART_INFO_OFS);
        *uart_info = port;
}

void arch_decomp_setup(void)
{
	unsigned char port = 0;
	do {
		DEBUG_LL_OMAP3(3, omap3_beagle);
	}while (0);
}

void putc(int c)
{
        if (!uart_base)
                return;

        /* Check for UART 16x mode */
        if ((uart_base[UART_OMAP_MDR1 << uart_shift] & MDR1_MODE_MASK) != 0)
                return;

        while (!(uart_base[UART_LSR << uart_shift] & UART_LSR_THRE))
                barrier();
        uart_base[UART_TX << uart_shift] = c;
}

void flush(void)
{
}
#else
void arch_decomp_setup(void)
{
}
#endif
