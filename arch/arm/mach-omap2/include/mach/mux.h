
#ifndef __INC_MACH_MUX_H
#define __INC_MACH_MUX_H

#define OMAP_MUX_MODE0      0
#define OMAP_MUX_MODE1      1
#define OMAP_MUX_MODE2      2
#define OMAP_MUX_MODE3      3
#define OMAP_MUX_MODE4      4
#define OMAP_MUX_MODE5      5
#define OMAP_MUX_MODE6      6
#define OMAP_MUX_MODE7      7



/* am33xx specific mux bit defines */
#define AM33XX_SLEWCTRL_FAST		(0 << 6)
#define AM33XX_SLEWCTRL_SLOW		(1 << 6)
#define AM33XX_INPUT_EN			(1 << 5)
#define AM33XX_PULL_UP			(1 << 4)
/* bit 3: 0 - enable, 1 - disable for pull enable */
#define AM33XX_PULL_DISA		(1 << 3)
#define AM33XX_PULL_ENBL		(0 << 3)


#define	AM33XX_PIN_OUTPUT		(0)
#define	AM33XX_PIN_OUTPUT_PULLUP	(AM33XX_PULL_UP)
#define	AM33XX_PIN_INPUT		(AM33XX_INPUT_EN | AM33XX_PULL_DISA)
#define	AM33XX_PIN_INPUT_PULLUP		(AM33XX_INPUT_EN | AM33XX_PULL_UP)
#define	AM33XX_PIN_INPUT_PULLDOWN	(AM33XX_INPUT_EN)


struct omap_pin_mux {
	unsigned int pin_offset; 
	int pin_value; /* Options for the mux register value */
};


#endif

