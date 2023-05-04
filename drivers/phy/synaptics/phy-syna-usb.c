/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Benson Gui <begu@synaptics.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

struct phy_syna_usb2_priv {
	void __iomem		*base;
	struct reset_control	*rst;
	enum phy_mode mode;
};

#define AS370_USB_PHY_CTRL0		0x0
#define AS370_USB_PHY_CTRL1		0x4
#define AS370_USB_PHY_RB		0x10
#define  AS370_USB_PHY_RB_CLK_RDY	(1 << 0)

static int phy_as370_usb_power_on(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);
	u32 val, count;

	reset_control_assert(priv->rst);
	udelay(100);
	/* setup USB_PHY_CTRL0 */
	writel(0x0EB35E84, priv->base + AS370_USB_PHY_CTRL0);
	/* setup USB_PHY_CTRL1 */
	writel(0x80E9F004, priv->base + AS370_USB_PHY_CTRL1);
	reset_control_deassert(priv->rst);
	udelay(100);

	count = 10000;
	while (count) {
		val = readl(priv->base + AS370_USB_PHY_RB);
		if (val & AS370_USB_PHY_RB_CLK_RDY)
			break;
		udelay(1);
		count--;
	}

	return count ? 0 : -ETIMEDOUT;
}

static const struct phy_ops phy_as370_usb_ops = {
	.power_on	= phy_as370_usb_power_on,
	.owner		= THIS_MODULE,
};

#define VS680_USB_PHY_CTRL0		0x0
#define VS680_USB_PHY_CTRL1		0x4
#define VS680_USB_PHY_CTRL2		0x8
#define VS680_PHY_CTRL0_DEF		0x533DADF0
#define VS680_PHY_CTRL1_DEF		0x01B10000
#define VS680_PHY_CTRL2_SIDDQ		BIT(1)

static int phy_vs680_usb_power_on(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);

	reset_control_assert(priv->rst);
	/* setup USB_PHY_CTRL0 */
	writel(VS680_PHY_CTRL0_DEF, priv->base + VS680_USB_PHY_CTRL0);
	/* setup USB_PHY_CTRL1 */
	writel(VS680_PHY_CTRL1_DEF, priv->base + VS680_USB_PHY_CTRL1);
	reset_control_deassert(priv->rst);
	udelay(100);

	return 0;
}

static int phy_vs680_usb_power_off(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);
	u32 val;

	/* power down the USB_PHY */
	val = readl(priv->base + VS680_USB_PHY_CTRL2);
	val |= VS680_PHY_CTRL2_SIDDQ;
	writel(val, priv->base + VS680_USB_PHY_CTRL2);

	return 0;
}

static const struct phy_ops phy_vs680_usb_ops = {
	.power_on	= phy_vs680_usb_power_on,
	.power_off	= phy_vs680_usb_power_off,
	.owner		= THIS_MODULE,
};

#define USB3_PHY_CLK_CTRL		0x001C
#define USB3_PHY_TEST_CTRL		0x0020
#define USB3_PHY_CLK_REF_SSP_EN		BIT(16)
#define USB3_PHY_TEST_PD_HSP		BIT(2)
#define USB3_PHY_TEST_PD_SSP		BIT(3)

static int phy_vs680_usb3_power_on(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);
	u32 val;

	reset_control_assert(priv->rst);
	/* enable the clock */
	val = readl(priv->base + USB3_PHY_CLK_CTRL);
	val |= USB3_PHY_CLK_REF_SSP_EN;
	writel(val, priv->base + USB3_PHY_CLK_CTRL);
	udelay(1);

	reset_control_deassert(priv->rst);
	udelay(10);

	return 0;
}

static int phy_vs680_usb3_power_off(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);
	u32 val;

	/* power down the USB_PHY */
	val = readl(priv->base + USB3_PHY_TEST_CTRL);
	val |= USB3_PHY_TEST_PD_SSP | USB3_PHY_TEST_PD_HSP;
	writel(val, priv->base + USB3_PHY_TEST_CTRL);

	return 0;
}

static int phy_vs680_usb3_init(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);

	reset_control_assert(priv->rst);
	udelay(1);

	return 0;
}

static const struct phy_ops phy_vs680_usb3_ops = {
	.init		= phy_vs680_usb3_init,
	.power_on	= phy_vs680_usb3_power_on,
	.power_off	= phy_vs680_usb3_power_off,
	.owner		= THIS_MODULE,
};

static int phy_berlin_dummy_usb_power_on(struct phy *phy)
{
	struct phy_syna_usb2_priv *priv = phy_get_drvdata(phy);

	/* dummy phy, only need to release the reset */
	reset_control_deassert(priv->rst);
	udelay(10);

	return 0;
}

static const struct phy_ops phy_berlin_dummy_usb_ops = {
	.power_on	= phy_berlin_dummy_usb_power_on,
	.owner		= THIS_MODULE,
};

static const struct of_device_id phy_syna_usb2_of_match[] = {
	{
		.compatible = "syna,as370-usb2-phy",
		.data = &phy_as370_usb_ops,
	},
	{
		.compatible = "syna,vs680-usb2-phy",
		.data = &phy_vs680_usb_ops,
	},
	{
		.compatible = "syna,vs680-usb3-phy",
		.data = &phy_vs680_usb3_ops,
	},
	{
		/* for FPGA test */
		.compatible = "syna,berlin-dummy-usb2-phy",
		.data = &phy_berlin_dummy_usb_ops,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, phy_syna_usb2_of_match);

static int phy_syna_usb2_probe(struct platform_device *pdev)
{
	struct phy_syna_usb2_priv *priv;
	struct resource *res;
	struct phy *phy;
	struct phy_provider *phy_provider;
	const struct phy_ops *ops;
	struct device *dev = &pdev->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->rst = devm_reset_control_get(dev, NULL);
	if (IS_ERR(priv->rst)) {
		return PTR_ERR(priv->rst);
	}

	ops = of_device_get_match_data(dev);
	phy = devm_phy_create(dev, NULL, ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(phy);
	}

	priv->mode = PHY_MODE_USB_HOST;
	phy_set_drvdata(phy, priv);

	phy_provider =
		devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver phy_syna_usb2_driver = {
	.probe	= phy_syna_usb2_probe,
	.driver	= {
		.name		= "phy-syna-usb2",
		.of_match_table	= phy_syna_usb2_of_match,
	},
};
module_platform_driver(phy_syna_usb2_driver);

MODULE_AUTHOR("Benson Gui <begu@synaptics.com>");
MODULE_DESCRIPTION("Synaptics AS/VS serial USB2 PHY driver");
MODULE_LICENSE("GPL");
