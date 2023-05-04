// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>

#include "tee_ca_vpp.h"

#define bTST(x, b)      (((x) >> (b)) & 1)

#define DEFAULT_SESSION_INDEX 0

#undef VPP_CA_ENABLE_DIAG_PRT
//#define VPP_CA_ENABLE_DIAG_PRT
#ifdef VPP_CA_ENABLE_DIAG_PRT
#define VPP_CA_DBG_PRINT(fmt, ...) pr_info("%d(%s:%d)--" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define VPP_CA_DBG_PRINT(fmt, ...) do {} while (0)
#endif

typedef struct __VPP_CA_CONTEXT__ {
	ENUM_TA_UUID_TYPE uuidType;
	int initialized;
	TEEC_Context context;
} VPP_CA_CONTEXT;
typedef struct __VPP_CA_SESSION__ {
	int initialized;
	TEEC_Session session;
} VPP_CA_SESSION;

static VPP_CA_CONTEXT g_vppCaContext;
static VPP_CA_SESSION TAVPPInstance[MAX_TAVPP_INSTANCE_NUM];

static void VPP_CA_getVppTaUUID(ENUM_TA_UUID_TYPE uuidType, TEEC_UUID *pUUID)
{
	const TEEC_UUID TAVPP_CA_UUID = TAVPP_UUID;
	const TEEC_UUID TAVPP_CA_fastlogo_UUID = TA_FASTLOGO_UUID;

	//Get the mode of bootup recovery/normal and select the ta accordingly
	g_vppCaContext.uuidType = uuidType;
	pr_err("%s:%d: loading recovery ta:%x(%s)\n", __func__, __LINE__,
		g_vppCaContext.uuidType, (g_vppCaContext.uuidType == TA_UUID_VPP) ? "vpp.ta" : "fastlogo.ta");
	memcpy(pUUID, (g_vppCaContext.uuidType == TA_UUID_VPP) ? &TAVPP_CA_UUID : &TAVPP_CA_fastlogo_UUID, sizeof(TEEC_UUID));
}

int VPP_CA_Initialize(ENUM_TA_UUID_TYPE uuidType)
{
	int index;
	TEEC_Result result;
	TEEC_UUID TAVPP_CA_UUID;

	if (g_vppCaContext.initialized)
		return TEEC_SUCCESS;

	g_vppCaContext.initialized = true;

	VPP_CA_getVppTaUUID(uuidType, &TAVPP_CA_UUID);
	/* ========================================================================
	 *  [1] Connect to TEE
	 * ========================================================================
	 */
	result = TEEC_InitializeContext(NULL, &g_vppCaContext.context);
		pr_err("%s:%d: result:%x\n", __func__, __LINE__, result);
	if (result != TEEC_SUCCESS) {
		pr_err("%s:%d: result:%x\n", __func__, __LINE__, result);
		goto cleanup1;
	}

	/* ========================================================================
	 *  [2] Allocate DHUB SHM
	 * ========================================================================
	 */

	/* ========================================================================
	 * [3] Open session with TEE application
	 * ========================================================================
	 */
	for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {
		result = TEEC_OpenSession(&g_vppCaContext.context,
					&(TAVPPInstance[index].session), &TAVPP_CA_UUID, TEEC_LOGIN_USER,
					NULL,	/* No connection data needed for TEEC_LOGIN_USER. */
					NULL,    /* No payload, and do not want cancellation. */
					NULL);
		pr_err("%s:%d: TEEC_OpenSession:%d result:%x\n", __func__, __LINE__, index, result);
		if (result != TEEC_SUCCESS) {
			pr_err("%s:%d: result:%x\n", __func__, __LINE__, result);
			goto cleanup2;
		}
		TAVPPInstance[index].initialized = true;
	}

	return TEEC_SUCCESS;

cleanup2:
	TEEC_FinalizeContext(&g_vppCaContext.context);
cleanup1:
	return result;
}

void VPP_CA_Finalize(void)
{
	int index;

	if (!g_vppCaContext.initialized)
		return;

	g_vppCaContext.initialized = 0;

	for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {
		if (TAVPPInstance[index].initialized) {
			TEEC_CloseSession(&(TAVPPInstance[index].session));
			memset(&TAVPPInstance[index], 0, sizeof(TAVPPInstance[index]));
			TAVPPInstance[index].initialized = false;
		}
	}
	TEEC_FinalizeContext(&g_vppCaContext.context);
}

static inline int VPP_CA_GetInstanceID(void)
{
	int InstanceID = TAVPP_API_INSTANCE;

	return InstanceID;
}

static TEEC_Result InvokeCommandHelper(int session_index, TEEC_Session *pSession,
	VPP_CMD_ID commandID, TEEC_Operation *pOperation, UINT32 *returnOrigin)
{
	TEEC_Result result;
	int cmdID = commandID;
	int instID = DEFAULT_SESSION_INDEX;

	commandID = CREATE_CMD_ID(cmdID, instID);
	VPP_CA_DBG_PRINT("%s:%d: session_index:%d, pSession:%p commandID:%d, pOperation:%p, initialsized:%d\n",
		__func__, __LINE__, session_index, pSession, commandID, pOperation, g_vppCaContext.initialized);

	if (g_vppCaContext.initialized)
		result = TEEC_InvokeCommand(pSession, commandID, pOperation, NULL);
	else
		result = TEEC_SUCCESS;

	return result;
}

int VPP_CA_InitVPPS(UINT32 vpp_addr, UINT32 ta_heapHandle)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result = 0x0;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[1].value.a = 0xdeadbeef;
	operation.params[0].value.a = vpp_addr;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	operation.params[0].value.b = ta_heapHandle;
#endif
	operation.started = 1;
	result = InvokeCommandHelper(index, pSession, VPP_INITIATE_VPPS, &operation, NULL);
	VPP_TEEC_LOGIFERROR(result);
	result = operation.params[1].value.a;

	return result;
}

int VPP_CA_GetCPCBOutputResolution(int cpcbID, int *pResID)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = cpcbID;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;
	operation.params[1].value.b = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index, pSession, VPP_GETCPCBOUTRES, &operation, NULL);
	*pResID = operation.params[1].value.a;

	return operation.params[1].value.b;
}

int VPP_CA_GetResolutionDescription(VPP_RESOLUTION_DESCRIPTION *pResDesc_PhyAddr,
		VPP_SHM_ID AddrId, unsigned int Size, unsigned int ResId)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;
	TEEC_SharedMemory TzShm;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(&TzShm, 0, sizeof(TEEC_SharedMemory));
	//Need to pass the physical address instead of virtual address - use .phyAddr instead of .buffer
	TzShm.phyAddr = pResDesc_PhyAddr;
	TzShm.size = Size;
	TzShm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	TEEC_RegisterSharedMemory(&g_vppCaContext.context, &TzShm);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_MEMREF_WHOLE,
			TEEC_VALUE_INOUT,
			TEEC_NONE);

	operation.params[0].value.a = AddrId;
	operation.params[1].memref.parent = &TzShm;
	operation.params[1].memref.size = TzShm.size;
	operation.params[1].memref.offset = 0;
	operation.params[2].value.a = ResId;

	operation.started = 1;
	result = InvokeCommandHelper(index, pSession, VPP_PASSSHM, &operation, NULL);

	return operation.params[2].value.a;
}

int VPP_CA_GetCurrentHDCPVersion(int *pHDCPVersion)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(&operation, 0, sizeof(TEEC_Operation));
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INOUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);
	operation.params[0].value.a = 0x0;

	/*Clear the return value*/
	operation.params[0].value.b = 0xdeadbeef;


	operation.started = 1;
	result = InvokeCommandHelper(index, pSession, VPP_GETCURHDCPVERSION, &operation, NULL);
	VPP_TEEC_LOGIFERROR(result);

	*pHDCPVersion = ((operation.params[0].value.a) & 0xFF);

	return operation.params[0].value.b;
}

int VPP_CA_PassVbufInfo_Phy(void *Vbuf, unsigned int VbufSize,
						 void *Clut, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	/* register Clut info*/
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INOUT);

	operation.params[0].value.a = ShmID;
	operation.params[0].value.b = (int)(long)Vbuf;
	operation.params[3].value.a = PlaneID;

	/* clear result */
	operation.params[3].value.b = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_PASSPHY,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[3].value.b;
}

int VPP_CA_PassVbufInfo(void *Vbuf_phyAddr, unsigned int VbufSize,
						 void *Clut_phyAddr, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
	int index;
	TEEC_Session *pSession;

	TEEC_Result result;
	TEEC_Operation operation;
	TEEC_SharedMemory TzShmVbuf, TzShmClut;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	VPP_CA_DBG_PRINT("%s:%d: vbuf_info:%p, VbufSize:%x, Clut:%p, ClutSize:%x, PlaneID:%d, ClutValid:%d\n",
		__func__, __LINE__, Vbuf, VbufSize, Clut, ClutSize, PlaneID, ClutValid);

	memset(&operation, 0, sizeof(TEEC_Operation));
	/* register Vbuf info*/
	memset(&TzShmVbuf, 0, sizeof(TEEC_SharedMemory));
	//Need to pass the physical address instead of virtual address
	TzShmVbuf.phyAddr = Vbuf_phyAddr;
	TzShmVbuf.size = VbufSize;
	TzShmVbuf.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	result = TEEC_RegisterSharedMemory(&g_vppCaContext.context, &TzShmVbuf);
	VPP_CA_DBG_PRINT("%s:%d: result:%x\n", __func__, __LINE__, result);
	/* register Clut info*/
	if (ClutValid) {
		memset(&TzShmClut, 0, sizeof(TEEC_SharedMemory));
		TzShmClut.phyAddr = Clut_phyAddr;
		TzShmClut.size = ClutSize;
		TzShmClut.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

		result = TEEC_RegisterSharedMemory(&g_vppCaContext.context, &TzShmClut);

		operation.params[2].memref.parent = &TzShmClut;
		operation.params[2].memref.size = TzShmClut.size;
		operation.params[2].memref.offset = 0;
		operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_MEMREF_WHOLE,
			TEEC_MEMREF_WHOLE,
			TEEC_VALUE_INOUT);
	} else {
		operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_MEMREF_WHOLE,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INOUT);
	}

	operation.params[0].value.a = ShmID;
	operation.params[1].memref.parent = &TzShmVbuf;
	operation.params[1].memref.size = TzShmVbuf.size;
	operation.params[1].memref.offset = 0;
	operation.params[3].value.a = PlaneID;

	/* clear result */
	operation.params[3].value.b = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index, pSession, VPP_PASSSHM, &operation, NULL);

	return operation.params[3].value.b;
}

int VPP_CA_Init(VPP_INIT_PARM *vpp_init_parm)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;
	int k;
	UINT32 vpp_addr;
	UINT32 ta_heapHandle;
	TEEC_Operation operation2;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
		operation.paramTypes = TEEC_PARAM_TYPES(
				TEEC_VALUE_INOUT,
				TEEC_VALUE_OUTPUT,
				TEEC_VALUE_INPUT,
				TEEC_NONE);
	operation.params[0].value.a = vpp_init_parm->iHDMIEnable;
	operation.params[0].value.b = vpp_init_parm->iVdacEnable;

#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	operation.params[2].value.a = vpp_init_parm->uiShmPA;
	operation.params[2].value.b = vpp_init_parm->uiShmSize;
#endif
	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;
	operation.params[1].value.b = 0x0;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_INIT,
			&operation,
			NULL);

	VPP_TEEC_LOGIFERROR(result);

	vpp_init_parm->g_vpp = operation.params[1].value.b;
	vpp_addr = vpp_init_parm->g_vpp;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	vpp_init_parm->gMemhandle = operation.params[0].value.b;
	ta_heapHandle = vpp_init_parm->gMemhandle;
#endif
	operation2.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation2.params[1].value.a = 0xdeadbeef;
	operation2.params[0].value.a = vpp_addr;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	operation2.params[0].value.b = ta_heapHandle;
#endif
	operation2.started = 1;

	for (k = TAVPP_API_INSTANCE; k < MAX_TAVPP_INSTANCE_NUM; k++) {
		pSession = &(TAVPPInstance[k].session);
		InvokeCommandHelper(k,
			pSession,
			VPP_INITIATE_VPPS,
			&operation2,
			NULL);
	}

	return operation.params[1].value.a;

}

int VPP_CA_Create(void)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	/* clear result */
	operation.params[0].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_CREATE,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[0].value.a;
}


int VPP_CA_Reset(void)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	/* clear result */
	operation.params[0].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_RESET,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[0].value.a;
}


int VPP_CA_Config(void)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	/* clear result */
	operation.params[0].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_CONFIG,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[0].value.a;
}

int VPP_CA_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
#ifdef TAVPP_USE_SEPARATE_ISR_INSTANCE
	index = TAVPP_ISR_INSTANCE;
#endif //TAVPP_USE_SEPARATE_ISR_INSTANCE
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = MsgId;
	operation.params[0].value.b = IntSts;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_HANDLEINT,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[1].value.a;
}


int VPP_CA_SetOutRes(int CpcbId, int ResId, int BitDepth)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE);

	operation.params[0].value.a = CpcbId;
	operation.params[0].value.b = ResId;
	operation.params[1].value.a = BitDepth;

	/* clear result */
	operation.params[2].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_SETRES,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[2].value.a;
}

int VPP_CA_OpenDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INOUT);

	operation.params[0].value.a = PlaneId;
	operation.params[0].value.b = WinX;
	operation.params[1].value.a = WinY;
	operation.params[1].value.b = WinW;
	operation.params[2].value.a = WinH;
	operation.params[2].value.b = BgClr;
	operation.params[3].value.a = Alpha;

	/* clear result */
	operation.params[3].value.b = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_OPENDISPWIN,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[3].value.b;
}

int VPP_CA_RecycleFrame(int PlaneId)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = PlaneId;


	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_RECYCLEFRAME,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[1].value.a;
}

int VPP_CA_SetDispMode(int PlaneId, int Mode)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = PlaneId;
	operation.params[0].value.b = Mode;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_SETDISPMODE,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[1].value.a;
}

int VPP_CA_HdmiSetVidFmt(int ColorFmt, int BitDepth, int PixelRep)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE);

	operation.params[0].value.a = ColorFmt;
	operation.params[0].value.b = BitDepth;
	operation.params[1].value.a = PixelRep;

	/* clear result */
	operation.params[2].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_HDMISETVIDFMT,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[2].value.a;
}


int VPP_CA_SetRefWin(int PlaneId, int WinX, int WinY, int WinW, int WinH)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT);

	operation.params[0].value.a = PlaneId;
	operation.params[0].value.b = WinX;
	operation.params[1].value.a = WinY;
	operation.params[1].value.b = WinW;
	operation.params[2].value.a = WinH;

	/* clear result */
	operation.params[3].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_SETREFWIN,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[3].value.a;
}

int VPP_CA_ChangeDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INOUT);

	operation.params[0].value.a = PlaneId;
	operation.params[0].value.b = WinX;
	operation.params[1].value.a = WinY;
	operation.params[1].value.b = WinW;
	operation.params[2].value.a = WinH;
	operation.params[2].value.b = BgClr;
	operation.params[3].value.a = Alpha;

	/* clear result */
	operation.params[3].value.b = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_CHANGEDISPWIN,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[3].value.b;
}


int VPP_CA_SetPlaneMute(int planeID, int mute)
{
	TEEC_Result result;
	TEEC_Operation operation;
	int index;
	TEEC_Session *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = planeID;
	operation.params[0].value.b = mute;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_SETPLANEMUTE,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[1].value.a;
}

int VPP_CA_Suspend(int enable)
{
	TEEC_Result result;
	TEEC_Operation operation;
	int index;
	TEEC_Session *pSession;
	VPP_CMD_ID cmd;

	cmd = enable ? VPP_SUSPEND : VPP_RESUME;
	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INOUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = 0xdeadbeef;
	operation.params[0].value.b = 0;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			cmd,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[0].value.a;
}

int VPP_CA_SetHdmiTxControl(int enable)
{
	TEEC_Result result;
	TEEC_Operation operation;
	int index;
	TEEC_Session *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE);

	operation.params[0].value.a = enable;

	/* clear result */
	operation.params[1].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_HDMISETTXCTRL,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[1].value.a;
}

int VPP_CA_stop(void)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	/* clear result */
	operation.params[0].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_STOP,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[0].value.a;
}

int VPP_CA_Destroy(void)
{
	int index;
	TEEC_Session *pSession;
	TEEC_Result result;
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	/* clear result */
	operation.params[0].value.a = 0xdeadbeef;

	operation.started = 1;
	result = InvokeCommandHelper(index,
			pSession,
			VPP_DESTROY,
			&operation,
			NULL);
	VPP_TEEC_LOGIFERROR(result);

	return operation.params[0].value.a;
}

#ifdef VPP_CA_ENABLE_PASS_VBUFINFO_BY_PARAMETER
int VPP_CA_PassVbufInfoPar(unsigned int *Vbuf, unsigned int VbufSize,
						 unsigned int *Clut, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
	int index, i, flag;
	TEEC_Session *pSession;
	TEEC_Operation operation;
	VBUF_INFO *pVBufInfo = (VBUF_INFO *) Vbuf;
	TEEC_Result result;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	operation.paramTypes = TEEC_PARAM_TYPES(
		TEEC_VALUE_INOUT,
		TEEC_VALUE_INOUT,
		TEEC_VALUE_INOUT,
		TEEC_VALUE_INOUT);

	for (i = 0; i < 5; i++) {
		flag = 0;
		switch (i) {
		case 0:
			operation.params[1].value.a = (unsigned int)((intptr_t)pVBufInfo->m_pbuf_start);
			operation.params[1].value.b = pVBufInfo->m_bytes_per_pixel;
			operation.params[2].value.a = pVBufInfo->m_bits_per_pixel;
			operation.params[2].value.b = pVBufInfo->m_srcfmt;
			operation.params[3].value.a = pVBufInfo->m_order;
			flag = 1;
			break;
		case 1:
			operation.params[1].value.a = pVBufInfo->m_content_width;
			operation.params[1].value.b = pVBufInfo->m_content_height;
			operation.params[2].value.a = pVBufInfo->m_buf_stride;
			operation.params[2].value.b = pVBufInfo->m_buf_pbuf_start_UV;
			operation.params[3].value.a = pVBufInfo->m_buf_size;
			flag = 1;
			break;
		case 2:
			operation.params[1].value.a = pVBufInfo->m_allocate_type;
			operation.params[1].value.b = pVBufInfo->m_buf_type;
			operation.params[2].value.a = pVBufInfo->m_buf_use_state;
			operation.params[2].value.b = pVBufInfo->m_flags;
			operation.params[3].value.a = pVBufInfo->m_is_frame_seq;
			flag = 1;
			break;
		case 3:
			operation.params[1].value.a = pVBufInfo->m_frame_rate_num;
			operation.params[1].value.b = pVBufInfo->m_frame_rate_den;
			operation.params[2].value.a = pVBufInfo->m_active_width;
			operation.params[2].value.b = pVBufInfo->m_active_height;
			operation.params[3].value.a = pVBufInfo->m_active_left;
			flag = 1;
			break;
		case 4:
			operation.params[1].value.a = pVBufInfo->m_active_top;
			operation.params[1].value.b = pVBufInfo->m_content_offset;
			operation.params[2].value.a = pVBufInfo->m_is_progressive_pic;
			operation.params[2].value.b = pVBufInfo->m_hDesc;
			flag = 1;
			break;
		}

		if (flag) {
			operation.params[0].value.a = (ShmID | (PlaneID << 8));
			operation.params[0].value.b = i;
			/* clear result */
			operation.params[3].value.b = 0xdeadbeef;

			operation.started = 1;
			result = InvokeCommandHelper(index,
					pSession,
					VPP_PASSPAR,
					&operation,
					NULL);
		}
	}

	return result;
}
#endif //VPP_CA_ENABLE_PASS_VBUFINFO_BY_PARAMETER


int VPP_CA_SemOper(int cmd_id, int sem_id, int *pParam)
{
	int index;
	TEEC_Session *pSession;
	#ifdef VPP_CA_ENABLE_CMD_VPP_SEMOPER
	TEEC_Result result;
	#endif
	TEEC_Operation operation;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);
	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INPUT,
			TEEC_VALUE_INOUT);

	operation.params[0].value.a = cmd_id;
	operation.params[0].value.b = sem_id;
	if (pParam)
		operation.params[1].value.a = *pParam;

	/* clear result */
	operation.params[1].value.b = 0xdeadbeef;

	operation.started = 1;
	#ifdef VPP_CA_ENABLE_CMD_VPP_SEMOPER
	result = InvokeCommandHelper(index, pSession, VPP_SEMOPER, &operation, NULL);
	VPP_TEEC_LOGIFERROR(result);
	#endif

	if (pParam)
		*pParam = operation.params[1].value.a;

	return operation.params[1].value.b;
}
