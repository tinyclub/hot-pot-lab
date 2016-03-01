#ifndef _PHOENIX_TIME_H
#define _PHOENIX_TIME_H

#include <linux/time.h>
#include <dim-sum/magic.h>
#include <asm/div64.h>

#ifdef  __KERNEL__  

struct phy_timer
{
	char	*name;
	unsigned long freq;
	unsigned int mult, shift; 
	u64 maxns;
	unsigned int maxtick;
	unsigned long feature;
};


struct phy_clock
{
	char	*name;
	unsigned long freq;
	unsigned int shift; 
	u64 mult,maxns;
	unsigned int maxtick;
	u64 (* read)(struct phy_clock *clock);
	unsigned long (* read_low)(struct hal_clock *clock);
	unsigned long (* read_high)(struct hal_clock *clock);
};

extern struct phy_clock *active_clock;
extern struct phy_timer *active_timer;


extern u64 sysuptime;

int time_init(void);

static inline unsigned int khz2mult(unsigned int khz, unsigned int shift_constant)
{
	u64 tmp = ((u64)1000000) << shift_constant;

	tmp += khz /2; /* round for do_div */
	do_div(tmp, khz);

	return (unsigned int)tmp;
}

static inline unsigned int hz2mult(unsigned int hz, unsigned int shift_constant)
{
	u64 tmp = ((u64)1000000000) << shift_constant;

	tmp += (u64)(hz>>1); /* round for do_div */
	do_div(tmp, hz);

	return (unsigned int)tmp;
}
static inline unsigned long long  hz2mult_arm(unsigned int hz, unsigned int shift_constant)
{
	u64 tmp = ((u64)1000000000) << shift_constant;
	tmp += (u64)(hz>>1); /* round for do_div */
	do_div(tmp, hz);
	return tmp;
}

static inline unsigned long div_sc(unsigned long ticks, unsigned long nsec,
				   int shift)
{
	u64 tmp = ((u64)ticks) << shift;

	do_div(tmp, nsec);
	return (unsigned long) tmp;
}

static inline u64 cyc2ns(struct hal_clock *clock, u64 cycles)
{
	unsigned int rem;
	u64 ret = (u64)cycles;

	if (ret > (u64)clock->maxtick) {
		rem = do_div(ret, clock->maxtick);
		ret = ret * clock->maxns + (((u64)rem * clock->mult) >> clock->shift);
	} else
		ret = (ret * (u64)clock->mult) >> clock->shift;

	return ret;
}

static inline u64 ns2cycle(struct hal_timer *timer, u64 ns)
{
	unsigned int rem;
	u64 ret = (u64)ns;

	if (ret > timer->maxtick) {
		rem = do_div(ret, timer->maxtick);
		ret = ret * timer->maxns + (((u64)rem * timer->mult) >> timer->shift);
	} else
		ret = (ret * (u64)timer->mult) >> timer->shift;

	return ret;
}

static struct phy_timer* hwtimer(void)
{
	return active_timer;
}

static struct phy_clock* hwclock(void)
{
	return active_clock;
}

static inline u64 clock_freq(void)
{
	if (active_clock)
		return active_clock->freq;
	return ERROR_NODEV;
}

static inline u64 clock_read(void)
{
	if (active_clock)
		return active_clock->read(active_clock);
	return ERROR_NODEV;
}

static inline unsigned long clock_read_low(void)
{
	if (active_clock)
		return active_clock->read_low(active_clock);
	return ERROR_NODEV;
}

static inline unsigned long clock_read_high(void)
{
	if (active_clock)
		return active_clock->read_high(active_clock);
	return ERROR_NODEV;
}

static inline u64 uptime(void)
{
	return 0;//cyc2ns(active_clock, dim_sum_clock_read() - sysuptime);
}
#endif

#endif
