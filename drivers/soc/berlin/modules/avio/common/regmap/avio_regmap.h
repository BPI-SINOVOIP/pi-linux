// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _AVIO_REGMAP_H__
#define _AVIO_REGMAP_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>

#include "avio_debug.h"
#include "avio_type.h"
#include "avio_io.h"
#include "regmap_base.h"

int avio_create_devioremap(struct platform_device *pdev);

int avio_destroy_deviounmap(void);

#ifdef AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION
void *avio_memmap_phy_to_vir(unsigned int phyaddr);

unsigned int avio_memmap_vir_to_phy(void *vir_addr);
#endif

int avio_driver_memmap(struct file *file, struct vm_area_struct *vma);

#endif //_AVIO_REGMAP_H__


