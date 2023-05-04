// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __PDM_H__
#define __PDM_H__

#include <linux/types.h>

#define PDM_PDMI_RATES   (SNDRV_PCM_RATE_8000_96000)
#define PDM_PDMI_FORMATS (SNDRV_PCM_FMTBIT_S32_LE)
#define MAX_PDM_CH         4

struct pdm_pdmi_priv {
	const char *dev_name;
	u32 irqc;
	unsigned int irq[MAX_PDM_CH];
	bool irq_requested;
	u32 chid[MAX_PDM_CH];
	u32 max_ch_inuse; //optional. Define chnum actually used.
	bool interleaved_mode;
	void *aio_handle;
};

#endif
