// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of_device.h>

#include "vpp_defines.h"
#include "tz_driver.h"
#include "avio_common.h"

#define AVIO_VPP_CLK          "avio_vclk0"
#define AVIO_VPP_DPICLK       "avio_dpiclk"

static int is_recovery;
static int vpp_initialized;
static struct clk *g_vpp_clk;
static struct clk *g_dpi_vpp_clk;

vpp_config_modes vpp_config_mode;

static int __init recovery_setup(char *__unused)
{
	is_recovery = 1;
	return 1;
}

static int __init vppta_enable_setup(char *__unused)
{
	vpp_config_mode |= VPP_WITH_NORMAL_MODE;
	return 1;
}

int is_vpp_driver_initialized(void)
{
	return vpp_initialized ? 1 : 0;
}
EXPORT_SYMBOL(is_vpp_driver_initialized);

__setup("vppta", vppta_enable_setup);
__setup("recovery", recovery_setup);

int is_recovery_mode(void)
{
	return is_recovery;
}
EXPORT_SYMBOL(is_recovery_mode);

int VPP_Clock_Set_Rate(unsigned int clk_rate)
{
	int res = -EINVAL;

	if (g_vpp_clk != NULL) {
		if (clk_rate == clk_round_rate(g_vpp_clk, clk_rate)) {
			res = clk_set_rate(g_vpp_clk, clk_rate);
			if (res) {
				pr_err("%s:%d: Failed to set clock rate vpp clock\n", __func__, __LINE__);
			}
		} else {
			pr_err("%s:%d: Unsupported clock rate vpp clock\n", __func__, __LINE__);
		}
	} else {
		pr_err("%s:%d: VPP clock not probed\n", __func__, __LINE__);
	}

	return res;
}

int VPP_Clock_Set_Rate_Ext(unsigned int clk_rate)
{
	int res = 0;
	long clk_rrate;

	clk_rrate = clk_round_rate(g_dpi_vpp_clk, clk_rate);
	if (clk_rrate) {
		if (clk_rate == clk_rrate) {
			res = clk_set_rate(g_dpi_vpp_clk, clk_rate);
			if (res)
				pr_err("%s:%d: Failed to set clock rate vpp dpi clock\n", __func__, __LINE__);
		} else
			pr_err("%s:%d: Unsupported clock rate vpp dpi clock\n", __func__, __LINE__);
	}

	return res;
}

static int vpp_clock_config(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	g_vpp_clk = devm_clk_get(dev, AVIO_VPP_CLK);
	if (IS_ERR(g_vpp_clk)) {
		pr_err("VPP: Failed to get VPP clock");
		return PTR_ERR(g_vpp_clk);
	}

	ret = clk_prepare_enable(g_vpp_clk);
	if (ret < 0) {
		pr_err("VPP: Failed to enable VPP clock\n");
		return -EINVAL;
	}

	return 0;
}

static int vpp_clock_config_ext(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;

	g_dpi_vpp_clk = devm_clk_get_optional(dev, AVIO_VPP_DPICLK);
	if (IS_ERR(g_dpi_vpp_clk)) {
		pr_err("VPP: Failed to get VPP DPI clock");
		return PTR_ERR(g_dpi_vpp_clk);
	}

	ret = clk_prepare_enable(g_dpi_vpp_clk);
	if (ret < 0)
		pr_err("VPP: Failed to enable VPP DPI clock\n");

	return ret;
}

static int syna_vpp_platform_probe(struct platform_device *pdev)
{
	int ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	if (is_recovery) {
		ret = vpp_clock_config(pdev);
		if (ret) {
			pr_err("VPP: Clock configuration failed\n");
			return ret;
		}
		ret = vpp_clock_config_ext(pdev);
		if (ret) {
			pr_err("VPP: DPI Clock configuration failed\n");
			return ret;
		}
	}

	vpp_initialized = 1;
	return 0;
}

static int syna_vpp_platform_remove(struct platform_device *pdev)
{
	vpp_initialized = 0;
	if (is_recovery) {
		clk_disable_unprepare(g_vpp_clk);
	}
	return 0;
}

static const struct of_device_id syna_vpp_of_match[] = {
	{ .compatible = "syna,vpp-drv",
	},
	{},
};

static struct platform_driver syna_vpp_platform_driver = {
	.probe = syna_vpp_platform_probe,
	.remove = syna_vpp_platform_remove,
	.driver = {
		.name = "syna-vpp-disp",
		.of_match_table = syna_vpp_of_match,
	},
};

static int VPP_ModuleInit(void)
{
	int ret;
	if (is_recovery) {
		vpp_config_mode |= VPP_WITH_RECOVERY_MODE;
		pr_info("%s:Recovery mode, ta_flag:%d\n",
			__func__, vpp_config_mode);
	} else {
		pr_info("%s:Normal mode, ta_flag:%d\n",
			__func__, vpp_config_mode);
			vpp_config_mode |= VPP_WITH_NORMAL_MODE;
	}

	ret = platform_driver_register(&syna_vpp_platform_driver);

	if (!ret)
		pr_info("vpp : register successful, mode: %s\n", is_recovery ? "RECOVERY" : "NORMAL");

	return ret;
}

static void VPP_ModuleExit(void)
{
	platform_driver_unregister(&syna_vpp_platform_driver);
}

device_initcall(VPP_ModuleInit);
module_exit(VPP_ModuleExit);

