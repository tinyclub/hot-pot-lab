#ifndef _HOT_POT_SCHED_H
#define _HOT_POT_SCHED_H

#include <hot-pot/thread_info.h>

union thread_stack
{
	struct thread_info thread_info;
	unsigned long stack[STACK_SIZE/sizeof(long)];
};

extern union thread_stack init_thread_stack;

#endif
