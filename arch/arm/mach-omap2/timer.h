
#ifndef __ASM_ARCH_DMTIMER_H
#define __ASM_ARCH_DMTIMER_H

#include <mach/io.h>

struct omap_dm_timer {
	int id;
	int irq;
	void 	*io_base;
	void 	*sys_stat;	/* TISTAT timer status */
	void 	*irq_stat;	/* TISR/IRQSTATUS interrupt status */
	void 	*irq_ena;	/* irq enable */
	void 	*irq_dis;	/* irq disable, only on v2 ip */
	void 	*pend;		/* write pending */
	void 	*func_base;	/* function register base */
	unsigned long rate;
	int revision;
	unsigned posted;
};

#define OMAP_TIMER_DMTIMER2_PHYS 0x48040000
#define OMAP_TIMER_DMTIMER3_PHYS 0x48042000

#define OMAP_TIMER_V1_SYS_STAT_OFFSET	0x14
#define OMAP_TIMER_V1_STAT_OFFSET	0x18
#define OMAP_TIMER_V1_INT_EN_OFFSET	0x1c

#define OMAP_TIMER_V2_IRQSTATUS_RAW	0x24
#define OMAP_TIMER_V2_IRQSTATUS		0x28
#define OMAP_TIMER_V2_IRQENABLE_SET	0x2c
#define OMAP_TIMER_V2_IRQENABLE_CLR	0x30

#define OMAP_TIMER_V2_FUNC_OFFSET		0x14

#define _OMAP_TIMER_WRITE_PEND_OFFSET	0x34
#define _OMAP_TIMER_IF_CTRL_OFFSET	0x40
#define _OMAP_TIMER_CTRL_OFFSET		0x24
#define _OMAP_TIMER_COUNTER_OFFSET	0x28
#define _OMAP_TIMER_LOAD_OFFSET		0x2c

#define _OMAP_TIMER_WAKEUP_EN_OFFSET	0x20

#define	WPSHIFT					16
#define		WP_NONE			0
#define		WP_TCLR			(1 << 0)
#define		WP_TCRR			(1 << 1)
#define		WP_TLDR			(1 << 2)


#define OMAP_TIMER_WAKEUP_EN_REG		(_OMAP_TIMER_WAKEUP_EN_OFFSET \
							| (WP_NONE << WPSHIFT))

#define OMAP_TIMER_IF_CTRL_REG			(_OMAP_TIMER_IF_CTRL_OFFSET \
							| (WP_NONE << WPSHIFT))

#define OMAP_TIMER_CTRL_REG			(_OMAP_TIMER_CTRL_OFFSET \
							| (WP_TCLR << WPSHIFT))
#define OMAP_TIMER_COUNTER_REG			(_OMAP_TIMER_COUNTER_OFFSET \
								| (WP_TCRR << WPSHIFT))
#define OMAP_TIMER_LOAD_REG			(_OMAP_TIMER_LOAD_OFFSET \
								| (WP_TLDR << WPSHIFT))


#define		OMAP_TIMER_CTRL_POSTED		(1 << 2)
#define		OMAP_TIMER_CTRL_ST		(1 << 0)
#define		OMAP_TIMER_CTRL_AR		(1 << 1)

#define OMAP_TIMER_NONPOSTED			0x00
#define OMAP_TIMER_POSTED				0x01

#define OMAP_TIMER_INT_OVERFLOW			(1 << 1)



static inline u32 __omap_dm_timer_read(struct omap_dm_timer *timer, u32 reg,
						int posted)
{
	if (posted)
		while (__raw_readl(timer->pend) & (reg >> WPSHIFT));
			//cpu_relax();

	return __raw_readl(timer->func_base + (reg & 0xff));
}


static inline void __omap_dm_timer_write(struct omap_dm_timer *timer,
					u32 reg, u32 val, int posted)
{
	if (posted)
		while (__raw_readl(timer->pend) & (reg >> WPSHIFT));
			//cpu_relax();

	__raw_writel(val, timer->func_base + (reg & 0xff));
}


static inline void __omap_dm_timer_int_enable(struct omap_dm_timer *timer,
						unsigned int value)
{
	__raw_writel(value, timer->irq_ena);
	__omap_dm_timer_write(timer, OMAP_TIMER_WAKEUP_EN_REG, value, 0);
}


static inline void __omap_dm_timer_stop(struct omap_dm_timer *timer,
					int posted, unsigned long rate)
{
	u32 l;

	l = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, posted);
	if (l & OMAP_TIMER_CTRL_ST) {
		l &= ~0x1;
		__omap_dm_timer_write(timer, OMAP_TIMER_CTRL_REG, l, posted);
	}

	/* Ack possibly pending interrupt */
	__raw_writel(OMAP_TIMER_INT_OVERFLOW, timer->irq_stat);
}

static inline void __omap_dm_timer_load_start(struct omap_dm_timer *timer,
						u32 ctrl, unsigned int load,
						int posted)
{
	__omap_dm_timer_write(timer, OMAP_TIMER_COUNTER_REG, load, posted);
	__omap_dm_timer_write(timer, OMAP_TIMER_CTRL_REG, ctrl, posted);
}

static inline unsigned int
__omap_dm_timer_read_counter(struct omap_dm_timer *timer, int posted)
{
	return __omap_dm_timer_read(timer, OMAP_TIMER_COUNTER_REG, posted);
}

static inline void __omap_dm_timer_write_status(struct omap_dm_timer *timer,
						unsigned int value)
{
	__raw_writel(value, timer->irq_stat);
}


extern void am33xx_timer_init(void);

#endif
