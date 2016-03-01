
#include <linux/linkage.h>
#include <linux/kernel.h>
#include <dim-sum/irq.h>
#include <dim-sum/hard_irq.h>
#include <dim-sum/init.h>
#include <asm/mach/arch.h>
#include <asm/exception.h>
#include <asm/mach-types.h>
#include <dim-sum/mem.h>
#include <asm/current.h>
#include <dim-sum/sched.h>

struct irq_desc irq_desc_array[256];


static inline void mask_ack_irq(unsigned int irq)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	if (desc->irq_mask_ack)
		desc->irq_mask_ack(irq);
	else {
		desc->irq_mask(irq);
		if (desc->irq_ack)
			desc->irq_ack(irq);
	}
}

void mask_irq(unsigned int irq)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	if (desc->irq_mask) {
		desc->irq_mask(irq);
	}
}

void unmask_irq(unsigned int irq)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	if (desc->irq_unmask) {
		desc->irq_unmask(irq);
	}
}

void set_irq_desc_handler(unsigned int irq, irq_handler_t handler)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	desc->handler = handler;
	
}

void set_irq_desc_ops(unsigned int irq, struct irq_chip *chip)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	desc->irq_ack = chip->irq_ack;
	desc->irq_mask_ack = chip->irq_mask_ack;
	desc->irq_mask = chip->irq_mask;
	desc->irq_unmask = chip->irq_unmask;
}

void setup_irq(unsigned int irq, irq_handler_t handler, void *data)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	set_irq_desc_handler(irq, handler);
	desc->data = data;
	unmask_irq(irq);
}

irqreturn_t
handle_irq_event(unsigned int irq)
{
	irqreturn_t retval = IRQ_NONE;
	struct irq_desc *desc = &irq_desc_array[irq];
	
	retval = desc->handler(irq, desc->data);
	
	return retval;
}


void
handle_level_irq(unsigned int irq)
{

	mask_ack_irq(irq);

	handle_irq_event(irq);

	unmask_irq(irq);

}


void handle_IRQ(unsigned int irq, struct pt_regs *regs)
{
	//struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter(irq);

	if (!current->uregs) {
		current->uregs = regs;
	}
	handle_level_irq(irq);

	irq_exit(irq, regs);
	//set_irq_regs(old_regs);
}

asmlinkage void bad_mode(struct pt_regs *regs, int reason)
{
	printk(KERN_CRIT "Bad mode in %d handler detected\n", reason);
}

void __init init_IRQ(void)
{
	machine_desc->init_irq();
	//IRQ_STACK_START = irq_stack;
}

void do_irq (struct pt_regs *regs)
{
	printk("int\n");
	handle_IRQ(68, regs);
}

void chained_irq_enter(unsigned int irq)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	
	/* FastEOI controllers require no action on entry. */
	if (desc->irq_eoi)
		return;

	if (desc->irq_mask_ack) {
		desc->irq_mask_ack(irq);
	} else {
		desc->irq_mask(irq);
		if (desc->irq_ack)
			desc->irq_ack(irq);
	}
}

void chained_irq_exit(unsigned int irq)
{
	struct irq_desc *desc = &irq_desc_array[irq];
	if (desc->irq_eoi)
		desc->irq_eoi(irq);
	else
		desc->irq_unmask(irq);
}

