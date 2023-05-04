// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "drv_vpp_cfg.h"
#include "avioDhub.h"
#include "drv_hdmitx.h"

#define MAX_VPP_CLOCKS 12
static VPP_CLOCK vpp_clocks[MAX_VPP_CLOCKS] = {
	{.clk_id = VPP_CLK_ID_AVIOSYSCLK,	.clk_name = "avio_sysclk"},
	{.clk_id = VPP_CLK_ID_VPPSYSCLK,	.clk_name = "avio_vppsysclk"},
	{.clk_id = VPP_CLK_ID_BIUCLK,	.clk_name = "avio_biuclk"},
	{.clk_id = VPP_CLK_ID_VPPPIPECLK,	.clk_name = "avio_vippipeclk"},
	{.clk_id = VPP_CLK_ID_OVPCLK,	.clk_name = "avio_ovpclk"},
	{.clk_id = VPP_CLK_ID_FPLL400CLK,	.clk_name = "avio_fpll400clk"},
	{.clk_id = VPP_CLK_ID_TXESCCLK,	.clk_name = "avio_txescclk"},
	{.clk_id = VPP_CLK_ID_VCLK0,	.clk_name = "avio_vclk0"},
	{.clk_id = VPP_CLK_ID_DPICLK,	.clk_name = "avio_dpiclk"},
	{.clk_id = VPP_CLK_ID_AIOSYSCLK,	.clk_name = "avio_aiosysclk"},
	{.clk_id = VPP_CLK_ID_ESMHPICLK,        .clk_name = "avio_hpiclk"},
	{.clk_id = VPP_CLK_ID_ESMCLK,   .clk_name = "avio_hdmirxrefclk"},
};

static void drv_vpp_read_clk_cfg(VPP_CTX *hVppCtx, struct device_node *np)
{
	int i;

	for (i = 0; i < MAX_VPP_CLOCKS; i++) {
		drv_vpp_add_vpp_clock(hVppCtx, np,
			vpp_clocks[i].clk_id, vpp_clocks[i].clk_name);
	}
}

static void drv_vpp_get_hdmi_5V(VPP_CTX *hVppCtx, struct device_node *np)
{
	struct device *dev;
	dev = hVppCtx->dev;

	hVppCtx->gpio_hdmitx_5v = devm_fwnode_get_index_gpiod_from_child(dev, "hdtx5v", 0,
						 &np->fwnode, GPIOD_OUT_HIGH, "hdmitx5v");
	if (IS_ERR_OR_NULL(hVppCtx->gpio_hdmitx_5v)) {
		avio_trace("could not get hdmitx 5v gpio, err : %ld\n",
				PTR_ERR(hVppCtx->gpio_hdmitx_5v));
	}
}

static void drv_vpp_get_mipi_rst(VPP_CTX *hVppCtx, struct device_node *np)
{
	hVppCtx->gpio_mipirst = devm_fwnode_get_index_gpiod_from_child(hVppCtx->dev, "mipirst", 0,
										&np->fwnode, GPIOD_OUT_HIGH, "mipi");

	if (IS_ERR_OR_NULL(hVppCtx->gpio_mipirst)) {
		avio_trace("%s:%d Gpio desc Fail ERR[%ld]\n", __func__,
					__LINE__, PTR_ERR(hVppCtx->gpio_mipirst));
	}
}

void drv_vpp_read_vpp_cfg(VPP_CTX *hVppCtx, void *dev)
{
	struct device_node *np, *iter;
	const char *vpp_node_name = "syna,berlin-vpp";
	struct platform_device *pdev = (struct platform_device *)dev;
	int nodeFound = 0;

	np = pdev->dev.of_node;
	if (np) {
		for_each_child_of_node(np, iter) {
			if (of_device_is_compatible(iter, vpp_node_name)) {
				nodeFound = 1;
				break;
			}
		}
	}

	if (nodeFound) {
		np = iter;
		//enumerate vpp clocks
		drv_vpp_read_clk_cfg(hVppCtx, np);
		drv_vpp_get_hdmi_5V(hVppCtx, np);

		/* Configure Mipi*/
		drv_vpp_get_mipi_rst(hVppCtx, np);
	}

	avio_trace("%s:%d - start\n", __func__, __LINE__);

	//VOP1/DP1 - VBI, VDE
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr0, VPP_INTR_TYPE_VBI);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr6, VPP_INTR_TYPE_VDE);

	//VOP2/DP2 - VOP_TG, EoF
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr13, VPP_INTR_TYPE_NONE);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr14, VPP_INTR_TYPE_CPCB1_VBI);

	//OVP - uv,y
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr11, VPP_INTR_TYPE_OVP);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr12, VPP_INTR_TYPE_OVP);

	//HDMI_TX - Tx, hdcp22, trng, phy, eARC
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr2, VPP_INTR_TYPE_HDMI_TX);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr3, VPP_INTR_TYPE_HDMI_TX);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr4, VPP_INTR_TYPE_HDMI_TX);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr5, VPP_INTR_TYPE_HDMI_TX);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr15, VPP_INTR_TYPE_HDMI_TX);

	//HDMI_RX - EoF, Rx
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr9, VPP_INTR_TYPE_HDMI_RX);
	drv_vpp_add_vpp_interrupt_num(hVppCtx, avioDhubSemMap_vpp128b_vpp_inr10, VPP_INTR_TYPE_HDMI_RX);

	drv_hdmitx_init(hVppCtx);
}
