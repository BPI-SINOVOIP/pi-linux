// SPDX-License-Identifier: GPL-2.0
/*
 * dwc3-syna.c - synaptics DWC3 Specific Glue layer
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Benson Gui <begu@synaptics.com>
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/reset.h>
#include <linux/phy/phy.h>

#define USB3_PHY_CLK_CTRL		0x001C
#define REF_SSP_EN			BIT(16)

struct dwc3_syna {
	struct device		*dev;
	struct phy		*phy;
	struct reset_control	*reset_phy;
	struct reset_control	*reset_sync;
	struct clk		*core_clk;
	struct regulator	*vbus;
};

static int dwc3_syna_remove_child(struct device *dev, void *unused)
{
	struct platform_device *pdev = to_platform_device(dev);

	platform_device_unregister(pdev);

	return 0;
}

static int dwc3_syna_probe(struct platform_device *pdev)
{
	struct dwc3_syna	*syna;
	struct device		*dev = &pdev->dev;
	struct device_node	*node = dev->of_node;
	int			ret;

	syna = devm_kzalloc(dev, sizeof(*syna), GFP_KERNEL);
	if (!syna)
		return -ENOMEM;

	platform_set_drvdata(pdev, syna);

	syna->dev = dev;
	syna->core_clk = devm_clk_get(dev, "core_clk");
	if (IS_ERR(syna->core_clk))
		syna->core_clk = NULL;

	syna->reset_sync = devm_reset_control_get(dev, "rst-sync");
	if (IS_ERR(syna->reset_sync)) {
		ret = PTR_ERR(syna->reset_sync);
		dev_err(dev, "error getting sync reset: %d\n", ret);
		return ret;
	}

	syna->phy = devm_phy_get(dev, "usb-phy");
	if (IS_ERR(syna->phy)) {
		ret = PTR_ERR(syna->phy);
		dev_err(dev, "error getting phy %d\n", ret);
		return ret;
	}

	syna->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(syna->vbus)) {
		ret = PTR_ERR(syna->vbus);
		return ret;
	}

	ret = clk_prepare_enable(syna->core_clk);
	if (ret)
		return ret;

	phy_init(syna->phy);

	reset_control_reset(syna->reset_sync);
	udelay(1000);
	phy_power_on(syna->phy);

	ret = regulator_enable(syna->vbus);
	if (ret) {
		dev_err(dev, "Failed to enable vbus supply\n");
		goto err_disable_clk;
	}

	if (node) {
		ret = of_platform_populate(node, NULL, NULL, dev);
		if (ret) {
			dev_err(dev, "failed to add dwc3 core\n");
			goto populate_err;
		}
	} else {
		dev_err(dev, "no device node, failed to add dwc3 core\n");
		ret = -ENODEV;
		goto populate_err;
	}
	return 0;

populate_err:
	regulator_disable(syna->vbus);
err_disable_clk:
	clk_disable_unprepare(syna->core_clk);

	return ret;
}

static int dwc3_syna_remove(struct platform_device *pdev)
{
	struct dwc3_syna *syna = platform_get_drvdata(pdev);

	device_for_each_child(&pdev->dev, NULL, dwc3_syna_remove_child);

	clk_disable_unprepare(syna->core_clk);
	regulator_disable(syna->vbus);

	return 0;
}

static void dwc3_syna_shutdown(struct platform_device *pdev)
{
	struct dwc3_syna *syna = platform_get_drvdata(pdev);

	clk_disable_unprepare(syna->core_clk);
	/* to get better vbus timing for reboot */
	regulator_disable(syna->vbus);
}

static const struct of_device_id syna_dwc3_match[] = {
	{ .compatible = "syna,vs680-dwusb3" },
	{},
};
MODULE_DEVICE_TABLE(of, syna_dwc3_match);

#ifdef CONFIG_PM_SLEEP
static int dwc3_syna_suspend(struct device *dev)
{
	struct dwc3_syna *syna = dev_get_drvdata(dev);

	phy_power_off(syna->phy);
	clk_disable(syna->core_clk);
	regulator_disable(syna->vbus);

	return 0;
}

static int dwc3_syna_resume(struct device *dev)
{
	struct dwc3_syna *syna = dev_get_drvdata(dev);
	int ret;

	ret = regulator_enable(syna->vbus);
	if (ret) {
		dev_err(dev, "Failed to enable vbus supply\n");
		return ret;
	}

	clk_enable(syna->core_clk);
	phy_init(syna->phy);
	reset_control_reset(syna->reset_sync);
	udelay(1000);
	phy_power_on(syna->phy);

	/* runtime set active to reflect active state. */
	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}

static const struct dev_pm_ops dwc3_syna_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_syna_suspend, dwc3_syna_resume)
};

#define DEV_PM_OPS	(&dwc3_syna_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver dwc3_syna_driver = {
	.probe		= dwc3_syna_probe,
	.remove		= dwc3_syna_remove,
	.shutdown	= dwc3_syna_shutdown,
	.driver		= {
		.name	= "syna-dwc3",
		.of_match_table = syna_dwc3_match,
		.pm	= DEV_PM_OPS,
	},
};

module_platform_driver(dwc3_syna_driver);

MODULE_AUTHOR("Benson Gui <begu@synaptics.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 synaptics Glue Layer");
