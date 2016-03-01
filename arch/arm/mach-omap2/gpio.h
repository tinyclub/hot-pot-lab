#ifndef __OMAP_GPIO_H
#define __OMAP_GPIO_H


#define OMAP4_GPIO_REVISION		0x0000
#define OMAP4_GPIO_EOI			0x0020
#define OMAP4_GPIO_IRQSTATUSRAW0	0x0024
#define OMAP4_GPIO_IRQSTATUSRAW1	0x0028
#define OMAP4_GPIO_IRQSTATUS0		0x002c
#define OMAP4_GPIO_IRQSTATUS1		0x0030
#define OMAP4_GPIO_IRQSTATUSSET0	0x0034
#define OMAP4_GPIO_IRQSTATUSSET1	0x0038
#define OMAP4_GPIO_IRQSTATUSCLR0	0x003c
#define OMAP4_GPIO_IRQSTATUSCLR1	0x0040
#define OMAP4_GPIO_IRQWAKEN0		0x0044
#define OMAP4_GPIO_IRQWAKEN1		0x0048
#define OMAP4_GPIO_IRQENABLE1		0x011c
#define OMAP4_GPIO_WAKE_EN		0x0120
#define OMAP4_GPIO_IRQSTATUS2		0x0128
#define OMAP4_GPIO_IRQENABLE2		0x012c
#define OMAP4_GPIO_CTRL			0x0130
#define OMAP4_GPIO_OE			0x0134
#define OMAP4_GPIO_DATAIN		0x0138
#define OMAP4_GPIO_DATAOUT		0x013c
#define OMAP4_GPIO_LEVELDETECT0		0x0140
#define OMAP4_GPIO_LEVELDETECT1		0x0144
#define OMAP4_GPIO_RISINGDETECT		0x0148
#define OMAP4_GPIO_FALLINGDETECT	0x014c
#define OMAP4_GPIO_DEBOUNCENABLE	0x0150
#define OMAP4_GPIO_DEBOUNCINGTIME	0x0154
#define OMAP4_GPIO_CLEARIRQENABLE1	0x0160
#define OMAP4_GPIO_SETIRQENABLE1	0x0164
#define OMAP4_GPIO_CLEARWKUENA		0x0180
#define OMAP4_GPIO_SETWKUENA		0x0184
#define OMAP4_GPIO_CLEARDATAOUT		0x0190
#define OMAP4_GPIO_SETDATAOUT		0x0194

#define OMAP_MAX_GPIO_LINES	192

#define IH2_BASE		32

#define IH_GPIO_BASE		(128 + IH2_BASE)

#define OMAP_GPIO_IRQ(nr)	(IH_GPIO_BASE + (nr))

#define METHOD_MPUIO		0
#define METHOD_GPIO_1510	1
#define METHOD_GPIO_1610	2
#define METHOD_GPIO_7XX		3
#define METHOD_GPIO_24XX	5
#define METHOD_GPIO_44XX	6

struct omap_gpio_reg_offs {
	unsigned int revision;
	unsigned int direction;
	unsigned int datain;
	unsigned int dataout;
	unsigned int set_dataout;
	unsigned int clr_dataout;
	unsigned int irqstatus;
	unsigned int irqstatus2;
	unsigned int irqenable;
	unsigned int set_irqenable;
	unsigned int clr_irqenable;
	unsigned int debounce;
	unsigned int debounce_en;

	bool irqenable_inv;
};


static inline int irq_to_gpio(unsigned irq)
{
	int tmp;

	/* SOC gpio */
	tmp = irq - IH_GPIO_BASE;
	if (tmp < OMAP_MAX_GPIO_LINES)
		return tmp;

	/* we don't supply reverse mappings for non-SOC gpios */
	return -5;
}

static inline int irq_to_bankid(unsigned irq)
{
	int tmp;
	int id;
	tmp = irq - IH_GPIO_BASE;

	if (tmp < 32) {
		id = 0;
	} else if (tmp < 64) {
		id = 1;
	} else if (tmp < 96) {
		id = 2;
	} else if (tmp < 128) {
		id = 3;
	} else
		id = -1;

	return id;
}



#endif
