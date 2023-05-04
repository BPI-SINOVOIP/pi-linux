// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __MIC_H__
#define __MIC_H__

struct mic_common_cfg {
	bool is_tdm;
	bool is_master;
	bool invbclk;
	bool invfsync;
	bool disable_mic_mute;
	bool isleftjfy;
	u32 data_fmt;
};

#endif
