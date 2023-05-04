// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Jie Xi <Jie.Xi@synaptics.com>
 *
 */

#define pr_fmt(fmt) "[berlin_bm kernel driver] " fmt

#include <linux/slab.h>
#include "tee_client_api.h"
#include "uapi/bm.h"
#include "ptm_wrapper.h"

#define TA_PTM_UUID {0x1316a183, 0x894d, 0x43fe, \
                     {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x2d} }
static TEEC_UUID ptm_ta_uuid = TA_PTM_UUID;

enum {
        PTM_INVALID,
        PTM_DEBUG,
        PTM_CONSTRUCT_PT,
        PTM_FREE_PT,
        PTM_CMD_MAX
};

struct ptm_ta_param {
	u32 flags;
	struct bm_fb_param param;
};

static int check_ctx_session(struct ptm_context *ptm_ctx)
{
	TEEC_Result ret = TEEC_SUCCESS;

	if (!ptm_ctx->ctx_created)
		return TEEC_ERROR_BAD_STATE;

	if (!ptm_ctx->session_opened)
		ret = TEEC_OpenSession(&ptm_ctx->teec_ctx,
				       &ptm_ctx->teec_session,
				       &ptm_ta_uuid, TEEC_LOGIN_USER,
				       NULL, NULL, NULL);

	if (ret != TEEC_SUCCESS)
		pr_err("fail to open TEEC session: %x\n", ret);
	else
		ptm_ctx->session_opened = 1;

	return ret;
}

static int ptm_wrapper_alloc(struct ptm_context *ptm_ctx, unsigned int len,
		      void **handle)
{
	TEEC_SharedMemory *shm;
	TEEC_Result ret = TEEC_SUCCESS;

	shm = kzalloc(sizeof(TEEC_SharedMemory), GFP_KERNEL);
	if (!shm)
		return -ENOMEM;

	shm->size = len;
	shm->flags = TEEC_MEM_INPUT;
	ret = TEEC_AllocateSharedMemory(&ptm_ctx->teec_ctx, shm);
	if (ret != TEEC_SUCCESS) {
		pr_err("fail to allocate TEEC share memory: %x\n", ret);
		kfree(shm);
		return ret;
	}
	*handle = shm;
	return ret;
}

static void ptm_wrapper_free(void *handle)
{
	TEEC_ReleaseSharedMemory(handle);
	kfree(handle);
}

int ptm_wrapper_init(struct ptm_context *ptm_ctx)
{
	TEEC_Result ret;

	ret = TEEC_InitializeContext(NULL, &ptm_ctx->teec_ctx);
	if (ret != TEEC_SUCCESS) {
		pr_err("fail to initialize TEEC context: %x\n", ret);
		ptm_ctx->ctx_created = 0;
	} else
		ptm_ctx->ctx_created = 1;

	ptm_ctx->session_opened = 0;
	mutex_init(&ptm_ctx->lock);

	ret = ptm_wrapper_alloc(ptm_ctx, PTM_MAX_SG_SHM,
			(void **)&ptm_ctx->sg_shm);
	if (ret) {
		pr_err("fail to alloc ptm sg shm %zx", PTM_MAX_SG_SHM);
		goto error;
	}
	ret = ptm_wrapper_alloc(ptm_ctx, sizeof(struct ptm_ta_param),
			(void **)&ptm_ctx->param_shm);
	if (ret) {
		pr_err("fail to alloc ptm param shm %zx",
				sizeof(struct ptm_ta_param));
		goto error;
	}
	return ret;

error:
	if (ptm_ctx->sg_shm)
		ptm_wrapper_free(ptm_ctx->sg_shm);
	if (ptm_ctx->param_shm)
		ptm_wrapper_free(ptm_ctx->param_shm);
	return ret;
}

void ptm_wrapper_exit(struct ptm_context *ptm_ctx)
{
	if (ptm_ctx->sg_shm)
		ptm_wrapper_free(ptm_ctx->sg_shm);
	if (ptm_ctx->param_shm)
		ptm_wrapper_free(ptm_ctx->param_shm);

	if (ptm_ctx->session_opened)
		TEEC_CloseSession(&ptm_ctx->teec_session);
	if (ptm_ctx->ctx_created)
		TEEC_FinalizeContext(&ptm_ctx->teec_ctx);

	ptm_ctx->ctx_created = 0;
	ptm_ctx->session_opened = 0;
}

int ptm_wrapper_construct_pt(struct ptm_context *ptm_ctx,
			     void *handle, unsigned int len,
			     struct bm_pt_param *pt_param, u32 flags,
			     struct bm_fb_param *fb_param)
{
	TEEC_Result ret = TEEC_SUCCESS;
	TEEC_Operation op;
	TEEC_SharedMemory *param_handle = NULL;
	struct ptm_ta_param *p = NULL;

	mutex_lock(&ptm_ctx->lock);
	ret = check_ctx_session(ptm_ctx);

	if (ret != TEEC_SUCCESS)
		goto error;

	param_handle = ptm_ctx->param_shm;
	p = param_handle->buffer;

	p->flags = flags;
	p->param.fb_type = fb_param->fb_type;
	p->param.uva_param.y_size = fb_param->uva_param.y_size;
	op.paramTypes = TEEC_PARAM_TYPES(
		TEEC_MEMREF_PARTIAL_INPUT,
		TEEC_MEMREF_PARTIAL_INPUT,
		TEEC_VALUE_OUTPUT,
		TEEC_VALUE_OUTPUT);
	op.params[0].memref.parent = handle;
	op.params[0].memref.size = len;
	op.params[0].memref.offset = 0;
	op.params[1].memref.parent = param_handle;
	op.params[1].memref.size = sizeof(struct ptm_ta_param);
	op.params[1].memref.offset = 0;
	ret = TEEC_InvokeCommand(&ptm_ctx->teec_session,
				 PTM_CONSTRUCT_PT, &op, NULL);
	if (ret != TEEC_SUCCESS)
		pr_err("fail to invoke command to PTM TA: %x\n", ret);
	else {
		//TODO: handle possible 64bit physical address
		pt_param->phy_addr = op.params[2].value.a;
		pt_param->len = op.params[2].value.b;
		pt_param->mem_id = op.params[3].value.a;
	}

error:
	mutex_unlock(&ptm_ctx->lock);

	return ret;
}

int ptm_wrapper_free_pt(struct ptm_context *ptm_ctx,
			phys_addr_t addr, unsigned int len)
{
	TEEC_Result ret = TEEC_SUCCESS;
	TEEC_Operation op;

	mutex_lock(&ptm_ctx->lock);
	ret = check_ctx_session(ptm_ctx);

	if (ret == TEEC_SUCCESS) {
		op.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);
		//TODO: handle possible 64bit physical address
		op.params[0].value.a = (uint32_t)(addr & 0xffffffff);
		op.params[0].value.b = len;
		ret = TEEC_InvokeCommand(&ptm_ctx->teec_session,
					 PTM_FREE_PT, &op, NULL);
		if (ret != TEEC_SUCCESS)
			pr_err("fail to free pt to PTM TA: %x with addr %pa\n",
				  ret, &addr);
	}
	mutex_unlock(&ptm_ctx->lock);

	return ret;
}
