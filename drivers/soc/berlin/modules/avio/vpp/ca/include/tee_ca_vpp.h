// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _TEE_VPP_CA_H_
#define _TEE_VPP_CA_H_
/** VPP_API TA wrapper
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "avio_type.h"
#include "avio_io.h"
#include "tee_client_api.h"
#include "vpp_vbuf.h"
#include "tee_ca_common.h"
#include "vpp_cmd.h"
#include "hal_vpp.h"



int VPP_CA_InitVPPS(UINT32 vpp_addr, UINT32 ta_heapHandle);
int VPP_CA_GetCPCBOutputResolution(int cpcbID, int *resID);
int VPP_CA_GetResolutionDescription(VPP_RESOLUTION_DESCRIPTION *pResDesc,
		VPP_SHM_ID AddrId, unsigned int Size, unsigned int ResId);
int VPP_CA_GetCurrentHDCPVersion(int *pHDCPVersion);
int VPP_CA_PassVbufInfo(void *Vbuf, unsigned int VbufSize,
						 void *Clut, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID);

int VPP_CA_Initialize(ENUM_TA_UUID_TYPE uuidType);
void VPP_CA_Finalize(void);

int VPP_CA_PassVbufInfo_Phy(void *Vbuf, unsigned int VbufSize,
						 void *Clut, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID);
int VPP_CA_Init(VPP_INIT_PARM *vpp_init_parm);
int VPP_CA_Create(void);
int VPP_CA_Reset(void);
int VPP_CA_Config(void);
int VPP_CA_SetOutRes(int CpcbId, int ResId, int BitDepth);
int VPP_CA_SetRefWin(int PlaneId, int WinX, int WinY, int WinW, int WinH);
int VPP_CA_OpenDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha);
int VPP_CA_ChangeDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha);
int VPP_CA_RecycleFrame(int PlaneId);
int VPP_CA_SetDispMode(int PlaneId, int Mode);
int VPP_CA_HdmiSetVidFmt(int ColorFmt, int BitDepth, int PixelRep);
int VPP_CA_SetPlaneMute(int planeID, int mute);
int VPP_CA_SetHdmiTxControl(int enable);
int VPP_CA_Suspend(int enable);

int VPP_CA_stop(void);
int VPP_CA_Destroy(void);
int VPP_CA_IsrHandler(unsigned int MsgId, unsigned int IntSts);

int VPP_CA_SemOper(int cmd_id, int sem_id, int *pParam);
#ifdef __cplusplus
}
#endif
/** VPP_API in trust zone
 */
#endif /* _TEE_VPP_CA_H_ */
/** ENDOFFILE: tee_ca_vpp.h
 */
