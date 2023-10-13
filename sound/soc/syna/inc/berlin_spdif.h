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

enum burst_data_type {
	BURST_NULL = 0,
	BURST_AC3 = 1,
	BURST_PAUSE = 3,
	BURST_MPEG1_LAYER1 = 4,
	BURST_MPEG1_LAYER2 = 5,
	BURST_MPEG2_EXTENSION = 6,
	BURST_GNTR_AAC = 7,
	BURST_MPEG2_LAYER1 = 8,
	BURST_MPEG2_LAYER2 = 9,
	BURST_MPEG2_LAYER3 = 10,
	BURST_DTS_LAYER1 = 11,
	BURST_DTS_LAYER2 = 12,
	BURST_DTS_LAYER4 = 17,
	BURST_WMA_PRO = 18,
	BURST_MPEG2_AAC = 19,
	BURST_MPEG4_AAC = 20,
	BURST_DDP = 21,
	BURST_MAT = 22,
} ;
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

struct hdmi_spdif_frame {
	u32 reserve_b   	  :8;
	u32 reserve 		  :3;
	u32 Data_Low		  :5;
	u32 Data_Mid		  :8;
	u32 Data_High   	  :3;
	u32 unValidFlag 	  :1;
	u32 unUserData  	  :1;
	u32 unChannelStatus   :1;
	u32 unParityBit 	  :1;
	u32 unBbit  		  :1;
};

#define SPDIF_BLOCK_SIZE       192

u32 spdif_init_channel_status(struct spdif_cs *chnsts, u32 fs, u32 digital);
u8 spdif_get_channel_status(u8 *chnsts, u32 spdif_frames);
u8 hdmi_get_channel_status(u8 *chnsts, u32 spdif_frames);
u32 hdmi_init_channel_status(struct spdif_cs *chnsts, u32 fs,
	struct hdmi_spdif_frame *pSpdifData);
#endif
