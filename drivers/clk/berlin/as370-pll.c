// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_device.h>

#include "clk.h"

#define CTRL		0x0
#define  PD		BIT(0)
#define  RESETN		BIT(1)
#define  DM_SHIFT	2
#define  DM_MASK	(0x3f << 2)
#define  DN_SHIFT	8
#define  DN_MASK	(0x7ff << 8)
#define  MODE_SHIFT	19
#define  MODE_MASK	(0x3 << 19)
#define  MODE_INT	0x0
#define  MODE_FRAC	0x1
#define  MODE_SSC	0x2
#define CTRL1		0x4
#define CTRL2		0x8
#define CTRL3		0xC
#define  PDDP		BIT(24)
#define  DP_SHIFT	25
#define  DP_MASK	(0x7 << 25)
#define  PDDP1		BIT(28)
#define  DP1_SHIFT	29
#define  DP1_MASK	(0x7 << 29)
#define CTRL4		0x10
#define  BYPASS		BIT(0)
#define STATUS		0x14
#define  PLL_LOCK	BIT(0)

#define FREQ_FACTOR		(1000)
#define VCO_HIGH_LIMIT		((3200 * 1000 * 1000UL) / FREQ_FACTOR)
#define VCO_LOW_LIMIT		((800 * 1000 * 1000UL) / FREQ_FACTOR)
#define MIN_FREQM_INT		(4 * 1000 * 1000)
#define MIN_FREQM_FRAC		(10 * 1000 * 1000)
#define MAX_DP			((1 << 3) - 1)
#define MAX_DM			((1 << 6) - 1)
#define MAX_DN			((1 << 11) - 1)
#define FRAC_BITS		24
#define FRAC_MASK		((1 << FRAC_BITS) - 1)

struct as370_pll {
	void __iomem *ctrl;
	void __iomem *bypass;
	u8 bypass_shift;
	bool bypass_invert;
	struct clk *clks[2];
	struct clk_hw hw;
	struct clk_hw hw1;
	struct clk_onecell_data onecell_data;
};

#define hw_to_as370_pll(hw) container_of(hw, struct as370_pll, hw)
#define hw1_to_as370_pll(hw) container_of(hw, struct as370_pll, hw1)

static inline u32 rdl(struct as370_pll *pll, u32 offset)
{
	return readl_relaxed(pll->ctrl + offset);
}

static inline void wrl(struct as370_pll *pll, u32 offset, u32 data)
{
	writel_relaxed(data, pll->ctrl + offset);
}

static unsigned long
as370_pll_clko_recalc_rate(struct clk_hw *hw,
			   unsigned long parent_rate)
{
	u32 val, dp, dn, dm, mode;
	unsigned long rate;
	struct as370_pll *pll = hw_to_as370_pll(hw);

	val = rdl(pll, CTRL4);
	if (val & BYPASS)
		return parent_rate;

	val = rdl(pll, CTRL3);
	if (val & PDDP)
		return 0;
	dp = (val & DP_MASK) >> DP_SHIFT;

	val = rdl(pll, CTRL);
	mode = (val & MODE_MASK) >> MODE_SHIFT;
	dn = (val & DN_MASK) >> DN_SHIFT;
	dm = (val & DM_MASK) >> DM_SHIFT;
	rate = parent_rate * dn / dm / dp;
	if (mode == MODE_INT) {
		return rate;
	} else if (mode == MODE_FRAC || mode == MODE_SSC) {
		val = rdl(pll, CTRL1);
		return rate + parent_rate * val / dm / dp / (1 << 24);
	} else {
		return 0;
	}
}

static unsigned long
as370_pll_clko1_recalc_rate(struct clk_hw *hw,
			    unsigned long parent_rate)
{
	u32 val, dp, dn, dm, mode;
	unsigned long rate;
	struct as370_pll *pll = hw1_to_as370_pll(hw);

	val = rdl(pll, CTRL4);
	if (val & BYPASS)
		return parent_rate;

	val = rdl(pll, CTRL3);
	if (val & PDDP1)
		return 0;
	dp = (val & DP1_MASK) >> DP1_SHIFT;

	val = rdl(pll, CTRL);
	mode = (val & MODE_MASK) >> MODE_SHIFT;
	dn = (val & DN_MASK) >> DN_SHIFT;
	dm = (val & DM_MASK) >> DM_SHIFT;
	rate = parent_rate * dn / dm / dp;
	if (mode == MODE_INT) {
		return rate;
	} else if (mode == MODE_FRAC || mode == MODE_SSC) {
		val = rdl(pll, CTRL1);
		return rate + parent_rate * val / dm / dp / (1 << 24);
	} else {
		return 0;
	}
}

static long
as370_pll_clko_round_rate(struct clk_hw *hw, unsigned long rate,
			  unsigned long *parent_rate)
{
	u32 dp, dn, dm, vco, div;
	unsigned long parent = *parent_rate;
	u64 frac = 0;

	parent /= FREQ_FACTOR;
	rate /= FREQ_FACTOR;

	if (!parent || !rate)
		return as370_pll_clko_recalc_rate(hw, *parent_rate);

	if (rate > VCO_HIGH_LIMIT)
		return as370_pll_clko_recalc_rate(hw, *parent_rate);

	if (rate < VCO_LOW_LIMIT) {
		dp = VCO_LOW_LIMIT / rate + 1;
		if (dp > MAX_DP)
			return as370_pll_clko_recalc_rate(hw, *parent_rate);

		vco = rate * dp;
	} else {
		dp = 1;
		vco = rate;
	}

	div = gcd(vco, parent);
	dn = vco / div;
	dm = parent / div;

	if ((dm > MAX_DM) || (dn > MAX_DN)) {
		/* frac mode, recaculate dn, dm */
		u32 dm_t;
		unsigned long min_freqm;

		if ((dn / dm) > MAX_DN)
			return as370_pll_clko_recalc_rate(hw, *parent_rate);

		dm_t = 1;
		min_freqm = (2 * MIN_FREQM_FRAC/FREQ_FACTOR);
		/*
		   try to get the greatest dn for better frac accuracy.
		   parent_rate/dm should < MIN_FREQM_FRAC/FREQ_FACTOR
		   dm = dm_t
		   dn = dn * dm_t / dm
		 */
		while ((dm_t < (MAX_DM/2)) && ((dn * dm_t / dm) < (MAX_DN/2))
			&& (parent >= min_freqm * dm_t)) {
			dm_t *= 2;
		}
		frac = dn * dm_t;
		frac = (frac << FRAC_BITS) / dm;
		dn = frac >> FRAC_BITS;
		dm = dm_t;
		frac = frac & FRAC_MASK;
	}

	rate = parent * dn / dm / dp;
	if (frac) {
		rate += parent * frac / dm / dp / (1 << 24);
	}
	return rate * FREQ_FACTOR;
}

static int
as370_pll_clko_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	u32 val, dp, dn, dm, vco, div;
	u64 frac = 0;
	struct as370_pll *pll = hw_to_as370_pll(hw);
	unsigned long flags;

	parent_rate /= FREQ_FACTOR;
	rate /= FREQ_FACTOR;

	if (!parent_rate || !rate)
		return -EPERM;

	if (rate > VCO_HIGH_LIMIT)
		return -EPERM;

	if (rate < VCO_LOW_LIMIT) {
		dp = VCO_LOW_LIMIT / rate + 1;
		if (dp > MAX_DP)
			return -EPERM;

		vco = rate * dp;
	} else {
		dp = 1;
		vco = rate;
	}

	div = gcd(vco, parent_rate);
	dn = vco / div;
	dm = parent_rate / div;

	if ((dm > MAX_DM) || (dn > MAX_DN)) {
		/* frac mode, recaculate dn, dm */
		u32 dm_t;
		unsigned long min_freqm;

		if ((dn / dm) > MAX_DN)
			return -EPERM;

		dm_t = 1;
		min_freqm = (2 * MIN_FREQM_FRAC/FREQ_FACTOR);
		/*
		   try to get the greatest dn for better frac accuracy.
		   parent_rate/dm should < MIN_FREQM_FRAC/FREQ_FACTOR
		   dm = dm_t
		   dn = dn * dm_t / dm
		 */
		while ((dm_t < (MAX_DM/2)) && ((dn * dm_t / dm) < (MAX_DN/2))
			&& (parent_rate >= min_freqm * dm_t)) {
			dm_t *= 2;
		}
		frac = dn * dm_t;
		frac = (frac << FRAC_BITS) / dm;
		dn = frac >> FRAC_BITS;
		dm = dm_t;
		frac = frac & FRAC_MASK;
	}

	local_irq_save(flags);

	val = readl_relaxed(pll->bypass);
	if (pll->bypass_invert)
		val &= ~(1 << pll->bypass_shift);
	else
		val |= 1 << pll->bypass_shift;
	writel_relaxed(val, pll->bypass);

	/* enable PLL bypass */
	val = rdl(pll, CTRL4);
	wrl(pll, CTRL4, val | BYPASS);

	/* power down pll*/
	val = rdl(pll, CTRL3);
	wrl(pll, CTRL3, val | PDDP);

	val = rdl(pll, CTRL);
	if (frac) {
		val &= ~RESETN;
	}
	wrl(pll, CTRL, val);

	/* update vco*/
	val &= (~MODE_MASK) & (~DM_MASK) & (~DN_MASK);
	if(frac) {
		val |= MODE_FRAC << MODE_SHIFT;
		wrl(pll, CTRL1, frac);
	}
	val |= dn << DN_SHIFT;
	val |= dm << DM_SHIFT;
	wrl(pll, CTRL, val);

	/* udpate dp*/
	val = rdl(pll, CTRL3);
	val &= ~DP_MASK;
	val |= (dp << DP_SHIFT);
	wrl(pll, CTRL3, val);

	/* wait at least 2 ref_clock */
	udelay(1);

	/* power up pll*/
	val = rdl(pll, CTRL);
	if (frac) {
		val |= RESETN;
	}
	wrl(pll, CTRL, val);

	val = rdl(pll, CTRL3);
	val &= ~PDDP;
	wrl(pll, CTRL3, val);

	/* delay 500 reference divided cycles */
	udelay(50);

	/* wait pll lock */
	val = rdl(pll, STATUS);
	while (!(val & PLL_LOCK)) {
		val = rdl(pll, STATUS);
		cpu_relax();
	}

	/* disable bypass */
	val = rdl(pll, CTRL4);
	wrl(pll, CTRL4, val & (~BYPASS));

	val = readl_relaxed(pll->bypass);
	if (pll->bypass_invert)
		val |= 1 << pll->bypass_shift;
	else
		val &= ~(1 << pll->bypass_shift);
	writel_relaxed(val, pll->bypass);

	local_irq_restore(flags);

	return 0;
}

static const struct clk_ops as370_pll_clko_ops = {
	.recalc_rate	= as370_pll_clko_recalc_rate,
	.round_rate		= as370_pll_clko_round_rate,
	.set_rate		= as370_pll_clko_set_rate,
};

static const struct clk_ops as370_pll_clko1_ops = {
	.recalc_rate	= as370_pll_clko1_recalc_rate,
};

static int as370_pll_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk_init_data init;
	struct as370_pll *pll;
	const char *parent_name;
	struct resource *res;
	char name[16];
	int ret;

	pll = devm_kzalloc(dev, sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pll->ctrl = devm_ioremap(dev, res->start, resource_size(res));
	if (!pll->ctrl)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	pll->bypass = devm_ioremap(dev, res->start, resource_size(res));
	if (!pll->bypass)
		return -ENOMEM;

	ret = of_property_read_u8(dev->of_node, "bypass-shift", &pll->bypass_shift);
	if (WARN_ON(ret))
		return -EINVAL;

	pll->bypass_invert = of_property_read_bool(dev->of_node, "bypass-invert");

	parent_name = of_clk_get_parent_name(dev->of_node, 0);

	init.name = name;
	snprintf(name, sizeof(name), "%s_clko", dev->of_node->name);
	init.ops = &as370_pll_clko_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	pll->hw.init = &init;

	pll->clks[0] = clk_register(NULL, &pll->hw);
	if (WARN_ON(IS_ERR(pll->clks[0])))
		return PTR_ERR(pll->clks[0]);

	snprintf(name, sizeof(name), "%s_clko1", dev->of_node->name);
	init.ops = &as370_pll_clko1_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	pll->hw1.init = &init;

	pll->clks[1] = clk_register(NULL, &pll->hw1);
	if (WARN_ON(IS_ERR(pll->clks[1]))) {
		ret = PTR_ERR(pll->clks[1]);
		goto err_clk1_register;
	}

	pll->onecell_data.clks = pll->clks;
	pll->onecell_data.clk_num = 2;

	ret = of_clk_add_provider(dev->of_node, of_clk_src_onecell_get,
				  &pll->onecell_data);
	if (WARN_ON(ret))
		goto err_clk_add;

	platform_set_drvdata(pdev, pll);
	return 0;

err_clk_add:
	clk_unregister(pll->clks[1]);
err_clk1_register:
	clk_unregister(pll->clks[0]);
	return ret;
}

static const struct of_device_id as370_pll_match_table[] = {
	{ .compatible = "syna,as370-pll"},
	{ }
};
MODULE_DEVICE_TABLE(of, as370_pll_match_table);

static struct platform_driver as370_pll_driver = {
	.probe		= as370_pll_setup,
	.driver		= {
		.name	= "syna-as370-pll",
		.of_match_table = as370_pll_match_table,
	},
};

static int __init syna_as370_pll_init(void)
{
	return platform_driver_register(&as370_pll_driver);
}
core_initcall(syna_as370_pll_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synaptics as370 pll Driver");
