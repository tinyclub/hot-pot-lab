/*
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
 * This header provides constants specific to DRA7XX pinctrl bindings.
 */

#ifndef _DT_BINDINGS_PINCTRL_DRA7XX_H_
#define _DT_BINDINGS_PINCTRL_DRA7XX_H_

/* dra7xx specific mux bit defines */
#define MUX_MODE0	0
#define MUX_MODE1	1
#define MUX_MODE2	2
#define MUX_MODE3	3
#define MUX_MODE4	4
#define MUX_MODE5	5
#define MUX_MODE6	6
#define MUX_MODE7	7

#define PULL_ENA		(1 << 16)
#define PULL_UP			(1 << 17)
#define INPUT_EN		(1 << 18)
#define SLEWCTRL_FAST		(1 << 19)
#define WAKEUP_EN		(1 << 24)
#define WAKEUP_EVENT		(1 << 25)

/*
 * Delay value - to be added to control module register to some pins
 * to garuntee IO timings. Add delay mode select
 * and delay value for respective pins.
 */

#define DELAYMODE_SELECT	(1 << 8)

/*
 * DSS_VOUT3 is not timing closed wrt clock on vin1a_fld0 port.
 * To mux vin1a_fld0 to DSS_VOUT3, we need to add delay.
 * Below delay value ensure the timing is closed
 */

#define DELAY_VIN1A		(15 << 4)

/* Active pin states */
#define PIN_OUTPUT		0
#define PIN_OUTPUT_PULLUP	(PIN_OUTPUT | PULL_ENA | PULL_UP)
#define PIN_OUTPUT_PULLDOWN	(PIN_OUTPUT | PULL_ENA)
#define PIN_INPUT		INPUT_EN
#define PIN_INPUT_PULLUP	(PULL_ENA | INPUT_EN | PULL_UP)
#define PIN_INPUT_PULLDOWN	(PULL_ENA | INPUT_EN)

/* Off mode states */
#define PIN_OFF_NONE		0
#define PIN_OFF_OUTPUT_HIGH	(OFF_EN | OFFOUT_EN | OFFOUT_VAL)
#define PIN_OFF_OUTPUT_LOW	(OFF_EN | OFFOUT_EN)
#define PIN_OFF_INPUT_PULLUP	(OFF_EN | OFF_PULL_EN | OFF_PULL_UP)
#define PIN_OFF_INPUT_PULLDOWN	(OFF_EN | OFF_PULL_EN)
#define PIN_OFF_WAKEUPENABLE	WAKEUP_EN

#endif
