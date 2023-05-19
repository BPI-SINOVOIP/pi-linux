// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef __SPDIFI_COMMON_H__
#define __SPDIFI_COMMON_H__

#include "aio.h"

enum spdifi_fs {
	SPDIFI_FS_32KHZ = 0,
	SPDIFI_FS_44KHZ,
	SPDIFI_FS_48KHZ,
	SPDIFI_FS_88KHZ,
	SPDIFI_FS_96KHZ,
	SPDIFI_FS_176KHZ,
	SPDIFI_FS_192KHZ,
	SPDIFI_FS_MAX,
};

struct spdifi_sample_rate_margin {
	const char *sample_rate_name;
	u32 config_value;
	u32 default_value;
};

struct spdifi_sample_rate_margin *aio_spdifi_get_srm(u32 idx);
void aio_spdifi_set_srm(u32 idx, u32 margin);
int aio_spdifi_config(void *hd, u32 fs, u32 width);
void aio_spdifi_config_reset(void *hd);
void aio_spdifi_sw_reset(void *hd);

#endif
