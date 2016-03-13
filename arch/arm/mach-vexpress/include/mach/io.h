
#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include <asm/sizes.h>

#ifdef __ASSEMBLER__
#define IOMEM(x)		(x)
#else
#define IOMEM(x)		((void __force __iomem *)(x))
#endif

#define L4_WK_AM33XX_BASE     0x10000000
//#define L4_WK_AM33XX_BASE	0x49020000
//#define L4_WK_AM33XX_BASE	0x44C00000
#define L4_PER_AM33XX_BASE	0x48000000
#define L4_FAST_AM33XX_BASE	0x4A000000


#define AM33XX_L4_WK_IO_OFFSET	0xb5000000
#define AM33XX_L4_WK_IO_ADDRESS(pa)	IOMEM((pa) + AM33XX_L4_WK_IO_OFFSET)

#define OMAP2_L4_PER_IO_OFFSET	0xb2000000
#define OMAP2_L4_PER_IO_ADDRESS(pa)	IOMEM((pa) + OMAP2_L4_PER_IO_OFFSET)

#define OMAP2_L4_FAST_IO_OFFSET	0xB1000000
#define OMAP2_L4_FAST_IO_ADDRESS(pa)	IOMEM((pa) + OMAP2_L4_FAST_IO_OFFSET)



#define L4_WK_AM33XX_PHYS	L4_WK_AM33XX_BASE
#define L4_WK_AM33XX_VIRT	(L4_WK_AM33XX_PHYS + AM33XX_L4_WK_IO_OFFSET)
#define L4_WK_AM33XX_SIZE	SZ_4M   /* 1MB of 128MB used, want 1MB sect */


#define L4_PER_AM33XX_PHYS		L4_PER_AM33XX_BASE	/* 0x48000000 --> 0xfa000000 */
#define L4_PER_AM33XX_VIRT		(L4_PER_AM33XX_PHYS + OMAP2_L4_PER_IO_OFFSET)
#define L4_PER_AM33XX_SIZE		SZ_16M   /* 1MB of 128MB used, want 1MB sect */


#define L4_FAST_AM33XX_PHYS		L4_FAST_AM33XX_BASE
#define L4_FAST_AM33XX_VIRT		(L4_FAST_AM33XX_PHYS + OMAP2_L4_FAST_IO_OFFSET)
#define L4_FAST_AM33XX_SIZE		SZ_16M   /* 1MB of 128MB used, want 1MB sect */

#define L3_44XX_BASE			0x44000000
#define OMAP4_L3_IO_OFFSET	0xb4000000

#define L3_44XX_PHYS		L3_44XX_BASE	/* 0x44000000 --> 0xf8000000 */
#define L3_44XX_VIRT		(L3_44XX_PHYS + OMAP4_L3_IO_OFFSET)
#define L3_44XX_SIZE		SZ_1M

#define L4_44XX_BASE			0x4a000000
#define OMAP2_L4_IO_OFFSET	0xb2000000

#define L4_44XX_PHYS		L4_44XX_BASE	/* 0x4a000000 --> 0xfc000000 */
#define L4_44XX_VIRT		(L4_44XX_PHYS + OMAP2_L4_IO_OFFSET)
#define L4_44XX_SIZE		SZ_4M

#define L4_PER_44XX_BASE		0x48000000
#define L4_PER_44XX_PHYS	L4_PER_44XX_BASE

						/* 0x48000000 --> 0xfa000000 */
#define L4_PER_44XX_VIRT	(L4_PER_44XX_PHYS + OMAP2_L4_IO_OFFSET)
#define L4_PER_44XX_SIZE	SZ_4M

#define L4_PER_34XX_BASE	0x49000000
#define L4_PER_34XX_PHYS	L4_PER_34XX_BASE
						/* 0x49000000 --> 0xfb000000 */
#define L4_PER_34XX_VIRT	(L4_PER_34XX_PHYS + OMAP2_L4_IO_OFFSET)
#define L4_PER_34XX_SIZE	SZ_1M


#define OMAP34XX_IC_BASE	0x48200000

#define OMAP2_CTRL_BASE     0x44E10000

#define OMAP_CTRL_REGADDR(reg)		(OMAP2_CTRL_BASE + (reg))


#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a) = (v))
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile unsigned short __force *)(a) = (v))
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a) = (v))

#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a))
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile unsigned short __force *)(a))
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a))


#define writeb(v,a)	__raw_writeb(v,a)
#define writew(v,a)	__raw_writew(v,a)
#define writel(v,a)	__raw_writel(v,a)

#define readb(a) __raw_readb(a)
#define readw(a) __raw_readw(a)
#define readl(a) __raw_readl(a)

#endif

