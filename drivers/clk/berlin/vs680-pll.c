// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Synaptics Incorporated
 *
 * Author: Benson Gui <begu@synaptics>
 *
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_device.h>

#include "clk.h"

#define FRAC_BITS		24
#define FRAC_MASK		((1<<FRAC_BITS) - 1)

#define CTRLA			0x0
#define  A_RESET		BIT(0)
#define  A_BYPASS		BIT(1)
#define  A_NEWDIV		BIT(2)
#define  A_RANGE_SHIFT		3
#define  A_RANGE_MASK		(0x7 << A_RANGE_SHIFT)

#define CTRLB			0x4
#define  B_SSE			BIT(9)

#define CTRLC			0x8
#define  C_DIVR_SHIFT		0
#define  C_DIVR_MASK		(0x1ff << C_DIVR_SHIFT)

#define CTRLD			0xc
#define  DIVF_SHIFT		0
#define  DIVF_MASK		(0x1ff << DIVF_SHIFT)

#define CTRLE			0x10
#define  DIVFF_SHIFT		0
#define  DIVFF_MASK		(((1<<FRAC_BITS) - 1) << DIVFF_SHIFT)

#define CTRLF			0x14
#define  DIVQ_SHIFT		0
#define  DIVQ_MASK		(0x1f << DIVQ_SHIFT)

#define CTRLG			0x18
#define  DIVQF_SHIFT		0
#define  DIVQF_MASK		(0x7 << DIVQF_SHIFT)

#define STATUS			0x1c
#define  PLL_LOCK		BIT(0)
#define  DIV_ACK		BIT(1)

#define FREQ_FACTOR		(1000)
#define FRAC_MIN_FPFD		((5 * 1000 * 1000UL) / FREQ_FACTOR)
#define FRAC_MAX_FPFD		((7500 * 1000UL) / FREQ_FACTOR)

#define VCO_HIGH_LIMIT		((6000 * 1000 * 1000UL) / FREQ_FACTOR)
#define VCO_LOW_LIMIT		((1200 * 1000 * 1000UL) / FREQ_FACTOR)
#define FREQ_OUT_HIGH_LIMIT	((3000 * 1000 * 1000UL) / FREQ_FACTOR)
#define FREQ_OUT_LOW_LIMIT	((20 * 1000 * 1000UL) / FREQ_FACTOR)
#define FREQ_OUTF_HIGH_LIMIT	((6000 * 1000 * 1000UL) / FREQ_FACTOR)
#define FREQ_OUTF_LOW_LIMIT	((200 * 1000 * 1000UL) / FREQ_FACTOR)

#define FREQ_IN_LLIMIT		((10 * 1000 * 1000UL) / FREQ_FACTOR)
#define FREQ_IN_HLIMIT		((300 * 1000 * 1000UL) / FREQ_FACTOR)

#define MAX_DP0			((1 << 5) * 2)
#define MIN_DP0			2
#define MAX_DP0_S		(MAX_DP0 / 2)
#define MIN_DP0_S		(MIN_DP0 / 2)
#define MAX_DP1			7
#define MIN_DP1			1

#define MAX_DM			(1 << 6)
#define MAX_DN			(1 << 7)
#define DIVF_DEF_MULT		4

#define PLL_SET_MASK		(BIT(0) | BIT(1))

/*
 * currently we support 4 modes for dual output set rate.
 * PAIR_NO_CARE:	when setting one output, don't care about the other one,
 *			this mode is compatible with before.
 * PAIR_FOLLOW:		the other output will use the same freq.
 * PAIR_KEEP_RATIO:	the pll0/pll1's ratio will be keepped when freq changed.
 *			some freqency will not be avilable.
 * PAIR_KEEP:		keep the other output's freq nnot changed. this is the hardest
 *			conidtion, lots of freqency can't be accepted under this mode.
 */
enum {
	PAIR_NO_CARE = 0,
	PAIR_FOLLOW,
	PAIR_KEEP_RATIO,
	PAIR_KEEP,
	PAIR_MODE_MAX,
};

enum {
	PLL_BYPASS_PD = 0,
	PLL_NORMAL,
};

enum {
	CTRLA_BK = 0,
	CTRLB_BK,
	CTRLC_BK,
	CTRLD_BK,
	CTRLE_BK,
	CTRLF_BK,
	CTRLG_BK,
	CTRLBK_MAX,
};

struct vs680_pll {
	void __iomem *ctrl;
	void __iomem *bypass;
	u8 bypass_shift;
	u8 pair_clk_mode;
	bool pd_bypass;
	bool suspend_resume;
	bool bypassed;
	unsigned long rates[2];
	u32 regs_bk[CTRLBK_MAX];
	struct clk *bypass_clk;
	struct clk *clks[2];
	struct clk_hw hw;
	struct clk_hw hw1;
	struct clk_onecell_data	onecell_data;
};

struct pll_preset_param {
	int out_idx;
	unsigned long rate;	/* unit in KHZ */
	u32 dm;
	u32 dn;
	u32 frac;
	int dp0;
	int dp1;
};

static const struct pll_preset_param preset_tab[] = {
	{
		.out_idx = 0,
		.rate = 593333,
		.dm = 2,
		.dn = 177,
		.frac = 0,
		.dp0 = 5,
		.dp1 = 0,
	}
};

#define hw_to_vs680_pll(hw) container_of(hw, struct vs680_pll, hw)
#define hw1_to_vs680_pll(hw) container_of(hw, struct vs680_pll, hw1)

static inline u32 rdl(struct vs680_pll *pll, u32 offset)
{
	return readl_relaxed(pll->ctrl + offset);
}

static inline void wrl(struct vs680_pll *pll, u32 offset, u32 data)
{
	writel_relaxed(data, pll->ctrl + offset);
}

static unsigned long
vs680_pll_clko_recalc_rate(struct clk_hw *hw,
			   unsigned long parent_rate)
{
	u32 val, dp, dm, dn;
	u64 frac;
	unsigned long rate;
	struct vs680_pll *pll = hw_to_vs680_pll(hw);

	if (pll->bypassed)
		return clk_get_rate(pll->bypass_clk);

	val = rdl(pll, CTRLC);
	dm = ((val & C_DIVR_MASK) >> C_DIVR_SHIFT) + 1;

	val = rdl(pll, CTRLD);
	dn = ((val & DIVF_MASK) >> DIVF_SHIFT) + 1;

	val = rdl(pll, CTRLE);
	frac = (val & DIVFF_MASK) >> DIVFF_SHIFT;

	val = rdl(pll, CTRLF);
	dp = (val & DIVQ_MASK) >> DIVQ_SHIFT;
	dp = (dp + 1) * 2;

	rate = parent_rate * dn;
	rate += (parent_rate * frac + (1 << (FRAC_BITS-1))) / (1 << FRAC_BITS);
	rate = (rate * DIVF_DEF_MULT + dm * dp / 2) / dm / dp;

	return rate;
}

static unsigned long
vs680_pll_clko1_recalc_rate(struct clk_hw *hw,
			    unsigned long parent_rate)
{
	u32 val, dp, dm, dn;
	u64 frac;
	unsigned long rate;
	struct vs680_pll *pll = hw1_to_vs680_pll(hw);

	if (pll->bypassed)
		return clk_get_rate(pll->bypass_clk);

	val = rdl(pll, CTRLC);
	dm = ((val & C_DIVR_MASK) >> C_DIVR_SHIFT) + 1;

	val = rdl(pll, CTRLD);
	dn = ((val & DIVF_MASK) >> DIVF_SHIFT) + 1;

	val = rdl(pll, CTRLE);
	frac = (val & DIVFF_MASK) >> DIVFF_SHIFT;

	val = rdl(pll, CTRLG);
	dp = ((val & DIVQF_MASK) >> DIVQF_SHIFT) + 1;

	rate = parent_rate * dn;
	rate += (parent_rate * frac + (1 << (FRAC_BITS-1))) / (1 << FRAC_BITS);
	rate = (rate * DIVF_DEF_MULT + dm * dp / 2) / dm / dp;

	return rate;
}

static long
vs680_pll_clko_round_rate(struct clk_hw *hw, unsigned long rate,
			  unsigned long *parent)
{
	u32 dp, dm, dn, gcdv;
	u64 frac, parent_rate = *parent;

	parent_rate /= FREQ_FACTOR;
	rate /= FREQ_FACTOR;

	if ((rate > FREQ_OUT_HIGH_LIMIT) || (rate < FREQ_OUT_LOW_LIMIT))
		return vs680_pll_clko_recalc_rate(hw, *parent);

	dp = MIN_DP0;
	gcdv = gcd(rate, parent_rate);
	dn = rate / gcdv;
	dm = parent_rate / gcdv * (DIVF_DEF_MULT / dp);

	frac = 0;
	if ((dm > MAX_DM) || (dn > MAX_DN)) {
		/*
		 * For frac mode, the FPFD is limited from 5M to 7.5M.
		 * For 25M input, dm is only valid for 5 or 4, here we got
		 * dm = parent / FRAC_MIN_FPFD.
		 */
		dm = parent_rate / FRAC_MIN_FPFD;

		if (((parent_rate / dm) >= FRAC_MAX_FPFD)
			|| (dm > MAX_DM)) {
			return vs680_pll_clko_recalc_rate(hw, *parent);
		}

		frac = dp * rate;
		frac = (frac << FRAC_BITS) * dm / (DIVF_DEF_MULT * parent_rate);
		dn = frac >> FRAC_BITS;
		frac = frac & FRAC_MASK;
	}

	rate = parent_rate * FREQ_FACTOR * dn;
	rate += (parent_rate * FREQ_FACTOR * frac + (1 << (FRAC_BITS-1)))
				/ (1 << FRAC_BITS);
	rate = (rate * DIVF_DEF_MULT + dm * dp / 2) / dm / dp;

	return rate;
}

static long
vs680_pll_clko1_round_rate(struct clk_hw *hw, unsigned long rate,
			  unsigned long *parent)
{
	u32 dp, dm, dn, gcdv;
	u64 frac, parent_rate = *parent;

	parent_rate /= FREQ_FACTOR;
	rate /= FREQ_FACTOR;

	if ((rate > FREQ_OUTF_HIGH_LIMIT) || (rate < FREQ_OUTF_LOW_LIMIT))
		return vs680_pll_clko1_recalc_rate(hw, *parent);

	dp = MIN_DP1;
	gcdv = gcd(rate, parent_rate);
	dn = rate / gcdv;
	dm = parent_rate / gcdv * (DIVF_DEF_MULT / dp);

	frac = 0;
	if ((dm > MAX_DM) || (dn > MAX_DN)) {
		/*
		 * For frac mode, the FPFD is limited from 5M to 7.5M.
		 * For 25M input, dm is only valid for 5 or 4, here we got
		 * dm = parent / FRAC_MIN_FPFD.
		 */
		dm = parent_rate / FRAC_MIN_FPFD;

		if (((parent_rate / dm) >= FRAC_MAX_FPFD)
			|| (dm > MAX_DM)) {
			return vs680_pll_clko_recalc_rate(hw, *parent);
		}

		frac = dp * rate;
		frac = (frac << FRAC_BITS) * dm / (DIVF_DEF_MULT * parent_rate);
		dn = frac >> FRAC_BITS;
		frac = frac & FRAC_MASK;
	}

	rate = parent_rate * FREQ_FACTOR * dn;
	rate += (parent_rate * FREQ_FACTOR * frac + (1 << (FRAC_BITS-1)))
				/ (1 << FRAC_BITS);
	rate = (rate * DIVF_DEF_MULT + dm * dp / 2) / dm / dp;

	return rate;
}

static u32 vs680_pll_get_pfdrange(u32 freq)
{
	u32 r;

	if (freq >= 50 && freq < 75) {
		r=0;
	} else if (freq >= 75 && freq < 110) {
		r=1;
	} else if (freq >= 110 && freq < 180) {
		r=2;
	} else if (freq >= 180 && freq < 300) {
		r=3;
	} else if (freq >= 300 && freq < 500) {
		r=4;
	} else if (freq >= 500 && freq < 800) {
		r=5;
	} else if (freq >= 800 && freq < 1300) {
		r=6;
	} else if (freq >= 1300 && freq <= 2000) {
		r=7;
	} else {
		r=0;
	}

	return r;
}

static void vs680_pll_update_setting(struct vs680_pll *pll,
			unsigned long parent_rate,
			u32 dm, u32 dn, u32 frac, int dp0, int dp1)
{
	u32 freq, range, val, timeout;
	unsigned long flags;

	freq = parent_rate / (1000000 / FREQ_FACTOR) * 10 / (dm + 1);
	range = vs680_pll_get_pfdrange(freq);

	local_irq_save(flags);

	val = readl_relaxed(pll->bypass);
	val |= 1 << pll->bypass_shift;
	writel_relaxed(val, pll->bypass);

	/* set internal bypass and hold reset */
	val = rdl(pll, CTRLA);
	wrl(pll, CTRLA, val | A_BYPASS | A_RESET);

	/* disable ssc */
	val = rdl(pll, CTRLB);
	wrl(pll, CTRLB, val & (~B_SSE));

	/* set the vco */
	val = rdl(pll, CTRLC);
	val &= ~C_DIVR_MASK;
	val |= dm << C_DIVR_SHIFT;
	wrl(pll, CTRLC, val);

	val = rdl(pll, CTRLD);
	val &= ~DIVF_MASK;
	wrl(pll, CTRLD, val | (dn << DIVF_SHIFT));

	val = rdl(pll, CTRLE);
	val &= ~DIVFF_MASK;
	wrl(pll, CTRLE, val | (frac << DIVFF_SHIFT));

	/* set the range */
	val = rdl(pll, CTRLA);
	val &= ~A_RANGE_MASK;
	wrl(pll, CTRLA, val | (range << A_RANGE_SHIFT));

	/* set the divq */
	if (dp0 >= 0) {
		val = rdl(pll, CTRLF);
		val &= ~DIVQ_MASK;
		wrl(pll, CTRLF, val | (dp0 << DIVQ_SHIFT));
	}

	/* set the divq */
	if (dp1 >= 0) {
		val = rdl(pll, CTRLG);
		val &= ~DIVQF_MASK;
		wrl(pll, CTRLG, val | (dp1 << DIVQF_SHIFT));
	}

	/* release reset */
	val = rdl(pll, CTRLA);
	val &= ~A_RESET;
	wrl(pll, CTRLA, val);

	/* release bypass */
	val &= ~A_BYPASS;
	wrl(pll, CTRLA, val);

	if (pll->pd_bypass) {
		val = readl_relaxed(pll->bypass);
		val &= ~(1 << pll->bypass_shift);
		writel_relaxed(val, pll->bypass);
	}

	udelay(120);
	/*
	 * according to SPEC and diag team's feedback, the lock bit is not
	 * a must have, below is the part from SPEC.
	 * It is recommended that LOCK is only used for test and system status
	 * information, and is not used for critical system functions without
	 * thorough characterization in the host system.
	 * So, here we use timeout to print a warning when lock bit is not set.
	 */
	timeout = 100;
	val = rdl(pll, STATUS);
	while (!(val & PLL_LOCK)) {
		udelay(1);
		timeout--;
		if (!timeout) {
			printk("%s: pll lock failed and continue..\n",
				clk_hw_get_name(&pll->hw));
			break;
		}
		val = rdl(pll, STATUS);
	}

	if (!pll->pd_bypass) {
		val = readl_relaxed(pll->bypass);
		val &= ~(1 << pll->bypass_shift);
		writel_relaxed(val, pll->bypass);
	}
	local_irq_restore(flags);
	pll->bypassed = 0;
}

static int
vs680_pll_set_rates(struct vs680_pll *pll, unsigned long rate0,
			unsigned long rate1, u32 pll_set_flag,
			unsigned long parent_rate)
{
	u32 dn, dm, gcdv;
	int dp0, dp1;
	u64 frac;
	unsigned long vco;

	/* caculate the vco and dp */
	if ((pll_set_flag & PLL_SET_MASK) == BIT(0)) {
		/* let vco near the high limit to get smaller jitter */
		dp0 = VCO_HIGH_LIMIT / rate0;
		if (dp0 > MAX_DP0_S)
			dp0 = MAX_DP0_S;
		vco = rate0 * dp0;
		dp1 = 0; /* don't need to set dp1 */
	} else if ((pll_set_flag & PLL_SET_MASK) == BIT(1)) {
		dp1 = VCO_HIGH_LIMIT / rate1;
		if (dp1 > MAX_DP1)
			dp1 = MAX_DP1;
		vco = rate1 * dp1;
		dp0 = 0; /* don't need to set dp0 */
	} else if ((pll_set_flag & PLL_SET_MASK) == PLL_SET_MASK) {
		vco = rate0 / gcd(rate0, rate1) * rate1;
		if (vco < VCO_LOW_LIMIT) {
			if ((VCO_LOW_LIMIT % vco) == 0)
				vco = VCO_LOW_LIMIT;
			else
				vco *= VCO_LOW_LIMIT / vco + 1;
		} else if (vco > VCO_HIGH_LIMIT) {
			pr_err("invalid vco: %s: vco %lu, rate0 %lu, rate1 %lu\n",
				clk_hw_get_name(&pll->hw), vco, rate0/2, rate1);
			return -EPERM;
		}

		dp0 = vco / rate0;
		dp1 = vco / rate1;

		if ((dp0 > MAX_DP0_S) || (dp1 > MAX_DP1)) {
			pr_err("invalid dp: %s: vco %lu, rate0 %lu, rate1 %lu, dp %d %d\n",
				clk_hw_get_name(&pll->hw), vco, rate0/2, rate1, dp0, dp1);
			return -EPERM;
		}

		/* try to let vco near the high limit */
		while ((dp0 <= (MAX_DP0_S / 2)) && (dp1 <= (MAX_DP1 / 2))
			&& (vco <= VCO_HIGH_LIMIT /2)) {
			vco *= 2;
			dp0 *= 2;
			dp1 *= 2;
		}
	} else
		return -EPERM;

	/* caculate the dn, dm */
	gcdv = gcd(vco, parent_rate);
	dn = vco / gcdv;
	dm = parent_rate / gcdv * DIVF_DEF_MULT;
	frac = 0;
	/* If dn and dm are valid values, we don't use frac mode. */
	if ((dm > MAX_DM) || (dn > MAX_DN)) {
		/*
		 * For frac mode, the FPFD is limited from 5M to 7.5M.
		 * For 25M input, dm is only valid for 5 or 4, here we got
		 * dm = parent / FRAC_MIN_FPFD.
		 */
		dm = parent_rate / FRAC_MIN_FPFD;
		if (((parent_rate / dm) >= FRAC_MAX_FPFD)
			|| (dm > MAX_DM)) {
			return -EPERM;
		}

		frac = (vco << FRAC_BITS) * dm / (DIVF_DEF_MULT * parent_rate);
		dn = frac >> FRAC_BITS;
		frac = frac & FRAC_MASK;
	}
	pll->rates[0] = rate0 / 2;
	pll->rates[1] = rate1;

	vs680_pll_update_setting(pll, parent_rate,
				dm - 1, dn - 1,
				frac, dp0 - 1, dp1 - 1);
	return 0;
}

static void vs680_pll_bypass_and_pd(struct vs680_pll *pll)
{
	u32 val;

	/* global bypass */
	val = readl_relaxed(pll->bypass);
	val |= 1 << pll->bypass_shift;
	writel_relaxed(val, pll->bypass);

	/* power down the pll */
	val = rdl(pll, CTRLA);
	wrl(pll, CTRLA, val | A_BYPASS | A_RESET);

	pll->bypassed = 1;
}

static int vs680_pll_try_bypass(struct vs680_pll *pll, unsigned long rate,
					    int cur, int pair)
{
	if (((clk_get_rate(pll->bypass_clk) / FREQ_FACTOR) == rate)
		&& !pll->pd_bypass) {
		switch (pll->pair_clk_mode) {
		case PAIR_NO_CARE:
		case PAIR_FOLLOW:
			pll->rates[0] = pll->rates[1] = rate;
			vs680_pll_bypass_and_pd(pll);
			return PLL_BYPASS_PD;
		case PAIR_KEEP:
			if (pll->rates[pair] != rate)
				break;

			pll->rates[cur] = rate;
			vs680_pll_bypass_and_pd(pll);
			return PLL_BYPASS_PD;
		}
	}
	return PLL_NORMAL;
}

static bool vs680_pll_use_preset_param(struct vs680_pll *pll,
					unsigned long parent_rate,
					unsigned long rate, int idx)
{
	int i;

	if (pll->pair_clk_mode != PAIR_NO_CARE)
		return false;

	for (i = 0; i < ARRAY_SIZE(preset_tab); i++) {
		if ((preset_tab[i].out_idx == idx) && (preset_tab[i].rate == rate)) {
			pll->rates[idx] = rate;
			vs680_pll_update_setting(pll, parent_rate,
				preset_tab[i].dm, preset_tab[i].dn,
				preset_tab[i].frac,
				preset_tab[i].dp0 - 1,
				preset_tab[i].dp1 - 1);
			return true;
		}
	}
	return false;
}

static int
vs680_pll_clko1_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct vs680_pll *pll = hw1_to_vs680_pll(hw);
	u32 pll_set_flag;
	int ret;
	unsigned long rate0, rate1;

	parent_rate /= FREQ_FACTOR;
	rate /= FREQ_FACTOR;

	if ((rate > FREQ_OUTF_HIGH_LIMIT) || (rate < FREQ_OUTF_LOW_LIMIT))
		return -EPERM;

	if (vs680_pll_try_bypass(pll, rate, 1, 0) == PLL_BYPASS_PD)
		return 0;

	if (vs680_pll_use_preset_param(pll, parent_rate, rate, 1))
		return 0;

	rate1 = rate;
	if (pll->pair_clk_mode == PAIR_NO_CARE) {
		/* let vco near the high limit to get smaller jitter */
		rate0 = 0;
		pll_set_flag = BIT(1);
	} else if (pll->pair_clk_mode == PAIR_FOLLOW) {
		/* let vco near the high limit to get smaller jitter */
		rate0 = rate * 2;
		pll_set_flag = PLL_SET_MASK;
	} else {
		rate0 = pll->rates[0];
		if (pll->pair_clk_mode == PAIR_KEEP_RATIO)
			rate0 = rate0 * rate1 / (pll->rates[1]);
		if ((rate0 > FREQ_OUT_HIGH_LIMIT) || (rate0 < FREQ_OUT_LOW_LIMIT))
			return -EPERM;
		rate0 *= 2;
		pll_set_flag = PLL_SET_MASK;
	}

	ret = vs680_pll_set_rates(pll, rate0, rate1, pll_set_flag, parent_rate);
	return ret;
}

static int
vs680_pll_clko_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct vs680_pll *pll = hw_to_vs680_pll(hw);
	u32 pll_set_flag;
	int ret;
	unsigned long rate0, rate1;

	parent_rate /= FREQ_FACTOR;
	rate /= FREQ_FACTOR;

	if ((rate > FREQ_OUT_HIGH_LIMIT) || (rate < FREQ_OUT_LOW_LIMIT))
		return -EPERM;

	if (vs680_pll_try_bypass(pll, rate, 0, 1) == PLL_BYPASS_PD)
		return 0;

	if (vs680_pll_use_preset_param(pll, parent_rate, rate, 0))
		return 0;

	/*
	 * because dp0 is 2, 4, 6 ... 64, here we use rate*2,
	 * then take dp0 as 1, 2, 3 ... 32
	 */
	rate0 = rate * 2;

	if (pll->pair_clk_mode == PAIR_NO_CARE) {
		/* let vco near the high limit to get smaller jitter */
		rate1 = 0;
		pll_set_flag = BIT(0);
	} else if (pll->pair_clk_mode == PAIR_FOLLOW) {
		/* let vco near the high limit to get smaller jitter */
		rate1 = rate;
		pll_set_flag = PLL_SET_MASK;
	} else {
		rate1 = pll->rates[1];
		if (pll->pair_clk_mode == PAIR_KEEP_RATIO)
			rate1 = (rate0 / 2) * rate1 / (pll->rates[0]);

		if ((rate1 > FREQ_OUTF_HIGH_LIMIT) || (rate1 < FREQ_OUTF_LOW_LIMIT))
			return -EPERM;
		pll_set_flag = PLL_SET_MASK;
	}

	ret = vs680_pll_set_rates(pll, rate0, rate1, pll_set_flag, parent_rate);
	return ret;
}

static const struct clk_ops vs680_pll_clko_ops = {
	.recalc_rate	= vs680_pll_clko_recalc_rate,
	.round_rate	= vs680_pll_clko_round_rate,
	.set_rate	= vs680_pll_clko_set_rate,
};

static const struct clk_ops vs680_pll_clko1_ops = {
	.recalc_rate	= vs680_pll_clko1_recalc_rate,
	.round_rate	= vs680_pll_clko1_round_rate,
	.set_rate	= vs680_pll_clko1_set_rate,
};

static int vs680_pll_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk_init_data init;
	struct vs680_pll *pll;
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

	ret = of_property_read_u8(dev->of_node, "pair-mode", &pll->pair_clk_mode);
	if (ret) {
		pll->pair_clk_mode = PAIR_NO_CARE;
	}
	if (WARN_ON(pll->pair_clk_mode >= PAIR_MODE_MAX))
		return -EINVAL;

	pll->bypass_clk = of_clk_get(dev->of_node, 1);
	if (IS_ERR(pll->bypass_clk))
		pll->bypass_clk = NULL;

	pll->pd_bypass = of_property_read_bool(dev->of_node, "pd-bypass");
	pll->suspend_resume = of_property_read_bool(dev->of_node, "suspend-resume");

	parent_name = of_clk_get_parent_name(dev->of_node, 0);

	init.name = name;
	snprintf(name, sizeof(name), "%s_clko", dev->of_node->name);
	init.ops = &vs680_pll_clko_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = CLK_GET_RATE_NOCACHE;

	pll->hw.init = &init;

	pll->clks[0] = clk_register(NULL, &pll->hw);
	if (WARN_ON(IS_ERR(pll->clks[0])))
		return PTR_ERR(pll->clks[0]);

	snprintf(name, sizeof(name), "%s_clko1", dev->of_node->name);
	init.ops = &vs680_pll_clko1_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = CLK_GET_RATE_NOCACHE;

	pll->hw1.init = &init;

	pll->clks[1] = clk_register(NULL, &pll->hw1);
	if (WARN_ON(IS_ERR(pll->clks[1]))) {
		ret = PTR_ERR(pll->clks[1]);
		goto err_clk1_register;
	}

	pll->onecell_data.clks = pll->clks;
	pll->onecell_data.clk_num = 2;

	pll->rates[0] = clk_get_rate(pll->clks[0]);
	pll->rates[1] = clk_get_rate(pll->clks[1]);
	pll->rates[0] = (pll->rates[0] + FREQ_FACTOR / 2) / FREQ_FACTOR;
	pll->rates[1] = (pll->rates[1] + FREQ_FACTOR / 2) / FREQ_FACTOR;

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

#ifdef CONFIG_PM_SLEEP
static int vs680_pll_restore(struct vs680_pll *pll)
{
	unsigned long flags;
	u32 val, timeout;

	local_irq_save(flags);

	val = readl_relaxed(pll->bypass);
	val |= 1 << pll->bypass_shift;
	writel_relaxed(val, pll->bypass);

	val = pll->regs_bk[CTRLA_BK] | A_BYPASS | A_RESET;
	wrl(pll, CTRLA, val);

	/* restore vco related regs */
	wrl(pll, CTRLC, pll->regs_bk[CTRLC_BK]);
	wrl(pll, CTRLD, pll->regs_bk[CTRLD_BK]);
	wrl(pll, CTRLE, pll->regs_bk[CTRLE_BK]);

	/* restore ssc reg */
	wrl(pll, CTRLB, pll->regs_bk[CTRLB_BK]);

	/* restore dp regs */
	wrl(pll, CTRLF, pll->regs_bk[CTRLF_BK]);
	wrl(pll, CTRLG, pll->regs_bk[CTRLG_BK]);

	/* release reset */
	val &= ~A_RESET;
	wrl(pll, CTRLA, val);

	/* release bypass */
	val &= ~A_BYPASS;
	wrl(pll, CTRLA, val);

	if (pll->pd_bypass) {
		val = readl_relaxed(pll->bypass);
		val &= ~(1 << pll->bypass_shift);
		writel_relaxed(val, pll->bypass);
	}

	udelay(120);

	/*
	 * according to SPEC and diag team's feedback, the lock bit is not
	 * a must have, below is the part from SPEC.
	 * It is recommended that LOCK is only used for test and system status
	 * information, and is not used for critical system functions without
	 * thorough characterization in the host system.
	 * So, here we use timeout to print a warning when lock bit is not set.
	 */
	timeout = 100;
	val = rdl(pll, STATUS);
	while (!(val & PLL_LOCK)) {
		udelay(1);
		timeout--;
		if (!timeout) {
			printk("%s: pll lock failed and continue..\n",
				clk_hw_get_name(&pll->hw));
			break;
		}
		val = rdl(pll, STATUS);
	}

	if (!pll->pd_bypass) {
		val = readl_relaxed(pll->bypass);
		val &= ~(1 << pll->bypass_shift);
		writel_relaxed(val, pll->bypass);
	}
	local_irq_restore(flags);

	return 0;
}

static int vs680_pll_suspend(struct device *dev)
{
	struct vs680_pll *pll = dev_get_drvdata(dev);

	if (pll->suspend_resume) {
		pll->regs_bk[CTRLA_BK] = rdl(pll, CTRLA);
		pll->regs_bk[CTRLB_BK] = rdl(pll, CTRLB);
		pll->regs_bk[CTRLC_BK] = rdl(pll, CTRLC);
		pll->regs_bk[CTRLD_BK] = rdl(pll, CTRLD);
		pll->regs_bk[CTRLE_BK] = rdl(pll, CTRLE);
		pll->regs_bk[CTRLF_BK] = rdl(pll, CTRLF);
		pll->regs_bk[CTRLG_BK] = rdl(pll, CTRLG);
	}
	return 0;
}

static int vs680_pll_resume(struct device *dev)
{
	struct vs680_pll *pll = dev_get_drvdata(dev);

	if (pll->bypassed) {
		vs680_pll_bypass_and_pd(pll);
	} else {
		if (!pll->suspend_resume)
			return 0;

		return vs680_pll_restore(pll);
	}
	return 0;
}

static const struct dev_pm_ops vs680_plls_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vs680_pll_suspend, vs680_pll_resume)
};

#define DEV_PM_OPS	(&vs680_plls_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id vs680_pll_match_table[] = {
	{ .compatible = "syna,vs680-pll"},
	{ }
};
MODULE_DEVICE_TABLE(of, vs680_pll_match_table);

static struct platform_driver vs680_pll_driver = {
	.probe		= vs680_pll_setup,
	.driver		= {
		.name	= "syna-vs680-pll",
		.of_match_table = vs680_pll_match_table,
		.pm	= DEV_PM_OPS,
	},
};

static int __init syna_vs680_pll_init(void)
{
	return platform_driver_register(&vs680_pll_driver);
}
core_initcall(syna_vs680_pll_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synaptics vs680 pll Driver");
