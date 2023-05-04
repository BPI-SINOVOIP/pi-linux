// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/atomic.h>
#include <linux/printk.h>
#include <linux/err.h>

#include "aio_common.h"
#include "aio_hal.h"
#include "pdm_common.h"

static void aio_pdm_ctrl1_set_clk_sel(void *hd, u8 clk_sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 val, offset;

	offset = RA_AIO_PDM + RA_PDM_CTRL1;
	val = aio_read(aio, offset);
	SET32PDM_CTRL1_CLKINT_SEL(val, clk_sel);
	aio_write(aio, offset, val);
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

	switch(chanid) {
	case AIO_MIC_CH0:
		ret = RA_AIO_PDM + RA_PDM_PDM0;
		break;
	case AIO_MIC_CH1:
		ret = RA_AIO_PDM  + RA_PDM_PDM1;
		break;
	default:
		printk(KERN_ERR "illegal pdm channd id %d\n", chanid);
		return -EINVAL;
	}

	return ret;
}

void xfeed_set_pdm_sel(void *hd, u32 pdm_clk_sel, u32 pdmc_sel, u32 pdm_sel)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_XFEED;
	val = aio_read(aio, offset);
	SET32AIO_XFEED_PDM_CLK_SEL(val, pdm_clk_sel);
	SET32AIO_XFEED_PDMC_SEL(val, pdmc_sel);
	SET32AIO_XFEED_PDM_SEL(val, pdm_sel);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(xfeed_set_pdm_sel);
