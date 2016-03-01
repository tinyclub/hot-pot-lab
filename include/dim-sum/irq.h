#ifndef _LINUX_IRQ_H
#define _LINUX_IRQ_H

#include <asm/ptrace.h>

enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};

enum {
	IRQ_TYPE_NONE		= 0x00000000,
	IRQ_TYPE_EDGE_RISING	= 0x00000001,
	IRQ_TYPE_EDGE_FALLING	= 0x00000002,
	IRQ_TYPE_EDGE_BOTH	= (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
	IRQ_TYPE_LEVEL_HIGH	= 0x00000004,
	IRQ_TYPE_LEVEL_LOW	= 0x00000008,
	IRQ_TYPE_LEVEL_MASK	= (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH),
	IRQ_TYPE_SENSE_MASK	= 0x0000000f,

	IRQ_TYPE_PROBE		= 0x00000010,

	IRQ_LEVEL		= (1 <<  8),
	IRQ_PER_CPU		= (1 <<  9),
	IRQ_NOPROBE		= (1 << 10),
	IRQ_NOREQUEST		= (1 << 11),
	IRQ_NOAUTOEN		= (1 << 12),
	IRQ_NO_BALANCING	= (1 << 13),
	IRQ_MOVE_PCNTXT		= (1 << 14),
	IRQ_NESTED_THREAD	= (1 << 15),
	IRQ_NOTHREAD		= (1 << 16),
	IRQ_PER_CPU_DEVID	= (1 << 17),
};


typedef enum irqreturn irqreturn_t;

typedef irqreturn_t (*irq_handler_t)(unsigned int irq, void *data);

struct irq_desc {
	irq_handler_t		handler;
	void		(*irq_ack)(unsigned int irq);
	void		(*irq_mask)(unsigned int irq);
	void		(*irq_mask_ack)(unsigned int irq);
	void		(*irq_unmask)(unsigned int irq);
	void		(*irq_eoi)(unsigned int irq);
	void 		*data;
};

struct irq_chip {
	void		(*irq_ack)(unsigned int irq);
	void		(*irq_mask)(unsigned int irq);
	void		(*irq_mask_ack)(unsigned int irq);
	void		(*irq_unmask)(unsigned int irq);
	void		(*irq_eoi)(unsigned int irq);
};


extern struct irq_desc irq_desc_array[256];
extern void handle_level_irq(unsigned int irq);
extern void handle_IRQ(unsigned int irq, struct pt_regs *regs);
extern void set_irq_desc_handler(unsigned int irq, irq_handler_t handler);
extern void set_irq_desc_ops(unsigned int irq, struct irq_chip *chip);
extern void setup_irq(unsigned int irq, irq_handler_t handler, void *data);
extern void init_IRQ(void);
extern void chained_irq_enter(unsigned int irq);
extern void chained_irq_exit(unsigned int irq);
extern irqreturn_t handle_irq_event(unsigned int irq);
#endif

