// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */
#ifndef __AVIO_DHUB_CFG_PRV_H__
#define __AVIO_DHUB_CFG_PRV_H__
#define VPP_NUM_OF_CHANNELS		(avioDhubChMap_vpp128b_1DSCL_W0-1)
#define AG_NUM_OF_CHANNELS		(avioDhubChMap_aio64b_SPDIF_W+1)

#define VPP_DHUB_BANK0_START_ADDR	avioDhubTcmMap_vpp128bDhub_BANK0_START_ADDR
#define AG_DHUB_BANK0_START_ADDR	avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR

#define AVIO_DHUB_INITCHANNELAXQOS(pdhubHandle, dhub_config, chanId, i, cfgQ) \
	dhub_channel_axqos(                 \
			&pdhubHandle->dhub,         \
			chanId,                     \
			dhub_config[i].chanAxQosLO, \
			dhub_config[i].chanAxQosHI, \
			dhub_config[i].chanAxQosLO, \
			dhub_config[i].chanAxQosHI, \
			cfgQ                        \
			);

#define AVIO_DHUB_MTR_CFG_QOS_EN	1
#define AVIO_DHUB_MTR_CFG_QOS		0xF
#define AVIO_DHUB_DESC_OVRDQOS		1
#define AVIO_DHUB_DESC_QOSSEL		1

#define IS_SINGLE_DISPLAY()         1

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
	UNSG32 chanAxQosLO;
	UNSG32 chanAxQosHI;
} DHUB_channel_config;

#endif /*__AVIO_DHUB_CFG_PRV_H__*/
