// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _DRV_AIO_H_
#define _DRV_AIO_H_
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include "avio_base.h"

enum aio_clk {
	AIO_CLK_0 = 0,
	AIO_CLK_1,
	AIO_CLK_SYS,
	AIO_CLK_MAX,
};

struct reg_item {
	u32 addr;
	u32 v;
};

struct aio_priv {
	struct device *dev;
	void __iomem *pbase;
	struct resource base_res;
	void __iomem *gbl_base;
	struct resource gbl_res;
	struct clk *a_clk[AIO_CLK_MAX];
	struct reg_item *context;
	u32 item_cn;
};

int avio_module_drv_aio_probe(struct platform_device *dev);
#endif
