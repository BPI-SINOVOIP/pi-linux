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
	case avioDhubChMap_aio64b_MIC1_W0:
		tsd = AIO_TSD0;
		break;

	default:
		tsd = MAX_TSD;
	}

	return tsd;
}
EXPORT_SYMBOL(aio_get_tsd_from_chid);

int aio_setspdif_en(void *hd, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32AUDCH_CTRL reg;

	address = RA_AIO_SPDIF + RA_SPDIF_SPDIF + RA_AUDCH_CTRL;
	reg.u32 = aio_read(aio, address);
	reg.uCTRL_ENABLE = CutTo(enable, bAUDCH_CTRL_ENABLE);

	aio_write(aio, address, reg.u32);

	return 0;
}

int aio_setspdifclk(void *hd, u32 div)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32SPDIF_CLKDIV reg;

	address = RA_AIO_SPDIF + RA_SPDIF_CLKDIV;
	reg.u32 = aio_read(aio, address);
	reg.uCLKDIV_SETTING = CutTo(div, bSPDIF_CLKDIV_SETTING);
	aio_write(aio, address, reg.u32);

	return 0;
}

u32 aio_get_offset_extra(u32 ID, u32 tsd)
{
	u32 offset = 0;

	switch (ID) {
	case AIO_ID_SPDIF_TX:
		offset = I2S_SPDIF_OFFSET;
		break;
	default:
		offset = 0;
		break;
	}

	return offset;
}
