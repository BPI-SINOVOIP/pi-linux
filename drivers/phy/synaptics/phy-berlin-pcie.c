// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

struct phy_berlin_pcie_priv {
	void __iomem *base;
	void __iomem *pad_base;
	struct reset_control *rst;
};

static int phy_berlin_pcie_power_on(struct phy *phy)
{
	u32 val;
	int count;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	/* Turn On PCIE REF CLOCK BUFFER */
	writel(0x4560, priv->pad_base);
	msleep(1);
	writel(0x4568, priv->pad_base);

	writel(0x21, priv->base + 0x704);
	writel(0x40, priv->base + 0x70c);
	msleep(1);
	writel(0x400, priv->base + 0x094);
	writel(0x911, priv->base + 0x098);
	writel(0xfc62, priv->base + 0x004);
	writel(0x2a52, priv->base + 0x03c);
	writel(0x1060, priv->base + 0x120);
	writel(0xa0dd, priv->base + 0x13c);
	writel(0x107, priv->base + 0x740);
	writel(0x6430, priv->base + 0x008);
	writel(0x20, priv->base + 0x704);

	count = 1000;
	while (count--) {
		val = readl(priv->base + 0x60C);
		if (val & 1)
			break;
		usleep_range(100, 1000);
	}
	if (!count) {
		dev_err(&phy->dev, "PCIe comphy init Fail: %x\n", val);
		return -ETIMEDOUT;
	}

	return 0;
}

static int phy_berlin4cdp_pcie_power_on(struct phy *phy)
{
	u32 val;
	int count;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	/* Turn On PCIE REF CLOCK BUFFER */
	writel(0x4560, priv->pad_base);
	msleep(1);
	writel(0x4568, priv->pad_base);

	writel(0x21, priv->base + 0x704);
	writel(0x40, priv->base + 0x70c);
	msleep(1);
	writel(0x400, priv->base + 0x094);
	writel(0x911, priv->base + 0x098);
	writel(0xfc62, priv->base + 0x004);
	writel(0x2a52, priv->base + 0x03c);
	writel(0x1060, priv->base + 0x120);
	writel(0x02c0, priv->base + 0x13c);
	writel(0x107, priv->base + 0x740);
	writel(0x6430, priv->base + 0x008);
	val = readl(priv->base + 0x448);
	val &= 0xffffff00;
	val |= 0x2f;
	writel(val, priv->base + 0x448);
	writel(0x20, priv->base + 0x704);

	count = 1000;
	while (count--) {
		val = readl(priv->base + 0x60C);
		if (val & 1)
			break;
		usleep_range(100, 1000);
	}
	if (!count) {
		dev_err(&phy->dev, "PCIe comphy init Fail: %x\n", val);
		return -ETIMEDOUT;
	}

	return 0;
}

#define GET_BIT_MASK(N_BIT)	((1<<N_BIT) - 1)
#define SET_BIT(VARIABLE, VALUE, BIT_POS, N_BIT)	(VARIABLE = (VARIABLE & (~(GET_BIT_MASK(N_BIT) << BIT_POS))) | ((VALUE & GET_BIT_MASK(N_BIT)) << BIT_POS))
static int phy_as370_pcie_init(struct phy *phy)
{
	u32 val;
	void __iomem *addr;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	addr = priv->base + 0x164;
	val = readl(addr);
	SET_BIT(val, 0, 23, 1);
	writel(val, addr);
	readl(addr);

	addr = priv->base + 0x10000 + 30 * 4;
	val = readl(addr);

	addr = priv->base + 0x158;
	val = readl(addr);
	SET_BIT(val, 0xC8, 0, 9);
	writel(val, addr);
	readl(addr);

	addr = priv->base + 0x10000 + 9 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 3, 1);
	writel(val, addr);
	readl(addr);

	addr = priv->base + 0x10000 + 2 * 4;
	val = readl(addr);
	SET_BIT(val, 0x0, 14, 2);
	writel(val, addr);
	readl(addr);

	addr = priv->base + 0x10000 + 9 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 4, 3);
	writel(val, addr);
	readl(addr);

	return 0;
}

static int phy_as370_pcie_power_on(struct phy *phy)
{
	u32 val;
	int count;
	void __iomem *addr;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	addr = priv->base + 0x10000 + 0;
	val = readl(addr);
	SET_BIT(val, 0x1, 1, 2);
	SET_BIT(val, 0x1, 11, 2);
	writel(val, addr);

	addr = priv->base + 0x10000 + 1 * 4;
	val = readl(addr);
	SET_BIT(val, 0x3, 5, 5);
	SET_BIT(val, 0x1, 2, 1);
	writel(val, addr);

	addr = priv->base + 0x10000 + 2 * 4;
	val = readl(addr);
	SET_BIT(val, 0x4, 1, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 5 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 14, 2);
	writel(val, addr);

	addr = priv->base + 0x10000 + 6 * 4;
	val = readl(addr);
	SET_BIT(val, 0xC, 4, 5);
	writel(val, addr);

	addr = priv->base + 0x10000 + 8 * 4;
	val = readl(addr);
	SET_BIT(val, 0x5, 5, 3);
	writel(val, addr);

	addr = priv->base + 0x10000 + 10 * 4;
	val = readl(addr);
	SET_BIT(val, 0x0, 10, 1);
	writel(val, addr);

	addr = priv->base + 0x10000 + 12 * 4;
	val = readl(addr);
	SET_BIT(val, 0xF, 8, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 32 * 4;
	val = readl(addr);
	SET_BIT(val, 3, 9, 2);
	writel(val, addr);

	addr = priv->base + 0x10000 + 9 * 4;
	val = readl(addr);
	SET_BIT(val, 0, 1, 1);
	SET_BIT(val, 0, 2, 1);
	writel(val, addr);

	addr = priv->base + 0x10000 + 10 * 4;
	val = readl(addr);
	SET_BIT(val, 1, 0, 1);
	writel(val, addr);

	addr = priv->base + 0x10000 + 13 * 4;
	val = readl(addr);
	SET_BIT(val, 2, 4, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 14 * 4;
	val = readl(addr);
	SET_BIT(val, 3, 0, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 12 * 4;
	val = readl(addr);
	SET_BIT(val, 2, 4, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 6 * 4;
	val = readl(addr);
	SET_BIT(val, 0, 9, 2);
	writel(val, addr);

	addr = priv->base + 0x10000 + 32 * 4;
	val = readl(addr);
	SET_BIT(val, 0, 7, 2);
	writel(val, addr);

	addr = priv->base + 0x10800 + 147 * 4;
	val = readl(addr);
	SET_BIT(val, 7, 10, 3);
	SET_BIT(val, 4, 13, 3);
	writel(val, addr);

	addr = priv->base + 0x10000 + 12 * 4;
	val = readl(addr);
	SET_BIT(val, 0x5, 8, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 31 * 4;
	val = readl(addr);
	SET_BIT(val, 0xf, 0, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 37 * 4;
	val = readl(addr);
	SET_BIT(val, 0xf, 12, 4);
	SET_BIT(val, 0xf, 0, 4);
	writel(val, addr);

	addr = priv->base + 0x10800 + 131 * 4;
	val = readl(addr);
	SET_BIT(val, 0xf, 0, 16);
	writel(val, addr);

	addr = priv->base + 0x10000 + 34 * 4;
	val = readl(addr);
	SET_BIT(val, 0xf, 0, 4);
	SET_BIT(val, 0x4, 4, 4);
	SET_BIT(val, 0xb, 8, 4);
	SET_BIT(val, 0x0, 12, 4);
	writel(val, addr);

	addr = priv->base + 0x10800 + 0 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1C, 11, 5);
	SET_BIT(val, 0x1C, 6, 5);
	SET_BIT(val, 0x1C, 1, 5);
	writel(val, addr);

	addr = priv->base + 0x10800 + 1 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1C, 7, 5);
	writel(val, addr);

	addr = priv->base + 0x10800 + 1 * 4;
	val = readl(addr);
	SET_BIT(val, 0x7, 12, 4);
	writel(val, addr);

	addr = priv->base + 0x10800 + 1 * 4;
	val = readl(addr);
	SET_BIT(val, 0x3, 3, 2);
	SET_BIT(val, 0x0, 5, 2);
	SET_BIT(val, 0x0, 1, 2);
	writel(val, addr);

	addr = priv->base + 0x10800 + 129 * 4;
	val = readl(addr);
	SET_BIT(val, 0x3, 14, 2);
	writel(val, addr);

	addr = priv->base + 0x10800 + 22 * 4;
	val = readl(addr);
	SET_BIT(val, 0x7, 0, 16);
	writel(val, addr);

	addr = priv->base + 0x10800 + 91 * 4;
	readl(addr);
	addr = priv->base + 0x10800 + 92 * 4;
	readl(addr);
	addr = priv->base + 0x10800 + 93 * 4;
	readl(addr);

	addr = priv->base + 0x10800 + 88 * 4;
	writel(0x94a4, addr);
	addr = priv->base + 0x10800 + 89 * 4;
	writel(0x94a4, addr);
	addr = priv->base + 0x10800 + 90 * 4;
	writel(0x94a4, addr);

	addr = priv->base + 0x10800 + 53 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 3, 1);
	writel(val, addr);

	addr = priv->base + 0x10800 + 7 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 11, 1);
	writel(val, addr);

	addr = priv->base + 0x10800 + 7 * 4;
	val = readl(addr);
	SET_BIT(val, 0x2, 4, 4);
	writel(val, addr);

	addr = priv->base + 0x10800 + 119 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 3, 1);
	writel(val, addr);

	addr = priv->base + 0x10800 + 114 * 4;
	val = readl(addr);
	SET_BIT(val, 0xC, 9, 7);
	writel(val, addr);

	addr = priv->base + 0x10800 + 128 * 4;
	val = readl(addr);
	SET_BIT(val, 0x3, 10, 2);
	writel(val, addr);

	addr = priv->base + 0x154;
	writel(0x472f0028, addr);

	addr = priv->base + 0x10800 + 53 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 1, 1);
	writel(val, addr);

	addr = priv->base + 0x10000 + 5 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 8, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 7 * 4;
	val = readl(addr);
	SET_BIT(val, 0xF, 5, 5);
	SET_BIT(val, 0xF, 0, 5);
	writel(val, addr);

	addr = priv->base + 0x10000 + 1 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 5, 5);
	writel(val, addr);

	addr = priv->base + 0x10000 + 2 * 4;
	val = readl(addr);
	SET_BIT(val, 0x8, 1, 4);
	writel(val, addr);

	addr = priv->base + 0x10000 + 5 * 4;
	val = readl(addr);
	SET_BIT(val, 0x3, 14, 2);
	writel(val, addr);

	addr = priv->base + 0x10800 + 0 * 4;
	val = readl(addr);
	SET_BIT(val, 0, 11, 5);
	writel(val, addr);

	addr = priv->base + 0x10800 + 1 * 4;
	val = readl(addr);
	SET_BIT(val, 3, 7, 5);
	writel(val, addr);

	addr = priv->base + 0x10800 + 12 * 4;
	val = readl(addr);
	SET_BIT(val, 3, 7, 2);
	writel(val, addr);

	addr = priv->base + 0x10800 + 128 * 4;
	val = readl(addr);
	SET_BIT(val, 5, 8, 4);
	SET_BIT(val, 0, 0, 2);
	writel(val, addr);

	addr = priv->base + 0x10800 + 93 * 4;
	val = readl(addr);
	SET_BIT(val, 7, 1, 5);
	writel(val, addr);

	addr = priv->base + 0x10800 + 114 * 4;
	val = readl(addr);
	SET_BIT(val, 5, 9, 7);
	writel(val, addr);

	addr = priv->base + 0x10800 + 53 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1BA2, 0, 16);
	writel(val, addr);

	addr = priv->base + 0x10000 + 32 * 4;
	val = readl(addr);
	SET_BIT(val, 0x3, 7, 2);
	writel(val, addr);

	addr = priv->base + 0x10000 + 12 * 4;
	val = readl(addr);
	SET_BIT(val, 0x1, 10, 1);
	writel(val, addr);

	addr = priv->base + 0x10000 + 6 * 4;
	val = readl(addr);
	SET_BIT(val, 0x0, 9, 2);
	SET_BIT(val, 0x1, 7, 1);
	writel(val, addr);

	/*
	 * improve device detection
	 * RXTX_REG2[3:2] tx_rcvdet_sel
	 * 0=300mV
	 * 1=350mV
	 * 2=400mV
	 * 3=450mV
	 */
	addr = priv->base + 0x10800 + 2 * 4;
	val = readl(addr);
	SET_BIT(val, 1, 2, 2);
	writel(val, addr);

	reset_control_deassert(priv->rst);

	addr = priv->base + 0x10000 + 7 * 4;
	count = 1000000;
	while (--count) {
		val = readl(addr);
		if ((val & (1 << 14)) && (val & (1 << 15)))
			break;
		udelay(1);
	}
	if (val & (1 << 14)) {
		dev_info(&phy->dev, "PLL calib pass\n");
	} else {
		dev_err(&phy->dev, "PLL calib fail:%d %x\n", count, val);
		return -ETIMEDOUT;
	}
	if (val & (1 << 15)) {
		dev_info(&phy->dev, "PLL lock pass\n");
	} else {
		dev_err(&phy->dev, "PLL lock fail\n");
		return -ETIMEDOUT;
	}

	addr = priv->base + 0x10000 + 15 * 4;
	count = 100000;
	while (--count) {
		val = readl(addr);
		if (val & (1 << 8))
			break;
		udelay(1);
	}
	if (val & (1 << 8)) {
		dev_info(&phy->dev, "Tx_ready asserted pass\n");
	} else {
		dev_err(&phy->dev, "Tx_ready asserted fail\n");
		return -ETIMEDOUT;
	}

	count = 100000;
	while (--count) {
		val = readl(addr);
		if (val & (1 << 0))
			break;
		udelay(1);
	}
	if (val & (1 << 0)) {
		dev_info(&phy->dev, "Rx_ready asserted pass\n");
	} else {
		dev_err(&phy->dev, "Rx_ready asserted fail\n");
		return -ETIMEDOUT;
	}

	return 0;
}

/* this is not real reset but a workaround to PHY RX frozen issue */
static int phy_as370_pcie_reset(struct phy *phy)
{
	u32 val;
	void __iomem *addr;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	addr = priv->base + 0x10800 + 6 * 4;
	val = readl(addr);
	SET_BIT(val, 0, 8, 1);
	writel(val, addr);
	udelay(50);
	val = readl(addr);
	SET_BIT(val, 1, 8, 1);
	writel(val, addr);

	return 0;
}

static int phy_vs680_pcie_init(struct phy *phy)
{
	u32 val;
	int count;
	void __iomem *addr;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	reset_control_deassert(priv->rst);

	addr = priv->base + 0xc;
	val = readl(addr);
	SET_BIT(val, 0, 0, 1);
	writel(val, addr);

	/* reset */
	addr = priv->base + 0x4;
	val = readl(addr);
	SET_BIT(val, 1, 0, 1);
	writel(val, addr);
	udelay(1);

	/* config */
	addr = priv->base + 0x0;
	val = readl(addr);
	SET_BIT(val, 1, 3, 1);
	writel(val, addr);
	addr = priv->base + 0x4;
	val = readl(addr);
	SET_BIT(val, 1, 5, 1);
	SET_BIT(val, 0, 2, 3);
	writel(val, addr);
	udelay(1);

	/* release reset */
	val = readl(addr);
	SET_BIT(val, 0, 0, 1);
	writel(val, addr);
	udelay(10);

	count = 100000;
	addr = priv->base + 0x8;
	while (--count) {
		val = readl(addr);
		if (val & (1 << 0))
			break;
		udelay(5);
	}

	if (!count) {
		dev_info(&phy->dev, "PCIe PLL is not locked.\n");
		return -ETIMEDOUT;
	} else {
		dev_info(&phy->dev, "PCIe PLL is locked.\n");
		return 0;
	};
}

int phy_vs680_pcie_power_off(struct phy *phy)
{
	u32 val;
	void __iomem *addr;
	struct phy_berlin_pcie_priv *priv = phy_get_drvdata(phy);

	addr = priv->base + 0x0;
	val = readl(addr);
	SET_BIT(val, 0, 3, 1);
	writel(val, addr);

	addr = priv->base + 0x4;
	val = readl(addr);
	SET_BIT(val, 1, 0, 1);
	writel(val, addr);

	addr = priv->base + 0xc;
	val = readl(addr);
	SET_BIT(val, 1, 0, 1);
	writel(val, addr);

	return 0;
}

static const struct phy_ops phy_berlin_pcie_ops = {
	.power_on	= phy_berlin_pcie_power_on,
	.owner		= THIS_MODULE,
};

static const struct phy_ops phy_berlin4cdp_pcie_ops = {
	.power_on	= phy_berlin4cdp_pcie_power_on,
	.owner		= THIS_MODULE,
};

static const struct phy_ops phy_as370_pcie_ops = {
	.init		= phy_as370_pcie_init,
	.power_on	= phy_as370_pcie_power_on,
	.reset		= phy_as370_pcie_reset,
	.owner		= THIS_MODULE,
};

static const struct phy_ops phy_vs680_pcie_ops = {
	.init		= phy_vs680_pcie_init,
	.power_off	= phy_vs680_pcie_power_off,
	.owner		= THIS_MODULE,
};

static const struct of_device_id phy_berlin_pcie_of_match[] = {
	{
		.compatible = "marvell,berlin-pcie-phy",
		.data = &phy_berlin_pcie_ops,
	},
	{
		.compatible = "marvell,berlin4cdp-pcie-phy",
		.data = &phy_berlin4cdp_pcie_ops,
	},
	{
		.compatible = "syna,as370-pcie-phy",
		.data = &phy_as370_pcie_ops,
	},
	{
		.compatible = "syna,vs680-pcie-phy",
		.data = &phy_vs680_pcie_ops,
	},
	{},
};
MODULE_DEVICE_TABLE(of, phy_berlin_pcie_of_match);

static int phy_berlin_pcie_probe(struct platform_device *pdev)
{
	struct phy_berlin_pcie_priv *priv;
	struct phy_provider *phy_provider;
	struct resource *res;
	struct phy *phy;
	const struct phy_ops *ops;
	struct device *dev = &pdev->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->rst = devm_reset_control_get_optional(dev, NULL);
	if (IS_ERR(priv->rst))
		return PTR_ERR(priv->rst);

	reset_control_assert(priv->rst);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		priv->pad_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(priv->pad_base))
			return PTR_ERR(priv->pad_base);
	}

	ops = of_device_get_match_data(dev);
	phy = devm_phy_create(dev, NULL, ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(phy);
	}

	phy_set_drvdata(phy, priv);

	phy_provider =
		devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver phy_berlin_pcie_driver = {
	.probe	= phy_berlin_pcie_probe,
	.driver	= {
		.name		= "phy-berlin-pcie",
		.of_match_table	= phy_berlin_pcie_of_match,
	},
};
module_platform_driver(phy_berlin_pcie_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics Berlin PCIe PHY driver");
MODULE_LICENSE("GPL v2");
