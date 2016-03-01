#include <asm/types.h>
#include <dim-sum/magic.h>
#include <dim-sum/threads.h>
#include <dim-sum/timer.h>
#include <dim-sum/smp.h>
#include <dim-sum/irqflags.h>
#include <dim-sum/sched.h>
#include <asm/current.h>

static struct timer_base timers[NR_CPUS];

u64 SysUpTime(void)
{
	return get_jiffies_64() * 100 * 1000 * 1000;
}
#if 0
u64 get_timer(void)
{
	return get_jiffies_64() * 100 * 1000 * 1000;
}
#endif

u64 get_timer(ulong base)
{
        return (get_jiffies_64() - base);
}

void reset_timer(void)
{

}

#define __jiffy_data  __attribute__((section(".data")))
u64 __jiffy_data jiffies_64 = 0;

u64 get_jiffies_64(void)
{
	return jiffies_64;
}

void add_jiffies_64(unsigned long ticks)
{
	jiffies_64 += ticks;
}

static int timer_insert(struct timer_base *base, struct timer *timer)
{
	struct rb_node **link = &base->rbroot.rb_node;
	struct rb_node *parent = NULL, *rbprev = NULL;
	struct timer *entry;

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct timer, rbnode);

		if (timer->expire < entry->expire)
			link = &(*link)->rb_left;
		else {
			rbprev = parent;
			link = &(*link)->rb_right;
		}
	}
	rb_link_node(&timer->rbnode, parent, link);
	rb_insert_color(&timer->rbnode, &base->rbroot);

	if (rbprev) {
		entry = rb_entry(rbprev, struct timer, rbnode);
		list_add(&timer->node, &entry->node);
	} else {
		list_add(&timer->node, &base->timers);
	}
	timer->flag |= TIMER_WAITING;

	return 0;
}

static void timer_dequeue(struct timer *timer)
{
	rb_erase(&timer->rbnode, &timer->base->rbroot);
	list_del(&timer->node);
	timer->flag &= ~TIMER_WAITING;
}


int timer_set(struct timer *timer, u64 expire)
{
	int ret = 0;
	unsigned long flag;
	int cpu = smp_processor_id();
	struct timer_base *base = &timers[cpu];

	 local_irq_save(flag);
	if (timer->flag & TIMER_WAITING)
		timer_dequeue(timer);
	else {
		timer->base = base;
	}

	timer->delta = expire;
	timer->expire = get_jiffies_64() + timer->delta;
	timer_insert(base, timer);
	timer->flag &= ~TIMER_IDLE;

	local_irq_restore(flag);
	return ret;
}

int timer_remove(struct timer *timer)
{
	unsigned long flag;

	local_irq_save(flag);
	if (unlikely(timer->flag & TIMER_IDLE))
		goto out;
	if (timer->flag & TIMER_WAITING)
		timer_dequeue(timer);
	timer->flag |= TIMER_IDLE;

out:
	local_irq_restore(flag);
	return 0;
}

int timer_init(struct timer *timer)
{
	memset(timer, 0, sizeof(*timer));
	timer->flag = TIMER_IDLE;
	return 0;
}


/**
 * 在时钟中断中调用，运行当前CPU上的定时器
 */
void RunLocalTimer(void)
{
	int cpu = smp_processor_id();
	struct timer_base *base = &timers[cpu];
	u64 now = get_jiffies_64();
	struct timer *timer;
	int (* function)(struct timer *timer);

again:
	while (likely(!list_empty(&base->timers))) {
		timer = list_entry(base->timers.next, struct timer, node);

		if (timer->expire > now)
			break;
	
		timer_dequeue(timer);
		
		if (timer->flag & TIMER_SYSTEM)	timer->magic= TIMER_VERIFY_MAGIC;
		timer->flag |= TIMER_RUNING;
		if (timer->flag & TIMER_OPEN_IRQ) {
			function = timer->function;
			local_irq_enable();
			function(timer);
			local_irq_disable();
		}
		else
		{
			timer->function(timer);
		}

		if (!(timer->flag & TIMER_WAITING)) {
			if (!(timer->flag & TIMER_IDLE)) {
				if (timer->flag & TIMER_PERIODIC) {
					timer->expire += timer->delta;
					timer_insert(base, timer);
				} else
					timer->flag |= TIMER_IDLE;
			}
		}
		timer->flag &= ~TIMER_RUNING;
		if ((timer->flag & (TIMER_IDLE | TIMER_KILLING)) == (TIMER_IDLE | TIMER_KILLING))
			if (timer->free) timer->free(timer);
	}

	//在运行过程中，可能有新的定时器到期
	if (likely(!list_empty(&base->timers))) {
		timer = list_entry(base->timers.next, struct timer, node);

		now = get_jiffies_64();
		if (timer->expire <= now)
			goto again;
	}
}

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC (1000000000L)
#endif

#ifndef NSEC_PER_USEC
#define NSEC_PER_USEC (1000L)
#endif

#if HZ >= 12 && HZ < 24
# define SHIFT_HZ	4
#elif HZ >= 24 && HZ < 48
# define SHIFT_HZ	5
#elif HZ >= 48 && HZ < 96
# define SHIFT_HZ	6
#elif HZ >= 96 && HZ < 192
# define SHIFT_HZ	7
#elif HZ >= 192 && HZ < 384
# define SHIFT_HZ	8
#elif HZ >= 384 && HZ < 768
# define SHIFT_HZ	9
#elif HZ >= 768 && HZ < 1536
# define SHIFT_HZ	10
#else
# error You lose.
#endif

#define SH_DIV(NOM,DEN,LSH) (   ((NOM / DEN) << LSH)                    \
                             + (((NOM % DEN) << LSH) + DEN / 2) / DEN)

#define ACTHZ HZ
/* TICK_NSEC is the time between ticks in nsec assuming real ACTHZ */
#define TICK_NSEC (SH_DIV (1000000UL * 1000, ACTHZ, 8))

#define SEC_JIFFIE_SC (31 - SHIFT_HZ)
#if !((((NSEC_PER_SEC << 2) / TICK_NSEC) << (SEC_JIFFIE_SC - 2)) & 0x80000000)
#undef SEC_JIFFIE_SC
#define SEC_JIFFIE_SC (32 - SHIFT_HZ)
#endif

#define MAX_JIFFY_OFFSET ((~0UL >> 1)-1)
#if BITS_PER_LONG < 64
# define MAX_SEC_IN_JIFFIES \
	(long)((u64)((u64)MAX_JIFFY_OFFSET * TICK_NSEC) / NSEC_PER_SEC)
#else	/* take care of overflow on 64 bits machines */
# define MAX_SEC_IN_JIFFIES \
	(SH_DIV((MAX_JIFFY_OFFSET >> SEC_JIFFIE_SC) * TICK_NSEC, NSEC_PER_SEC, 1) - 1)

#endif



#define NSEC_CONVERSION ((unsigned long)((((u64)1 << NSEC_JIFFIE_SC) +\
                                        TICK_NSEC -1) / (u64)TICK_NSEC))
#define NSEC_JIFFIE_SC (SEC_JIFFIE_SC + 29)
#define SEC_CONVERSION ((unsigned long)((((u64)NSEC_PER_SEC << SEC_JIFFIE_SC) +\
                                TICK_NSEC -1) / (u64)TICK_NSEC))


static __inline__ unsigned long
timespec_to_jiffies(const struct timespec *value)
{
	unsigned long sec = value->tv_sec;
	long nsec = value->tv_nsec + TICK_NSEC - 1;

	if (sec >= MAX_SEC_IN_JIFFIES){
		sec = MAX_SEC_IN_JIFFIES;
		nsec = 0;
	}
	return (((u64)sec * SEC_CONVERSION) +
		(((u64)nsec * NSEC_CONVERSION) >>
		 (NSEC_JIFFIE_SC - SEC_JIFFIE_SC))) >> SEC_JIFFIE_SC;

}

#ifndef div_long_long_rem
#include <asm/div64.h>

#define div_long_long_rem(dividend,divisor,remainder) ({ \
		       u64 result = dividend;		\
		       *remainder = do_div(result,divisor); \
		       result; })

#endif

static __inline__ void
jiffies_to_timespec(const unsigned long jiffies, struct timespec *value)
{
	/*
	 * Convert jiffies to nanoseconds and separate with
	 * one divide.
	 */
	u64 nsec = (u64)jiffies * TICK_NSEC;
	value->tv_sec = div_long_long_rem(nsec, NSEC_PER_SEC, &value->tv_nsec);
}

/**
 * nanosleep系统调用的服务例程。
 * 将进程挂起直到指定的时间间隔用完。
 */
asmlinkage long sys_nanosleep(struct timespec __user *rqtp, struct timespec __user *rmtp)
{
	struct timespec t;
	unsigned long expire;
	long ret;

	/**
	 * 首先调用copy_frome_user将饮食在timerspec结构中的值复制到局部变量t中。
	 */
	if (copy_from_user(&t, rqtp, sizeof(t)))
		return -EFAULT;

	/**
	 * 假定是一个有效的延迟。
	 */
	if ((t.tv_nsec >= 1000000000L) || (t.tv_nsec < 0) || (t.tv_sec < 0))
		return -EINVAL;

	/**
	 * timespec_to_jiffies将t中的时间间隔转换成节拍数。
	 * 再加上t.tv_sec || t.tv_nsec，是为了保险起见。
	 * 即，计算出来的节拍数始终会被加一。
	 */
	expire = timespec_to_jiffies(&t) + (t.tv_sec || t.tv_nsec);
	/**
	 * schedule_timeout会调用动态定时器。实现进程的延时。
	 */
	current->state = TASK_INTERRUPTIBLE;
	/**
	 * 可能会返回一个剩余节拍数。
	 */
	expire = schedule_timeout(expire);

	ret = 0;
#ifdef xby_debug
	/**
	 * schedule_timeout返回一个剩余节拍数。可能是系统调用被信号打断了。
	 * 在此自动重启系统调用。
	 */
	if (expire) {
		struct restart_block *restart;
		jiffies_to_timespec(expire, &t);
		if (rmtp && copy_to_user(rmtp, &t, sizeof(t)))
			return -EFAULT;

		restart = &current_thread_info()->restart_block;
		restart->fn = nanosleep_restart;
		restart->arg0 = jiffies + expire;
		restart->arg1 = (unsigned long) rmtp;
		/**
		 * ERESTART_RESTARTBLOCK表示系统调用需要重启。但是重启的方式不是简单的重新执行系统调用。
		 * 而是执行一个指定的函数restart_block。让系统调用再睡眠一段时间。
		 */
		ret = -ERESTART_RESTARTBLOCK;
	}
#endif
	return ret;
}

void setup_timer(void)
{
	int i;
	for (i = 0; i < NR_CPUS; i++) {
		INIT_LIST_HEAD(&timers[i].timers);
		INIT_LIST_HEAD(&timers[i].prepare_timers);
	}
}