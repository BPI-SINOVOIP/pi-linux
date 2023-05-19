// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __DMIC_H__
#define __DMIC_H__
#include "pdm.h"
#include "berlin_pcm.h"

struct dmic_clk_cfg {
	u32 fs;
	u32 aud_pll;
	u32 div_core;
	u32 div_m;
	u32 div_n;
	u32 div_p;
	u32 cic_ratio;
};

struct dmic_data {
	const char *dev_name;
	bool enable[MAX_PDM_CH];
	u32 irqc;
	unsigned int irq[MAX_PDM_CH];
	u32 chid[MAX_PDM_CH];
	u32 ch_num;
	u32 channel_map;
	u32 max_ch_inuse;
	u32 gain[MAX_CHANNELS];
	bool irq_requested;
	bool disable_mic_mute;
	bool enable_dc_filter;
	bool ch_shift_check; /* for dmic multi-channel shift check */
	u32 pdm_clk_sel;
	u32 pdm_type;
	struct gpio_desc *pdm_data_sel_gpio;

	//decimation configuration
	u32 fir_filter_sel;
	u32 cic_ratio_pcm;
	u32 cic_ratio_pcm_D;
	u32 pdm_bits_per_slot;

	//div
	u32 clk_div_m;
	u32 clk_div_n;
	u32 clk_div_p;
	u32 clk_source;

	u32 dpath_clk_div;
	bool dpath_clk_en;

	void *aio_handle;
};

#endif
