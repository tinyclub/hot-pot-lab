
#include <linux/linkage.h>
#include <linux/kernel.h>
#include <dim-sum/irq.h>
#include <dim-sum/init.h>
#include <asm/mach/arch.h>
#include <asm/exception.h>
#include <asm/mach-types.h>
#include <mach/io.h>
#include <linux/bug.h>

/* selected INTC register offsets */

#define INTC_REVISION		0x0000
#define INTC_SYSCONFIG		0x0010
#define INTC_SYSSTATUS		0x0014
#define INTC_SIR		0x0040
#define INTC_CONTROL		0x0048
#define INTC_PROTECTION		0x004C
#define INTC_IDLE		0x0050
#define INTC_THRESHOLD		0x0068
#define INTC_MIR0		0x0084
#define INTC_MIR_CLEAR0		0x0088
#define INTC_MIR_SET0		0x008c
#define INTC_PENDING_IRQ0	0x0098
/* Number of IRQ state bits in each MIR register */
#define IRQ_BITS_PER_REG	32

#define OMAP3_IRQ_BASE		OMAP2_L4_PER_IO_ADDRESS(OMAP34XX_IC_BASE)
#define INTCPS_SIR_IRQ_OFFSET	0x0040	/* omap2/3 active interrupt offset */
#define ACTIVEIRQ_MASK		0x7f	/* omap2/3 active interrupt bits */


static struct omap_irq_bank {
	void __iomem *base_reg;
	unsigned int nr_irqs;
	unsigned int nr_regs_req;
} __attribute__ ((aligned(4))) irq_banks[] = {
	{
		/* MPU INTC */
		.nr_irqs	= 96,
		.nr_regs_req	= 3,
	},
};

/* INTC bank register get/set */

static void intc_bank_write_reg(u32 val, struct omap_irq_bank *bank, u16 reg)
{
	__raw_writel(val, bank->base_reg + reg);
}

static u32 intc_bank_read_reg(struct omap_irq_bank *bank, u16 reg)
{
	return __raw_readl(bank->base_reg + reg);
}


static void __init omap_irq_bank_init_one(struct omap_irq_bank *bank)
{
	unsigned long tmp;

	tmp = intc_bank_read_reg(bank, INTC_REVISION) & 0xff;
	printk(KERN_INFO "IRQ: Found an INTC at 0x%p "
			 "(revision %ld.%ld) with %d interrupts\n",
			 bank->base_reg, tmp >> 4, tmp & 0xf, bank->nr_irqs);

	tmp = intc_bank_read_reg(bank, INTC_SYSCONFIG);
	tmp |= 1 << 1;	/* soft reset */
	intc_bank_write_reg(tmp, bank, INTC_SYSCONFIG);

	while (!(intc_bank_read_reg(bank, INTC_SYSSTATUS) & 0x1))
		/* Wait for reset to complete */;

	/* Enable autoidle */
	intc_bank_write_reg(1 << 0, bank, INTC_SYSCONFIG);
}

unsigned int omap_irq_to_mask(unsigned int irq)
{
	unsigned int tmp = irq / 32;

	unsigned int mask = 1 << (irq - 32 * tmp);

	return mask;
}

u16 omap_irq_to_reg(unsigned int irq, u16 reg_base)
{
	unsigned int tmp = irq / 32;
	
	u16 reg;
	
	reg = reg_base + 32 * tmp;

	return reg;
}


static void omap_ack_irq(unsigned int irq)
{
	intc_bank_write_reg(0x1, &irq_banks[0], INTC_CONTROL);
}

static void omap_mask_ack_irq(unsigned int irq)
{
	unsigned int mask;
	u16 reg;
	mask = omap_irq_to_mask(irq);
	reg = omap_irq_to_reg(irq, INTC_MIR_SET0);
	intc_bank_write_reg(mask, &irq_banks[0], reg);
	
	omap_ack_irq(irq);
}

static void omap_mask_irq(unsigned int irq)
{
	unsigned int mask;
	u16 reg;
	mask = omap_irq_to_mask(irq);
	reg = omap_irq_to_reg(irq, INTC_MIR_SET0);
	intc_bank_write_reg(mask, &irq_banks[0], reg);
}

static void omap_unmask_irq(unsigned int irq)
{
	unsigned int mask;
	u16 reg;
	mask = omap_irq_to_mask(irq);
	reg = omap_irq_to_reg(irq, INTC_MIR_CLEAR0);
	intc_bank_write_reg(mask, &irq_banks[0], reg);
	//printk("omap_unmask_irq %d\n", irq);
}

static void __init omap_init_irq_desc(unsigned int irq)
{
	struct irq_chip chip;
	chip.irq_ack = omap_mask_ack_irq;
	chip.irq_mask_ack = omap_mask_ack_irq;
	chip.irq_mask = omap_mask_irq;
	chip.irq_unmask = omap_unmask_irq;
	set_irq_desc_ops(irq, &chip);
}


static void __init omap_init_irq(u32 base, int nr_irqs)
{
	unsigned long nr_of_irqs = 0;
	unsigned int nr_banks = 0;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(irq_banks); i++) {
		struct omap_irq_bank *bank = irq_banks + i;
		bank->nr_irqs = nr_irqs;
		bank->base_reg = OMAP2_L4_PER_IO_ADDRESS(base);
		omap_irq_bank_init_one(bank);

		for (j = 0; j < bank->nr_irqs; j++) {
			omap_init_irq_desc(j);
			bank->nr_regs_req++;
		}
		
		nr_of_irqs += bank->nr_irqs;
		nr_banks++;
	}

	printk("Total of %ld interrupts on %d active controller%s\n",
	       nr_of_irqs, nr_banks, nr_banks > 1 ? "s" : "");
}

void __init ti81xx_init_irq(void)
{
	omap_init_irq(OMAP34XX_IC_BASE, 128);
}

static inline void omap_intc_handle_irq(void __iomem *base_addr, struct pt_regs *regs)
{
	u32 irqnr;

	do {
		irqnr = __raw_readl(base_addr + 0x98);
		if (irqnr)
			goto out;

		irqnr = __raw_readl(base_addr + 0xb8);
		if (irqnr)
			goto out;

		irqnr = __raw_readl(base_addr + 0xd8);
#if IS_ENABLED(CONFIG_SOC_TI81XX) || IS_ENABLED(CONFIG_SOC_AM33XX)
		if (irqnr)
			goto out;
		irqnr = __raw_readl(base_addr + 0xf8);
#endif

out:
		if (!irqnr)
			break;

		irqnr = __raw_readl(base_addr + INTCPS_SIR_IRQ_OFFSET);
		irqnr &= ACTIVEIRQ_MASK;

		if (irqnr) {
			handle_IRQ(irqnr, regs);
		}
	} while (irqnr);
}



asmlinkage void __exception_irq_entry omap3_intc_handle_irq(struct pt_regs *regs)
{
	void __iomem *base_addr = OMAP3_IRQ_BASE;
	omap_intc_handle_irq(base_addr, regs);
}

