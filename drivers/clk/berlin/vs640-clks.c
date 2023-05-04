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

static const struct gateclk_desc vs640_gates[] = {
	{ "usb0coreclk",	"perifsysclk",	0 },
	{ "sdiosysclk",		"perifsysclk",	1 },
	{ "pcie0sysclk",	"perifsysclk",	2 },
	{ "emmcsysclk",		"perifsysclk",	3 },
	{ "pbridgecoreclk",	"perifsysclk",	4 },
	{ "npuaxiclk",		"gfx3dsysclk",	5 },
	{ "gethrgmiisysclk",	"perifsysclk",	6 },
	{ "gfxaxiclk",		"perifsysclk",	7 },
	{ "usbotgclk",		"perifsysclk",	8 },
};

static int vs640_gateclk_setup(struct platform_device *pdev)
{
	int n = ARRAY_SIZE(vs640_gates);
	int ret;

	ret = berlin_gateclk_setup(pdev, vs640_gates, &gateclk_data, n);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, &gateclk_data);

	return 0;
}

static const struct clk_desc vs640_descs[] = {
	/* regs from 0x0 ~ 0x3f are protected as RO */
	{ "cpufastrefclk",		0x0, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "memfastrefclk",		0x4, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "cfgclk",			0x20, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "perifsysclk",		0x24, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "atbclk",			0x28, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "decoderclk",			0x2c, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "encoderclk",			0x34, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "ovpcoreclk",			0x38, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "gfx3dcoreclk",		0x40, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "apbcoreclk",			0x80, CLK_IS_CRITICAL, CLK_RATE_NO_CHANGE },
	{ "emmcclk",			0x90 },
	{ "sd0clk",			0x94 },
	{ "periftest125mclk",		0xc0 },
	{ "usb2testclk",		0xd0 },
	{ "periftest250mclk",		0xd4 },
	{ "usb3coreclk",		0xd8 },
	{ "npuclk",			0xdc },
	{ "hdmirxrefclk",		0xe0 },
	{ "usb2test480mg0clk",		0xe4 },
	{ "usb2test480mg1clk",		0xe8 },
	{ "usb2test480mg2clk",		0xec },
	{ "usb2test100mg0clk",		0xf0 },
	{ "usb2test100mg1clk",		0xf4 },
	{ "usb2test100mg2clk",		0xf8 },
	{ "usb2test100mg3clk",		0xfc },
	{ "usb2test100mg4clk",		0x100 },
	{ "periftest200mg0clk",		0x104 },
	{ "periftest200mg1clk",		0x108 },
	{ "periftest500mg0clk",		0x10c },
	{ "aiosysclk",			0x110 },
	{ "usimclk",			0x114 },
	{ "periftest50mg0clk",		0x118 },
};

static int vs640_clk_setup(struct platform_device *pdev)
{
	int n = ARRAY_SIZE(vs640_descs);
	int ret;

	ret = berlin_clk_setup(pdev, vs640_descs, &clk_data, n);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, &clk_data);

	return 0;
}

static const struct of_device_id vs640_clks_match_table[] = {
	{ .compatible = "syna,vs640-clk",
	  .data = vs640_clk_setup },
	{ .compatible = "syna,vs640-gateclk",
	  .data = vs640_gateclk_setup },
	{ }
};
MODULE_DEVICE_TABLE(of, vs640_clks_match_table);

static int vs640_clks_probe(struct platform_device *pdev)
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

static struct platform_driver syna_clks_driver = {
	.probe		= vs640_clks_probe,
	.driver		= {
		.name	= "syna-vs640-clks",
		.of_match_table = vs640_clks_match_table,
	},
};

static int __init vs640_clks_init(void)
{
	return platform_driver_register(&syna_clks_driver);
}
core_initcall(vs640_clks_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synaptics vs640 clks Driver");
