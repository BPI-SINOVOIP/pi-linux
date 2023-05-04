// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#include "ispdhub_cfg.h"
#include "ispDhub.h"
#include "ispdhub_sema.h"

void isp_drv_dhub_config_ctx(void *h_dhub_ctx, void *ispss_base)
{
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)h_dhub_ctx;

	hDhubCtx->mod_base  = ispss_base;
	hDhubCtx->tsb_bcm_base   = ispss_base +
		ISPSS_MEMMAP_BCM_REG_BASE;
	hDhubCtx->dhub_base[ISP_DHUB_ID_TSB]  = ispss_base +
		ISPSS_MEMMAP_TSB_DHUB_REG_BASE + RA_ispDhubTSB_dHub0;
	hDhubCtx->sram_base[ISP_DHUB_ID_TSB]  = ispss_base +
		ISPSS_MEMMAP_TSB_DHUB_REG_BASE + RA_ispDhubTSB_tcm0;
	hDhubCtx->dhub_base[ISP_DHUB_ID_FWR]  = ispss_base +
		ISPSS_MEMMAP_FWR_DHUB_REG_BASE + RA_ispDhubFWR_dHub0;
	hDhubCtx->sram_base[ISP_DHUB_ID_FWR]  = ispss_base +
		ISPSS_MEMMAP_FWR_DHUB_REG_BASE + RA_ispDhubFWR_tcm0;

	hDhubCtx->ctl.pop_offset = RA_SemaHub_POP;
	hDhubCtx->ctl.full_offset = RA_SemaHub_full;
	hDhubCtx->ctl.cell_offset = RA_SemaHub_cell;
	hDhubCtx->ctl.cell_size = Semaphore_cell_size;

	hDhubCtx->fops.semaphore_chk_full = ispdhub_semaphore_chk_full;
	hDhubCtx->fops.semaphore_clr_full = ispdhub_semaphore_clr_full;
	hDhubCtx->fops.semaphore_pop = ispdhub_semaphore_pop;
}
