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

#ifndef _HAL_DHUB_FASTCALL_WRAP_H_
#define _HAL_DHUB_FASTCALL_WRAP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ion.h>

#include "ctypes.h"
#include "avio_type.h"
#include "hal_dhub.h"
#include "avioDhub.h"
#include "hal_dhub_wrap.h"
#include "avio_debug.h"

#include "tz_utils.h"

#define VPP_ENABLE_DIRECT_REG_ACCESS

#ifdef VPP_ENABLE_DIRECT_REG_ACCESS

#define VPP_ENABLE_FASTCALL_FOR_REG_ACCESS
//#define VPP_ENABLE_FASTCALL_FOR_REG_ACCESS_IN_NTZ
//#define VPP_ENABLE_FASTCALL_DEBUG_LOG

#ifdef VPP_ENABLE_FASTCALL_DEBUG_LOG
#define VPP_FASTCALL_DEBUG_LOG(...)	 avio_trace(__VA_ARGS__)
#else //!VPP_ENABLE_FASTCALL_DEBUG_LOG
#define VPP_FASTCALL_DEBUG_LOG(...)
#endif //!VPP_ENABLE_FASTCALL_DEBUG_LOG

void wrap_register_map_add_entry(uint32_t regPhyAddr,
		tz_secure_reg secureRegNdx);
UNSG32 wrap_fastcall_semaphore_chk_full(void *hdl, SIGN32 id);
void wrap_fastcall_semaphore_pop(void *hdl, SIGN32 id, SIGN32 delta);
void wrap_fastcall_semaphore_clr_full(void *hdl, SIGN32 id);
void wrap_fastcall_semaphore_intr_enable(void *hdl, SIGN32 id, SIGN32 empty,
	SIGN32 full, SIGN32 almostEmpty, SIGN32 almostFull, SIGN32 cpu);
#endif //VPP_ENABLE_DIRECT_REG_ACCESS

#endif //_HAL_DHUB_FASTCALL_WRAP_H_
