// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef __AVIO_DHUB_CFG_PRV_H__
#define __AVIO_DHUB_CFG_PRV_H__

#define AG_NUM_OF_CHANNELS			(avioDhubChMap_aio64b_SPDIF_W+1)

#define AG_DHUB_BANK0_START_ADDR	avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR

#define AVIO_DHUB_INITCHANNELAXQOS(pdhubHandle, dhub_config, chanId, i, cfgQ)
#define AVIO_DHUB_MTR_CFG_QOS_EN	0
#define AVIO_DHUB_MTR_CFG_QOS		0
#define AVIO_DHUB_DESC_OVRDQOS		0
#define AVIO_DHUB_DESC_QOSSEL		0

typedef struct DHUB_channel_config {
	SIGN32 chanId;
	UNSG32 chanCmdBase;
	UNSG32 chanDataBase;
	SIGN32 chanCmdSize;
	SIGN32 chanDataSize;
	SIGN32 chanMtuSize;
	SIGN32 chanQos;
	SIGN32 chanSelfLoop;
	SIGN32 chanEnable;
} DHUB_channel_config;

#endif /*__AVIO_DHUB_CFG_PRV_H__*/
