// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>

#include "tee_client_api.h"
#include "tee_ca_dhub.h"
#include "dhub_cmd.h"
#include "vpp_cmd.h"

#define bTST(x, b)      (((x) >> (b)) & 1)

static int initialized;
static TEEC_Context context;
static TEEC_Session session;

int DhubInitialize(void)
{
	TEEC_Result result;
	const TEEC_UUID TADhub_UUID = TA_DHUB_UUID;

	if (initialized)
		return TEEC_SUCCESS;

	initialized = true;

	/* ========================================================================
	 *  [1] Connect to TEE
	 * ========================================================================
	 */
	result = TEEC_InitializeContext(NULL, &context);
	if (result != TEEC_SUCCESS) {
		printk("ret=0x%08x\n", result);
		goto cleanup1;
	}

	/* ========================================================================
	 *  [2] Allocate DHUB SHM
	 * ========================================================================
	 */
	/* ========================================================================
	 *  [3] Open session with TEE application
	 * ========================================================================
	 */

	result = TEEC_OpenSession(&context, &session, &TADhub_UUID, TEEC_LOGIN_USER,
				  NULL,	/* No connection data needed for TEEC_LOGIN_USER. */
				  NULL,	/* No payload, and do not want cancellation. */
				  NULL);
	if (result != TEEC_SUCCESS) {
		printk("ret=0x%08x\n", result);
		goto cleanup2;
	}

	return TEEC_SUCCESS;

cleanup2:
	TEEC_FinalizeContext(&context);
cleanup1:
	return result;
}

static int Fastlogo_EnableAutoPush(bool enable)
{
	TEEC_Result result;
	TEEC_Context context;
	TEEC_Session session;
	TEEC_Operation operation;
	const TEEC_UUID TAVPP_CA_fastlogo_UUID = TA_FASTLOGO_UUID;

	/* ========================================================================
	 *[1] Connect to TEE
	 *========================================================================
	 */
	result = TEEC_InitializeContext(NULL, &context);
	if (result != TEEC_SUCCESS) {
		pr_err("%s:%d: result:%x\n", __func__, __LINE__, result);
		goto cleanup1;
	}

	/* ========================================================================
	 *[2] Open session with TEE application
	 * ========================================================================
	 */
	result = TEEC_OpenSession(&context, &session, &TAVPP_CA_fastlogo_UUID, TEEC_LOGIN_USER,
								NULL, /* No connection data needed for TEEC_LOGIN_USER. */
								NULL, /* No payload, and do not want cancellation. */
								NULL);
	if (result != TEEC_SUCCESS) {
		pr_err("%s:%d: result:%x\n", __func__, __LINE__, result);
		goto cleanup2;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(
		TEEC_VALUE_INPUT,
		TEEC_VALUE_INOUT,
		TEEC_VALUE_INPUT,
		TEEC_VALUE_INOUT);

	operation.params[0].value.a = enable;
	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session, VPP_ENABLEAUTOPUSH, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		pr_err("%s:%d: result:%x\n", __func__, __LINE__, result);
		goto cleanup3;
	}
	//store the result
	result = operation.params[1].value.a;

cleanup3:
	TEEC_CloseSession(&session);
cleanup2:
	TEEC_FinalizeContext(&context);
cleanup1:
	return result;
}

void DhubEnableAutoPush(bool enable, bool useFastLogoTa)
{
	TEEC_Result result;
	TEEC_Operation operation;

	//If possible use fastlogo.ta to disable autopush, otherwise use dhub.ta
	if (useFastLogoTa) {
		result = Fastlogo_EnableAutoPush(enable);
		if (result == TEEC_SUCCESS) {
			printk("AutoPush Disabled using fastlogo.ta : 0x%08x\n", result);
			return;
		}
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	operation.params[0].value.a = enable;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session, DHUB_AUTOPUSH_EN, &operation, NULL);
}

void DhubFinalize(void)
{
	if (!initialized)
		return;
	initialized = 0;

	TEEC_CloseSession(&session);
	TEEC_FinalizeContext(&context);
}

void tz_DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr,
			   UNSG32 hboSramAddr, int *pdhubHandle,
			   int *dhub_config, SIGN32 numOfChans)
{
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT, TEEC_NONE);

	operation.params[0].value.a = cpuId;
	operation.params[0].value.b = dHubBaseAddr;
	operation.params[1].value.a = hboSramAddr;
	operation.params[1].value.b = numOfChans;

	/* clear result */
	operation.params[2].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session, DHUB_INIT, &operation, NULL);
}

void tz_DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	operation.params[0].value.a = id;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
					DHUB_CHANNEL_CLEAR, &operation, NULL);
}

UNSG32 tz_dhub_channel_write_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				 SIGN32 id,	/*! Channel ID in $dHubReg ! */
				 UNSG32 addr,	/*! CMD: buffer address ! */
				 SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				 SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				 SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				 SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				 SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				 T64b cfgQ[],	/*! Pass NULL to directly update dHub, or
								 * Pass non-zero to receive programming sequence
								 * in (adr,data) pairs
								 *!
								 */
				 UNSG32 *ptr   /*! Pass in current cmdQ pointer (in 64b word),
								*& receive updated new pointer,
								*Pass NULL to read from HW
								*!
								*/
	)
{
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INOUT);

	operation.params[0].value.a = id;
	operation.params[0].value.b = addr;
	operation.params[1].value.a = size;
	operation.params[1].value.b = semOnMTU;

	operation.params[2].value.a = chkSemId;
	operation.params[2].value.b = updSemId;
	operation.params[3].value.a = interrupt;

	/* clear result */
	operation.params[3].value.b = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
					DHUB_CHANNEL_WRITECMD, &operation, NULL);

	return operation.params[3].value.b;
}

void tz_dhub_channel_generate_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				  SIGN32 id,	/*! Channel ID in $dHubReg ! */
				  UNSG32 addr,	/*! CMD: buffer address ! */
				  SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				  SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				  SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				  SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				  SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				  SIGN32 *pData)
{
	TEEC_Result result;
	TEEC_Operation operation;

	if (!pData)
		return;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT);

	operation.params[0].value.a = id;
	operation.params[0].value.b = addr;
	operation.params[1].value.a = size;
	operation.params[1].value.b = semOnMTU;

	operation.params[2].value.a = chkSemId;
	operation.params[2].value.b = updSemId;
	operation.params[3].value.a = interrupt;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
					DHUB_CHANNEL_GENERATECMD, &operation, NULL);
	pData[0] = operation.params[0].value.a;
	pData[1] = operation.params[0].value.b;
}

void tz_semaphore_pop(void *hdl,	/*  Handle to HDL_semaphore */
			  SIGN32 id,	/*  Semaphore ID in $SemaHub */
			  SIGN32 delta)	/*  Delta to pop as a consumer */
{
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	operation.params[0].value.a = id;
	operation.params[0].value.b = delta;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session, DHUB_SEM_POP, &operation, NULL);

	//return operation.params[1].value.a;
}

void tz_semaphore_clr_full(void *hdl,	/*  Handle to HDL_semaphore */
			   SIGN32 id)	/*  Semaphore ID in $SemaHub */
{
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	operation.params[0].value.a = id;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
					DHUB_SEM_CLR_FULL, &operation, NULL);

	//  return operation.params[1].value.a;
}

UNSG32 tz_semaphore_chk_full(void *hdl,	/*Handle to HDL_semaphore */
				 SIGN32 id)	/*Semaphore ID in $SemaHub
							 * -1 to return all 32b of the interrupt status
							 */
{
	TEEC_Result result;
	TEEC_Operation operation;
	UNSG32 d;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	operation.params[0].value.a = id;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;
	operation.params[1].value.b = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
					DHUB_SEM_CHK_FULL, &operation, NULL);

	/*(d = value of(sem->ra + RA_SemaHub_full); */
	d = operation.params[1].value.a;
	return (id < 0) ? d : bTST(d, id);

}

void tz_semaphore_intr_enable(void *hdl,	/*! Handle to HDL_semaphore ! */
				  SIGN32 id,	/*! Semaphore ID in $SemaHub ! */
				  SIGN32 empty,	/*! Interrupt enable for CPU at condition 'empty' ! */
				  SIGN32 full,	/*! Interrupt enable for CPU at condition 'full' ! */
				  SIGN32 almostEmpty,	/*! Interrupt enable for CPU at condition 'almostEmpty' ! */
				  SIGN32 almostFull,	/*! Interrupt enable for CPU at condition 'almostFull' ! */
				  SIGN32 cpu)	/*! CPU ID (0/1/2) ! */
{
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT);

	operation.params[0].value.a = id;
	operation.params[0].value.b = empty;
	operation.params[1].value.a = full;
	operation.params[1].value.b = almostEmpty;

	operation.params[2].value.a = almostFull;
	operation.params[2].value.b = cpu;

	/* clear result */
	operation.params[3].value.a = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
					DHUB_SEM_INTR_ENABLE, &operation, NULL);

}
EXPORT_SYMBOL(DhubEnableAutoPush);
EXPORT_SYMBOL(DhubInitialize);
EXPORT_SYMBOL(DhubFinalize);
