// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/module.h>

#include "clk.h"

static struct clk_onecell_data gateclk_data;
static struct clk_onecell_data clk_data;

static const struct gateclk_desc as370_gates[] = {
	{ "tspsysclk",		"perifsysclk",	0, CLK_IGNORE_UNUSED },
	{ "usb0coreclk",	"perifsysclk",	1 },
	{ "sdiosysclk",		"perifsysclk",	2 },
	{ "pcie0sys",		"perifsysclk",	3 },
	{ "pcie1sys",		"perifsysclk",	4 },
	{ "nfcsysclk",		"perifsysclk",	5 },
	{ "emmcsysclk",		"perifsysclk",	6 },
	{ "pbridgecoreclk",	"perifsysclk",	7 },
};

static int as370_gateclk_setup(struct platform_device *pdev)
{
	int n = ARRAY_SIZE(as370_gates);
	int ret;

	ret = berlin_gateclk_setup(pdev, as370_gates, &gateclk_data, n);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, &gateclk_data);
	return 0;
}

static const struct clk_desc as370_descs[] = {
	{ "cpufastrefclk",		0x0, CLK_IS_CRITICAL },
	{ "memfastrefclk",		0x4 },
	{ "cfgclk",			0x8, CLK_IS_CRITICAL },
	{ "perifsysclk",		0xc, CLK_IS_CRITICAL },
	{ "atbclk",			0x10 },
	{ "aviosysclk",			0x14, CLK_IS_CRITICAL },
	{ "apbcoreclk",			0x18, CLK_IS_CRITICAL },
	{ "nnasysclk",			0x1c },
	{ "nnacoreclk",			0x20 },
	{ "emmcclk",			0x24 },
	{ "sd0clk",			0x28 },
	{ "pcie_500m_txtestclk",	0x2c },
	{ "pcie_250m_pipetestClk1",	0x30 },
	{ "pcie_250m_pipetestClk2",	0x34 },
	{ "pcie_500m_rxtestclk",	0x38 },
	{ "pcie_serdestestclk",		0x3c },
	{ "nfceccclk",			0x40 },
	{ "nfccoreclk",			0x44 },
	{ "usbOtg60mtestclk",		0x48 },
	{ "usbOtg50mtestclk",		0x4c },
	{ "usbOtg12mtestclk",		0x50 },
	{ "usbOtg480mtestclk",		0x54 },
	{ "bcmclk",			0x58, CLK_IS_CRITICAL },
};

static int as370_clk_setup(struct platform_device *pdev)
{
	int n = ARRAY_SIZE(as370_descs);
	int ret;

	ret = berlin_clk_setup(pdev, as370_descs, &clk_data, n);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, &clk_data);
	return 0;
}

static const struct of_device_id as370_clks_match_table[] = {
	{ .compatible = "syna,as370-clk",
	  .data = as370_clk_setup },
	{ .compatible = "syna,as370-gateclk",
	  .data = as370_gateclk_setup },
	{ }
};
MODULE_DEVICE_TABLE(of, as370_clks_match_table);

static int as370_clks_probe(struct platform_device *pdev)
{
	int (*clk_setup)(struct platform_device *pdev);
	int ret;

	clk_setup = of_device_get_match_data(&pdev->dev);
	if (!clk_setup)
		return -ENODEV;

	ret = clk_setup(pdev);
	if (ret)
		return ret;

	return 0;
}

static struct platform_driver as370_clks_driver = {
	.probe		= as370_clks_probe,
	.driver		= {
		.name	= "syna-as370-clks",
		.of_match_table = as370_clks_match_table,
	},
};

static int __init as370_clks_init(void)
{
	return platform_driver_register(&as370_clks_driver);
}
core_initcall(as370_clks_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synaptics as370 clks Driver");
