#include <linux/kernel.h>
#include <linux/linkage.h>
#include <dim-sum/irq.h>
#include <dim-sum/init.h>
#include <asm/mach/arch.h>
#include <asm/exception.h>
#include <asm/mach-types.h>
#include <asm/string.h>
#include <asm/param.h>
#include <mach/io.h>
#include "timer.h"
#include <dim-sum/timer.h>
#include <dim-sum/smp.h>

static struct omap_dm_timer clkev;
static struct omap_dm_timer clkev1;

static inline void __omap_dm_timer_init_regs(struct omap_dm_timer *timer)
{
	u32 tidr;
	printk("__omap_dm_timer_init_regs 11\n");
	printk("iobase %x\n", timer->io_base);
	/* Assume v1 ip if bits [31:16] are zero */
	tidr = __raw_readl(timer->io_base);
	printk("tidr 0x%x\n", tidr);
	if (!(tidr >> 16)) {
		timer->revision = 1;
		timer->sys_stat = timer->io_base +
				OMAP_TIMER_V1_SYS_STAT_OFFSET;
		timer->irq_stat = timer->io_base + OMAP_TIMER_V1_STAT_OFFSET;
		timer->irq_ena = timer->io_base + OMAP_TIMER_V1_INT_EN_OFFSET;
		timer->irq_dis = 0;
		timer->pend = timer->io_base + _OMAP_TIMER_WRITE_PEND_OFFSET;
		timer->func_base = timer->io_base;
	} else {
		timer->revision = 2;
		timer->sys_stat = 0;
		timer->irq_stat = timer->io_base + OMAP_TIMER_V2_IRQSTATUS;
		timer->irq_ena = timer->io_base + OMAP_TIMER_V2_IRQENABLE_SET;
		timer->irq_dis = timer->io_base + OMAP_TIMER_V2_IRQENABLE_CLR;
		timer->pend = timer->io_base +
			_OMAP_TIMER_WRITE_PEND_OFFSET +
				OMAP_TIMER_V2_FUNC_OFFSET;
		timer->func_base = timer->io_base + OMAP_TIMER_V2_FUNC_OFFSET;
	}
	printk("__omap_dm_timer_init_regs 22\n");
}


static inline void __omap_dm_timer_enable_posted(struct omap_dm_timer *timer)
{
	printk("__omap_dm_timer_enable_posted 11\n");
	if (timer->posted)
		return;

	__omap_dm_timer_write(timer, OMAP_TIMER_IF_CTRL_REG,
			OMAP_TIMER_CTRL_POSTED, 0);

	timer->posted = OMAP_TIMER_POSTED;
	printk("__omap_dm_timer_enable_posted 22\n");
}


static void omap2_gp_timer_set_mode(struct omap_dm_timer *timer)
{
	u32 period;
	printk("omap2_gp_timer_set_mode 11\n");
	__omap_dm_timer_stop(timer, 1, timer->rate);
	
	printk("omap2_gp_timer_set_mode 22\n");
	period = timer->rate / HZ;
	period -= 1;

	/* Looks like we need to first set the load value separately */
	__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG,
					0xffffffff - period, 1);
	__omap_dm_timer_load_start(timer,
					OMAP_TIMER_CTRL_AR | OMAP_TIMER_CTRL_ST,
						0xffffffff - period, 1);
	printk("clkev.rate %lu\n", timer->rate);
}

static irqreturn_t omap2_gp_timer_interrupt(unsigned int irq, void *data)
{
	__omap_dm_timer_write_status(&clkev, OMAP_TIMER_INT_OVERFLOW);

	if (smp_processor_id() == 0)
	{
		add_jiffies_64(1);
	}
	
	RunLocalTimer();

	return IRQ_HANDLED;
}


void am33xx_timer_init(void)
{
	unsigned int t1, t2;
	int i;
	memset(&clkev, 0 , sizeof(clkev));
	clkev.io_base = (void *)0xfa320000;//OMAP2_L4_PER_IO_ADDRESS(OMAP_TIMER_DMTIMER2_PHYS);
	clkev.rate = 24000000;
	clkev.irq = 68;

	early_print("xby_debug in am33xx_timer_init step 1\n");
	memset(&clkev1, 0 , sizeof(clkev1));
	clkev1.io_base = (void *)0xfa318000;//OMAP2_L4_PER_IO_ADDRESS(OMAP_TIMER_DMTIMER3_PHYS);
	clkev1.rate = 24000000;
	
	__omap_dm_timer_init_regs(&clkev);
	__omap_dm_timer_enable_posted(&clkev);
	early_print("xby_debug in am33xx_timer_init step 2\n");
	omap2_gp_timer_set_mode(&clkev);
	early_print("xby_debug in am33xx_timer_init step 2.1\n");

#if 0
	{
		unsigned int temp;
		unsigned int addr;
		int j = 1000;
		addr = (void *)AM33XX_L4_WK_IO_ADDRESS(0x44E00000 + 0x84);
		temp = readl(addr);
		temp &= ~0x3;
		temp |= 0x2;
		writel(temp, addr);
		early_print("xby_debug in am33xx_timer_init step 2.2\n");
		do {
			temp = readl(addr);
			temp = (temp >> 16) & 0x3;
			j--;
			if (!j)
				break;
		} while((temp == 1) || (temp == 3));
		
		printk("enable timer3 clock j %d\n", j);
		
		addr = (void *)AM33XX_L4_WK_IO_ADDRESS(0x44E00500 + 0xc);
		writel(0x1, addr);
	}	
	early_print("xby_debug in am33xx_timer_init step 3\n");
#endif

	__omap_dm_timer_init_regs(&clkev1);
	__omap_dm_timer_load_start(&clkev1,
				   OMAP_TIMER_CTRL_ST | OMAP_TIMER_CTRL_AR, 0,
				   OMAP_TIMER_NONPOSTED);

#if 1
	t1 = __omap_dm_timer_read_counter(&clkev1, 0);
	i = 10000;
	while(i--);
	t2 = __omap_dm_timer_read_counter(&clkev1, 0);
	printk("t1 0x%x t2 0x%x\n", t1, t2);
#endif			   
				   
	early_print("xby_debug in am33xx_timer_init step 4\n");
	setup_irq(clkev.irq, omap2_gp_timer_interrupt, NULL);
	early_print("xby_debug in am33xx_timer_init step 5\n");
	__omap_dm_timer_int_enable(&clkev, OMAP_TIMER_INT_OVERFLOW);
	early_print("xby_debug in am33xx_timer_init step 6\n");
	
}

unsigned int dim_sum_read_tick(void)
{
	return __omap_dm_timer_read_counter(&clkev1, 0);
}


