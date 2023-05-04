// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <sound/soc.h>

#include "berlin_pcm.h"
#include "aio_hal.h"

#define APLL0_RATE_1 (16384000 * 8)
#define APLL0_RATE_2 (22579200 * 8)
#define APLL0_RATE_3 (24576000 * 8)


int berlin_set_pll(void *aio_handle, u32 fs)
{
	unsigned long apll;

	switch (fs) {
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		apll = APLL0_RATE_2;
		break;
	case 8000:
	case 16000:
	case 32000:
	case 64000:
		apll = APLL0_RATE_1;
		break;
	case 12000:
	case 24000:
	case 48000:
	case 96000:
		apll = APLL0_RATE_3;
		break;
	default:
		apll = APLL0_RATE_3;
		break;
	}

	return aio_set_clk_rate(aio_handle, AIO_APLL_0, apll);
}
EXPORT_SYMBOL(berlin_set_pll);

u32 berlin_get_div(u32 fs)
{
	u32 div;

	switch (fs) {
	case 8000:
	case 11025:
	case 12000:
		div = AIO_DIV32;
		break;
	case 16000:
	case 22050:
	case 24000:
		div = AIO_DIV16;
		break;
	case 32000:
	case 44100:
	case 48000:
		div = AIO_DIV8;
		break;
	case 64000:
	case 88200:
	case 96000:
		div = AIO_DIV4;
		break;
	default:
		div = AIO_DIV8;
		break;
	}

	return div;
}
EXPORT_SYMBOL(berlin_get_div);

u32 berlin_get_dfm(u32 word_w)
{
	u32 dfm;

	switch (word_w) {
	case 16:
		dfm = AIO_16DFM;
		break;
	case 24:
		dfm = AIO_24DFM;
		break;
	case 32:
	default:
		dfm = AIO_32DFM;
		break;
	}

	return dfm;
}
EXPORT_SYMBOL(berlin_get_dfm);

u32 berlin_get_cfm(u32 word_s)
{
	u32 cfm;

	switch (word_s) {
	case 16:
		cfm = AIO_16CFM;
		break;
	case 24:
		cfm = AIO_24CFM;
		break;
	case 32:
	default:
		cfm = AIO_32CFM;
		break;
	}

	return cfm;
}
EXPORT_SYMBOL(berlin_get_cfm);
