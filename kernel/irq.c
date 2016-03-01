#include <dim-sum/irq.h>
#include <dim-sum/hard_irq.h>
#include <dim-sum/preempt.h>
#include <dim-sum/sched.h>
#include <asm/irq.h>
#include <asm/current.h>

void irq_enter(int irq)
{
	add_preempt_count(HARDIRQ_OFFSET);	
}

void irq_exit(int irq, struct pt_regs *regs)
{
	local_irq_disable();
	sub_preempt_count(IRQ_EXIT_OFFSET);
	preempt_enable_no_resched();
	//这里处理软中断
}

void svc_tail(struct pt_regs *regs)
{
	if (need_resched() && (is_irqctx_irqs_enabled(regs)))
	{
		unsigned long flags;
		local_irq_save(flags);
		PreemptSwitchInIrq();
		local_irq_restore(flags);
	}

	if (test_thread_flag(TIF_SIGPENDING)
		 && (!current->in_syscall)) {
		do_signal(regs);
	}
	if ((hardirq_count() == 0)
		 && (!current->in_syscall)) {
		current->uregs = NULL;
	}
}

