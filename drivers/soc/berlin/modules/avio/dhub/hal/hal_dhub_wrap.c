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

#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/delay.h>

#include "avio.h"
#include "avio_io.h"
#include "hal_dhub_wrap.h"
#include "avio_dhub_drv.h"
#include "tee_ca_dhub.h"
#include "drv_dhub.h"

#define AUTOPUSH_MAX_FRAME_DELAY                5

void wrap_DhubInitialization(DHUB_ID dHubId, DHUB_TYPE dHubType, SIGN32 cpuId,
				 UNSG32 dHubBaseAddr,
				 UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle,
				 DHUB_channel_config *dhub_config, SIGN32 numOfChans,
				 UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (dHubBaseAddr == hDhubCtx->vpp_dhub_base)
			tz_DhubInitialization(cpuId, dHubBaseAddr, hboSramAddr, 0, 0,
						  numOfChans);
		else
			DhubInitialization(dHubId, dHubType, cpuId, dHubBaseAddr, hboSramAddr,
						pdhubHandle, dhub_config, numOfChans,
						bcmDhubType, bcmBaseAddr, bcmDummyRegAddr);
	} else {
		DhubInitialization(dHubId, dHubType, cpuId, dHubBaseAddr, hboSramAddr, pdhubHandle,
					dhub_config, numOfChans,
					bcmDhubType, bcmBaseAddr, bcmDummyRegAddr);
	}
}

void wrap_DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			tz_DhubChannelClear(hdl, id, cfgQ);
		else
			DhubChannelClear(hdl, id, cfgQ);
	} else {
		DhubChannelClear(hdl, id, cfgQ);
	}
}

UNSG32 wrap_dhub_channel_write_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
					SIGN32 id,	/*! Channel ID in $dHubReg ! */
					UNSG32 addr,	/*! CMD: buffer address ! */
					SIGN32 size,	/*! CMD: number of bytes to transfer ! */
					SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
					SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
					SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
					SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
					T64b cfgQ[],/*! Pass NULL to directly update dHub, or
							     * Pass non-zero to receive programming sequence
							     * in (adr,data) pairs
							     */
					UNSG32 *ptr/*! Pass in current cmdQ pointer (in 64b word),
							     * & receive updated new pointer,
							     * Pass NULL to read from HW
							     */
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			return tz_dhub_channel_write_cmd(hdl, id, addr, size, semOnMTU,
							 chkSemId, updSemId, interrupt,
							 cfgQ, ptr);
		else
			return dhub_channel_write_cmd(hdl, id, addr, size, semOnMTU,
							  chkSemId, updSemId, interrupt,
							  cfgQ, ptr);
	} else {
		return dhub_channel_write_cmd(hdl, id, addr, size, semOnMTU, chkSemId,
						  updSemId, interrupt, cfgQ, ptr);
	}
}

void wrap_dhub_channel_generate_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
					SIGN32 id,	/*! Channel ID in $dHubReg ! */
					UNSG32 addr,	/*! CMD: buffer address ! */
					SIGN32 size,	/*! CMD: number of bytes to transfer ! */
					SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
					SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
					SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
					SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
					SIGN32 *pData)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			tz_dhub_channel_generate_cmd(hdl, id, addr, size, semOnMTU,
							 chkSemId, updSemId, interrupt,
							 pData);
		else
			dhub_channel_generate_cmd(hdl, id, addr, size, semOnMTU,
						  chkSemId, updSemId, interrupt, pData);
	} else {
		dhub_channel_generate_cmd(hdl, id, addr, size, semOnMTU, chkSemId,
					  updSemId, interrupt, pData);
	}
}

void *wrap_dhub_semaphore(void *hdl	/*  Handle to HDL_dhub */
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			return hdl;
		else
			return dhub_semaphore(hdl);
	} else {
		return dhub_semaphore(hdl);
	}
}

void wrap_semaphore_pop(void *hdl,	/*  Handle to HDL_semaphore */
			SIGN32 id,	/*  Semaphore ID in $SemaHub */
			SIGN32 delta	/*  Delta to pop as a consumer */
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			tz_semaphore_pop(hdl, id, delta);
		else
			semaphore_pop(hdl, id, delta);
	} else {
		semaphore_pop(hdl, id, delta);
	}
}

void wrap_semaphore_clr_full(void *hdl,	/*  Handle to HDL_semaphore */
				 SIGN32 id	/*  Semaphore ID in $SemaHub */
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			tz_semaphore_clr_full(hdl, id);
		else
			semaphore_clr_full(hdl, id);
	} else {
		semaphore_clr_full(hdl, id);
	}
}

UNSG32 wrap_semaphore_chk_full(void *hdl,	/*Handle to HDL_semaphore */
					SIGN32 id	/*Semaphore ID in $SemaHub
								*-1 to return all 32b of the interrupt status
								*/
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			return tz_semaphore_chk_full(hdl, id);
		else
			return semaphore_chk_full(hdl, id);
	} else {
		return semaphore_chk_full(hdl, id);
	}
}

void wrap_semaphore_intr_enable(void *hdl,	/*! Handle to HDL_semaphore ! */
				SIGN32 id,	/*! Semaphore ID in $SemaHub ! */
				SIGN32 empty,	/*! Interrupt enable for CPU at condition 'empty' ! */
				SIGN32 full,	/*! Interrupt enable for CPU at condition 'full' ! */
				SIGN32 almostEmpty,	/*! Interrupt enable for CPU at condition 'almostEmpty' ! */
				SIGN32 almostFull,	/*! Interrupt enable for CPU at condition 'almostFull' ! */
				SIGN32 cpu	/*! CPU ID (0/1/2) ! */
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled) {
		if (Dhub_is_VPP_dhubHandle(hdl))
			tz_semaphore_intr_enable(hdl, id, empty, full, almostEmpty,
						 almostFull, cpu);
		else
			semaphore_intr_enable(hdl, id, empty, full, almostEmpty,
						  almostFull, cpu);
	} else {
		semaphore_intr_enable(hdl, id, empty, full, almostEmpty, almostFull,
					  cpu);
	}
}

void wrap_DhubEnableAutoPush(bool enable,    /* enable/disable autopush command */
				bool useFastlogoTa,	/*! Flag to force use fastlogo.ta before usinsg dhub.ta ! */
				unsigned int frameRate/*! frame-rate in Hz, 60Hz/50Hz/etc. ! */
	)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled)
		DhubEnableAutoPush(enable, useFastlogoTa);
	else
		AVIO_REG_WORD32_WRITE(hDhubCtx->vpp_bcm_base + RA_AVIO_BCM_AUTOPUSH, enable); //disable h/w autopush on suspend

	/* Wait for 1-2 frames for the ongoing BCM command to complete */
	if (frameRate > 0)
		msleep((1000 / frameRate) * AUTOPUSH_MAX_FRAME_DELAY);
}

EXPORT_SYMBOL(wrap_dhub_semaphore);
EXPORT_SYMBOL(wrap_DhubInitialization);
EXPORT_SYMBOL(wrap_semaphore_pop);
EXPORT_SYMBOL(wrap_dhub_channel_write_cmd);
EXPORT_SYMBOL(wrap_semaphore_chk_full);
EXPORT_SYMBOL(wrap_semaphore_clr_full);
EXPORT_SYMBOL(wrap_DhubEnableAutoPush);
