/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __BERLIN_SPDIF_H__
#define __BERLIN_SPDIF_H__

struct spdif_cs {
	u32 consumer:1;
	u32 digital_data:1;
	u32 copyright:1;
	u32 nonlinear:3;
	u32 mode:2;
	u32 category:7;
	u32 lbit:1;
	u32 source_num:4;
	u32 channel_num:4;
	u32 sample_rate:4;
	u32 accuray:4;
	u32 word_length:4;
	u32 orig_sample_rate:4;
	u32 CGMSA:2;
	u32 reseverd_bits:22;
	u32 reseverd[4];
};

enum subframe_type {
	TYPE_B = 0,
	TYPE_M = 1,
	TYPE_W = 2,
};

/*
 * 32-bits packed spdif data
 */
struct spdif_frame {
	int16_t sync : 4;
	int16_t validflag : 1;
	int16_t user : 1;
	int16_t channelstatus : 1;
	int16_t paritybit : 1;
	int16_t audiosample1 : 8;
	int16_t audiosample2;
};

#define SPDIF_BLOCK_SIZE       192

u32 spdif_init_channel_status(struct spdif_cs *chnsts, u32 fs);
u8 spdif_get_channel_status(u8 *chnsts, u32 spdif_frames);

#endif
