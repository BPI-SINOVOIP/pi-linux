// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/string.h>
#include "berlin_spdif.h"

enum {
	SPDIF_FREQ_11K = 0x1,
	SPDIF_FREQ_12K = 0x1,
	SPDIF_FREQ_22K = 0x4,
	SPDIF_FREQ_24K = 0x6,
	SPDIF_FREQ_32K = 0x3,
	SPDIF_FREQ_44K = 0x0,
	SPDIF_FREQ_48K = 0x2,
	SPDIF_FREQ_88K = 0x8,
	SPDIF_FREQ_96K = 0xA,
	SPDIF_FREQ_176K = 0xC,
	SPDIF_FREQ_192K = 0xE,
};

u32 spdif_init_channel_status(struct spdif_cs *chnsts, u32 fs)
{
	if (chnsts == NULL)
		return -1;

	memset(chnsts, 0, sizeof(struct spdif_cs));
	chnsts->consumer = 0;
	chnsts->digital_data = 0;
	chnsts->copyright = 0;
	chnsts->nonlinear = 0;
	chnsts->mode = 0;
	chnsts->category = 9;   // 0b0001001
	chnsts->lbit = 0;
	chnsts->source_num = 0;
	chnsts->channel_num = 0;

	switch (fs) {
	case 32000:
		chnsts->sample_rate = SPDIF_FREQ_32K;
		break;
	case 44100:
		chnsts->sample_rate = SPDIF_FREQ_44K;
		break;
	case 48000:
		chnsts->sample_rate = SPDIF_FREQ_48K;
		break;
	case 88200:
		chnsts->sample_rate = SPDIF_FREQ_88K;
		break;
	case 96000:
		chnsts->sample_rate = SPDIF_FREQ_96K;
		break;
	default:
		break;
	}

	chnsts->word_length = (chnsts->digital_data == 1 ? 0x2 : 0xb);
	chnsts->CGMSA = 3;      // 0b11

	return 0;
}

u8 spdif_get_channel_status(u8 *chnsts, u32 spdif_frames)
{
	u32 bytes, bits;

	bytes = spdif_frames >> 3;
	bits = spdif_frames - (bytes << 3);
	return ((chnsts[bytes] >> bits) & 0x01);
}
