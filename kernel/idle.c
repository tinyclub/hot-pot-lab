#include <dim-sum/base.h>
#include <asm/asm-offsets.h>
#include <linux/kernel.h>
//#include <dim-sum/sched.h>

#include <dim-sum/adapter.h>
#include <dim-sum/idle.h>

void (*powersave)(void) = NULL;
void default_powersave(void)
{
	cpu_relax();
}

void default_idle(void)
{
	local_irq_disable();
	if (!need_resched())
	{
		if (powersave != NULL)
		{
			powersave();
		}
	}
	local_irq_enable();
}

void cpu_idle (void)
{
	while (1) {
		void (*idle)(void);

		//这里的读屏障是需要的，因为其他核可能触发本CPU的调度
		rmb();
		idle = NULL;

		if (!idle)
			idle = default_idle;

		preempt_disable();
		while (!need_resched())
		{
			idle();
		}
		preempt_enable();
		TaskSwitch();
	}
}
