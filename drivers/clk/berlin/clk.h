/*
 * Copyright (c) 2015 Marvell Technology Group Ltd.
 *
 * Author: Jisheng Zhang <jszhang at marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BERLIN_CLK_H
#define __BERLIN_CLK_H

#define CLK_RATE_NO_CHANGE		BIT(0) /* clock set rate is not permitted */

struct clk_desc {
	const char	*name;
	u32		offset;
	unsigned long	flags;
	unsigned long	priv_flags;
};

struct gateclk_desc {
	const char	*name;
	const char	*parent_name;
	u8		bit_idx;
	unsigned long	flags;
};

int berlin_clk_setup(struct platform_device *pdev,
			     const struct clk_desc *desc,
			     struct clk_onecell_data *clk_data,
			     int n);

int berlin_gateclk_setup(struct platform_device *pdev,
				 const struct gateclk_desc *descs,
				 struct clk_onecell_data *clk_data,
				 int n);
#endif
