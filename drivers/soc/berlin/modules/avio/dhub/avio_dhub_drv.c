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

#include <linux/module.h>
#include <linux/kernel.h>

#include "avioDhub.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "hal_dhub_fastcall_wrap.h"
#include "avio_dhub_drv.h"
#include "avio_dhub_cfg_prv.h"
#include "drv_dhub.h"

void Dhub_GetHalFops(int is_fastcall_fops, DHUB_HAL_FOPS *fops)
{
	if (!fops)
		return;

	if (is_fastcall_fops) {
		fops->semaphore_chk_full = wrap_fastcall_semaphore_chk_full;
		fops->semaphore_clr_full = wrap_fastcall_semaphore_clr_full;
		fops->semaphore_pop      = wrap_fastcall_semaphore_pop;
	} else {
		fops->semaphore_chk_full = wrap_semaphore_chk_full;
		fops->semaphore_clr_full = wrap_semaphore_clr_full;
		fops->semaphore_pop      = wrap_semaphore_pop;
	}
}

void Dhub_AddConfigInfo(DHUB_CTX *hDhubCtx, DHUB_ID dHubId, DHUB_TYPE dHubType,
	UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle,
	DHUB_channel_config *dhub_config, SIGN32 numOfChans,
	UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr,
	DHUB_HAL_FOPS *fops)
{
	int cfg_count = hDhubCtx->dhub_config_count;

	if (fops && ((cfg_count + 1) < DHUB_ID_MAX)) {
		DHUB_CONFIG_INFO *p_dhub_cfg_info;
		DHUB_HAL_FOPS *p_cfg_fops;

		p_dhub_cfg_info = &hDhubCtx->dhub_config_info[cfg_count];
		p_cfg_fops = &p_dhub_cfg_info->fops;

		p_dhub_cfg_info->dHubId = dHubId;
		p_dhub_cfg_info->dHubType = dHubType;
		p_dhub_cfg_info->dHubBaseAddr = dHubBaseAddr;
		p_dhub_cfg_info->hboSramAddr = hboSramAddr;
		p_dhub_cfg_info->numOfChans = numOfChans;
		p_dhub_cfg_info->pDhubHandle = pdhubHandle;
		p_dhub_cfg_info->pDhubConfig = dhub_config;
		p_dhub_cfg_info->bcmInfo.bcmDhubType = bcmDhubType;
		p_dhub_cfg_info->bcmInfo.bcmBaseAddr = bcmBaseAddr;
		p_dhub_cfg_info->bcmInfo.bcmDummyRegAddr = bcmDummyRegAddr;

		p_cfg_fops->semaphore_chk_full = fops->semaphore_chk_full;
		p_cfg_fops->semaphore_clr_full = fops->semaphore_clr_full;
		p_cfg_fops->semaphore_pop      = fops->semaphore_pop;

		hDhubCtx->dhub_config_count++;
	}
}

static int Dhub_Match_DhubID(void *hdl, DHUB_ID dhub_id)
{
	DHUB_CONFIG_INFO *dhub_cfg_info;
	int ret = 0;

	dhub_cfg_info = Dhub_GetConfigInfo_ByDhubHandle(hdl);

	if (dhub_cfg_info)
		ret = (dhub_cfg_info->dHubId == dhub_id);

	return ret;
}

static int getDhubCMDDiv(DHUB_TYPE dHubType)
{
	int divider;

	switch (dHubType) {
	case DHUB_TYPE_128BIT:
		divider = 16;
		break;
	case DHUB_TYPE_64BIT:
	default:
		divider = 8;
		break;
	}

	return divider;
}


int Dhub_is_VPP_dhubHandle(void *hdl)
{
	return Dhub_Match_DhubID(hdl, DHUB_ID_VPP_DHUB);
}

int Dhub_is_AG_dhubHandle(void *hdl)
{
	return Dhub_Match_DhubID(hdl, DHUB_ID_AG_DHUB);
}

/******************************************************************************
 *	  Function: GetChannelInfo
 *	  Description: Get the Dhub configuration of requested channel.
 *	  Parameter : pdhubHandle ----- pointer to 2D dhubHandle
 *			 IChannel		----- Channel of the dhub
 *			 cfg		----- Configuration need to be updated here.
 *	  Return:  0 	----	Success
 ******************************************************************************/
int Dhub_GetChannelInfo(HDL_dhub2d *pdhubHandle, SIGN32 IChannel,
			   T32dHubChannel_CFG *cfg)
{
	DHUB_CONFIG_INFO *dhub_cfg_info;
	int ret = 0;

	dhub_cfg_info = Dhub_GetConfigInfo_ByDhub2dHandle(pdhubHandle);
	if (dhub_cfg_info) {
		DHUB_channel_config *dhub_config;

		dhub_config = dhub_cfg_info->pDhubConfig;

		//Update the MTU, QOS and self loop paramteres.
		cfg->uCFG_MTU = dhub_config[IChannel].chanMtuSize;
		cfg->uCFG_QoS = dhub_config[IChannel].chanQos;
		cfg->uCFG_selfLoop = dhub_config[IChannel].chanSelfLoop;
		ret = -1;
	}

	return ret;
}

DHUB_CONFIG_INFO *Dhub_GetConfigInfo(int nCfgNdx)
{
	DHUB_CONFIG_INFO *p_dhub_config_info = NULL;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if ((hDhubCtx->dhub_config_count + 1) < DHUB_ID_MAX)
		p_dhub_config_info = &hDhubCtx->dhub_config_info[nCfgNdx];

	return p_dhub_config_info;
}

DHUB_CONFIG_INFO *Dhub_GetConfigInfo_ByDhubId(DHUB_ID dhub_id)
{
	DHUB_CONFIG_INFO *p_dhub_config_info;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int i;

	for (i = 0; i < hDhubCtx->dhub_config_count; i++) {
		p_dhub_config_info = &hDhubCtx->dhub_config_info[i];
		if ((p_dhub_config_info->dHubId) == dhub_id)
			return p_dhub_config_info;
	}
	return NULL;
}

HDL_dhub2d *Dhub_GetDhub2dHandle_ByDhubId(DHUB_ID dhub_id)
{
	DHUB_CONFIG_INFO *p_dhub_config_info;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int i;

	for (i = 0; i < hDhubCtx->dhub_config_count; i++) {
		p_dhub_config_info = &hDhubCtx->dhub_config_info[i];
		if ((p_dhub_config_info->dHubId) == dhub_id)
			return p_dhub_config_info->pDhubHandle;
	}

	return NULL;
}

HDL_dhub *Dhub_GetDhubHandle_ByDhubId(DHUB_ID dhub_id)
{
	DHUB_CONFIG_INFO *p_dhub_config_info = NULL;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int i;

	for (i = 0; i < hDhubCtx->dhub_config_count; i++) {
		p_dhub_config_info = &hDhubCtx->dhub_config_info[i];
		if ((p_dhub_config_info->dHubId) == dhub_id)
			return &p_dhub_config_info->pDhubHandle->dhub;
	}

	return NULL;
}

DHUB_CONFIG_INFO *Dhub_GetConfigInfo_ByDhub2dHandle(HDL_dhub2d *pdhubHandle)
{
	DHUB_CONFIG_INFO *p_dhub_config_info;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int i;

	for (i = 0; i < hDhubCtx->dhub_config_count; i++) {
		p_dhub_config_info = &hDhubCtx->dhub_config_info[i];
		if ((p_dhub_config_info->pDhubHandle) == pdhubHandle)
			return p_dhub_config_info;
	}

	return NULL;
}

DHUB_CONFIG_INFO *Dhub_GetConfigInfo_ByDhubHandle(HDL_dhub *pdhubHandle)
{
	DHUB_CONFIG_INFO *p_dhub_config_info;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int i;

	for (i = 0; i < hDhubCtx->dhub_config_count; i++) {
		p_dhub_config_info = &hDhubCtx->dhub_config_info[i];
		if (&(p_dhub_config_info->pDhubHandle->dhub) == pdhubHandle)
			return p_dhub_config_info;
	}

	return NULL;
}

static void Dhub_IntrUnRegisterHandler(DHUB_ID dhub_id, UNSG32 intr_num)
{
	DHUB_CONFIG_INFO *p_cfg_info;
	DHUB_INTR_HANDLER_INFO *p_intr_info;
	UNSG32 intr_mask = (1 << intr_num);

	//Find the entry
	p_cfg_info = Dhub_GetConfigInfo_ByDhubId(dhub_id);
	p_intr_info = &p_cfg_info->intrHandler[intr_num];

	if (p_cfg_info->intrMask & intr_mask) {
		//Already registered, then allow un-registration

		//Remove the entry
		p_intr_info->pIntrHandler = NULL;
		p_intr_info->pIntrHandlerArgs = NULL;
		p_intr_info->intrMask = 0;
		p_intr_info->intrNum = 0;
		p_cfg_info->intrMask &= ~intr_mask;

		//Decrement the ISR count for only removed entry
		p_cfg_info->intrHandlerCount--;

		avio_debug("%s:%d Removed from DHUB-%d/%d : %d / %x\n",
			__func__, __LINE__, dhub_id,
			p_cfg_info->intrHandlerCount, intr_num, intr_mask);
	}
}

void Dhub_IntrRegisterHandler(DHUB_ID dhub_id, UNSG32 intr_num,
			void *pArgs, DHUB_INTR_HANDLER intrHandler)
{
	DHUB_CONFIG_INFO *p_cfg_info;
	DHUB_INTR_HANDLER_INFO *p_intr_info;
	UNSG32 intr_mask = (1 << intr_num);
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	p_cfg_info = Dhub_GetConfigInfo_ByDhubId(dhub_id);
	if (!p_cfg_info || (intr_num >= DHUB_INTR_HANDLER_MAX))
		return;

	spin_lock(&hDhubCtx->dhub_cfg_spinlock);

	p_intr_info = &p_cfg_info->intrHandler[intr_num];
	if (!(p_cfg_info->intrMask & intr_mask)) {

		//Add/OR the entry
		p_intr_info->pIntrHandler = intrHandler;
		p_intr_info->pIntrHandlerArgs = pArgs;
		p_intr_info->intrMask |= intr_mask;
		p_intr_info->intrNum = intr_num;
		p_cfg_info->intrMask |= intr_mask;

		//increment the ISR count for only new entry
		p_cfg_info->intrHandlerCount++;

		avio_debug("%s:%d Added to DHUB-%d/%d : %d / %x\n",
			__func__, __LINE__, dhub_id,
			p_cfg_info->intrHandlerCount, intr_num, intr_mask);

	} else if (!intrHandler) {

		Dhub_IntrUnRegisterHandler(dhub_id, intr_num);

	}

	spin_unlock(&hDhubCtx->dhub_cfg_spinlock);
}

/*******************************************************************************
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
		UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle,
		DHUB_channel_config *dhub_config, SIGN32 numOfChans,
		UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr)
{
	HDL_semaphore *pSemHandle;
	SIGN32 i;
	SIGN32 chanId;
	SIGN32 cmdDiv = 8;
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	DHUB_HAL_FOPS fops;

	Dhub_GetHalFops(0, &fops);
	Dhub_AddConfigInfo(hDhubCtx, dHubId, dHubType,
						dHubBaseAddr, hboSramAddr, pdhubHandle,
						dhub_config, numOfChans,
						bcmDhubType, bcmBaseAddr, bcmDummyRegAddr, &fops);

	cmdDiv = getDhubCMDDiv(dHubType);
	if (!cmdDiv)
		return;

	//Initialize HDL_dhub with a $dHub BIU instance.
	dhub2d_hdl(hboSramAddr,	/*!	 Base address of dHub.HBO SRAM ! */
		   dHubBaseAddr,	/*!	 Base address of a BIU instance of $dHub ! */
		   pdhubHandle	/*!	 Handle to HDL_dhub2d ! */
		);
	//set up semaphore to trigger cmd done interrupt
	//note that this set of semaphores are different from the HBO semaphores
	//the ID must match the dhub ID because they are hardwired.
	pSemHandle = dhub_semaphore(&pdhubHandle->dhub);

	for (i = 0; i < numOfChans; i++) {
		//Configurate a dHub channel
		//note that in this function, it also configured right HBO channels(cmdQ and dataQ) and semaphores
		chanId = dhub_config[i].chanId;
		AVIO_DHUB_INITCHANNELAXQOS(pdhubHandle, dhub_config, chanId, i, 0);
		dhub_channel_cfg(&pdhubHandle->dhub,	/*!	 Handle to HDL_dhub ! */
			 chanId,	/*!	 Channel ID in $dHubReg ! */
			 dhub_config[i].chanCmdBase,	//UNSG32 baseCmd,	 /*Channel FIFO base address (byte address) for cmdQ !*/
			 dhub_config[i].chanDataBase,	//UNSG32 baseData,	 /*!Channel FIFO base address (byte address) for dataQ !*/
			 dhub_config[i].chanCmdSize / cmdDiv,	//SIGN32	depthCmd,	/*!	 Channel FIFO depth for cmdQ, in 64b word !*/
			 dhub_config[i].chanDataSize / cmdDiv,	//SIGN32	depthData,	/*!	 Channel FIFO depth for dataQ, in 64b word !*/
			 dhub_config[i].chanMtuSize,	/*!	 See 'dHubChannel.CFG.MTU', 0/1/2 for 8/32/128 bytes ! */
			 dhub_config[i].chanQos,	/*!	 See 'dHubChannel.CFG.QoS' ! */
			 dhub_config[i].chanSelfLoop,	/*!	 See 'dHubChannel.CFG.selfLoop' ! */
			 dhub_config[i].chanEnable,	/*!	 0 to disable, 1 to enable ! */
			 0	/*!	 Pass NULL to directly init dHub, or
				 *	   Pass non-zero to receive programming sequence
				 *	   in (adr,data) pairs
				 *	   ! */
		);
		// setup interrupt for channel chanId
		//configure the semaphore depth to be 1
		semaphore_cfg(pSemHandle, chanId, 1, 0);
	}
}

/******************************************************************************
 *	Function: DhubChannelClear
 *	Description: Clear corresponding DHUB channel.
 *	Parameter:	hdl  ---------- handle to HDL_dhub
 *			id   ---------- channel ID in dHubReg
 *			cfgQ ---------- pass null to directly init dhub, or pass non-zero to receive programming
 *					sequence in (adr, data) pairs
 *	Return:		void
 ******************************************************************************/
void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id);
	UNSG32 dataID = dhub_id2hbo_data(id);
	HDL_dhub *dhub = (HDL_dhub *) hdl;
	HDL_hbo *hbo = &(dhub->hbo);

	/* 1.Software stops the command queue in HBO (please refer to HBO.sxw for details) */
	hbo_queue_enable(hbo, cmdID, 0, cfgQ);
	/* 2.Software stops the channel in dHub by writing zero to  dHubChannel.START.EN */
	dhub_channel_enable(dhub, id, 0, cfgQ);
	/* 3.Software clears the channel in dHub by writing one to  dHubChannel.CLEAR.EN */
	dhub_channel_clear(dhub, id);
	/* 4.Software waits for the register bits dHubChannel.PENDING.ST and dHubChannel.BUSY.ST to be 0 */
	dhub_channel_clear_done(dhub, id);
	/* 5.Software stops and clears the command queue */
	hbo_queue_enable(hbo, cmdID, 0, cfgQ);
	hbo_queue_clear(hbo, cmdID);
	/* 6.Software wait for the corresponding busy bit to be 0 */
	hbo_queue_clear_done(hbo, cmdID);
	/* 7.Software stops and clears the data queue */
	hbo_queue_enable(hbo, dataID, 0, cfgQ);
	hbo_queue_clear(hbo, dataID);
	/* 8.Software wait for the corresponding data Q busy bit to be 0 */
	hbo_queue_clear_done(hbo, dataID);
	/* 9.Software enable dHub and HBO */
	dhub_channel_enable(dhub, id, 1, cfgQ);
	hbo_queue_enable(hbo, cmdID, 1, cfgQ);
	hbo_queue_enable(hbo, dataID, 1, cfgQ);
}

EXPORT_SYMBOL(DhubChannelClear);
EXPORT_SYMBOL(Dhub_GetDhub2dHandle_ByDhubId);
EXPORT_SYMBOL(Dhub_GetDhubHandle_ByDhubId);
