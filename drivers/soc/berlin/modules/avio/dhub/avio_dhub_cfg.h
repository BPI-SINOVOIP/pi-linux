// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef __AVIO_DHUB_CFG_H__
#define __AVIO_DHUB_CFG_H__

#include "avioDhub.h"
#include "hal_dhub.h"
#include "avio_dhub_cfg_prv.h"

#define DHUB_INTR_HANDLER_MAX 32

typedef int (*DHUB_INTR_HANDLER)(UNSG32 intrMask, void *pArgs);

typedef enum DHUB_128bBCM_TYPE_s {
	DHUB_128bBCM_TYPE_64b,
	DHUB_128bBCM_TYPE_128b,
} DHUB_128bBCM_TYPE;

typedef enum _DHUB_TYPE_ {
	DHUB_TYPE_64BIT,
	DHUB_TYPE_128BIT,
} DHUB_TYPE;

typedef enum _DHUB_ID_ {
	DHUB_ID_AG_DHUB,
	DHUB_ID_VPP_DHUB,
	DHUB_ID_VIP_DHUB,
	DHUB_ID_AIP_DHUB,
	DHUB_ID_MAX,
} DHUB_ID;

typedef struct DHUB_BCM_INFO_s {
	//0:64b, 1:128b(Add filler to MSB-64b as they are ignored)
	UNSG32 bcmDhubType;
	//Base address of the BCM register space
	UNSG32 bcmBaseAddr;
	//Dummy register write to add for odd size BCM buffer
	UNSG32 bcmDummyRegAddr;
} DHUB_BCM_INFO;

typedef struct DHUB_INTR_HANDLER_INFO_s {
	DHUB_INTR_HANDLER pIntrHandler;
	void *pIntrHandlerArgs;
	UNSG32 intrMask;
	UNSG32 intrNum;
} DHUB_INTR_HANDLER_INFO;

typedef struct DHUB_HAL_FOPS_s {
	UNSG32 (*semaphore_chk_full)(void *hdl, SIGN32 id);
	void (*semaphore_clr_full)(void *hdl, SIGN32 id);
	void (*semaphore_pop)(void *hdl, SIGN32 id, SIGN32 delta);
} DHUB_HAL_FOPS;

typedef struct DHUB_CONFIG_INFO_s {
	DHUB_ID dHubId;
	DHUB_TYPE dHubType;
	UNSG32 dHubBaseAddr;
	UNSG32 hboSramAddr;
	SIGN32 numOfChans;
	DHUB_BCM_INFO bcmInfo;

	HDL_dhub2d *pDhubHandle;
	DHUB_channel_config *pDhubConfig;

	DHUB_HAL_FOPS fops;

	UNSG32 irq_num;
	int IsrRegistered;
	DHUB_INTR_HANDLER_INFO intrHandler[DHUB_INTR_HANDLER_MAX];
	UNSG32 intrHandlerCount;
	UNSG32 intrMask;
} DHUB_CONFIG_INFO;

int drv_dhub_initialize_dhub(void *h_dhub_ctx);
void drv_dhub_finalize_dhub(void *h_dhub_ctx);
void drv_dhub_config_ctx(void *h_dhub_ctx, UNSG32 avio_base);

#endif //__AVIO_DHUB_CFG_H__
