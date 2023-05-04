// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/atomic.h>
#include <linux/printk.h>
#include <linux/err.h>

#include "aio_common.h"
#include "aio_hal.h"
#include "pdm_common.h"
#include "aio_priv.h"

static void aio_pdm_ctrl1_set_clk_sel(void *hd, u8 clk_sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	T32PDM_CTRL1 reg;

	offset = RA_AIO_PDM + RA_PDM_CTRL1;
	reg.u32 = aio_read(aio, offset);
	reg.uCTRL1_INVCLK_INT = clk_sel;
	aio_write(aio, offset, reg.u32);
}

void aio_pdm_set_ctrl1_div(void *hd, u32 fs)
{
	u32 div;

	div = aio_pdm_get_div(fs);
	aio_pdm_ctrl1_set(hd, div, 1, 1, 4, 0, 0);
	aio_pdm_ctrl1_set_clk_sel(hd, 0);
}

int aio_get_pdm_offset(u32 chanid)
{
	int ret = 0;

	if(chanid < AIO_MIC_CH_NUM) {
		ret = RA_AIO_PDM + RA_PDM_PDM0 + (chanid*sizeof(SIE_PDMCH));
	} else {
		printk(KERN_ERR "illegal pdm channd id %d\n", chanid);
		ret = -EINVAL;
	}

	return ret;
}

void xfeed_set_pdm_sel(void *hd, u32 pdm_clk_sel, u32 pdmc_sel, u32 pdm_sel)
{
	u32 offset;
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_XFEED reg;

	offset = RA_AIO_XFEED;
	reg.u32 = aio_read(aio, offset);
	reg.uXFEED_PDM_CLK_SEL = pdm_clk_sel;
	reg.uXFEED_PDMC_SEL = pdmc_sel;
	reg.uXFEED_PDM_SEL = pdm_sel;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(xfeed_set_pdm_sel);
