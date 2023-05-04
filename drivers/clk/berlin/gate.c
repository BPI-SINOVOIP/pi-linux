// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Synaptics Incorporated
 * Copyright (c) 2015 Marvell Technology Group Ltd.
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 */

#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/spinlock_types.h>
#include <linux/platform_device.h>

#include "clk.h"

static DEFINE_SPINLOCK(berlin_gateclk_lock);

int berlin_gateclk_setup(struct platform_device *pdev,
			const struct gateclk_desc *descs,
			struct clk_onecell_data *clk_data,
			int n)
{
	int i, ret;
	void __iomem *base;
	struct clk **clks;
	struct resource *res;

	clks = devm_kcalloc(&pdev->dev, n, sizeof(struct clk *), GFP_KERNEL);
	if (!clks)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (WARN_ON(!base))
		return -ENOMEM;

	for (i = 0; i < n; i++) {
		struct clk *clk;

		clk = clk_register_gate(NULL, descs[i].name,
				descs[i].parent_name,
				descs[i].flags, base,
				descs[i].bit_idx, 0,
				&berlin_gateclk_lock);
		if (WARN_ON(IS_ERR(clk))) {
			ret = PTR_ERR(clk);
			goto err_clk_register;
		}
		clks[i] = clk;
	}

	clk_data->clks = clks;
	clk_data->clk_num = i;

	ret = of_clk_add_provider(pdev->dev.of_node,
				of_clk_src_onecell_get,
				clk_data);
	if (WARN_ON(ret))
		goto err_clk_register;
	return 0;

err_clk_register:
	for (i = 0; i < n; i++)
		clk_unregister(clks[i]);
	return ret;
}
EXPORT_SYMBOL_GPL(berlin_gateclk_setup);
