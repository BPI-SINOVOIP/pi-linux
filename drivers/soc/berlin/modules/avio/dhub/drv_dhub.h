// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _DRV_DHUB_H_
#define _DRV_DHUB_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/printk.h>

#include "avio_debug.h"
#include "avio_type.h"
#include "avio_dhub_cfg.h"
#include "avio_sub_module.h"

typedef struct _DHUB_CONTEXT_ {
	unsigned char isTeeEnabled;

	/* Range of physical memory to be mapped for DHUB*/
	UINT32 vpp_dhub_base;
	UINT32 ag_dhub_base;
	UINT32 vpp_sram_base;
	UINT32 ag_sram_base;
	UINT32 vpp_bcm_base;

	DHUB_CONFIG_INFO dhub_config_info[DHUB_ID_MAX];
	UNSG32 dhub_config_count;
	UNSG32 irq_num[DHUB_ID_MAX];
	UNSG32 fastlogo_framerate;

	spinlock_t dhub_cfg_spinlock;
} DHUB_CTX;

int avio_module_drv_dhub_probe(struct platform_device *dev);

#endif //_DRV_DHUB_H_

