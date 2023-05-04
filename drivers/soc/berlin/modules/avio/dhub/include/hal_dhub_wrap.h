// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef HAL_DHUB_WRAP_H
#define HAL_DHUB_WRAP_H
/** DHUB_API wrap for normal and truzt zone
 */

#include "ctypes.h"
#include "avio_dhub_cfg.h"
#include "hal_dhub.h"

/*structure define*/

#ifdef __cplusplus
extern "C" {
#endif

void wrap_DhubInitialization(DHUB_ID dHubId, DHUB_TYPE dHubType, SIGN32 cpuId,
				UNSG32 dHubBaseAddr,
				UNSG32 hboSramAddr,
				HDL_dhub2d * pdhubHandle,
				DHUB_channel_config * dhub_config, SIGN32 numOfChans,
				UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr);

void wrap_DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

UNSG32 wrap_dhub_channel_write_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				SIGN32 id,	/*! Channel ID in $dHubReg ! */
				UNSG32 addr,	/*! CMD: buffer address ! */
				SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				T64b cfgQ[],	/*! Pass NULL to directly update dHub, or
						   * Pass non-zero to receive programming sequence
						   * in (adr,data) pairs !
						   */
				UNSG32 *ptr	/*! Pass in current cmdQ pointer (in 64b word),
						   * & receive updated new pointer,
						   * Pass NULL to read from HW !
						   */
	);

void wrap_dhub_channel_generate_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				SIGN32 id,	/*! Channel ID in $dHubReg ! */
				UNSG32 addr,	/*! CMD: buffer address ! */
				SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				SIGN32 *pData);

void *wrap_dhub_semaphore(void *hdl	/*  Handle to HDL_dhub */
	);

void wrap_semaphore_pop(void *hdl,	/*  Handle to HDL_semaphore */
				SIGN32 id,	/*  Semaphore ID in $SemaHub */
				SIGN32 delta	/*  Delta to pop as a consumer */
	);

void wrap_semaphore_clr_full(void *hdl,	/*  Handle to HDL_semaphore */
			   SIGN32 id	/*  Semaphore ID in $SemaHub */
	);

UNSG32 wrap_semaphore_chk_full(void *hdl,	/*Handle to HDL_semaphore */
			   SIGN32 id	/*Semaphore ID in $SemaHub
							   -1 to return all 32b of the interrupt status
							 */
	);

void wrap_semaphore_intr_enable(void *hdl,	/*! Handle to HDL_semaphore ! */
				SIGN32 id,	/*! Semaphore ID in $SemaHub ! */
				SIGN32 empty,	/*! Interrupt enable for CPU at condition 'empty' ! */
				SIGN32 full,	/*! Interrupt enable for CPU at condition 'full' ! */
				SIGN32 almostEmpty,	/*! Interrupt enable for CPU at condition 'almostEmpty' ! */
				SIGN32 almostFull,	/*! Interrupt enable for CPU at condition 'almostFull' ! */
				SIGN32 cpu	/*! CPU ID (0/1/2) ! */
	);
void wrap_DhubEnableAutoPush(bool enable,    /* enable/disable autopush command */
				bool useFastlogoTa,	/*! Flag to force use fastlogo.ta before usinsg dhub.ta ! */
				unsigned int frameRate/*! frame-rate in Hz, 60Hz/50Hz/etc. ! */
	);


#ifdef __cplusplus
}
#endif
/** DHUB_API Wrap
 */
#endif
/** ENDOFFILE: hal_dhub_wrap.h
 */
