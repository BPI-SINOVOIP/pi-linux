// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *        http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>

#include "avio_dhub_cfg.h"
#include "avio_dhub_cfg_prv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "avio_dhub_drv.h"
#include "hal_dhub_fastcall_wrap.h"
#include "tee_ca_dhub.h"
#include "drv_dhub.h"

extern DHUB_channel_config  VPP_config[];
extern DHUB_channel_config  AG_config[];
static HDL_dhub2d AG_dhubHandle;
static HDL_dhub2d VPP_dhubHandle;

#define CPUINDEX    0
#ifdef VPP_ENABLE_FASTCALL_FOR_REG_ACCESS

#define GET_VPP_DRV_SEMA_OFFSET(cpu_id, chan_id) \
		(RA_SemaHub_ARR + RA_Semaphore_INTR + \
			(cpu_id * sizeof(SIE_SemaINTR)) + \
			(chan_id * sizeof(SIE_Semaphore)))

static void wrap_fastcall_init(DHUB_CTX *hDhubCtx)
{
	uint32_t vpp_hdp_sema_off;
	uint32_t vpp_sema_addr;
	uint32_t vpp_hpd_intr;
	uint32_t phy_addr;

	vpp_sema_addr = (hDhubCtx->vpp_dhub_base + RA_dHubReg_SemaHub);
	vpp_hpd_intr = avioDhubSemMap_vpp128b_vpp_inr5;

	phy_addr = vpp_sema_addr + RA_SemaHub_full;
	wrap_register_map_add_entry(phy_addr, TZ_REG_SEM_CHK_FULL);

	phy_addr = vpp_sema_addr + RA_SemaHub_POP;
	wrap_register_map_add_entry(phy_addr, TZ_REG_SEM_POP);

	vpp_hdp_sema_off = GET_VPP_DRV_SEMA_OFFSET(0, vpp_hpd_intr);
	phy_addr = vpp_sema_addr + vpp_hdp_sema_off;
	wrap_register_map_add_entry(phy_addr, TZ_REG_SEM_INTR_ENABLE_1);

	vpp_hdp_sema_off = GET_VPP_DRV_SEMA_OFFSET(1, vpp_hpd_intr);
	phy_addr = vpp_sema_addr + vpp_hdp_sema_off;
	wrap_register_map_add_entry(phy_addr, TZ_REG_SEM_INTR_ENABLE_2);

	vpp_hdp_sema_off = GET_VPP_DRV_SEMA_OFFSET(2, vpp_hpd_intr);
	phy_addr = vpp_sema_addr + vpp_hdp_sema_off;
	wrap_register_map_add_entry(phy_addr, TZ_REG_SEM_INTR_ENABLE_3);
}

#else //VPP_ENABLE_FASTCALL_FOR_REG_ACCESS

#define wrap_fastcall_init(...)

#endif //VPP_ENABLE_FASTCALL_FOR_REG_ACCESS

static void drv_dhub_enable_fastcall(DHUB_ID dhub_id)
{
	DHUB_CONFIG_INFO *hDhubCfg;

	hDhubCfg = Dhub_GetConfigInfo_ByDhubId(dhub_id);
	if (!hDhubCfg) {
		avio_trace("%s:%d: DHUB not found : %d\n",
			__func__, __LINE__, dhub_id);
		return;
	}

	hDhubCfg->fops.semaphore_chk_full = wrap_fastcall_semaphore_chk_full;
	hDhubCfg->fops.semaphore_clr_full = wrap_fastcall_semaphore_clr_full;
	hDhubCfg->fops.semaphore_pop      = wrap_fastcall_semaphore_pop;
}


int drv_dhub_initialize_dhub(void *h_dhub_ctx)
{
	static atomic_t dhub_init_done = ATOMIC_INIT(0);

	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;
	DHUB_HAL_FOPS fops;

	//Allow DHUB initialization only once
	if (atomic_cmpxchg(&dhub_init_done, 0, 1))
		return 0;

	/* initialize dhub */
	if (hDhubCtx->isTeeEnabled)
		DhubInitialize();

	/*Disable Autopush before initialization of VPP DHUB*/
	wrap_DhubEnableAutoPush(false, true, hDhubCtx->fastlogo_framerate);

	wrap_DhubInitialization(DHUB_ID_VPP_DHUB, DHUB_TYPE_128BIT,
				CPUINDEX, hDhubCtx->vpp_dhub_base,
				hDhubCtx->vpp_sram_base, &VPP_dhubHandle,
				VPP_config, VPP_NUM_OF_CHANNELS,
				DHUB_TYPE_64BIT, hDhubCtx->vpp_bcm_base, 0);
	DhubInitialization(DHUB_ID_AG_DHUB, DHUB_TYPE_64BIT,
				CPUINDEX, hDhubCtx->ag_dhub_base,
				hDhubCtx->ag_sram_base,
				&AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS,
				DHUB_TYPE_64BIT, hDhubCtx->vpp_bcm_base, 0);

	//Register all wrap_DhubInit(tz_DhubInit) in local database also
	if (hDhubCtx->isTeeEnabled) {
		Dhub_GetHalFops(1, &fops);
		Dhub_AddConfigInfo(hDhubCtx, DHUB_ID_VPP_DHUB,
				DHUB_TYPE_128BIT, hDhubCtx->vpp_dhub_base,
				hDhubCtx->vpp_sram_base, &VPP_dhubHandle,
				VPP_config, VPP_NUM_OF_CHANNELS,
				DHUB_TYPE_64BIT, hDhubCtx->vpp_bcm_base,
				0, &fops);

		//Initialize HDL_dhub(base address of dHub.HBO, $dHub)
		// with a $dHub instance(HDL_dhub2d) - for fastcall support
		dhub2d_hdl(hDhubCtx->vpp_sram_base, hDhubCtx->vpp_dhub_base,
				&VPP_dhubHandle);

		wrap_fastcall_init(hDhubCtx);

		/*Enable fastcall for vpp dhub*/
		drv_dhub_enable_fastcall(DHUB_ID_VPP_DHUB);
	}

	return 0;
}

void drv_dhub_finalize_dhub(void *h_dhub_ctx)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	if (hDhubCtx->isTeeEnabled)
		DhubFinalize();
}

void drv_dhub_config_ctx(void *h_dhub_ctx, UNSG32 avio_base)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	hDhubCtx->vpp_bcm_base   = avio_base +
		AVIO_MEMMAP_AVIO_BCM_REG_BASE;
	hDhubCtx->vpp_dhub_base  = avio_base +
		AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_dHub0;
	hDhubCtx->ag_dhub_base   = avio_base +
		AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_dHub0;
	hDhubCtx->vpp_sram_base  = avio_base +
		AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_tcm0;
	hDhubCtx->ag_sram_base   = avio_base +
		AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_tcm0;
}
