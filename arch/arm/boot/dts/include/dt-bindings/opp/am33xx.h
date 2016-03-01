/*
 * This header provides constants for OMAP pinctrl bindings.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __DT_BINDINGS_OPP_OPP_MODIFIER_H__
#define __DT_BINDINGS_OPP_OPP_MODIFIER_H__

#define AM33XX_EFUSE_SMA_OPP_50_300MHZ_BIT              (1 << 4)
#define AM33XX_EFUSE_SMA_OPP_100_300MHZ_BIT             (1 << 5)
#define AM33XX_EFUSE_SMA_OPP_100_600MHZ_BIT             (1 << 6)
#define AM33XX_EFUSE_SMA_OPP_120_720MHZ_BIT             (1 << 7)
#define AM33XX_EFUSE_SMA_OPP_TURBO_800MHZ_BIT		(1 << 8)
#define AM33XX_EFUSE_SMA_OPP_NITRO_1GHZ_BIT             (1 << 9)

#define AM43XX_EFUSE_SMA_OPP_50_300MHZ_BIT              (1 << 0)
#define AM43XX_EFUSE_SMA_OPP_100_600MHZ_BIT             (1 << 2)
#define AM43XX_EFUSE_SMA_OPP_120_720MHZ_BIT             (1 << 3)
#define AM43XX_EFUSE_SMA_OPP_TURBO_800MHZ_BIT		(1 << 4)
#define AM43XX_EFUSE_SMA_OPP_NITRO_1GHZ_BIT             (1 << 5)

#define OPP_REV(maj, min)  (((1 << (maj)) << 16) | (1 << (min)))
#define OPP_REV_CMP(opp1, opp2)  (((opp1) & (opp2) & 0xFFFF000) && \
				 ((opp1) & (opp2) & 0x0000FFFF))

#endif		/* __DT_BINDINGS_OPP_OPP_MODIFIER_H__ */
