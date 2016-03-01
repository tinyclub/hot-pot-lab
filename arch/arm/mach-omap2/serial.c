
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <dim-sum/irq.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <dim-sum/init.h>
#include <dim-sum/spinlock.h>
#include <asm/mach/arch.h>
#include <asm/exception.h>
#include <asm/mach-types.h>
#include <asm/string.h>
#include <asm/param.h>
#include <mach/io.h>
#include <mach/serial.h>
#include <mach/mux.h>
#include <dim-sum/delay.h>



#define OMAP_UART_SCR_RX_TRIG_GRANU1_MASK		(1 << 7)
#define OMAP_UART_SCR_TX_TRIG_GRANU1_MASK		(1 << 6)

#define OMAP_UART_FCR_RX_FIFO_TRIG_MASK			(0x3 << 6)
#define OMAP_UART_FCR_TX_FIFO_TRIG_MASK			(0x3 << 4)


struct uart_omap_port {
	unsigned char		ier;
	unsigned char		lcr;
	unsigned char		mcr;
	unsigned char		fcr;
	unsigned char		efr;
	unsigned char		dll;
	unsigned char		dlh;
	unsigned char		mdr1;
	unsigned char		scr;
	unsigned char		wer;

	unsigned char __iomem	*membase;		/* read/write[bwl] */
	unsigned int		irq;			/* irq number */
	unsigned char		regshift;		/* reg offset shift */
	struct serial_icounter_struct icount;
	unsigned int baud;
	unsigned int uartclk;
	struct omap_pin_mux pin_mux[2];
	unsigned int clkctrl_offset;
	unsigned int id;
};

struct uart_omap_port g_uart_omap_port[6] = {
		{
			.regshift = 2,
			.irq = 72,
			.membase = AM33XX_L4_WK_IO_ADDRESS(AM33XX_UART1_BASE),
			.baud = 115200,
			.uartclk = 48000000,
			{
				{
					.pin_offset = 0x970,
					.pin_value = OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP,
				},
				{
					.pin_offset = 0x974,
					.pin_value = OMAP_MUX_MODE0 | AM33XX_PULL_ENBL,
				},
			},
			.id = 0,
		},

		{
			.regshift = 2,
			.irq = 73,
			.membase = OMAP2_L4_PER_IO_ADDRESS(AM33XX_UART2_BASE),
			.baud = 115200,
			.uartclk = 48000000,
			{
				{
					.pin_offset = 0x978,
					.pin_value = OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP,
				},
				{
					.pin_offset = 0x97c,
					.pin_value = OMAP_MUX_MODE0 | AM33XX_PULL_ENBL,
				},
			},
			.clkctrl_offset = 0x6c,
			.id = 1,
		},

		{
			.regshift = 2,
			.irq = 74,
			.membase = OMAP2_L4_PER_IO_ADDRESS(AM33XX_UART3_BASE),
			.baud = 9600,
			.uartclk = 48000000,
			{
				{
					.pin_offset = 0x950,
					.pin_value = OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLUP,
				},
				{
					.pin_offset = 0x954,
					.pin_value = OMAP_MUX_MODE1 | AM33XX_PULL_ENBL,
				},
			},
			.clkctrl_offset = 0x70,
			.id = 2,
		},

		{
			.regshift = 2,
			.irq = 44,
			.membase = OMAP2_L4_PER_IO_ADDRESS(AM33XX_UART4_BASE),
			.baud = 9600,
			.uartclk = 48000000,
			{
				{
					.pin_offset = 0x934,
					.pin_value = OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLUP,
				},
				{
					.pin_offset = 0x938,
					.pin_value = OMAP_MUX_MODE1 | AM33XX_PULL_ENBL,
				},
			},
			.clkctrl_offset = 0x74,
			.id = 3,
		},

		{
			.regshift = 2,
			.irq = 45,
			.membase = OMAP2_L4_PER_IO_ADDRESS(AM33XX_UART5_BASE),
			.baud = 9600,
			.uartclk = 48000000,
			{
				{
					.pin_offset = 0x91c,
					.pin_value = OMAP_MUX_MODE3 | AM33XX_PIN_INPUT_PULLUP,
				},
				{
					.pin_offset = 0x920,
					.pin_value = OMAP_MUX_MODE3 | AM33XX_PULL_ENBL,
				},
			},
			.clkctrl_offset = 0x78,
			.id = 4,
		},

		{
			.regshift = 2,
			.irq = 46,
			.membase = OMAP2_L4_PER_IO_ADDRESS(AM33XX_UART6_BASE),
			.baud = 9600,
			.uartclk = 48000000,
			{
				{
					.pin_offset = 0x8c4,
					.pin_value = OMAP_MUX_MODE4 | AM33XX_PIN_INPUT_PULLUP,
				},
				{
					.pin_offset = 0x8c0,
					.pin_value = OMAP_MUX_MODE4 | AM33XX_PULL_ENBL,
				},
			},
		},
};


static inline unsigned int serial_in(struct uart_omap_port *up, int offset)
{
	offset <<= up->regshift;
	return __raw_readl(up->membase + offset);
}

static inline void serial_out(struct uart_omap_port *up, int offset, int value)
{
	offset <<= up->regshift;
	__raw_writel(value, up->membase + offset);
}


#define SERIAL_BUF_LEN 1024
static char serial_buf[SERIAL_BUF_LEN];
static int read_pos = 0, data_pos = 0;
DEFINE_SPINLOCK(serial_spin_lock);

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)


static inline void wait_for_xmitr(struct uart_omap_port *up)
{
	unsigned int status, tmout = 10000;
	do {
		status = serial_in(up, UART_LSR);
		if (--tmout == 0)
			break;
		udelay(1);
	} while ((status & BOTH_EMPTY) != BOTH_EMPTY);
}

static void serial_omap_putchar(struct uart_omap_port *up, int ch)
{
	wait_for_xmitr(up);
	serial_out(up, UART_TX, ch);
}


int read_serial_char(char *buf)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&serial_spin_lock,flags);
	if (read_pos == data_pos)
	{
		//no data
	}
	else
	{
		*buf = serial_buf[read_pos];
		ret = 1;
		read_pos = (read_pos + 1) & (SERIAL_BUF_LEN - 1);
	}
	spin_unlock_irqrestore(&serial_spin_lock,flags);

	return ret;
}

int write_serial_char(unsigned int id, char *buf, unsigned int count)
{
	unsigned long flags;
	int ret = 0;
	unsigned int ier;
	unsigned int i;
	struct uart_omap_port *up;
	if (id > 4)
		return -1;

	up = &g_uart_omap_port[id];

	spin_lock_irqsave(&serial_spin_lock,flags);
	ier = serial_in(up, UART_IER);
	serial_out(up, UART_IER, 0);

	for (i = 0; i < count; i++, buf++) {
		if (*buf == '\n')
			serial_omap_putchar(up, '\r');
		serial_omap_putchar(up, *buf);
	}

	wait_for_xmitr(up);
	serial_out(up, UART_IER, ier);
	spin_unlock_irqrestore(&serial_spin_lock,flags);

	return ret;
}

static void serial_omap_rdi(struct uart_omap_port *up, unsigned int lsr)
{
	unsigned char ch = 0;
	int tmp;
	unsigned long flags;
	if (!(lsr & UART_LSR_DR))
		return;
	
	ch = serial_in(up, UART_RX);
	up->icount.rx++;

	if (ch == '`')
	{
		dump_stack();
	}

	spin_lock_irqsave(&serial_spin_lock,flags);
	tmp = (data_pos + 1) & (SERIAL_BUF_LEN - 1);
	if (tmp == read_pos)
	{
		//oh, no buffer
	}
	else
	{
		serial_buf[data_pos] = ch;
		data_pos = tmp;
	}

	spin_unlock_irqrestore(&serial_spin_lock,flags);
}

static void serial_omap_rlsi(struct uart_omap_port *up, unsigned int lsr)
{
	if (likely(lsr & UART_LSR_DR)) {
		serial_in(up, UART_RX);
	}

	if (lsr & UART_LSR_BI) {
		lsr &= ~(UART_LSR_FE | UART_LSR_PE);
		up->icount.brk++;
	}

	if (lsr & UART_LSR_PE) {
		up->icount.parity++;
	}

	if (lsr & UART_LSR_FE) {
		up->icount.frame++;
	}

	if (lsr & UART_LSR_OE)
		up->icount.overrun++;
}




static irqreturn_t serial_omap_irq(unsigned int irq, void *dev_id)
{
	struct uart_omap_port *up = dev_id;
	unsigned int iir, lsr;
	unsigned int type;
	irqreturn_t ret = IRQ_NONE;
	int max_count = 256;

	do {
		iir = serial_in(up, UART_IIR);
		if (iir & UART_IIR_NO_INT)
			break;

		ret = IRQ_HANDLED;
		lsr = serial_in(up, UART_LSR);

		/* extract IRQ type from IIR register */
		type = iir & 0x3e;

		switch (type) {
		case UART_IIR_MSI:
			//check_modem_status(up);
			break;
		case UART_IIR_THRI:
			//transmit_chars(up, lsr);
			break;
		case UART_IIR_RX_TIMEOUT:
			/* FALLTHROUGH */
		case UART_IIR_RDI:
			serial_omap_rdi(up, lsr);
			break;
		case UART_IIR_RLSI:
			serial_omap_rlsi(up, lsr);
			break;
		case UART_IIR_CTS_RTS_DSR:
			/* simply try again */
			break;
		case UART_IIR_XOFF:
			/* FALLTHROUGH */
		default:
			break;
		}
	} while (!(iir & UART_IIR_NO_INT) && max_count--);

	return ret;
}


static inline void serial_omap_clear_fifos(struct uart_omap_port *up)
{
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
		       UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	serial_out(up, UART_FCR, 0);
}

static bool
serial_omap_baud_is_mode16(struct uart_omap_port *up)
{
	unsigned int baud = up->baud;
	unsigned int n13 = up->uartclk / (13 * baud);
	unsigned int n16 = up->uartclk / (16 * baud);
	int baudAbsDiff13 = baud - (up->uartclk / (13 * n13));
	int baudAbsDiff16 = baud - (up->uartclk / (16 * n16));
	if (baudAbsDiff13 < 0)
		baudAbsDiff13 = -baudAbsDiff13;
	if (baudAbsDiff16 < 0)
		baudAbsDiff16 = -baudAbsDiff16;

	return (baudAbsDiff13 >= baudAbsDiff16);
}


static unsigned int
serial_omap_get_divisor(struct uart_omap_port *up)
{
	unsigned int mode;

	if (!serial_omap_baud_is_mode16(up))
		mode = 13;
	else
		mode = 16;
	return up->uartclk/(mode * up->baud);
}




static void
serial_omap_set_termios(struct uart_omap_port *up)
{
	unsigned int quot;
	unsigned char cval = 0;
	
	quot = serial_omap_get_divisor(up);

	cval = UART_LCR_WLEN8;
	
	up->dll = quot & 0xff;
	up->dlh = quot >> 8;
	up->mdr1 = UART_OMAP_MDR1_DISABLE;
	printk("quot 0x%x\n", quot);
	up->fcr = UART_FCR_R_TRIG_01 | UART_FCR_T_TRIG_01 |
			UART_FCR_ENABLE_FIFO;

	up->ier &= ~UART_IER_MSI;
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, cval);		/* reset DLAB */
	up->lcr = cval;
	up->scr = 0;

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_DLL, 0);
	serial_out(up, UART_DLM, 0);
	serial_out(up, UART_LCR, 0);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	up->efr = serial_in(up, UART_EFR) & ~UART_EFR_ECB;
	up->efr &= ~UART_EFR_SCD;
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	up->mcr = serial_in(up, UART_MCR) & ~UART_MCR_TCRTLR;
	serial_out(up, UART_MCR, up->mcr | UART_MCR_TCRTLR);
	/* FIFO ENABLE, DMA MODE */

	up->scr |= OMAP_UART_SCR_RX_TRIG_GRANU1_MASK;
	/*
	 * NOTE: Setting OMAP_UART_SCR_RX_TRIG_GRANU1_MASK
	 * sets Enables the granularity of 1 for TRIGGER RX
	 * level. Along with setting RX FIFO trigger level
	 * to 1 (as noted below, 16 characters) and TLR[3:0]
	 * to zero this will result RX FIFO threshold level
	 * to 1 character, instead of 16 as noted in comment
	 * below.
	 */

	/* Set receive FIFO threshold to 16 characters and
	 * transmit FIFO threshold to 32 spaces
	 */
	up->fcr &= ~OMAP_UART_FCR_RX_FIFO_TRIG_MASK;
	up->fcr &= ~OMAP_UART_FCR_TX_FIFO_TRIG_MASK;
	up->fcr |= UART_FCR6_R_TRIGGER_16 | UART_FCR6_T_TRIGGER_24 |
				UART_FCR_ENABLE_FIFO;

	serial_out(up, UART_FCR, up->fcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_OMAP_SCR, up->scr);

	/* Reset UART_MCR_TCRTLR: this must be done with the EFR_ECB bit set */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);

	/* Protocol, Baud Rate, and Interrupt Settings */

	//if (up->errata & UART_ERRATA_i202_MDR1_ACCESS)
	//	serial_omap_mdr1_errataset(up, up->mdr1);
	//else
	//	serial_out(up, UART_OMAP_MDR1, up->mdr1);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, 0);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_DLL, up->dll);	/* LS of divisor */
	serial_out(up, UART_DLM, up->dlh);	/* MS of divisor */

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, cval);

	if (!serial_omap_baud_is_mode16(up))
		up->mdr1 = UART_OMAP_MDR1_13X_MODE;
	else
		up->mdr1 = UART_OMAP_MDR1_16X_MODE;

	//if (up->errata & UART_ERRATA_i202_MDR1_ACCESS)
	//	serial_omap_mdr1_errataset(up, up->mdr1);
	//else
	//	serial_out(up, UART_OMAP_MDR1, up->mdr1);

	/* Configure flow control */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	/* XON1/XOFF1 accessible mode B, TCRTLR=0, ECB=0 */
	//serial_out(up, UART_XON1, termios->c_cc[VSTART]);
	//serial_out(up, UART_XOFF1, termios->c_cc[VSTOP]);

	/* Enable access to TCR/TLR */
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_MCR, up->mcr | UART_MCR_TCRTLR);

		serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, up->lcr);
}


static void
serial_omap_set_mux_conf(struct uart_omap_port *up)
{
	unsigned char __iomem *base = AM33XX_L4_WK_IO_ADDRESS(OMAP2_CTRL_BASE + up->pin_mux[0].pin_offset);
	__raw_writel(up->pin_mux[0].pin_value, base);

	base = AM33XX_L4_WK_IO_ADDRESS(OMAP2_CTRL_BASE + up->pin_mux[1].pin_offset);
	__raw_writel(up->pin_mux[1].pin_value, base);
}

static void serial_omap_soft_reset(struct uart_omap_port *up)
{
	unsigned int temp;
	temp = serial_in(up, UART_OMAP_SYSC);
	temp |= 1 << 1;
	serial_out(up, UART_OMAP_SYSC, temp);
	while ((serial_in(up, UART_OMAP_SYSS) & 0x1) != 0x1);
	temp = serial_in(up, UART_OMAP_SYSC);
	temp |= 1 << 3;
	serial_out(up, UART_OMAP_SYSC, temp);
}



static int calc_divisor (struct uart_omap_port *up)
{
	#define MODE_X_DIV 16
	return (up->uartclk + (up->baud * (MODE_X_DIV / 2))) /
		(MODE_X_DIV * up->baud);
}

#define UART_LCR_BKSE 0x80
#define UART_LCRVAL 0x3
#define UART_MCRVAL (0x1 | 0x2)
#define UART_FCRVAL (0x1 | 0x2 | 0x4)

static void serial_omap_early_init(struct uart_omap_port *up, int div)
{
	while(!(serial_in(up, UART_LSR) & UART_LSR_TEMT));
	serial_out(up, UART_IER, 0);
	serial_out(up, UART_OMAP_MDR1, 0x7);

	serial_out(up, UART_LCR, UART_LCR_BKSE | UART_LCRVAL);
	serial_out(up, UART_DLL, 0);
	serial_out(up, UART_DLM, 0);
	serial_out(up, UART_LCR, UART_LCRVAL);
	serial_out(up, UART_MCR, UART_MCRVAL);
	serial_out(up, UART_FCR, UART_FCRVAL);
	serial_out(up, UART_LCR, UART_LCR_BKSE | UART_LCRVAL);
	serial_out(up, UART_DLL, div & 0xff);
	serial_out(up, UART_DLM, (div >> 8) & 0xff);
	serial_out(up, UART_LCR, UART_LCRVAL);

	serial_out(up, UART_OMAP_MDR1, 0);
}

void omap_serial_init(void)
{
	struct uart_omap_port *up;
	int i;

	for (i = 0; i < 5; i++) {
		up = &g_uart_omap_port[i];

		serial_omap_set_mux_conf(up);
		setup_irq(up->irq, serial_omap_irq, up);

		if (i != 0) {
			unsigned int temp;
			void *addr;
			int j = 1000;

			addr = AM33XX_L4_WK_IO_ADDRESS(0x44e00000 + up->clkctrl_offset);
			temp = readl(addr);

			temp &= ~0x3;
			temp |= 0x2;
			writel(temp, addr);
			
			do {
				temp = readl(addr);
				temp = (temp >> 16) & 0x3;
				j--;
				if (!j)
					break;
			} while((temp == 1) || (temp == 3));
		
			serial_omap_soft_reset(up);
			temp = calc_divisor(up);
			serial_omap_early_init(up, temp);
		}

		serial_omap_clear_fifos(up);
		/*
	 	* Clear the interrupt registers.
	 	*/
		(void) serial_in(up, UART_LSR);
		if (serial_in(up, UART_LSR) & UART_LSR_DR)
			(void) serial_in(up, UART_RX);
		(void) serial_in(up, UART_IIR);
		(void) serial_in(up, UART_MSR);

		up->ier = UART_IER_RLSI | UART_IER_RDI;
		serial_out(up, UART_IER, up->ier);

		serial_omap_set_termios(up);
	}

	printk("omap_serial_init\n");
}

