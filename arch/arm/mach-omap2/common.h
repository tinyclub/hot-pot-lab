#ifndef __ARCH_ARM_MACH_OMAP2PLUS_COMMON_H
#define __ARCH_ARM_MACH_OMAP2PLUS_COMMON_H

void omap2_init_irq(void);
void ti81xx_init_irq(void);
void omap3_intc_handle_irq(struct pt_regs *regs);


#endif

