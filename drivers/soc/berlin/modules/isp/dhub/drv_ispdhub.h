// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#ifndef _DRV_ISP_DHUB_H_
#define _DRV_ISP_DHUB_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>

int isp_dhub_mod_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_exit(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_open(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_close(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_suspend(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_resume(isp_device *isp_dev, isp_module_ctx *mod_ctx);
long isp_dhub_mod_ioctl(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg);

#endif //_DRV_ISP_DHUB_H_

