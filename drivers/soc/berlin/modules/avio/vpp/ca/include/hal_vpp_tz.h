// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _HAL_VPP_TZ_H_
#define _HAL_VPP_TZ_H_
/** hal_vpp_tz.h for TZ
 */

#include "hal_vpp.h"

#ifdef __cplusplus
extern "C" {
#endif

//API's     --
//vpp_init_parm[0] => vpp_addr, vpp_init_parm[1] => ta_heapHandle
int TZ_MV_VPP_InitVPPS(ENUM_TA_UUID_TYPE uuidType, unsigned int *vpp_init_parm);

int TZ_MV_VPPOBJ_GetCPCBOutputResolution(int cpcbID, int *resID);

int TZ_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc);

int TZ_MV_VPP_GetCurrentHDCPVersion(int *pHDCPVersion);

int TZ_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew, void **pold);

int TZ_MV_VPP_Init(ENUM_TA_UUID_TYPE uuidType, VPP_INIT_PARM *vpp_init_parm);
int TZ_MV_VPP_DeInit(void);
int TZ_MV_VPPOBJ_Create(int base_addr, int *handle);
int TZ_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
		const int *pzorder_cfg, const int *pvoutport_cfg);
int TZ_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth);
int TZ_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept);
int TZ_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);
int TZ_MV_VPPOBJ_SetDisplayMode(int planeID, int mode);
int TZ_MV_VPPOBJ_DisplayFrame(int planeID, void *frame);
int TZ_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win);
int TZ_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);
int TZ_MV_VPPOBJ_SetPlaneMute(int planeID, int mute);
int TZ_MV_VPPOBJ_Suspend(int enable);
int TZ_MV_VPPOBJ_SetHdmiTxControl(int enable);
int TZ_MV_VPPOBJ_RecycleFrames(int planeID);
int TZ_MV_VPPOBJ_Stop(void);
int TZ_MV_VPPOBJ_Reset(void);
int TZ_MV_VPPOBJ_Destroy(void);
int TZ_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts);
int TZ_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam);
int TZ_MV_VPPOBJ_EnableHdmiAudioFmt(int enable);
int TZ_MV_VPPOBJ_InvokePassShm_Helper(void *pBuffer, unsigned int shmCmdId,
		unsigned int sBufferSize);
int TZ_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock);
int TZ_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size);
#ifdef __cplusplus
}
#endif
/** _HAL_VPP_TZ_H_
 */
#endif // _HAL_VPP_TZ_H_
/** ENDOFFILE: hal_vpp_tz.h
 */
