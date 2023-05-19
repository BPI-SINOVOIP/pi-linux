// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _HAL_VPP_WRAP_H_
#define _HAL_VPP_WRAP_H_
/** VPP_API wrap for NTZ and TZ world
 */

#include "ctypes.h"
#include "hal_vpp.h"

/*structure define*/

#ifdef __cplusplus
extern "C" {
#endif

int wrap_MV_VPP_InitVPPS(ENUM_TA_UUID_TYPE uuidType, unsigned int *vpp_init_parm);

int wrap_MV_VPPOBJ_GetCPCBOutputResolution(int cpcbID, int *resID);

int wrap_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc);

int wrap_MV_VPP_GetCurrentHDCPVersion(int *pHDCPVersion);

int wrap_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew);

int wrap_MV_VPP_Init(VPP_INIT_PARM *vpp_init_parm);
int wrap_MV_VPP_DeInit(void);
int wrap_MV_VPPOBJ_Create(int base_addr, int *handle);
int wrap_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
		const int *pzorder_cfg, const int *pvoutport_cfg);
int wrap_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth);
int wrap_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept);
int wrap_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);
int wrap_MV_VPPOBJ_SetDisplayMode(int planeID, int mode);
int wrap_MV_VPPOBJ_DisplayFrame(int planeID, void *frame);
int wrap_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win);
int wrap_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);
int wrap_MV_VPPOBJ_SetPlaneMute(int planeID, int mute);
int wrap_MV_VPPOBJ_SetHdmiTxControl(int enable);
int wrap_MV_VPPOBJ_Suspend(int enable);
int wrap_MV_VPPOBJ_RecycleFrames(int planeID);
int wrap_MV_VPPOBJ_Stop(void);
int wrap_MV_VPPOBJ_Reset(void);
int wrap_MV_VPPOBJ_Destroy(void);
int wrap_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts);
int wrap_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam);
int wrap_MV_VPP_WaitVsync(void);
int wrap_MV_VPPOBJ_EnableHdmiAudioFmt(int enable);
int wrap_MV_VPPOBJ_InvokePassShm_Helper(void *pBuffer, unsigned int shmCmdId,
		unsigned int sBufferSize);
int wrap_MV_VPPOBJ_SetFormat(int cpcbID,
			VPP_DISP_OUT_PARAMS *pDispParams);
int wrap_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock);
int wrap_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size);
void wrap_MV_VPP_MIPI_Reset(int enable);
#ifdef __cplusplus
}
#endif
/** VPP_API Wrap
 */
#endif
/** ENDOFFILE: hal_vpp_wrap.h
 */
