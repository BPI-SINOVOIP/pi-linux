// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "aio_hal.h"
#include "aio_common.h"
#include "i2s_common.h"
#include "avioDhub.h"

int aio_setirq(void *hd,
		u32 pri, u32 sec,
		u32 mic, u32 spdif, u32 hdmi)
{
	/*not support in as370*/
	return 0;
}
EXPORT_SYMBOL(aio_setirq);

int aio_selhdport(void *hd, u32 sel)
{
	/*not support in as370*/
	return 0;
}
EXPORT_SYMBOL(aio_selhdport);

int aio_selhdsource(void *hd, u32 sel)
{
	/*not support in as370*/
	return 0;
}
EXPORT_SYMBOL(aio_selhdsource);

int aio_i2s_set_clock(void *hd, u32 id, u32 clkSwitch,
	 u32 clkD3Switch, u32 clkSel, u32 pllUsed, u32 en)
{
	/*not support in as370*/
	return 0;
}
EXPORT_SYMBOL(aio_i2s_set_clock);

u32 aio_get_tsd_from_chid(void *hd, u32 chid)
{
	u32 tsd = MAX_TSD;

	switch (chid) {
	case avioDhubChMap_aio64b_MIC1_W1:
		tsd = AIO_TSD0;
		break;

	default:
		tsd = MAX_TSD;
	}

	return tsd;
}
EXPORT_SYMBOL(aio_get_tsd_from_chid);

int aio_set_xfeed_mode(void *hd, s32 i2sid, u32 lrclk, u32 bclk)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_XFEED reg;
	u32 offset;

	offset = RA_AIO_XFEED;
	reg.u32 = aio_read(aio, offset);

	switch (i2sid) {
	case I2S1_ID:
		reg.uXFEED_I2S1_LRCKIO_MODE = lrclk;
		reg.uXFEED_I2S1_BCLKIO_MODE = bclk;
		break;
	case I2S2_ID:
		reg.uXFEED_I2S2_LRCKIO_MODE = lrclk;
		reg.uXFEED_I2S2_BCLKIO_MODE = bclk;
		break;
	case I2S3_ID:
		reg.uXFEED_I2S3_LRCKIO_MODE = lrclk;
		reg.uXFEED_I2S3_BCLKIO_MODE = bclk;
		break;
	default:
		pr_err("%s, id(%d) is not supported\n", __func__, i2sid);
		return -EINVAL;
	}
	aio_write(aio, offset, reg.u32);

	return 0;
}
