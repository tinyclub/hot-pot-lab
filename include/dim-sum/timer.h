#ifndef _PHOENIX_TIMER_H
#define _PHOENIX_TIMER_H

#include <base/list.h>
#include <base/rbtree.h>
#include <kapi/dim-sum/time.h>

//一次性定时器，已经被运行了
#define TIMER_IDLE			0x1
#define TIMER_SYSTEM		0x2
#define TIMER_PERIODIC		0x4
#define TIMER_RUNING		0x10
//正在队列上排除等待运行
#define TIMER_WAITING		0x20
#define TIMER_OPEN_IRQ		0x40
#define TIMER_KILLING		0x80


struct timer_base
{
	struct list_head timers;
	struct rb_root rbroot;
	struct list_head prepare_timers;
};

struct timer
{
	struct rb_node rbnode;
	struct timer_base *base;
	struct list_head node, pnode;
	u64	expire;
	u64	delta;
	int (* function)(struct timer *timer);
	void (* free)(struct timer *timer);
	void *data;
	unsigned long magic;
	unsigned int flag;
	unsigned int affinity;
	int  overrun;
	int  verify;
};


int timer_init(struct timer *timer);
int timer_set(struct timer *timer, u64 expire);
int timer_remove(struct timer *timer);


extern void add_jiffies_64(unsigned long ticks);
extern void RunLocalTimer(void);
extern void setup_timer(void);
extern u64 SysUpTime(void);
extern u64 get_timer(ulong base);
extern void reset_timer(void);


#define HZ CONFIG_HZ
#endif
