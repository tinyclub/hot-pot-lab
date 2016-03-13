#ifndef _CPSW_H_
#define _CPSW_H_

//#include <mach/io.h>


typedef unsigned char u8;
typedef unsigned int u32;

#if 0
#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))

#define __arch_putb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a)		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))

#define writeb(v,a)			__arch_putb(v,a)
#define writew(v,a)			__arch_putw(v,a)
#define writel(v,a)			__arch_putl(v,a)

#define readb(a)			__arch_getb(a)
#define readw(a)			__arch_getw(a)
#define readl(a)			__arch_getl(a)
#endif

#define DIV_ROUND(n,d)		(((n) + ((d)/2)) / (d))
#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))

/* CPSW Config space */
#define CPSW_MDIO_BASE			(0x4A101000 + OMAP2_L4_FAST_IO_OFFSET)

/* CPSW Config space */
#define CPSW_BASE			(0x4A100000 + OMAP2_L4_FAST_IO_OFFSET)

#define GMII1_SEL_MII		0x0
#define GMII1_SEL_RMII		0x1
#define GMII1_SEL_RGMII		0x2
#define GMII2_SEL_MII		0x0
#define GMII2_SEL_RMII		0x4
#define GMII2_SEL_RGMII		0x8
#define RGMII1_IDMODE		BIT(4)
#define RGMII2_IDMODE		BIT(5)
#define RMII1_IO_CLK_EN		BIT(6)
#define RMII2_IO_CLK_EN		BIT(7)

#define MII_MODE_ENABLE		(GMII1_SEL_MII | GMII2_SEL_MII)
#define RMII_MODE_ENABLE        (GMII1_SEL_RMII | GMII2_SEL_RMII)
#define RGMII_MODE_ENABLE	(GMII1_SEL_RGMII | GMII2_SEL_RGMII)
#define RGMII_INT_DELAY		(RGMII1_IDMODE | RGMII2_IDMODE)
#define RMII_CHIPCKL_ENABLE     (RMII1_IO_CLK_EN | RMII2_IO_CLK_EN)

typedef enum {
	PHY_INTERFACE_MODE_MII,
	PHY_INTERFACE_MODE_GMII,
	PHY_INTERFACE_MODE_SGMII,
	PHY_INTERFACE_MODE_QSGMII,
	PHY_INTERFACE_MODE_TBI,
	PHY_INTERFACE_MODE_RMII,
	PHY_INTERFACE_MODE_RGMII,
	PHY_INTERFACE_MODE_RGMII_ID,
	PHY_INTERFACE_MODE_RGMII_RXID,
	PHY_INTERFACE_MODE_RGMII_TXID,
	PHY_INTERFACE_MODE_RTBI,
	PHY_INTERFACE_MODE_XGMII,
	PHY_INTERFACE_MODE_NONE	/* Must be last */
} phy_interface_t;


struct cpsw_slave_data {
	u32		slave_reg_ofs;
	u32		sliver_reg_ofs;
	int		phy_addr;
	int		phy_if;
};

enum {
	CPSW_CTRL_VERSION_1 = 0,
	CPSW_CTRL_VERSION_2	/* am33xx like devices */
};

struct cpsw_platform_data {
	u32	mdio_base;
	u32	cpsw_base;
	int	mdio_div;
	int	channels;	/* number of cpdma channels (symmetric)	*/
	u32	cpdma_reg_ofs;	/* cpdma register offset		*/
	int	slaves;		/* number of slave cpgmac ports		*/
	u32	ale_reg_ofs;	/* address lookup engine reg offset	*/
	int	ale_entries;	/* ale table size			*/
	u32	host_port_reg_ofs;	/* cpdma host port registers	*/
	u32	hw_stats_reg_ofs;	/* cpsw hw stats counters	*/
	u32	bd_ram_ofs;		/* Buffer Descriptor RAM offset */
	u32	mac_control;
	struct cpsw_slave_data	*slave_data;
	void	(*control)(int enabled);
	u32	host_port_num;
	u8	version;
};

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_addr	= 0,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_addr	= 1,
	},
};

static void cpsw_control(int enabled)
{
	/* VTP can be added here */

	return;
}

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= CPSW_MDIO_BASE,
	.cpsw_base		= CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 1,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.bd_ram_ofs		= 0x2000,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};

/* Control Device Register */
struct ctrl_dev {
	unsigned int deviceid;		/* offset 0x00 */
	unsigned int resv1[7];
	unsigned int usb_ctrl0;		/* offset 0x20 */
	unsigned int resv2;
	unsigned int usb_ctrl1;		/* offset 0x28 */
	unsigned int resv3;
	unsigned int macid0l;		/* offset 0x30 */
	unsigned int macid0h;		/* offset 0x34 */
	unsigned int macid1l;		/* offset 0x38 */
	unsigned int macid1h;		/* offset 0x3c */
	unsigned int resv4[4];
	unsigned int miisel;		/* offset 0x50 */
	unsigned int resv5[106];
	unsigned int efuse_sma;		/* offset 0x1FC */
};

#endif /* _CPSW_H_  */


