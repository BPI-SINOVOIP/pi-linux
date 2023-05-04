// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * based on as370.c
 *
 * Author: Benson Gui <begu@synaptics.com>
 *
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/module.h>

#include "clk.h"

static struct clk_onecell_data gateclk_data;
static struct clk_onecell_data clk_data;

static const struct gateclk_desc as470_gates[] = {
	{ "usb0coreclk",	"perifsysclk",	0 },
	{ "sdiosysclk",		"perifsysclk",	1 },
	{ "emmcsysclk",		"perifsysclk",	2 },
	{ "pbridgecoreclk",	"perifsysclk",	3 },
	{ "npuaxiclk",		"mempll_clko",	4 },
};

static int as470_gateclk_setup(struct platform_device *pdev)
{
	int n = ARRAY_SIZE(as470_gates);
	int ret;

	ret = berlin_gateclk_setup(pdev, as470_gates, &gateclk_data, n);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, &gateclk_data);

	return 0;
}

static const struct clk_desc as470_descs[] = {
	{ "cpufastrefclk",		0x0, CLK_IS_CRITICAL },
	{ "memfastrefclk",		0x4 },
	{ "cfgclk",			0x8, CLK_IS_CRITICAL },
	{ "perifsysclk",		0xc, CLK_IS_CRITICAL },
	{ "atbclk",			0x10, CLK_IS_CRITICAL },
	{ "apbcoreclk",			0x14, CLK_IS_CRITICAL },
	{ "emmcclk",			0x18 },
	{ "sd0clk",			0x1c },
	{ "usb2testclk",		0x20 },
	{ "npuclk",			0x28 },
	{ "sysclk",			0x2c, CLK_IS_CRITICAL },
	{ "usb2test480mg0clk",		0x30 },
	{ "usb2test480mg1clk",		0x34 },
	{ "usb2test480mg2clk",		0x38 },
	{ "usb2test100mg0clk",		0x3c },
	{ "usb2test100mg1clk",		0x40 },
	{ "usb2test100mg2clk",		0x44 },
	{ "usb2test100mg3clk",		0x48 },
	{ "aiosysclk",			0x4c },
};

static int as470_clk_setup(struct platform_device *pdev)
{
	int n = ARRAY_SIZE(as470_descs);
	int ret;

	ret = berlin_clk_setup(pdev, as470_descs, &clk_data, n);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, &clk_data);

	return 0;
}

static const struct of_device_id as470_clks_match_table[] = {
	{ .compatible = "syna,as470-clk",
	  .data = as470_clk_setup },
	{ .compatible = "syna,as470-gateclk",
	  .data = as470_gateclk_setup },
	{ }
};
MODULE_DEVICE_TABLE(of, as470_clks_match_table);

static int as470_clks_probe(struct platform_device *pdev)
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

static struct platform_driver as470_clks_driver = {
	.probe		= as470_clks_probe,
	.driver		= {
		.name	= "syna-as470-clks",
		.of_match_table = as470_clks_match_table,
	},
};

static int __init as470_clks_init(void)
{
	return platform_driver_register(&as470_clks_driver);
}
core_initcall(as470_clks_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synaptics as470 clks Driver");
