// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>

#include "avio_dhub_cfg.h"
#include "avio_dhub_cfg_prv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "avio_dhub_drv.h"
#include "tee_ca_dhub.h"

#define CPUINDEX    0

static HDL_dhub2d AG_dhubHandle;

DHUB_channel_config  AG_config[AG_NUM_OF_CHANNELS] = {
	// Bank0
	{ avioDhubChMap_aio64b_MA0_R, AG_DHUB_BANK0_START_ADDR, \
	  AG_DHUB_BANK0_START_ADDR+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MA1_R, AG_DHUB_BANK0_START_ADDR+512, \
	  AG_DHUB_BANK0_START_ADDR+512+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MA2_R, AG_DHUB_BANK0_START_ADDR+512*2, \
	  AG_DHUB_BANK0_START_ADDR+512*2+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MA3_R, AG_DHUB_BANK0_START_ADDR+512*3, \
	  AG_DHUB_BANK0_START_ADDR+512*3+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MIC1_W0, AG_DHUB_BANK0_START_ADDR+512*4, \
	  AG_DHUB_BANK0_START_ADDR+512*4+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MIC1_W1, AG_DHUB_BANK0_START_ADDR+512*5, \
	  AG_DHUB_BANK0_START_ADDR+512*5+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MIC1_PDM_W0, AG_DHUB_BANK0_START_ADDR+512*6, \
	  AG_DHUB_BANK0_START_ADDR+512*6+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_SEC0_R, AG_DHUB_BANK0_START_ADDR+512*7, \
	  AG_DHUB_BANK0_START_ADDR+512*7+32, 32, (2048-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_BCM_R, AG_DHUB_BANK0_START_ADDR+512*11, \
	  AG_DHUB_BANK0_START_ADDR+512*11+128, 128, (512-128), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MIC1_PDM_W1, AG_DHUB_BANK0_START_ADDR+512*12, \
	  AG_DHUB_BANK0_START_ADDR+512*12+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_PDM_W0, AG_DHUB_BANK0_START_ADDR+512*13, \
	  AG_DHUB_BANK0_START_ADDR+512*13+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_PDM_W1, AG_DHUB_BANK0_START_ADDR+512*14, \
	  AG_DHUB_BANK0_START_ADDR+512*14+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_MIC2_W0, AG_DHUB_BANK0_START_ADDR+512*15, \
	  AG_DHUB_BANK0_START_ADDR+512*15+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_reserved_9, AG_DHUB_BANK0_START_ADDR+512*16, \
	  AG_DHUB_BANK0_START_ADDR+512*16+32, 32, (512-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_SPDIF_R, AG_DHUB_BANK0_START_ADDR+1024*8+512, \
	  AG_DHUB_BANK0_START_ADDR+1024*8+512+32, 32, (2048-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
	{ avioDhubChMap_aio64b_SPDIF_W, AG_DHUB_BANK0_START_ADDR+1024*10+512, \
	  AG_DHUB_BANK0_START_ADDR+1024*10+512+32, 32, (2048-32), \
	  dHubChannel_CFG_MTU_128byte, 0, 0, 1},
};

int drv_dhub_initialize_dhub(void *h_dhub_ctx)
{
	static atomic_t dhub_init_done = ATOMIC_INIT(0);

	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	//Allow DHUB initialization only once
	if (atomic_cmpxchg(&dhub_init_done, 0, 1))
		return 0;

	DhubInitialization(DHUB_ID_AG_DHUB, DHUB_TYPE_64BIT, CPUINDEX, hDhubCtx->ag_dhub_base,
				hDhubCtx->ag_sram_base,
				&AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS,
				DHUB_TYPE_64BIT, hDhubCtx->vpp_bcm_base, 0);

	return 0;
}

void drv_dhub_config_ctx(void *h_dhub_ctx, UNSG32 avio_base)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	hDhubCtx->ag_dhub_base = avio_base +
				AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_dHub0;
	hDhubCtx->ag_sram_base = avio_base +
				AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_tcm0;
}
