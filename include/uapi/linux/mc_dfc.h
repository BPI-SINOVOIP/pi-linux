/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright (C) 2021 Synaptics Incorporated
*
* Author: Benson Gui <Benson.Gui@synaptics.com>
*
*/
#ifndef _UAPI_MC_DFC_H
#define _UAPI_MC_DFC_H

#include <linux/ioctl.h>

#define DFC_IOC_MAGIC		'D'
#define DFC_IOC_PMU_SW		_IO(DFC_IOC_MAGIC, 1)

#endif /* _UAPI_MC_DFC_H */
