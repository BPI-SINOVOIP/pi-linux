// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *      http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/kernel.h>

#include "ctypes.h"
#include "avio_io.h"

#include "avio_dhub_cfg.h"
#include "avio_dhub_cfg_prv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "avio_dhub_drv.h"
#include "drv_dhub.h"


/******************************************************************************************************************
 *   Function: dhub2nd_channel_cfg
 *   Description: Configurate a dHub2ND channel.
 *   Return:         UNSG32                      -   Number of (adr,pair) added to cfgQ
 ******************************************************************************************************************/
UNSG32  dhub2nd_channel_cfg(
	void *hdl,                 /*! Handle to HDL_dhub2d !*/
	SIGN32 id,                 /*! Channel ID in $dHubReg2D !*/
	UNSG32 addr,               /*! CMD: 2ND-buffer address !*/
	SIGN32 burst,              /*! CMD: line stride size in bytes !*/
	SIGN32 step1,              /*! CMD: buffer width in bytes !*/
	SIGN32 size1,              /*! CMD: buffer height in lines !*/
	SIGN32 step2,              /*! CMD: loop size (1~4) of semaphore operations !*/
	SIGN32 size2,              /*! CMD: semaphore operation at CMD/MTU (0/1) !*/
	SIGN32 chkSemId,           /*! CMD: semaphore loop pattern - non-zero to check !*/
	SIGN32 updSemId,           /*! CMD: semaphore loop pattern - non-zero to update !*/
	SIGN32 interrupt,          /*! CMD: raise interrupt at CMD finish !*/
	SIGN32 enable,             /*! 0 to disable, 1 to enable !*/
	T64b cfgQ[]				   /*! Pass NULL to directly init dHub2ND, or
								*Pass non-zero to receive programming sequence
								*in (adr,data) pairs
								*/
	)

{
	HDL_dhub2d *dhub2d = (HDL_dhub2d *)hdl;
	SIE_dHubCmd2ND cmd;
	SIGN32 semId_enable = 0;
	UNSG32 a, j = 0;

	T32dHubChannel_ROB_MAP stdHubChannelRob_Map;

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id * sizeof(SIE_dHubCmd2ND);
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_START, 0);

	stdHubChannelRob_Map.u32 = 0;
	if (0 != updSemId) {
		if (chkSemId & 0x1)/*Assuming all Luma channel will be odd and chroma will be even */
			stdHubChannelRob_Map.uROB_MAP_ID = 2;
		else
			stdHubChannelRob_Map.uROB_MAP_ID = 1;
	} else {
		stdHubChannelRob_Map.uROB_MAP_ID = 0;
	}
	a = dhub2d->ra + RA_dHubReg2D_dHub + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel) + RA_dHubChannel_ROB_MAP;
	IO32CFG(cfgQ, j, a, stdHubChannelRob_Map.u32);

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);

	cmd.uMEM_addr = addr;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_MEM, cmd.u32dHubCmd2ND_MEM);

	if (updSemId == 0)
		semId_enable = 0;
	else
		semId_enable = 1;

	cmd.u32dHubCmd2ND_DESC = 0;
	cmd.uDESC_burst = burst;
	cmd.uDESC_interrupt = interrupt;
	cmd.uDESC_chkSemId = chkSemId;
	cmd.uDESC_updSemId = updSemId;

	cmd.uDESC_ovrdQos = (semId_enable ? 1 : AVIO_DHUB_DESC_OVRDQOS);
	cmd.uDESC_disSem  = semId_enable;
	cmd.uDESC_qosSel  = (semId_enable ? 1 : AVIO_DHUB_DESC_QOSSEL);

	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC, cmd.u32dHubCmd2ND_DESC);

	cmd.uDESC_1D_ST_step = step1;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_1D_ST, cmd.u32dHubCmd2ND_DESC_1D_ST);
	cmd.uDESC_1D_SZ_size = size1;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_1D_SZ, cmd.u32dHubCmd2ND_DESC_1D_SZ);

	cmd.uDESC_2D_ST_step = step2;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_2D_ST, cmd.u32dHubCmd2ND_DESC_2D_ST);
	cmd.uDESC_2D_SZ_size = size2;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_2D_SZ, cmd.u32dHubCmd2ND_DESC_2D_SZ);

	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_START, enable);

	return j;
	/** ENDOFFUNCTION: dhub2nd_channel_cfg **/
}
EXPORT_SYMBOL(dhub2nd_channel_cfg);

/******************************************************************************************************************
 *   Function: dhub_channel_axqos
 *   Description: Configurate ax QoS channel.
 *   Return:         UNSG32                      -   Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 *                                                   0 if either cmdQ or dataQ in HBO is still busy
 ******************************************************************************************************************/
UNSG32  dhub_channel_axqos(
		void        *hdl,               /*! Handle to HDL_dhub !*/
		SIGN32      id,                 /*! Channel ID in $dHubReg !*/
		UNSG32      awQosLO,            /*! AWQOS value when low priority !*/
		UNSG32      awQosHI,            /*! AWQOS value when high priority !*/
		UNSG32      arQosLO,            /*! ARQOS value when low priority !*/
		UNSG32      arQosHI,            /*! ARQOS value when high priority !*/
		T64b        cfgQ[]              /*! Pass NULL to directly init dHub, or
						Pass non-zero to receive programming sequence
						in (adr,data) pairs
						!*/
		)
{
	HDL_dhub *dhub = (HDL_dhub*)hdl;
	T32dHubChannel_AWQOS awQos;
	T32dHubChannel_ARQOS arQos;
	UNSG32 i = 0, a;

	xdbg ("hal_dhub::  value of id is %0d \n" , id);

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);
	xdbg ("hal_dhub::  value of Channel Addr    is %0x \n" , a);

	awQos.u32 = 0; awQos.uAWQOS_LO = awQosLO; awQos.uAWQOS_HI = awQosHI;
	xdbg ("hal_dhub::  addr of ChannelCFG is %0x data is %0x \n" , a + RA_dHubChannel_AWQOS , awQos.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_AWQOS, awQos.u32);

	arQos.u32 = 0; arQos.uARQOS_LO = arQosLO; arQos.uARQOS_HI = arQosHI;
	xdbg ("hal_dhub::  addr of ChannelCFG is %0x data is %0x \n" , a + RA_dHubChannel_ARQOS , arQos.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_ARQOS, arQos.u32);

	return i;
	/** ENDOFFUNCTION: dhub_channel_axqos **/
}
