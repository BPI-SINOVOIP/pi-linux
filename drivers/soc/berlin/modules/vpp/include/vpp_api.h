// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _VPP_API_H_
#define _VPP_API_H_

#include <linux/ion.h>

#define VPP_SHM_STATIC		1
#define VPP_SHM_VIDEO_FB	2

#define MV_VPP_PANEL_RESOLUTION_NDX_START	2

typedef void (*CallbackHandler)(void* data);

typedef void *SHM_HANDLE;

typedef struct vpp_config_params {
	u32 frame_rate;
	u32 frame_size_ndx;
	u32 disp_res_ndx;
	u32 fb_count;
	int enable_frame_buf_copy;
	CallbackHandler callback;
	void *data;
}vpp_config_params;

typedef struct mrvl_vpp_dev {
	int major;
	int minor;
	void *ionhandle;
	void *ionvaddr;
	phys_addr_t ionpaddr;
	size_t phy_size;
} mrvl_vpp_dev;

bool MV_VPP_IsRecoveryMode(void);
void *MV_VPP_GetFrame(mrvl_vpp_dev *vdev, int x,
			int y, int w, int h, int stride);
int MV_VPP_Init(void);
void MV_VPP_Deinit(void);
void MV_VPP_DisplayFrame(mrvl_vpp_dev *vdev, int x,
			int y, int w, int h, int stride);
void MV_VPP_GetFrameSize(int *pWidth, int *pHeight);
int MV_VPP_SetHdmiTxControl(int enable);
void MV_VPP_ConfigParams(vpp_config_params* param);
int MV_VPP_Config(void);
int is_vpp_driver_initialized(void);
#endif //_VPP_API_H_
