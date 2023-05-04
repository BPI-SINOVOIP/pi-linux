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

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/timekeeping.h>

#include "avioDhub.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "hal_dhub_fastcall_wrap.h"

#include "drv_dhub.h"

#ifdef VPP_ENABLE_DIRECT_REG_ACCESS

#ifdef VPP_ENABLE_FASTCALL_FOR_REG_ACCESS

typedef struct _tz_secure_regmap_ {
	uint32_t regPhyAddr;
	tz_secure_reg secureRegNdx;
} tz_secure_regmap;

static tz_secure_regmap addr_to_ndx_map_list[TZ_REG_MAX];
static int addr_to_ndx_map_count;

void wrap_register_map_add_entry(uint32_t regPhyAddr,
		tz_secure_reg secureRegNdx)
{
	if (addr_to_ndx_map_count < TZ_REG_MAX) {
		int ndx = addr_to_ndx_map_count;

		addr_to_ndx_map_list[ndx].regPhyAddr = regPhyAddr;
		addr_to_ndx_map_list[ndx].secureRegNdx = secureRegNdx;
		addr_to_ndx_map_count++;
	}
}

static tz_secure_reg tz_phy_to_secure_reg(uint32_t regAddr)
{
	tz_secure_reg secureReg = TZ_REG_MAX;
	int i;

	for (i = 0; i < TZ_REG_MAX; i++) {
		if (addr_to_ndx_map_list[i].regPhyAddr == regAddr) {
			secureReg = addr_to_ndx_map_list[i].secureRegNdx;
			break;
		}
	}

	VPP_FASTCALL_DEBUG_LOG("%s:%d: regAddr:%x, secureReg:%d\n",
		__func__, __LINE__, regAddr, secureReg);

	return secureReg;
}

static void wrap_tz_secure_reg_rw(tz_secure_reg reg,
			uint32_t ops, uint32_t *value)
{
	if ((reg >= 0) && (reg < TZ_REG_MAX)) {
		int ret;

		//Valid register, so access it
		ret = tz_secure_reg_rw(reg, ops, value);
		VPP_FASTCALL_DEBUG_LOG(
			"%s:%d: reg:%x, ops:%d, val:%x, ret:0x%x/%d\n",
			__func__, __LINE__, reg, ops, *value, ret, ret);
		if (ret < 0)
			avio_trace(
				"%s:%d:ERR:  reg:%x, ops:%d, val:%x, ret:0x%x/%d\n",
				__func__, __LINE__, reg, ops, *value, ret, ret);
	} else {
		//Invalid register, log error
		avio_trace("%s:%d:INVALID: reg:%x, ops:%d, val:%x\n",
			__func__, __LINE__, reg, ops, *value);
	}
}

static void wrap_register_read(uint32_t regAddr, uint32_t *pRegVal)
{
	wrap_tz_secure_reg_rw(tz_phy_to_secure_reg(regAddr),
		TZ_SECURE_REG_READ, pRegVal);
}

static void wrap_register_write(uint32_t regAddr, uint32_t regVal)
{
	wrap_tz_secure_reg_rw(tz_phy_to_secure_reg(regAddr),
		TZ_SECURE_REG_WRITE, &regVal);
}

#else //!VPP_ENABLE_FASTCALL_FOR_REG_ACCESS

#define wrap_register_read(regAddr, pRegVal)	\
			AVIO_REG_WORD32_READ(regAddr, pRegVal)
#define wrap_register_write(regAddr, regVal)	\
			AVIO_REG_WORD32_WRITE(regAddr, regVal)

#endif //!VPP_ENABLE_FASTCALL_FOR_REG_ACCESS

static UINT32 isTzCompilationEnabled(void)
{
	DHUB_CTX *hDhubCtx =
		(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	UINT32 isEnableTEE = hDhubCtx->isTeeEnabled;
#ifndef VPP_ENABLE_FASTCALL_FOR_REG_ACCESS
	//Force direct reg rd/wr using GA_REG_XXXXX in NTZ
	isEnableTEE = 1;
#endif //VPP_ENABLE_FASTCALL_FOR_REG_ACCESS
#if defined(VPP_ENABLE_FASTCALL_FOR_REG_ACCESS) && \
	defined(VPP_ENABLE_FASTCALL_FOR_REG_ACCESS_IN_NTZ)
	//Force fastcall method t oaccess register in NTZ
	isEnableTEE = 1;
#endif
	return isEnableTEE;
}

UNSG32 wrap_fastcall_semaphore_chk_full(void *hdl, SIGN32 id)
{
	UINT32 regVal = 0;
	HDL_semaphore *hSemaphore = (HDL_semaphore *) hdl;
	UINT32 regAddr = (hSemaphore->ra + RA_SemaHub_full);

	if (isTzCompilationEnabled())
		wrap_register_read(regAddr, &regVal);
	else
		regVal = wrap_semaphore_chk_full(hdl, id);

	return regVal;
}

void wrap_fastcall_semaphore_pop(void *hdl, SIGN32 id, SIGN32 delta)
{
	T32SemaHub_POP pop;
	UINT32 regVal = 0;
	HDL_semaphore *hSemaphore = (HDL_semaphore *) hdl;
	UINT32 regAddr = (hSemaphore->ra + RA_SemaHub_POP);

	pop.u32 = 0;
	pop.uPOP_ID = id;
	pop.uPOP_delta = delta;
	regVal = pop.u32;

	if (isTzCompilationEnabled())
		wrap_register_write(regAddr, regVal);
	else
		wrap_semaphore_pop(hdl, id, delta);
}

void wrap_fastcall_semaphore_clr_full(void *hdl, SIGN32 id)
{
	UINT32 regVal = 0;
	HDL_semaphore *hSemaphore = (HDL_semaphore *) hdl;
	UINT32 regAddr = (hSemaphore->ra + RA_SemaHub_full);

	regVal = (1 << id);

	if (isTzCompilationEnabled())
		wrap_register_write(regAddr, regVal);
	else
		wrap_semaphore_clr_full(hdl, id);
}

void wrap_fastcall_semaphore_intr_enable(void *hdl, SIGN32 id, SIGN32 empty,
	SIGN32 full, SIGN32 almostEmpty, SIGN32 almostFull, SIGN32 cpu)
{
	UINT32 regVal = 0;
	HDL_semaphore *hSemaphore = (HDL_semaphore *) hdl;
	UINT32 regAddr = (hSemaphore->ra + RA_SemaHub_ARR);
	T32SemaINTR_mask mask;

	regAddr += id * sizeof(SIE_Semaphore);
	regAddr += RA_Semaphore_INTR + cpu * sizeof(SIE_SemaINTR);

	mask.u32 = 0;
	mask.umask_empty = empty;
	mask.umask_full = full;
	mask.umask_almostEmpty = almostEmpty;
	mask.umask_almostFull = almostFull;

	if (isTzCompilationEnabled())
		wrap_register_write(regAddr, regVal);
	else
		wrap_semaphore_intr_enable(
			hdl, id, empty, full, almostEmpty, almostFull, cpu);
}

#include <linux/module.h>
EXPORT_SYMBOL(wrap_fastcall_semaphore_chk_full);
EXPORT_SYMBOL(wrap_fastcall_semaphore_pop);
EXPORT_SYMBOL(wrap_fastcall_semaphore_clr_full);
EXPORT_SYMBOL(wrap_fastcall_semaphore_intr_enable);
#endif //VPP_ENABLE_DIRECT_REG_ACCESS

