/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ******************************************************************************/

#include "config.h"
#include "tee_client_api.h"
#include "tz_client_api.h"
#include "tee_ca_mgr_cmd.h"
#include "tee_ca_sys_cmd.h"
#include "tee_client_util.h"
#include "log.h"
#ifndef __KERNEL__
#include <string.h>
#else
#include <linux/string.h>
#endif /* __KERNEL__ */

#define TEE_DEVICE_NAME		"/dev/tz"

/**
 * @brief TEEC error string values
 *
 */
static const char* TEEC_ErrorList[] =
{
	"The operation succeeded",
	"Non-specific cause",
	"Access privileges are not sufficient",
	"The operation was cancelled",
	"Concurrent accesses caused conflict",
	"Too much data for the requested operation was passed",
	"Input data was of invalid format",
	"Input parameters were invalid",
	"Operation is not valid in the current state",
	"The requested data item is not found",
	"The requested operation should exist but is not yet implemented",
	"The requested operation is valid but is not supported in this Implementation",
	"Expected data was missing",
	"System ran out of resources",
	"The system is busy working on something else",
	"Communication with a remote party failed",
	"A security fault was detected",
	"The supplied buffer is too short for the generated output",
};


/**
 * @brief Service error string values
 */
static const char* TEEC_ServiceErrorList[] =
{
	"Service Success",
	"Service Pending",
	"Service Interrupted",
	"Service Error",
	"Service - Invalid Argument",
	"Service - Invalid Address",
	"Service No Support",
	"Service No Memory",
};

const char* TEEC_GetError(uint32_t error, uint32_t returnOrigin)
{
	if(returnOrigin == TEEC_ORIGIN_TRUSTED_APP)
		return TEEC_ServiceErrorList[error];
	else
		return TEEC_ErrorList[error];
}

TEEC_Result TEEC_InitializeContext(
		const char*   name,
		TEEC_Context* context)
{
	int ret = 0;

	if(context == NULL) {
		error("Context is null!\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if(name == NULL)
		name = TEE_DEVICE_NAME;

	ret = tzc_open(name);
	if(ret == -1) {
		error("device open failed\n");
		return TEEC_ERROR_GENERIC;
	} else {
		context->fd = ret;
		context->sessionCount = 0;
		context->sharedMemCount = 0;
	}
	return TEEC_SUCCESS;

}

void TEEC_FinalizeContext(TEEC_Context* context)
{
	if(!context)
		return;

	if(context->sessionCount != 0)
		error("pending open %d sessions\n",
				context->sessionCount);

	if(context->sharedMemCount != 0)
		error("unreleased shared memory %d blocks\n",
				context->sharedMemCount);

	if((context->sessionCount == 0) && (context->sharedMemCount == 0)) {
		trace("device closed\n");
		tzc_close(context->fd);
		context->fd = 0;
	}
}

TEEC_Result TEEC_AllocateSharedMemory(
		TEEC_Context*		context,
		TEEC_SharedMemory*	sharedMem)
{
	if(context == NULL || sharedMem == NULL ) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if((sharedMem->size == 0) ||
			((sharedMem->flags != TEEC_MEM_INPUT) &&
			 (sharedMem->flags != TEEC_MEM_OUTPUT) &&
			 (sharedMem->flags != (TEEC_MEM_INPUT | TEEC_MEM_OUTPUT)))) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	sharedMem->buffer = tzc_alloc_shm(context->fd, sharedMem->size, &sharedMem->phyAddr);

	if(NULL == sharedMem->buffer) {
		error("malloc failed\n");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	sharedMem->allocated = true;
	sharedMem->context = context;
	sharedMem->operationCount = 0;

	context->sharedMemCount++;
	return TEEC_SUCCESS;
}

TEEC_Result TEEC_RegisterSharedMemory(
		TEEC_Context*		context,
		TEEC_SharedMemory*	sharedMem)
{
	if (context == NULL || sharedMem == NULL ) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if ((sharedMem->size == 0) ||
			((sharedMem->flags != TEEC_MEM_INPUT) &&
			 (sharedMem->flags != TEEC_MEM_OUTPUT) &&
			 (sharedMem->flags != (TEEC_MEM_INPUT | TEEC_MEM_OUTPUT)))) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (sharedMem->buffer != NULL) {
		uint32_t attr;

		/* do a dummy read on virtual address to ensure page table is
		 * created.
		 * here, we use attr to store the result, it's to ensure read
		 * happens before tzc_get_mem_info().
		 */
		attr = *(uint32_t *)sharedMem->buffer;

		sharedMem->phyAddr = tzc_get_mem_info(context->fd, sharedMem->buffer, &attr);
		if (sharedMem->phyAddr == NULL) {
			error("can't get physical address for 0x%08x\n",
					(uint32_t)sharedMem->buffer);
			return TEEC_ERROR_BAD_PARAMETERS;
		}
		sharedMem->attr = attr;
	} else if (sharedMem->phyAddr != NULL) {
		/* in this case, we don't need do anything. the parameters
		 * phyAddr and attr are set by user directly.
		 */
	} else {
		error("shared memory buffer is NULL\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	sharedMem->allocated = false;
	sharedMem->context = context;
	sharedMem->operationCount = 0;

	context->sharedMemCount++;
	return TEEC_SUCCESS;

}

void TEEC_ReleaseSharedMemory(
		TEEC_SharedMemory*	sharedMem)
{
	if(sharedMem == NULL) {
		warn("sharedMem is NULL\n");
		return;
	}

	if(sharedMem->operationCount != 0) {
		error("%d pending operations! must be 0\n",
			sharedMem->operationCount);
		return;
	}

	if(sharedMem->allocated)
		tzc_free_shm(sharedMem->context->fd,
				sharedMem->buffer,
				sharedMem->phyAddr,
				sharedMem->size);

	sharedMem->buffer = NULL;
	sharedMem->phyAddr = NULL;
	sharedMem->size = 0;
	sharedMem->context->sharedMemCount--;
	sharedMem->context = NULL;
}

TEEC_Result TEEC_OpenSession (
		TEEC_Context*		context,
		TEEC_Session*		session,
		const TEEC_UUID*	destination,
		uint32_t		connectionMethod,
		const void*		connectionData,
		TEEC_Operation*		operation,
		uint32_t*		returnOrigin)
{
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t taskId = 0, sessionId = 0;

	if (returnOrigin)
		*returnOrigin = TEEC_ORIGIN_API;

	if((context == NULL) || (session == NULL) || (destination == NULL)) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	memset(session, 0, sizeof(*session));

	/* acquire session lock */
	session->lock = tzc_mutex_create();
	if (!session->lock)
		goto fail_out;

	/* acquire the communication channel first */
	session->comm = tzc_acquire_tee_comm_channel(context->fd);

	if (!session->comm){
		error("fail to get comm channel\n");
		res = TEE_ERROR_COMMUNICATION;
		goto fail_out;
	}

	/* create the instance first */
	res = TAMgr_CreateInstance(context->fd, session->comm, destination,
					returnOrigin, &taskId);

	if (res != TEEC_SUCCESS) {
		error("fail to create instance, result=0x%08x\n", res);
		goto fail_out;
	}

	res = TASysCmd_OpenSession(context->fd, session->comm, taskId,
					connectionMethod, connectionData,
					operation, returnOrigin, &sessionId);

	if (res != TEEC_SUCCESS) {
		error("fail to open session, result=0x%08x\n", res);
		goto fail_out;
	}

	context->sessionCount++;

	session->operationCount = 0;
	session->taskId = taskId;
	session->sessionId = sessionId;
	session->serviceId = *destination;
	session->device = context;

	return res;

fail_out:
	if (taskId > 0)
		TAMgr_DestroyInstance(context->fd, session->comm, taskId, NULL);

	if (session->comm)
		tzc_release_tee_comm_channel(context->fd, session->comm);

	if (session->lock)
		tzc_mutex_destroy(session->lock);

	/* reset the session context */
	memset(session, 0, sizeof(*session));

	return res;
}

void TEEC_CloseSession (
		TEEC_Session*		session)
{
	TEEC_Context *context;
	TEEC_Result res;
	uint32_t origin;
	bool instanceDead = false;

	if(!session || !session->comm || !session->device ||
		0 == session->taskId || 0 == session->sessionId) {
		warn("invalid session handle\n");
		return;
	}

	if(session->operationCount > 0) {
		error("%d Pending operations\n",
				session->operationCount);
		return;
	}

	context = session->device;

	res = TASysCmd_CloseSession(context->fd, session->comm,
				session->taskId, session->sessionId,
				&origin, &instanceDead);

	if (res != TEEC_SUCCESS) {
		error("fail to close session, result=0x%08x\n", res);
		return;
	}

	/* destroy instance is not necessary, TEE framework will do it
	 * automatically.
	 */
	if (instanceDead)
		TAMgr_DestroyInstance(context->fd, session->comm,
					session->taskId, NULL);

	tzc_release_tee_comm_channel(context->fd, session->comm);

	context->sessionCount--;

	tzc_mutex_destroy(session->lock);

	/* reset the session context */
	memset(session, 0, sizeof(*session));
}

TEE_Result REE_InvokeSysCommandEntryPoint(
		void*			sessionContext,
		uint32_t		commandID,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size);

static uint32_t tz_cmd_callback(void *userdata,
	                uint32_t cmd_id,
	                uint32_t param,
	                uint32_t *origin)
{
	TEEC_Session *session = userdata;
	TEEC_Result res;
	TEEC_Operation operation;
	struct tee_comm_param *cmd;

	if (!session)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (origin)
		*origin = TEEC_ORIGIN_API;

	cmd = &session->comm->callback_param;

	switch (cmd_id) {
	case TZ_CMD_TEE_SYS:
		res = REE_InvokeSysCommandEntryPoint(session,
				cmd->cmd_id, cmd->param_types, cmd->params,
				cmd->param_ext, cmd->param_ext_size);
		break;
	case TZ_CMD_TEE_USER:
		if (session->Callback == NULL) {
			error("callback is not registered\n");
			res = TEEC_ERROR_NOT_SUPPORTED;
		} else {
			res = TEEC_CallbackCommandToOperaction(&operation, cmd);
			if (res != TEEC_SUCCESS)
				break;
			res = session->Callback(session, cmd->cmd_id,
					&operation, session->callbackUserdata);
			if (res == TEEC_SUCCESS)
				TEEC_CallbackOperactionToCommand(cmd, &operation);
		}
		break;
	default:
		res = TEEC_ERROR_NOT_SUPPORTED;
		break;
	}

	return res;
}

TEEC_Result TEEC_InvokeCommand(
		TEEC_Session*	session,
		uint32_t	commandID,
		TEEC_Operation*	operation,
		uint32_t*	returnOrigin)
{
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;
	struct tee_comm_param *comm;

	if(!session || !session->comm || !session->device ||
		0 == session->taskId || 0 == session->sessionId) {
		warn("invalid session handle\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (operation)
		operation->started = true;

	tzc_mutex_lock(session->lock);

	session->operationCount++;

	comm = &session->comm->call_param;
	/* pack the command */
	TEEC_CallOperactionToCommand(session->device->fd, comm,
			commandID, operation);

	comm->session_id = session->sessionId;

	if (operation)
		operation->session = session;

	res = tzc_invoke_tee_user_command(session->device->fd, session->taskId,
				session->comm, &origin,
				tz_cmd_callback, session);

	/* invoke command */
	if (origin == TEEC_ORIGIN_TRUSTED_APP) {
		/* buffer results,
		 * and free the communication channel after communication done
		 */
		TEEC_CallCommandToOperaction(operation, &session->comm->call_param);
	}

	if (returnOrigin)
		*returnOrigin = origin;

	session->operationCount--;

	tzc_mutex_unlock(session->lock);

	return res;
}

void TEEC_RequestCancellation(
		TEEC_Operation* operation)
{
	struct tee_comm_param *cmd;

	if (!operation || !operation->session)
		return;

	cmd = &operation->session->comm->call_param;
	cmd->flags |= TZ_COMM_REQ_CANCELLED;
}

TEEC_Result TEEC_RegisterTA(
		TEEC_Context*		context,
		TEEC_Parameter*		taBin,
		uint32_t		paramType)
{
	return TEEC_RegisterTAExt(context, taBin, paramType, NULL);
}

TEEC_Result TEEC_RegisterTAExt(
		TEEC_Context*		context,
		TEEC_Parameter*		taBin,
		uint32_t		paramType,
		TEEC_Property*		property)
{
	TEEC_Result res = TEEC_SUCCESS;
	struct tee_comm *comm;
	TEEC_Operation operation = {0};
	uint32_t codeType = 4;

	if (!TEEC_PARAM_IS_MEMREF(paramType)) {
		error("input parameter type (0x%08x) is not memory reference\n",
				paramType);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	/* acquire the communication channel first */
	comm = tzc_acquire_tee_comm_channel(context->fd);
	if (!comm) {
		error("can't communication channel\n");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_MEMREF_PARTIAL_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_NONE,
			TEEC_NONE);

	if (property) {
		res = TEEC_GetPropertyTAType(property, &codeType);
		if (res) {
			error("get TA property failed\n");
			return res;
		}
	}

	operation.params[0].memref.parent = taBin->memref.parent;
	operation.params[0].memref.size = taBin->memref.size;
	operation.params[0].memref.offset = taBin->memref.offset;
	operation.params[1].value.a = codeType;

	res = TAMgr_Register(context->fd, comm, &operation);

	/* release the communication channel after done */
	tzc_release_tee_comm_channel(context->fd, comm);

	return res;
}

#if !defined(__KERNEL__) && !defined(__TRUSTZONE__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({		\
    typeof (exp) _rc;				\
    do {					\
	_rc = (exp);				\
    } while (_rc == -1 && errno == EINTR);	\
    _rc; })
#endif

TEEC_Result TEEC_LoadFile(
		const char*	filename,
		void*		buf,
		int		size)
{
	TEEC_Result result = TEEC_SUCCESS;
	int fd, ret, n;

	if (!buf || ((fd = open(filename, O_RDONLY)) < 0))
		return TEEC_ERROR_BAD_PARAMETERS;

	n = 0;
	while (n < size) {
		ret = TEMP_FAILURE_RETRY(read(fd, buf + n, size - n));
		if (ret < 0) {
			result = TEEC_ERROR_GENERIC;
			break;
		} else if (!ret)
			break;
		n += ret;
	}

	close(fd);

	return result;
}

TEEC_Result TEEC_LoadTA(
		TEEC_Context*		context,
		const char*		filename)
{
	return TEEC_LoadTAExt(context, filename, NULL);
}

TEEC_Result TEEC_LoadTAExt(
		TEEC_Context*		context,
		const char*		filename,
		TEEC_Property*		property)
{
	TEEC_SharedMemory shm;
	TEEC_Parameter parameter;
	TEEC_Result result = TEEC_SUCCESS;
	struct stat st;

	if (!context) {
		error("invalid TEE context\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (!filename) {
		error("file name must not be NULL\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (stat(filename, &st) < 0) {
		error("Failed to get TA image state, stat error (%d):%s\n",
				errno, strerror(errno));
		goto cleanup1;
	}

	shm.size = st.st_size;
	shm.flags = TEEC_MEM_INPUT;
	/* Use TEE Client API to allocate the underlying memory buffer. */
	result = TEEC_AllocateSharedMemory(
			context,
			&shm);
	if (result != TEEC_SUCCESS || NULL == shm.buffer) {
		error("fail to AllocateShareMemory, ret=0x%08x\n", result);
		goto cleanup1;
	}

	result = TEEC_LoadFile(filename, (void *)shm.buffer, shm.size);
	if (result != TEEC_SUCCESS) {
		error("Failed to read TA image, ret=0x%08x\n", result);
		goto cleanup2;
	}

	parameter.memref.parent = &shm;
	parameter.memref.size = shm.size;
	parameter.memref.offset = 0;

	result = TEEC_RegisterTAExt(context, &parameter,
			TEEC_MEMREF_PARTIAL_INPUT, property);

	if (result != TEEC_SUCCESS)
		error("fail to register TA, error=0x%08x\n", result);

cleanup2:
	TEEC_ReleaseSharedMemory(&shm);
cleanup1:
	return result;
}

TEEC_Result TEEC_ProvisionFeatureCert(
		TEEC_Context*		context,
		const char*		filename)
{
	TEEC_SharedMemory shm;
	TEEC_Result result = TEEC_SUCCESS;
	struct stat st;
	uint32_t cert_size;
	uint32_t name_size;
	struct tee_comm *comm;
	TEEC_Operation operation = {0};

	if (!context) {
		error("invalid TEE context\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (!filename) {
		error("file name must not be NULL\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (stat(filename, &st) < 0) {
		error("Failed to get TA image state, stat error (%d):%s\n",
				errno, strerror(errno));
		goto cleanup1;
	}

	cert_size = st.st_size;
	name_size = strlen(filename);

	shm.size = cert_size + name_size;
	shm.flags = TEEC_MEM_INPUT;
	/* Use TEE Client API to allocate the underlying memory buffer. */
	result = TEEC_AllocateSharedMemory(
			context,
			&shm);
	if (result != TEEC_SUCCESS || NULL == shm.buffer) {
		error("fail to AllocateShareMemory, ret=0x%08x\n", result);
		goto cleanup1;
	}

	result = TEEC_LoadFile(filename, (void *)shm.buffer, cert_size);
	if (result != TEEC_SUCCESS) {
		error("Failed to read TA image, ret=0x%08x\n", result);
		goto cleanup2;
	}

	memcpy(shm.buffer + cert_size, filename, name_size);

	comm = tzc_acquire_tee_comm_channel(context->fd);
	if (!comm) {
		error("can't communication channel\n");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_MEMREF_PARTIAL_INPUT,
			TEEC_MEMREF_PARTIAL_INPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].memref.parent = &shm;
	operation.params[0].memref.size = name_size;
	operation.params[0].memref.offset = cert_size;

	operation.params[1].memref.parent = &shm;
	operation.params[1].memref.size = cert_size;
	operation.params[1].memref.offset = 0;

	result = TAMgr_ProvisionFeatureCert(context->fd, comm, &operation);

	/* release the communication channel after done */
	tzc_release_tee_comm_channel(context->fd, comm);

	return result;

cleanup2:
	TEEC_ReleaseSharedMemory(&shm);
cleanup1:
	return result;
}

#endif /* !__KERNEL__ && !__TRUSTZONE__ */

TEEC_Result TEEC_RegisterCallback(
		TEEC_Session*	session,
		TEEC_Callback	callback,
		void *		userdata)
{
	if (!session) {
		warn("invalid session handle\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (session->Callback) {
		warn("already registered\n");
		return TEEC_ERROR_BAD_STATE;
	}

	session->Callback = callback;
	session->callbackUserdata = userdata;

	return TEEC_SUCCESS;
}

TEEC_Result TEEC_UnregisterCallback(
		TEEC_Session*	session)
{
	if (!session) {
		warn("invalid session handle\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (!session->Callback) {
		warn("not registered\n");
		return TEEC_ERROR_BAD_STATE;
	}

	session->Callback = NULL;
	session->callbackUserdata = NULL;

	return TEEC_SUCCESS;
}

TEEC_Result TEEC_SecureCacheClean(
		TEEC_Context*		context,
		void*			phyAddr,
		size_t			len)
{
	TEEC_Result result;

	if (context == NULL || phyAddr == NULL || len <= 0) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	result = tzc_fast_secure_cache_clean(context->fd, phyAddr, len);

	return result;
}

TEEC_Result TEEC_SecureCacheInvalidate(
		TEEC_Context*		context,
		void*			phyAddr,
		size_t			len)
{
	TEEC_Result result;

	if (context == NULL || phyAddr == NULL || len <= 0) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	result = tzc_fast_secure_cache_invalidate(context->fd, phyAddr, len);

	return result;
}

TEEC_Result TEEC_SecureCacheFlush(
		TEEC_Context*		context,
		void*			phyAddr,
		size_t			len)
{
	TEEC_Result result;

	if (context == NULL || phyAddr == NULL || len <= 0) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	result = tzc_fast_secure_cache_flush(context->fd, phyAddr, len);

	return result;
}

TEEC_Result TEEC_FastMemMove(
		TEEC_Context*		context,
		void*			dstPhyAddr,
		void*			srcPhyAddr,
		size_t			len)
{
	TEEC_Result result;

	if (context == NULL || len <= 0 ||
			dstPhyAddr == NULL || srcPhyAddr == NULL) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	result = tzc_fast_memmove(context->fd, dstPhyAddr, srcPhyAddr, len);

	return result;
}

TEEC_Result TEEC_FastSharedMemMove(
		TEEC_Context*		context,
		TEEC_SharedMemory*	dst,
		TEEC_SharedMemory*	src,
		size_t			dstOffset,
		size_t			srcOffset,
		size_t			len)
{
	void* dstPhyAddr;
	void* srcPhyAddr;

	if (context == NULL || len <= 0 || dst == NULL || src == NULL) {
		error("Illegal argument\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if ((dstOffset + len) > dst->size) {
		error("source memory overflow. shm size=0x%08x. "
				"offset=0x%08x, len=0x%08x\n",
				dst->size, dstOffset, len);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	if ((srcOffset + len) > src->size) {
		error("source memory overflow. shm size=0x%08x. "
				"offset=0x%08x, len=0x%08x\n",
				src->size, srcOffset, len);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	dstPhyAddr = dst->phyAddr + dstOffset;
	srcPhyAddr = src->phyAddr + srcOffset;

	return TEEC_FastMemMove(context, dstPhyAddr, srcPhyAddr, len);
}

TEEC_Result TEEC_AddPropertyTAType(
		TEEC_Property*		property,
		const char*		type)
{
	TEEC_Result result = TEEC_SUCCESS;

	if ((NULL == property) || (NULL == type))
		return TEE_ERROR_BAD_PARAMETERS;

	if (!strcmp(type, "MVLTA")) {
		result = TEEC_AddPropertyUint32(property, "TA_TYPE", 4);
		if (result != TEEC_SUCCESS)
			error("Add property failed: %x\n", result);
	} else if(!strcmp(type, "CUSTA")) {
		result = TEEC_AddPropertyUint32(property, "TA_TYPE", 6);
		if (result != TEEC_SUCCESS)
			error("Add property failed: %x\n", result);
	} else {
		error("Unknown TA type: %s\n", type);
		result = TEE_ERROR_BAD_PARAMETERS;
	}

	return result;
}

TEEC_Result TEEC_GetPropertyTAType(
		TEEC_Property*		property,
		uint32_t*		type)
{
	if ((NULL == property) || (NULL == type))
		return TEE_ERROR_BAD_PARAMETERS;
	return TEEC_GetPropertyUint32(property, "TA_TYPE", type);
}

TEEC_Result TEEC_LoadFeatureCert(TEEC_Context *context)
{
	TEEC_Result result = TEEC_SUCCESS;
	struct tee_comm *comm;

	if (!context) {
		error("invalid TEE context\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	comm = tzc_acquire_tee_comm_channel(context->fd);
	if (!comm) {
		error("can't communication channel\n");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	result = TAMgr_LoadFeatureCert(context->fd, comm);

	/* release the communication channel after done */
	tzc_release_tee_comm_channel(context->fd, comm);

	return result;

}

#ifdef __KERNEL__
EXPORT_SYMBOL(TEEC_InitializeContext);
EXPORT_SYMBOL(TEEC_FinalizeContext);
EXPORT_SYMBOL(TEEC_RegisterSharedMemory);
EXPORT_SYMBOL(TEEC_AllocateSharedMemory);
EXPORT_SYMBOL(TEEC_ReleaseSharedMemory);
EXPORT_SYMBOL(TEEC_OpenSession);
EXPORT_SYMBOL(TEEC_CloseSession);
EXPORT_SYMBOL(TEEC_InvokeCommand);
EXPORT_SYMBOL(TEEC_RequestCancellation);
EXPORT_SYMBOL(TEEC_GetError);
EXPORT_SYMBOL(TEEC_RegisterTA);
EXPORT_SYMBOL(TEEC_RegisterTAExt);
EXPORT_SYMBOL(TEEC_RegisterCallback);
EXPORT_SYMBOL(TEEC_UnregisterCallback);
EXPORT_SYMBOL(TEEC_FastMemMove);
EXPORT_SYMBOL(TEEC_FastSharedMemMove);
EXPORT_SYMBOL(TEEC_SecureCacheClean);
EXPORT_SYMBOL(TEEC_SecureCacheInvalidate);
EXPORT_SYMBOL(TEEC_SecureCacheFlush);
EXPORT_SYMBOL(TEEC_CreateProperty);
EXPORT_SYMBOL(TEEC_AddPropertyString);
EXPORT_SYMBOL(TEEC_AddPropertyUint32);
EXPORT_SYMBOL(TEEC_GetPropertyString);
EXPORT_SYMBOL(TEEC_GetPropertyUint32);
EXPORT_SYMBOL(TEEC_DeleteProperty);
EXPORT_SYMBOL(TEEC_DestroyProperty);
EXPORT_SYMBOL(TEEC_AddPropertyTAType);
EXPORT_SYMBOL(TEEC_GetPropertyTAType);
#endif /* __KERNEL__ */
