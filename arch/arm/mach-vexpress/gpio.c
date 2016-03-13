
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <dim-sum/irq.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <dim-sum/init.h>
#include <dim-sum/spinlock.h>
#include <asm/mach/arch.h>
#include <asm/exception.h>
#include <asm/mach-types.h>
#include <asm/string.h>
#include <asm/param.h>
#include <mach/io.h>
#include <mach/serial.h>
#include <mach/mux.h>
#include <dim-sum/delay.h>
#include "gpio.h"

struct gpio_bank {
	void __iomem *base;
	unsigned int irq;
	unsigned int virtual_irq_start;
	unsigned int id;
	unsigned int bank_type;
	unsigned int mod_usage;
	spinlock_t lock;
	unsigned int width;
	unsigned int level_mask;
	unsigned int trigger;
	struct omap_gpio_reg_offs *regs;
};

struct omap_gpio_reg_offs omap_gpio_regs;


struct gpio_bank omap_gpio_bank[4] = {
	{
		.base = 0,
	},
	{
		.base = OMAP2_L4_PER_IO_ADDRESS(0x4804c000),
		.irq = 98,
		.virtual_irq_start = IH_GPIO_BASE + 32,
		.id = 1,
		.bank_type = METHOD_GPIO_44XX,
		.width = 32,
		.mod_usage = 0,
	},
};

struct omap_pin_mux gpio1_16_pin_mux = {
	.pin_offset = 0x840,
	.pin_value = OMAP_MUX_MODE7 | AM33XX_PIN_INPUT,
};

struct omap_pin_mux gpio1_17_pin_mux = {
	.pin_offset = 0x844,
	.pin_value = OMAP_MUX_MODE7 | AM33XX_PIN_INPUT,
};



struct omap_pin_mux gpio1_20_pin_mux = {
	.pin_offset = 0x850,
	.pin_value = OMAP_MUX_MODE7 | AM33XX_PIN_INPUT,
};

struct omap_pin_mux gpio1_21_pin_mux = {
	.pin_offset = 0x854,
	.pin_value = OMAP_MUX_MODE7 | AM33XX_PIN_INPUT,
};

struct omap_pin_mux gpio1_22_pin_mux = {
	.pin_offset = 0x858,
	.pin_value = OMAP_MUX_MODE7 | AM33XX_PIN_INPUT,
};

struct omap_pin_mux gpio1_26_pin_mux = {
	.pin_offset = 0x868,
	.pin_value = OMAP_MUX_MODE7 | AM33XX_PIN_INPUT,
};


#define GPIO_INDEX(bank, gpio) (gpio % bank->width)
#define GPIO_BIT(bank, gpio) (1 << GPIO_INDEX(bank, gpio))

static void _set_gpio_direction(struct gpio_bank *bank, int gpio, int is_input)
{
	void __iomem *reg = bank->base;
	u32 l;

	reg += bank->regs->direction;
	l = __raw_readl(reg);
	if (is_input)
		l |= 1 << gpio;
	else
		l &= ~(1 << gpio);
	__raw_writel(l, reg);
}


/* set data out value using dedicate set/clear register */
static void _set_gpio_dataout_reg(struct gpio_bank *bank, int gpio, int enable)
{
	void __iomem *reg = bank->base;
	u32 l = GPIO_BIT(bank, gpio);

	if (enable)
		reg += bank->regs->set_dataout;
	else
		reg += bank->regs->clr_dataout;

	__raw_writel(l, reg);
}

#if 0
/* set data out value using mask register */
static void _set_gpio_dataout_mask(struct gpio_bank *bank, int gpio, int enable)
{
	void __iomem *reg = bank->base + bank->regs->dataout;
	u32 gpio_bit = GPIO_BIT(bank, gpio);
	u32 l;

	l = __raw_readl(reg);
	if (enable)
		l |= gpio_bit;
	else
		l &= ~gpio_bit;
	__raw_writel(l, reg);
}

static int _get_gpio_datain(struct gpio_bank *bank, int gpio)
{
	void __iomem *reg = bank->base + bank->regs->datain;

	return (__raw_readl(reg) & GPIO_BIT(bank, gpio)) != 0;
}

static int _get_gpio_dataout(struct gpio_bank *bank, int gpio)
{
	void __iomem *reg = bank->base + bank->regs->dataout;

	return (__raw_readl(reg) & GPIO_BIT(bank, gpio)) != 0;
}
#endif

static inline void _gpio_rmw(void __iomem *base, u32 reg, u32 mask, bool set)
{
	int l = __raw_readl(base + reg);

	if (set) 
		l |= mask;
	else
		l &= ~mask;

	__raw_writel(l, base + reg);
}

#if 0
static void _set_gpio_debounce(struct gpio_bank *bank, unsigned gpio,
		unsigned debounce)
{
	void __iomem		*reg;
	u32			val;
	u32			l;

	//if (!bank->dbck_flag)
	//	return;

	if (debounce < 32)
		debounce = 0x01;
	else if (debounce > 7936)
		debounce = 0xff;
	else
		debounce = (debounce / 0x1f) - 1;

	l = GPIO_BIT(bank, gpio);

	reg = bank->base + bank->regs->debounce;
	__raw_writel(debounce, reg);

	reg = bank->base + bank->regs->debounce_en;
	val = __raw_readl(reg);

	if (debounce) {
		val |= l;
		//clk_enable(bank->dbck);
	} else {
		val &= ~l;
		//clk_disable(bank->dbck);
	}
	//bank->dbck_enable_mask = val;

	__raw_writel(val, reg);
}
#endif

static inline void set_24xx_gpio_triggering(struct gpio_bank *bank, int gpio,
						int trigger)
{
	void __iomem *base = bank->base;
	u32 gpio_bit = 1 << gpio;

	_gpio_rmw(base, OMAP4_GPIO_LEVELDETECT0, gpio_bit,
			  trigger & IRQ_TYPE_LEVEL_LOW);
	_gpio_rmw(base, OMAP4_GPIO_LEVELDETECT1, gpio_bit,
			  trigger & IRQ_TYPE_LEVEL_HIGH);
	_gpio_rmw(base, OMAP4_GPIO_RISINGDETECT, gpio_bit,
			  trigger & IRQ_TYPE_EDGE_RISING);
	_gpio_rmw(base, OMAP4_GPIO_FALLINGDETECT, gpio_bit,
			  trigger & IRQ_TYPE_EDGE_FALLING);

	bank->level_mask =
			__raw_readl(bank->base + OMAP4_GPIO_LEVELDETECT0) |
			__raw_readl(bank->base + OMAP4_GPIO_LEVELDETECT1);
}

static int _set_gpio_triggering(struct gpio_bank *bank, int gpio, int trigger)
{
	void __iomem *reg = bank->base;
	u32 l = 0;
	switch (bank->bank_type) {
		case METHOD_GPIO_24XX:
		case METHOD_GPIO_44XX:
			set_24xx_gpio_triggering(bank, gpio, trigger);
			return 0;
			default:
		goto bad;
	}
	__raw_writel(l, reg);
	return 0;
bad:
	return -22;
}

#if 0
static int gpio_irq_type(void *data, unsigned type)
{
	struct gpio_bank *bank = (struct gpio_bank *)data;
	unsigned gpio;
	int retval;
	unsigned long flags;


	gpio = bank->irq - IH_GPIO_BASE;

	if (type & ~IRQ_TYPE_SENSE_MASK)
		return -22;

	spin_lock_irqsave(&bank->lock, flags);
	retval = _set_gpio_triggering(bank, GPIO_INDEX(bank, gpio), type);
	spin_unlock_irqrestore(&bank->lock, flags);
#if 0
	if (type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH))
		__irq_set_handler_locked(d->irq, handle_level_irq);
	else if (type & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		__irq_set_handler_locked(d->irq, handle_edge_irq);
#endif
	return retval;
}
#endif

static void _clear_gpio_irqbank(struct gpio_bank *bank, int gpio_mask)
{
	void __iomem *reg = bank->base;

	reg += bank->regs->irqstatus;
	__raw_writel(gpio_mask, reg);

	/* Workaround for clearing DSP GPIO interrupts to allow retention */
	if (bank->regs->irqstatus2) {
		reg = bank->base + bank->regs->irqstatus2;
		__raw_writel(gpio_mask, reg);
	}

	/* Flush posted write for the irq status to avoid spurious interrupts */
	__raw_readl(reg);
}


static inline void _clear_gpio_irqstatus(struct gpio_bank *bank, int gpio)
{
	_clear_gpio_irqbank(bank, GPIO_BIT(bank, gpio));
}

static u32 _get_gpio_irqbank_mask(struct gpio_bank *bank)
{
	void __iomem *reg = bank->base;
	u32 l;
	u32 mask = (1 << bank->width) - 1;

	reg += bank->regs->irqenable;
	l = __raw_readl(reg);
	if (bank->regs->irqenable_inv)
		l = ~l;
	l &= mask;
	return l;
}


static void _enable_gpio_irqbank(struct gpio_bank *bank, int gpio_mask)
{
	void __iomem *reg = bank->base;
	u32 l;

	if (bank->regs->set_irqenable) {
		reg += bank->regs->set_irqenable;
		l = gpio_mask;
	} else {
		reg += bank->regs->irqenable;
		l = __raw_readl(reg);
		if (bank->regs->irqenable_inv)
			l &= ~gpio_mask;
		else
			l |= gpio_mask;
	}

	__raw_writel(l, reg);
}


static void _disable_gpio_irqbank(struct gpio_bank *bank, int gpio_mask)
{
	void __iomem *reg = bank->base;
	u32 l;

	if (bank->regs->clr_irqenable) {
		reg += bank->regs->clr_irqenable;
		l = gpio_mask;
	} else {
		reg += bank->regs->irqenable;
		l = __raw_readl(reg);
		if (bank->regs->irqenable_inv)
			l |= gpio_mask;
		else
			l &= ~gpio_mask;
	}

	__raw_writel(l, reg);
}


static inline void _set_gpio_irqenable(struct gpio_bank *bank, int gpio, int enable)
{
	_enable_gpio_irqbank(bank, GPIO_BIT(bank, gpio));
}

#if 0
static void _reset_gpio(struct gpio_bank *bank, int gpio)
{
	_set_gpio_direction(bank, GPIO_INDEX(bank, gpio), 1);
	_set_gpio_irqenable(bank, gpio, 0);
	_clear_gpio_irqstatus(bank, gpio);
	_set_gpio_triggering(bank, GPIO_INDEX(bank, gpio), IRQ_TYPE_NONE);
}



//----------------------------------------------------------------
static void gpio_irq_shutdown(unsigned int irq)
{
	struct gpio_bank *bank;
	unsigned int id = irq_to_bankid(irq);
	unsigned int gpio = irq - IH_GPIO_BASE;
	unsigned long flags;
	bank = &omap_gpio_bank[id];

	spin_lock_irqsave(&bank->lock, flags);
	_reset_gpio(bank, gpio);
	spin_unlock_irqrestore(&bank->lock, flags);
}
#endif

static void gpio_ack_irq(unsigned int irq)
{
	struct gpio_bank *bank;
	unsigned int id = irq_to_bankid(irq);
	unsigned int gpio = irq - IH_GPIO_BASE;
	bank = &omap_gpio_bank[id];

	_clear_gpio_irqstatus(bank, gpio);
}

static void gpio_mask_irq(unsigned int irq)
{
	struct gpio_bank *bank;
	unsigned int id = irq_to_bankid(irq);
	unsigned int gpio = irq - IH_GPIO_BASE;
	unsigned long flags;
	bank = &omap_gpio_bank[id];
	spin_lock_irqsave(&bank->lock, flags);
	_set_gpio_irqenable(bank, gpio, 0);
	_set_gpio_triggering(bank, GPIO_INDEX(bank, gpio), IRQ_TYPE_NONE);
	spin_unlock_irqrestore(&bank->lock, flags);
}

static void gpio_unmask_irq(unsigned int irq)
{
	struct gpio_bank *bank;
	unsigned int id = irq_to_bankid(irq);
	unsigned int gpio = irq - IH_GPIO_BASE;
	unsigned int irq_mask;
	//u32 trigger = bank->trigger;
	unsigned long flags;
	bank = &omap_gpio_bank[id];
	irq_mask = GPIO_BIT(bank, gpio);

	spin_lock_irqsave(&bank->lock, flags);
	//if (trigger)
	//	_set_gpio_triggering(bank, GPIO_INDEX(bank, gpio), trigger);

	/* For level-triggered GPIOs, the clearing must be done after
	 * the HW source is cleared, thus after the handler has run */
	if (bank->level_mask & irq_mask) {
		_set_gpio_irqenable(bank, gpio, 0);
		_clear_gpio_irqstatus(bank, gpio);
	}

	_set_gpio_irqenable(bank, gpio, 1);
	spin_unlock_irqrestore(&bank->lock, flags);
}


//-----------------------------------------------------------------

static irqreturn_t gpio_irq_handler(unsigned int irq, void *data)
{
	void __iomem *isr_reg = NULL;
	u32 isr;
	unsigned int gpio_irq; //gpio_index;
	struct gpio_bank *bank = (struct gpio_bank *)data;
	u32 retrigger = 0;
	int unmasked = 0;
	
	//printk("gpio\n");
	
	chained_irq_enter(bank->irq);

	isr_reg = bank->base + bank->regs->irqstatus;

	if (!isr_reg)		
		goto exit;

	while(1) {
		u32 isr_saved, level_mask = 0;
		u32 enabled;

		enabled = _get_gpio_irqbank_mask(bank);
		isr_saved = isr = __raw_readl(isr_reg) & enabled;

		level_mask = bank->level_mask & enabled;

		/* clear edge sensitive interrupts before handler(s) are
		called so that we don't miss any interrupt occurred while
		executing them */
		_disable_gpio_irqbank(bank, isr_saved & ~level_mask);
		_clear_gpio_irqbank(bank, isr_saved & ~level_mask);
		_enable_gpio_irqbank(bank, isr_saved & ~level_mask);

		/* if there is only edge sensitive GPIO pin interrupts
		configured, we could unmask GPIO bank interrupt immediately */
		if (!level_mask && !unmasked) {
			unmasked = 1;
			chained_irq_exit(bank->irq);
		}

		isr |= retrigger;
		retrigger = 0;
		if (!isr)
			break;

		gpio_irq = bank->virtual_irq_start;
		for (; isr != 0; isr >>= 1, gpio_irq++) {
			//gpio_index = GPIO_INDEX(bank, irq_to_gpio(gpio_irq));

			if (!(isr & 1))
				continue;

			handle_irq_event(gpio_irq);
		}
	}
	/* if bank has any level sensitive GPIO pin interrupt
	configured, we must unmask the bank interrupt only after
	handler(s) are executed in order to avoid spurious bank
	interrupt */
exit:
	if (!unmasked)
		chained_irq_exit(bank->irq);

	return IRQ_HANDLED;
}

int lx_flag = 0;
static irqreturn_t gpio_simple_isr(unsigned int irq, void *data)
{
	struct gpio_bank *bank = (struct gpio_bank *)data;
	int offset = GPIO_INDEX(bank, 32 + 16);
	int offset1 = GPIO_INDEX(bank, 32 + 26);
	if (lx_flag) {
		lx_flag = 0;
		//_set_gpio_dataout_reg(bank, offset, 0);
		//_set_gpio_direction(bank, offset, 0);

		_set_gpio_dataout_reg(bank, offset1, 0);
		_set_gpio_direction(bank, offset1, 0);
	} else {
		lx_flag = 1;
		//_set_gpio_dataout_reg(bank, offset, 1);
		//_set_gpio_direction(bank, offset, 0);

		_set_gpio_dataout_reg(bank, offset1, 1);
		_set_gpio_direction(bank, offset1, 0);
	}
	return IRQ_HANDLED;
}

//------------------------------------------------------------------

static int omap_gpio_request(unsigned int id, unsigned offset)
{
	struct gpio_bank *bank = &omap_gpio_bank[id];
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);

	/* Set trigger to none. You need to enable the desired trigger with
	 * request_irq() or set_irq_type().
	 */
	_set_gpio_triggering(bank, offset, IRQ_TYPE_NONE);
	
	if (!bank->mod_usage) {
		void __iomem *reg = bank->base;
		u32 ctrl;
	

		reg += OMAP4_GPIO_CTRL;

		ctrl = __raw_readl(reg);
		/* Module is enabled, clocks are not gated */
		ctrl &= 0xFFFFFFFE;
		__raw_writel(ctrl, reg);
	}
	bank->mod_usage |= 1 << offset;
	
	spin_unlock_irqrestore(&bank->lock, flags);

	return 0;

}

static int omap_gpio_input(unsigned int id, unsigned offset)
{
	struct gpio_bank *bank = &omap_gpio_bank[id];
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);
	_set_gpio_direction(bank, offset, 1);
	spin_unlock_irqrestore(&bank->lock, flags);
	return 0;
}

static int omap_gpio_output(unsigned int id, unsigned offset, int value)
{
	struct gpio_bank *bank = &omap_gpio_bank[id];
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);
	_set_gpio_dataout_reg(bank, offset, value);
	_set_gpio_direction(bank, offset, 0);
	spin_unlock_irqrestore(&bank->lock, flags);
	return 0;
}

#if 0
static int gpio_debounce(unsigned int id, unsigned offset, unsigned debounce)
{
	struct gpio_bank *bank = &omap_gpio_bank[id];
	unsigned long flags;
#if 0
	if (!bank->dbck) {
		bank->dbck = clk_get(bank->dev, "dbclk");
		if (IS_ERR(bank->dbck))
			dev_err(bank->dev, "Could not get gpio dbck\n");
	}
#endif
	spin_lock_irqsave(&bank->lock, flags);
	_set_gpio_debounce(bank, offset, debounce);
	spin_unlock_irqrestore(&bank->lock, flags);

	return 0;
}
#endif

#if 0
static void gpio_set(unsigned int id, unsigned offset, int value)
{
	struct gpio_bank *bank = &omap_gpio_bank[id];
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);
	_set_gpio_dataout_reg(bank, offset, value);
	spin_unlock_irqrestore(&bank->lock, flags);
}
#endif

static int omap_gpio_settrigger(unsigned int id, unsigned offset, int type)
{
	struct gpio_bank *bank = &omap_gpio_bank[id];
	unsigned long flags;

	spin_lock_irqsave(&bank->lock, flags);
	_set_gpio_triggering(bank, offset, type);
	spin_unlock_irqrestore(&bank->lock, flags);
	return 0;
}


static void
omap_gpio_set_mux_conf(struct omap_pin_mux *pin_mux)
{
	void __iomem *base = AM33XX_L4_WK_IO_ADDRESS(OMAP2_CTRL_BASE + pin_mux->pin_offset);
	__raw_writel(pin_mux->pin_value, base);

	base = AM33XX_L4_WK_IO_ADDRESS(OMAP2_CTRL_BASE + pin_mux->pin_offset);
	__raw_writel(pin_mux->pin_value, base);
}

static void omap_gpio_early_init(void)
{
	unsigned int temp;
	void *addr;
	int j = 1000;
	addr = AM33XX_L4_WK_IO_ADDRESS(0x44e00000 + 0xac);
	temp = readl(addr);

	temp &= ~0x3;
	temp |= 0x2;
	writel(temp, addr);
			
	do {	
		temp = readl(addr);
		temp = (temp >> 16) & 0x3;
		j--;
		if (!j)
			break;
	} while((temp == 1) || (temp == 3));
}

static void omap_gpio_mod_init(struct gpio_bank *bank)
{
	__raw_writel(0xffffffff, bank->base +
					OMAP4_GPIO_IRQSTATUSCLR0);
	__raw_writel(0x00000000, bank->base +
					 OMAP4_GPIO_DEBOUNCENABLE);
	/* Initialize interface clk ungated, module enabled */
	__raw_writel(0, bank->base + OMAP4_GPIO_CTRL);
}


void omap_gpio_init(void)
{
	struct gpio_bank *bank;

	omap_gpio_early_init();

	omap_gpio_set_mux_conf(&gpio1_16_pin_mux);
	omap_gpio_set_mux_conf(&gpio1_17_pin_mux);

	omap_gpio_set_mux_conf(&gpio1_20_pin_mux);
	omap_gpio_set_mux_conf(&gpio1_21_pin_mux);
	omap_gpio_set_mux_conf(&gpio1_22_pin_mux);

	omap_gpio_set_mux_conf(&gpio1_26_pin_mux);

	bank = &omap_gpio_bank[1];
	bank->regs = &omap_gpio_regs;
	bank->regs->revision = OMAP4_GPIO_REVISION;
	bank->regs->direction = OMAP4_GPIO_OE;
	bank->regs->datain = OMAP4_GPIO_DATAIN;
	bank->regs->dataout = OMAP4_GPIO_DATAOUT;
	bank->regs->set_dataout = OMAP4_GPIO_SETDATAOUT;
	bank->regs->clr_dataout = OMAP4_GPIO_CLEARDATAOUT;
	bank->regs->irqstatus = OMAP4_GPIO_IRQSTATUS0;
	bank->regs->irqstatus2 = OMAP4_GPIO_IRQSTATUS1;
	bank->regs->irqenable = OMAP4_GPIO_IRQSTATUSSET0;
	bank->regs->set_irqenable = OMAP4_GPIO_IRQSTATUSSET0;
	bank->regs->clr_irqenable = OMAP4_GPIO_IRQSTATUSCLR0;
	bank->regs->debounce = OMAP4_GPIO_DEBOUNCINGTIME;
	bank->regs->debounce_en = OMAP4_GPIO_DEBOUNCENABLE;
	bank->regs->irqenable_inv = false;

	omap_gpio_mod_init(bank);

	spin_lock_init(&bank->lock);
	
	{
		struct irq_chip chip;
		int j;
		chip.irq_ack = gpio_ack_irq;
		chip.irq_mask = gpio_mask_irq;
		chip.irq_unmask = gpio_unmask_irq;
		for (j = bank->virtual_irq_start;
		     j < bank->virtual_irq_start + bank->width; j++) {
			set_irq_desc_ops(j, &chip);
			setup_irq(j, gpio_simple_isr, bank);
		}
	}

	setup_irq(bank->irq, gpio_irq_handler, bank);

	
	omap_gpio_request(1, 20);
	omap_gpio_input(1, 20);
	omap_gpio_settrigger(1, 20, IRQ_TYPE_EDGE_FALLING);

	omap_gpio_request(1, 16);
	omap_gpio_output(1, 16, 0);

	omap_gpio_request(1, 26);
	omap_gpio_output(1, 26, 0);
	printk("gpio init\n");
}



