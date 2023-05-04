// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#include "drv_aio.h"
#include "avio_io.h"
#include "avioGbl.h"
#include "avio_sub_module.h"
#include "avio_core.h"
#include "avio_memmap.h"
#include "aio.h"
#define AIO_MODULE_NAME "avio_module_aio"
#define DEFAULT_APLL0_RATE (24576000 * 8)
#define DEFAULT_APLL1_RATE (22579200 * 8)

#define _AVIO_GBL_REG_(d, a) {.addr = (d), .v = (a)}
static struct reg_item avio_gbl[] = {
	/* zero AVPLL_CTRL0 to have Normal operation,
	 * by default is power down status
	 */
	_AVIO_GBL_REG_(RA_avioGbl_AVPLL_CTRL0, 0x00),
	/* Clock controls for APLL CLK after APLL */
	_AVIO_GBL_REG_(RA_avioGbl_APLL_WRAP0, 0x260),
	_AVIO_GBL_REG_(RA_avioGbl_APLL_WRAP1, 0x260),
	/* AVPLLA_CLK_EN -- Enable channel output */
	_AVIO_GBL_REG_(RA_avioGbl_AVPLLA_CLK_EN, 0x4D),
	/* enable I2S1_MCLK_OEN& PDM_CLK_OEN to output mclk
	 * and PDM clock
	 * enable I2S3_BCLK_OEN & I2S3_LRCLK_OEN for i2s3
	 */
	_AVIO_GBL_REG_(RA_avioGbl_CTRL1, 0x3C00),
};

struct aio_priv *get_aio(void)
{
	return avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AIO);
}

static int aio_apll_config(struct aio_priv *aio)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(avio_gbl); i++)
		writel_relaxed(avio_gbl[i].v,
			aio->gbl_base + avio_gbl[i].addr);
	return 0;
}

static int drv_aio_init_clk(struct aio_priv *aio)
{
	int i, j, ret;
	unsigned long default_rate[AIO_CLK_MAX] = {
		DEFAULT_APLL0_RATE, DEFAULT_APLL1_RATE, 0};
	struct clk *l_clk;

	if (IS_ERR(aio->a_clk[0])) {
		dev_err(aio->dev, "clk handle not initialized\n");
		return -EINVAL;
	}

	for (i = 0; i < AIO_CLK_MAX; i++) {
		l_clk = aio->a_clk[i];
		ret = clk_prepare_enable(l_clk);
		if (ret) {
			avio_trace("clk(%d) enable error\n", i);
			goto error_enable_clk;
		}
		if (default_rate[i])
			clk_set_rate(l_clk, default_rate[i]);
	}

	return 0;

error_enable_clk:
	for (j = i - 1; j >= 0; j--) {
		l_clk = aio->a_clk[j];
		clk_disable_unprepare(l_clk);
	}

	for (i = 0; i < AIO_CLK_MAX; i++) {
		l_clk = aio->a_clk[i];
		clk_put(l_clk);
	}

	return -EINVAL;
}

static int drv_aio_init(void *h_aio_ctx)
{
	struct aio_priv *aio = h_aio_ctx;
	int ret;

	ret = drv_aio_init_clk(aio);
	if (ret) {
		dev_err(aio->dev, "aio clk init failed!\n");
		return -EINVAL;
	}

	if (resource_size(&aio->base_res) == 0) {
		avio_error("aio->base_res invalid\n");
		aio->pbase = NULL;
		return -EINVAL;
	}

	aio->pbase =
		(void __iomem *)avio_memmap_phy_to_vir(aio->base_res.start);
	avio_trace("aio->pbase = %p\n", aio->pbase);

	if (resource_size(&aio->gbl_res) == 0) {
		avio_error("aio->base_res invalid\n");
		aio->gbl_base = NULL;
		return -EINVAL;
	}

	aio->gbl_base =
		(void __iomem *)avio_memmap_phy_to_vir(aio->gbl_res.start);
	avio_trace("aio->gbl_base = %p\n", aio->gbl_base);

	aio_apll_config(aio);
	return 0;
}

static void drv_aio_exit(void *h_aio_ctx)
{
	struct aio_priv *aio = h_aio_ctx;
	struct clk *l_clk;
	int i;

	for (i = 0; i < AIO_CLK_MAX; i++) {
		l_clk = aio->a_clk[i];
		if (IS_ERR(l_clk)) {
			clk_disable_unprepare(l_clk);
			clk_put(l_clk);
		}
	}

	aio->pbase = NULL;
	aio->gbl_base = NULL;
	aio->context = NULL;
}

static int drv_aio_get_clk(struct aio_priv *aio, struct device_node *np)
{
	const char *clk_name[AIO_CLK_MAX] = {
		"audio0_clk", "audio1_clk", "aio_sysclk"};
	int i, j;
	struct clk *l_clk;

	for (i = 0; i < AIO_CLK_MAX; i++) {
		l_clk = of_clk_get_by_name(np, clk_name[i]);
		if (IS_ERR(l_clk)) {
			pr_err("%s, error in getting clk(%d) handle\n",
				   __func__, i);
			goto err_probe_clk;
		}
		aio->a_clk[i] = l_clk;
	}
	return 0;

err_probe_clk:
	for (j = i - 1; j >= 0; j--) {
		clk_put(aio->a_clk[j]);
		aio->a_clk[j] = NULL;
	}

	return -EINVAL;
}

static void avio_module_drv_aio_config(void *h_aio_ctx, void *p)
{
	struct device_node *np, *iter;
	const char *aio_node_name = "syna,berlin-aio";
	struct platform_device *pdev = (struct platform_device *)p;
	int nodeFound = 0;
	struct aio_priv *aio = h_aio_ctx;
	int ret;

	np = pdev->dev.of_node;
	if (np) {
		np = pdev->dev.of_node;
		for_each_child_of_node(np, iter) {
			if (of_device_is_compatible(iter, aio_node_name)) {
				nodeFound = 1;
				break;
			}
		}
	}

	if (!nodeFound) {
		avio_trace("%s:%d: Node not found %s!\n",
					__func__, __LINE__, aio_node_name);
		return;
	}

	np = iter;
	avio_trace("%s:%d: Node found %s!\n",
				__func__, __LINE__, aio_node_name);

	ret = of_address_to_resource(np, 0, &aio->base_res);
	if (ret) {
		pr_err("%s: unable to get resource\n", np->full_name);
		aio->base_res.start = aio->base_res.end = 0;
	}

	ret = of_address_to_resource(np, 1, &aio->gbl_res);
	if (ret) {
		pr_err("%s: unable to get resource\n", np->full_name);
		aio->gbl_res.start = aio->gbl_res.end = 0;
	}

	ret = drv_aio_get_clk(aio, np);
	if (ret < 0)
		dev_err(aio->dev, "Failed to get clk handle");
}

static int avio_module_drv_aio_suspend(void *h_aio_ctx)
{
	struct aio_priv *aio = h_aio_ctx;
	u32 i;
	struct reg_item *p;

	p = aio->context;
	for (i = 0; i < aio->item_cn; i++, p++) {
		p->addr = i * 4;
		p->v = readl_relaxed(aio->pbase + p->addr);
	}

	for (i = 0; i < ARRAY_SIZE(avio_gbl); i++)
		avio_gbl[i].v = readl_relaxed(aio->gbl_base + avio_gbl[i].addr);

	return 0;
}

static int avio_module_drv_aio_resume(void *h_aio_ctx)
{
	struct aio_priv *aio = h_aio_ctx;
	u32 i;
	struct reg_item *p;

	p = aio->context;
	for (i = 0; i < aio->item_cn; i++, p++)
		writel_relaxed(p->v, aio->pbase + p->addr);

	for (i = 0; i < ARRAY_SIZE(avio_gbl); i++)
		writel_relaxed(avio_gbl[i].v,
			aio->gbl_base + avio_gbl[i].addr);

	return 0;
}

static const AVIO_MODULE_FUNC_TABLE aio_drv_fops = {
	.init          = drv_aio_init,
	.exit          = drv_aio_exit,
	.config        = avio_module_drv_aio_config,
	.restore_state = avio_module_drv_aio_resume,
	.save_state    = avio_module_drv_aio_suspend
};

int avio_module_drv_aio_probe(struct platform_device *pdev)
{
	struct aio_priv *aio;
	struct device *dev = &pdev->dev;

	aio = devm_kzalloc(dev, sizeof(struct aio_priv),
				  GFP_KERNEL);
	if (!aio) {
		avio_error("no memory for aio\n");
		return -ENOMEM;
	}

	aio->item_cn = sizeof(SIE_AIO)/sizeof(u32);
	aio->context = devm_kzalloc(dev,
		sizeof(struct reg_item) * aio->item_cn, GFP_KERNEL);

	if (!aio->context) {
		avio_error("no memory for suspend\n");
		return -ENOMEM;
	}

	aio->dev = dev;
	avio_sub_module_register(AVIO_MODULE_TYPE_AIO,
			 AIO_MODULE_NAME, aio, &aio_drv_fops);
	return 0;
}
