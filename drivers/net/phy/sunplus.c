// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/net/phy/sunplus.c
 *
 * Driver for SUNPLUS PHYs
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/platform_device.h>

#define EPHY_CTRL		0
#define  EPHY_RST_N		(1 << 0)
#define  EPHY_SHUTDOWN		(1 << 1)
#define  EPHY_LED_POL		(1 << 14)
#define  EPHY_BGS_3		(1 << 15)
#define  EPHY_BGS_2		(1 << 16)
#define  EPHY_BGS_1		(1 << 17)
#define  EPHY_BGS_0		(1 << 18)

#define SUNPLUS_INT_STS		0x10
#define SUNPLUS_INT_MASK	0x11
#define  MGC_PKT_DET		(1 << 14)
#define  LNK_STS_CHG		(1 << 15)
#define SUNPLUS_GLOBAL_CONF	0x13
#define  RG_WOL_CHK_PSWD	(1 << 7)
#define  RG_WOL_RCV_BC		(1 << 8)
#define  EN_WOL			(1 << 10)
#define SUNPLUS_MAC0		0x16
#define SUNPLUS_MAC1		0x17
#define SUNPLUS_MAC2		0x18
#define SUNPLUS_STS		0x19
#define  FINAL_LINK		(1 << 0)
#define SUNPLUS_PAGE_SELECT	0x1f

struct sunplus_ephy {
	void __iomem *base;
	struct device *dev;
};

static u8 bgs;
module_param(bgs, byte, 0);

static int sunplus_read_page(struct phy_device *phydev)
{
	int ret = __phy_read(phydev, SUNPLUS_PAGE_SELECT);
	if (ret < 0)
		return ret;

	return (ret >> 8) & 0x1F;
}

static int sunplus_write_page(struct phy_device *phydev, int page)
{
	return __phy_write(phydev, SUNPLUS_PAGE_SELECT, (page & 0x1f) << 8);
}

static void sunplus_init(struct sunplus_ephy *priv)
{
	u32 val;
	int bgsval;

	val = readl(priv->base + EPHY_CTRL);
	val |= EPHY_SHUTDOWN;
	val &= ~EPHY_RST_N;
	writel(val, priv->base + EPHY_CTRL);

	val = readl(priv->base + EPHY_CTRL);
	val &= ~EPHY_SHUTDOWN;
	writel(val, priv->base + EPHY_CTRL);
	msleep(10);
	val |= EPHY_RST_N;
	writel(val, priv->base + EPHY_CTRL);
	udelay(12);
	val = readl(priv->base + EPHY_CTRL);
	val |= EPHY_LED_POL;
	if ((bgs & 0xf0) == 0xf0) {
		bgsval = bgs & 0xf;
		if (bgsval > 7)
			bgsval -= 16;
		bgsval += -2;
		if (bgsval < -8)
			bgsval = -8;
		printk("fephy bgs: %d\n", bgsval);
		if (bgsval < 0)
			bgsval += 16;
		if (bgsval & 1)
			val |= EPHY_BGS_0;
		if (bgsval & 2)
			val |= EPHY_BGS_1;
		if (bgsval & 4)
			val |= EPHY_BGS_2;
		if (bgsval & 8)
			val |= EPHY_BGS_3;
	}
	writel(val, priv->base + EPHY_CTRL);
}

static int sunplus_config_init(struct phy_device *phydev)
{
	int oldpage;

	oldpage = phy_select_page(phydev, 0x9);
	if (oldpage < 0)
		goto error;

	/* clear counters */
	__phy_write(phydev, 0x10, 0x0c);

	sunplus_write_page(phydev, 0x6);
	/* CP current optimization for PLL */
	__phy_write(phydev, 0x17, 0x0545);

	sunplus_write_page(phydev, 0x1);
	/* disable aps */
	__phy_write(phydev, 0x12, 0x4824);

	sunplus_write_page(phydev, 0x2);
	/* 10Base-T filter selector */
	__phy_write(phydev, 0x18, 0x0000);

	sunplus_write_page(phydev, 0x6);
	/* 4% FE ATE loss test */
	/* PHYAFE TX optimization and invert ADC clock */
	__phy_write(phydev, 0x19, 0x004c);
	/* Tx_level optimization */
	__phy_write(phydev, 0x15, 0x3038);
	/* PD enable control */
	__phy_write(phydev, 0x1c, 0x8880);

	sunplus_write_page(phydev, 0x8);
	/* disable a_TX_LEVEL_Auto calibration */
	__phy_write(phydev, 0x1d, 0x0844);

error:
	return phy_restore_page(phydev, oldpage, 0);
}

static int sunplus_ack_interrupt(struct phy_device *phydev)
{
	int val;

	val = phy_read(phydev, SUNPLUS_INT_STS);
	if (val <= 0)
		return val;

	phy_write(phydev, SUNPLUS_INT_STS, val);

	if (val & LNK_STS_CHG) {
		int oldpage, sts = phy_read(phydev, SUNPLUS_STS);

		oldpage = phy_select_page(phydev, 0x6);
		if (oldpage < 0)
			goto error;

		if (sts && FINAL_LINK) {
			__phy_write(phydev, 0x10, 0x5540);
			__phy_write(phydev, 0x12, 0x8400);
			__phy_write(phydev, 0x14, 0x1088);
		} else {
			__phy_write(phydev, 0x10, 0x5563);
			__phy_write(phydev, 0x12, 0x0400);
			__phy_write(phydev, 0x14, 0x7088);
		}
error:
		return phy_restore_page(phydev, oldpage, 0);
	}

	return 0;
}

static int sunplus_config_intr(struct phy_device *phydev)
{
	u16 val;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		val = LNK_STS_CHG | MGC_PKT_DET;
	else
		val = 0;

	return phy_write(phydev, SUNPLUS_INT_MASK, val);
}

static int sunplus_resume(struct phy_device *phydev)
{
	struct sunplus_ephy *priv = phydev->priv;
	u32 val;
	int ret;

	val = readl(priv->base + EPHY_CTRL);
	if (val & EPHY_SHUTDOWN) {
		sunplus_init(priv);

		ret = sunplus_config_init(phydev);
		if (ret < 0)
			return ret;
	}

	return genphy_resume(phydev);
}

static void sunplus_get_wol(struct phy_device *phydev,
			    struct ethtool_wolinfo *wol)
{
	int val;

	wol->supported = WAKE_MAGIC;
	wol->wolopts = 0;

	val = phy_read(phydev, SUNPLUS_GLOBAL_CONF);
	if (val < 0)
		return;

	if (val & EN_WOL)
		wol->wolopts |= WAKE_MAGIC;
}

static int sunplus_set_wol(struct phy_device *phydev,
			   struct ethtool_wolinfo *wol)
{
	if (wol->wolopts & WAKE_MAGIC) {
		/* Set the mac address for the magic packet */
		phy_write(phydev, SUNPLUS_MAC2,
				((phydev->attached_dev->dev_addr[4] << 8) |
				 phydev->attached_dev->dev_addr[5]));
		phy_write(phydev, SUNPLUS_MAC1,
				((phydev->attached_dev->dev_addr[2] << 8) |
				 phydev->attached_dev->dev_addr[3]));
		phy_write(phydev, SUNPLUS_MAC0,
				((phydev->attached_dev->dev_addr[0] << 8) |
				 phydev->attached_dev->dev_addr[1]));

		phy_modify(phydev, SUNPLUS_GLOBAL_CONF,
				RG_WOL_CHK_PSWD, EN_WOL | RG_WOL_RCV_BC);
	} else {
		phy_modify(phydev, SUNPLUS_GLOBAL_CONF, EN_WOL, 0);
	}

	return 0;
}

static void sunplus_shutdown(struct phy_device *phydev)
{
	struct sunplus_ephy *priv = phydev->priv;
	int phy_val;
	u32 val;

	phy_val = phy_read(phydev, SUNPLUS_GLOBAL_CONF);
	if (phy_val < 0 || (phy_val & EN_WOL))
		return;

	/* disable aps */
	phy_write_paged(phydev, 0x1, 0x12, 0x4824);

	/* PD enable control */
	phy_write_paged(phydev, 0x6, 0x1c, 0x8880);

	phy_write(phydev, 0x00, 0x3900);

	val = readl(priv->base + EPHY_CTRL);
	val |= EPHY_SHUTDOWN;
	writel(val, priv->base + EPHY_CTRL);
}

static int sunplus_probe(struct phy_device *phydev)
{
	struct device_node *np, *dev_np = phydev->mdio.dev.of_node;
	struct platform_device *pdev;
	struct sunplus_ephy *priv;
	int ret = 0;

	np = of_parse_phandle(dev_np, "ephy", 0);
	if (!np)
		return -ENODEV;

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		ret = -EPROBE_DEFER;
		goto err_find_dev;
	}

	priv = platform_get_drvdata(pdev);
	if (!priv) {
		put_device(&pdev->dev);
		ret = -EPROBE_DEFER;
		goto err_find_dev;
	}

	phydev->priv = priv;

err_find_dev:
	of_node_put(np);

	return ret;
}

static void sunplus_remove(struct phy_device *phydev)
{
	struct sunplus_ephy *priv = phydev->priv;

	put_device(priv->dev);
}

static struct phy_driver sunplus_drvs[] = {
	{
		PHY_ID_MATCH_EXACT(0x00441400),
		.name		= "SP Fast Ethernet",
		.config_init	= &sunplus_config_init,
		.ack_interrupt	= sunplus_ack_interrupt,
		.config_intr	= sunplus_config_intr,
		.suspend	= genphy_suspend,
		.resume		= sunplus_resume,
		.get_wol	= sunplus_get_wol,
		.set_wol	= sunplus_set_wol,
		.read_page	= sunplus_read_page,
		.write_page	= sunplus_write_page,
		.probe		= sunplus_probe,
		.remove		= sunplus_remove,
		.shutdown	= sunplus_shutdown,
	}
};

static const struct mdio_device_id __maybe_unused sunplus_tbl[] = {
	{ PHY_ID_MATCH_VENDOR(0x00441400) },
	{ }
};
MODULE_DEVICE_TABLE(mdio, sunplus_tbl);

static int sunplus_ephy_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct sunplus_ephy *priv;
	struct device *dev = &pdev->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->dev = dev;
	platform_set_drvdata(pdev, priv);

	sunplus_init(priv);

	return 0;
}

static const struct of_device_id sunplus_ephy_dt_ids[] = {
	{ .compatible = "sunplus,ephy" },
	{}
};
MODULE_DEVICE_TABLE(of, sunplus_ephy_dt_ids);

static struct platform_driver sunplus_ephy_driver = {
	.driver	= {
		.name	= "sunplus-ephy",
		.of_match_table = sunplus_ephy_dt_ids,
	},
	.probe	= sunplus_ephy_probe,
};

static int __init sunplus_ephy_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&sunplus_ephy_driver);
	if (ret < 0)
		return ret;

	return phy_drivers_register(sunplus_drvs, ARRAY_SIZE(sunplus_drvs),
				    THIS_MODULE);
}
module_init(sunplus_ephy_driver_init);

static void __exit sunplus_ephy_driver_exit(void)
{
	phy_drivers_unregister(sunplus_drvs, ARRAY_SIZE(sunplus_drvs));
	platform_driver_unregister(&sunplus_ephy_driver);
}
module_exit(sunplus_ephy_driver_exit);

MODULE_DESCRIPTION("Sunplus PHY driver");
MODULE_LICENSE("GPL v2");
