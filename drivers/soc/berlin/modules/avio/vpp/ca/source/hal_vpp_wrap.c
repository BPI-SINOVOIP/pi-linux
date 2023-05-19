// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/export.h>
#include <linux/delay.h>
#include "hal_vpp.h"
#include "hal_vpp_tz.h"
#include "hal_vpp_wrap.h"
#include "tee_ca_vpp.h"
#include "drv_dhub.h"
#include "drv_vpp.h"

int wrap_MV_VPP_InitVPPS(ENUM_TA_UUID_TYPE uuidType, unsigned int *vpp_init_parm)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled && vpp_init_parm)
		retVal = TZ_MV_VPP_InitVPPS(uuidType, vpp_init_parm);

	return retVal;
}

int wrap_MV_VPPOBJ_GetCPCBOutputResolution(int cpcbID, int *pResID)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled && pResID)
		retVal = TZ_MV_VPPOBJ_GetCPCBOutputResolution(cpcbID, pResID);

	return retVal;
}

int wrap_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc)
{
	int retVal = 0;

	if (pResDesc)
		retVal = TZ_MV_VPPOBJ_GetResolutionDescription(ResId, pResDesc);

	return retVal;
}

int wrap_MV_VPP_GetCurrentHDCPVersion(int *pHDCPVersion)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled && pHDCPVersion)
		retVal = TZ_MV_VPP_GetCurrentHDCPVersion(pHDCPVersion);

	return retVal;
}

int wrap_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;
	void **pold = 0;

	if (hDhubCtx->isTeeEnabled && pnew)
		retVal = TZ_MV_VPPOBJ_SetStillPicture(planeID, pnew, pold);

	return retVal;
}

extern int wait_vpp_primary_vsync(void);
int wrap_MV_VPP_WaitVsync() {
	return wait_vpp_primary_vsync();
}

int wrap_MV_VPP_Init(VPP_INIT_PARM *vpp_init_parm)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled && vpp_init_parm)
		retVal = TZ_MV_VPP_Init(TA_UUID_FASTLOGO, vpp_init_parm);

	return retVal;
}

int wrap_MV_VPP_DeInit(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPP_DeInit();

	return retVal;
}

int wrap_MV_VPPOBJ_Create(int base_addr, int *handle)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Create(base_addr, handle);

	return retVal;
}

int wrap_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
	const int *pzorder_cfg, const int *pvoutport_cfg)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Config(pvinport_cfg, pdv_cfg, pzorder_cfg, pvoutport_cfg);

	return retVal;
}

int wrap_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetCPCBOutputResolution(cpcbID, resID, bit_depth);

	return retVal;
}

int wrap_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetHdmiVideoFmt(color_fmt, bit_depth, pixel_rept);

	return retVal;
}

int wrap_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_OpenDispWindow(planeID, win, attr);

	return retVal;
}

int wrap_MV_VPPOBJ_SetDisplayMode(int planeID, int mode)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetDisplayMode(planeID, mode);

	return retVal;
}

int wrap_MV_VPPOBJ_DisplayFrame(int planeID, void *frame)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_DisplayFrame(planeID, frame);

	return retVal;
}

int wrap_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetRefWindow(planeID, win);

	return retVal;
}

int wrap_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_ChangeDispWindow(planeID, win, attr);

	return retVal;
}

int wrap_MV_VPPOBJ_SetPlaneMute(int planeID, int mute)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetPlaneMute(planeID, mute);

	return retVal;
}

int wrap_MV_VPPOBJ_Suspend(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Suspend(enable);

	return retVal;
}

int wrap_MV_VPPOBJ_SetHdmiTxControl(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetHdmiTxControl(enable);

	return retVal;
}

int wrap_MV_VPPOBJ_RecycleFrames(int planeID)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int frame = 0;

	if (hDhubCtx->isTeeEnabled)
		frame = TZ_MV_VPPOBJ_RecycleFrames(planeID);

	return frame;
}

int wrap_MV_VPPOBJ_Stop(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Stop();

	return retVal;
}

int wrap_MV_VPPOBJ_Reset(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Reset();

	return retVal;
}

int wrap_MV_VPPOBJ_Destroy(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Destroy();

	return retVal;
}

int wrap_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_IsrHandler(MsgId, IntSts);

	return retVal;
}

int wrap_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SemOper(cmd_id, sem_id, pParam);

	return retVal;
}

int wrap_MV_VPPOBJ_EnableHdmiAudioFmt(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_EnableHdmiAudioFmt(enable);

	return retVal;
}

int wrap_MV_VPPOBJ_InvokePassShm_Helper(void *pBuffer, unsigned int shmCmdId,
		unsigned int sBufferSize)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_InvokePassShm_Helper(pBuffer, shmCmdId, sBufferSize);

	return retVal;
}


INT wrap_MV_VPPOBJ_SetFormat (INT cpcbID, VPP_DISP_OUT_PARAMS *pDispParams)
{
	HRESULT Ret = MV_VPP_OK;
	UINT32 pDispParamsData[VPP_SETFORMAT_SIZE];

	memcpy(&pDispParamsData[0], &cpcbID, sizeof(INT));
	memcpy(&pDispParamsData[1], pDispParams, sizeof(VPP_DISP_OUT_PARAMS));
	Ret = TZ_MV_VPPOBJ_InvokePassShm_Helper(pDispParamsData, VPP_SETFORMAT, VPP_SETFORMAT_SIZE);

	return Ret;
}

int wrap_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock)
{
	HRESULT Ret = MV_VPP_OK;

	Ret = TZ_MV_VPPOBJ_GetCPCBOutputPixelClock(resID, pixel_clock);
	return Ret;
}

int wrap_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size)
{
	return  TZ_MV_VPPOBJ_GetDispOutParams(pdispParams, size);
}

void wrap_MV_VPP_MIPI_Reset(int enable)
{
	void *hVppCtx = (void *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_VPP);

	drv_mipi_reset(hVppCtx, enable);
}

EXPORT_SYMBOL(wrap_MV_VPP_InitVPPS);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetCPCBOutputResolution);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetResolutionDescription);
EXPORT_SYMBOL(wrap_MV_VPP_GetCurrentHDCPVersion);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetStillPicture);
EXPORT_SYMBOL(wrap_MV_VPP_WaitVsync);
EXPORT_SYMBOL(wrap_MV_VPP_Init);
EXPORT_SYMBOL(wrap_MV_VPP_DeInit);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Create);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Config);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetCPCBOutputResolution);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetHdmiVideoFmt);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_OpenDispWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetDisplayMode);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_DisplayFrame);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetRefWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_ChangeDispWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetPlaneMute);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Suspend);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetHdmiTxControl);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_RecycleFrames);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Stop);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Reset);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Destroy);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_IsrHandler);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SemOper);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_EnableHdmiAudioFmt);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_InvokePassShm_Helper);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetFormat);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetCPCBOutputPixelClock);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetDispOutParams);
EXPORT_SYMBOL(wrap_MV_VPP_MIPI_Reset);
