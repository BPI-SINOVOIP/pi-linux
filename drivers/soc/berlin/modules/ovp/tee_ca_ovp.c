// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "ovp_debug.h"
#include "tee_ca_ovp.h"

static const TEEC_UUID ta_ovp_uuid = {0x1316a183, 0x894d, 0x43fe, \
	{0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf4} };
static TEEC_Context context;
static TEEC_Session session;

static int tz_ovp_errcode_translate(TEEC_Result result)
{
	int ret;

	switch (result) {
	case TEEC_SUCCESS:
		ret = 0;
		break;
	case TEEC_ERROR_ACCESS_DENIED:
		ret = -EPERM;
		break;
	case TEEC_ERROR_BAD_PARAMETERS:
		ret = -EINVAL;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

int tz_ovp_invoke_cmd(OVP_CMD_ID cmd)
{
	TEEC_Result result = TEEC_SUCCESS;
	TEEC_Operation operation;

	/* Supported cmd - OVP_SUSPEND/OVP_RESUME */
	if ((cmd != OVP_SUSPEND) && (cmd != OVP_RESUME))
		return TEEC_ERROR_BAD_PARAMETERS;

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INOUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = 0xdeadbeef;
	operation.params[0].value.b = 0;

	operation.started = 1;
	result = TEEC_InvokeCommand(
			&session,
			cmd,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS)
		ovp_trace("OVP %s failed: 0x%x\n",
			cmd == OVP_SUSPEND ? "SUSPEND" : "RESUME", result);

	return operation.params[0].value.a;
}

int tz_ovp_initialize(void)
{
	TEEC_Result result = TEEC_SUCCESS;
	TEEC_Operation operation;

	/* [1] Connect to TEE */
	result = TEEC_InitializeContext(
			NULL,
			&context);
	if (result != TEEC_SUCCESS) {
		ovp_trace("TEEC_InitializeContext ret=0x%08x\n", result);
		goto fun_ret;
	} else
		ovp_trace("TEEC_InitializeContext success\n");

	/* [2] Open session with TEE application */

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	result = TEEC_OpenSession(
			&context,
			&session,
			&ta_ovp_uuid,
			TEEC_LOGIN_USER,
			NULL,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS) {
		TEEC_CloseSession(&session);
		TEEC_FinalizeContext(&context);
		ovp_trace("TEEC_OpenSession ret=0x%08x\n", result);
		goto fun_ret;
	}
	ovp_trace("TEEC_OpenSession success\n");

fun_ret:
	return tz_ovp_errcode_translate(result);
}

void tz_ovp_finalize(void)
{
	TEEC_CloseSession(&session);
	TEEC_FinalizeContext(&context);
}
