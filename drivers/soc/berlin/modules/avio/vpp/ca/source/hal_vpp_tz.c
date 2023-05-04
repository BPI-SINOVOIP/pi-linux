// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "tee_ca_vpp.h"
#include "hal_vpp_tz.h"

int TZ_MV_VPP_InitVPPS(ENUM_TA_UUID_TYPE uuidType, unsigned int *vpp_init_parm)
{
	int retVal = 0;
	unsigned int vpp_addr;
	unsigned int ta_heapHandle;

	if (vpp_init_parm) {
		vpp_addr        = vpp_init_parm[0];
		ta_heapHandle   = vpp_init_parm[1];

		//First initialize the VPP TA context/session
		VPP_CA_Initialize(uuidType);

		retVal = VPP_CA_InitVPPS(vpp_addr, ta_heapHandle);
	}

	return retVal;
}

int TZ_MV_VPPOBJ_GetCPCBOutputResolution(int cpcbID, int *pResID)
{
	int retVal = 0;

	if (pResID)
		retVal = VPP_CA_GetCPCBOutputResolution(cpcbID, pResID);

	return retVal;
}

int TZ_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc)
{
	int retVal = 0;

	if (pResDesc)
		retVal = VPP_CA_GetResolutionDescription(pResDesc, VPP_GET_RES_DESCRIPTION, sizeof(VPP_RESOLUTION_DESCRIPTION), ResId);

	return retVal;
}

int TZ_MV_VPP_GetCurrentHDCPVersion(int *pHDCPVersion)
{
	int retVal = 0;

	if (pHDCPVersion)
		retVal = VPP_CA_GetCurrentHDCPVersion(pHDCPVersion);

	return retVal;
}

int TZ_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew, void **pold)
{
	int retVal = 0;
	VPP_VBUF *pVBufInfo;

	pVBufInfo = (VPP_VBUF *)pnew;

#ifdef TZ_MV_ENABLE_CLUT_PASSING
	int ClutValid = 0;

	if ((pVBufInfo->m_srcfmt == SRCFMT_LUT8)
		&& (pVBufInfo->m_clut_ptr)
		&& (planeID == PLANE_GFX1))
		ClutValid = 1;

	/* pass frame info */
	VPP_CA_PassVbufInfo((unsigned int *)pVBufInfo, sizeof(VPP_VBUF),
					pVBufInfo->m_clut_ptr, pVBufInfo->m_clut_num_items * 4,
					planeID, ClutValid, SET_STILL_PICTURE);
#else
	/* pass frame info */
	VPP_CA_PassVbufInfo(pVBufInfo, sizeof(VPP_VBUF),
					0, 0, planeID, 0, SET_STILL_PICTURE);
#endif

	return retVal;
}

int TZ_MV_VPP_DeInit(void)
{
	VPP_CA_Finalize();

	return MV_VPP_OK;
}

/***********************************************************
 * FUNCTION: initialize VPP module
 * PARAMS: none
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADCALL - function called previously
 **********************************************************/
int TZ_MV_VPP_Init(ENUM_TA_UUID_TYPE uuidType, VPP_INIT_PARM *vpp_init_parm)
{

	VPP_CA_Initialize(uuidType);

	VPP_CA_Init(vpp_init_parm);

	return MV_VPP_OK;
}

/***********************************************
 * FUNCTION: create a VPP object
 * PARAMS: *handle - pointer to object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
int TZ_MV_VPPOBJ_Create(int base_addr, int *handle)
{
	VPP_CA_Create();

	return MV_VPP_OK;
}

/***************************************
 * FUNCTION: VPP profile configuration
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int TZ_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg, const int *pzorder_cfg, const int *pvoutport_cfg)
{

	return VPP_CA_Config();
}

/*******************************************************************
 * FUNCTION: set CPCB or DV output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        resID - id of output resolution
 *        bit_depth - HDMI deep color bit depth
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that CPCB or DV.
 *******************************************************************/
int TZ_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
	HRESULT Ret = MV_VPP_OK;

	Ret = VPP_CA_SetOutRes(cpcbID, resID, bit_depth);

	return Ret;
}

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int TZ_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept)
{
	return VPP_CA_HdmiSetVidFmt(color_fmt, bit_depth, pixel_rept);
}

/******************************************************************************
 * FUNCTION: open a window of a video/graphics plane for display.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int TZ_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	HRESULT Ret = MV_VPP_OK;
	int params[7];

	if (!win)
		return MV_VPP_EBADPARAM;

	if ((win->width <= 0) || (win->height <= 0))
		return MV_VPP_EBADPARAM;

	params[0] = planeID;
	params[1] = win->x;
	params[2] = win->y;
	params[3] = win->width;
	params[4] = win->height;

	/* set video plane background window */
	if (attr) {
		params[5] = attr->bgcolor;
		params[6] = attr->alpha;
	} else {
		params[5] = -1;
		params[6] = -1;
	}

	Ret = VPP_CA_OpenDispWin(params[0],
				   params[1],
				   params[2],
				   params[3],
				   params[4],
				   params[5],
				   params[6]);

	return Ret;
}

/*******************************************************************
 * FUNCTION: set display mode for a plane
 * INPUT: planeID - id of the plane
 *        mode - DISP_STILL_PIC: still picture, DISP_FRAME: frame
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int TZ_MV_VPPOBJ_SetDisplayMode(int planeID, int mode)
{
	return VPP_CA_SetDispMode(planeID, mode);
}

/*******************************************************************
 * FUNCTION: display a frame for a plane
 * INPUT: planeID - id of the plane
 *        *frame - frame descriptor
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int TZ_MV_VPPOBJ_DisplayFrame(int planeID, void *frame)
{
	/* pass frame info */
#ifndef CONFIG_VPP_ENABLE_PASSPAR
	VPP_CA_PassVbufInfo_Phy((unsigned int *)frame, sizeof(VPP_VBUF),
					(unsigned int *)NULL, sizeof(VPP_VBUF),
					planeID, 0, DISPLAY_FRAME);
#else
	VPP_CA_PassVbufInfoPar((unsigned int *)frame, sizeof(VPP_VBUF),
					(unsigned int *)NULL, sizeof(VPP_VBUF),
					planeID, 0, DISPLAY_FRAME);
#endif
	return MV_VPP_OK;
}


/***************************************************************
 * FUNCTION: set the reference window for a plane
 * INPUT: planeID - id of the plane
 *        *win - pointer to the reference window struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 **************************************************************/
int TZ_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win)
{
	HRESULT Ret = MV_VPP_OK;

	Ret = VPP_CA_SetRefWin(planeID,
			win->x,
			win->y,
			win->width,
			win->height);

	return Ret;
}

/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of a video/grahpics plane
 *        *win - pointer to a vpp window struct
 *        *attr - pointer to a vpp window attribute struct
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int TZ_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	HRESULT Ret = MV_VPP_OK;
	int params[7];

	if ((!win) && (!attr))
		return MV_VPP_EBADPARAM;

	params[0] = planeID;

	if (win) {
		if ((win->width <= 2) || (win->height <= 2))
			return MV_VPP_EBADPARAM;

		params[1] = win->x;
		params[2] = win->y;
		params[3] = win->width;
		params[4] = win->height;
	} else {
		params[1] = -1; /* no change */
		params[2] = -1;
		params[3] = -1;
		params[4] = -1;
	}
	if (attr) {
		params[5] = attr->bgcolor;
		params[6] = attr->alpha;
	} else {
		params[5] = -1; /* no change */
		params[6] = -1;
	}

	Ret = VPP_CA_ChangeDispWin(params[0],
			params[1],
			params[2],
			params[3],
			params[4],
			params[5],
			params[6]);

	return Ret;

}

/***************************************************
 * FUNCTION: mute/un-mute a plane
 * PARAMS:  planeID - plane to mute/un-mute
 *          mute - 1: mute, 0: un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int TZ_MV_VPPOBJ_SetPlaneMute(int planeID, int mute)
{
	return VPP_CA_SetPlaneMute(planeID, mute);
}

/***************************************************
 * FUNCTION: Suspend/Resume
 * PARAMS:  enable - 1: Suspend, 0: Resume
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int TZ_MV_VPPOBJ_Suspend(int enable)
{
	return VPP_CA_Suspend(enable);
}

/***************************************************
 * FUNCTION: enable/disable HDMI Tx output
 * PARAMS:  enable - 1: enable, 0: disable
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int TZ_MV_VPPOBJ_SetHdmiTxControl(int enable)
{
	return VPP_CA_SetHdmiTxControl(enable);
}

/************************************************************************
 * FUNCTION: recycle plane frame descriptors
 * INPUT: planeID - id of the plane
 * RETURN: NULL - no frame descriptor for recycle
 *         other - pointer to frame descriptor being recycled
 ************************************************************************/
int TZ_MV_VPPOBJ_RecycleFrames(int planeID)
{
	return VPP_CA_RecycleFrame(planeID);
}

int TZ_MV_VPPOBJ_Stop(void)
{
	return VPP_CA_stop();
}

/***************************************
 * FUNCTION: VPP reset
 * INPUT: NONE
 * RETURN: NONE
 **************************************/
int TZ_MV_VPPOBJ_Reset(void)
{
	return VPP_CA_Reset();
}

/***********************************************
 * FUNCTION: destroy a VPP object
 * PARAMS: handle - VPP object handle
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EUNCONFIG - not initialized
 *         MV_VPP_ENODEV - no device
 *         MV_VPP_ENOMEM - no memory
 ***********************************************/
int TZ_MV_VPPOBJ_Destroy(void)
{
	VPP_CA_Destroy();

	return MV_VPP_OK;
}

int TZ_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	VPP_CA_IsrHandler(MsgId, IntSts);

	return MV_VPP_OK;
}

int TZ_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam)
{
	HRESULT Ret = MV_VPP_OK;

	VPP_CA_SemOper(cmd_id, sem_id, pParam);

	return Ret;
}
