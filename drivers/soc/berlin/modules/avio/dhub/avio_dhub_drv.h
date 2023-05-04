// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __AVIO_DHUB_DRV_H__
#define __AVIO_DHUB_DRV_H__

#include "avioDhub.h"
#include "hal_dhub.h"
#include "avio_dhub_cfg.h"
#include "drv_dhub.h"

/******************************************************************************
 *	Function: DhubInitialization
 *	Description: Initialize DHUB .
 *	Parameter : cpuId ------------- cpu ID
 *			 dHubBaseAddr -------------  dHub Base address.
 *			 hboSramAddr ----- Sram Address for HBO.
 *			 pdhubHandle ----- pointer to 2D dhubHandle
 *			 dhub_config ----- configuration of AG
 *			 numOfChans	 ----- number of channels
 *	Return:		void
 ******************************************************************************/
void DhubInitialization(DHUB_ID dHubId, DHUB_TYPE dHubType, SIGN32 cpuId,
		UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
		HDL_dhub2d * pdhubHandle,
		DHUB_channel_config * dhub_config, SIGN32 numOfChans,
		UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

int Dhub_GetChannelInfo(HDL_dhub2d *pdhubHandle, SIGN32 IChannel,
			   T32dHubChannel_CFG *cfg);

DHUB_CONFIG_INFO *Dhub_GetConfigInfo(int nCfgNdx);
DHUB_CONFIG_INFO *Dhub_GetConfigInfo_ByDhubId(DHUB_ID dhub_id);
DHUB_CONFIG_INFO *Dhub_GetConfigInfo_ByDhub2dHandle(HDL_dhub2d *pdhubHandle);
DHUB_CONFIG_INFO *Dhub_GetConfigInfo_ByDhubHandle(HDL_dhub *pdhubHandle);
HDL_dhub2d *Dhub_GetDhub2dHandle_ByDhubId(DHUB_ID dhub_id);
HDL_dhub *Dhub_GetDhubHandle_ByDhubId(DHUB_ID dhub_id);

int Dhub_is_VPP_dhubHandle(void *hdl);
int Dhub_is_AG_dhubHandle(void *hdl);

void Dhub_IntrRegisterHandler(DHUB_ID dhub_id, UNSG32 intr_num,
		void *pArgs, DHUB_INTR_HANDLER intrHandler);

void Dhub_AddConfigInfo(DHUB_CTX *hDhubCtx, DHUB_ID dHubId, DHUB_TYPE dHubType,
		UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
		HDL_dhub2d *pdhubHandle,
		DHUB_channel_config *dhub_config, SIGN32 numOfChans,
		UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr,
		DHUB_HAL_FOPS *fops);

void Dhub_GetHalFops(int is_fastcall_fops, DHUB_HAL_FOPS *fops);
#endif //__AVIO_DHUB_DRV_H__
