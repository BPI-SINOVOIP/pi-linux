// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include "vpp_api.h"
#include "vpp_defines.h"
#include "vpp_mem.h"
#include "hal_dhub_wrap.h"
#include "hal_vpp_wrap.h"
#include "avio_sub_module.h"

#define VPP_CA_INIT_MAGIC_NUM 0xFACE
#define IS_VPP_IN_NORMAL_MODE()	(vpp_config_mode&VPP_WITH_NORMAL_MODE)
//#define VPP_ENABLE_USE_SET_STILL_PICTURE
/*Enable the above macro when planes other than GFX is used for display with fastlogo.ta*/

void VPP_EnableDhubInterrupt(bool enable);

typedef struct mrvl_frame_size_t {
	int width;
	int height;
} mrvl_frame_size;

extern vpp_config_modes vpp_config_mode;
static vpp_config_params vpp_config_param = { 0 };
static VBUF_INFO **gDescArray;
static int gDescIndex;

int VPP_Clock_Set_Rate(unsigned int clk_rate);
int VPP_Clock_Set_Rate_Ext(unsigned int clk_rate);
void VPP_CreateISRTask(void);
void VPP_StopISRTask(void);
void FlushCache(dma_addr_t phys_start, unsigned int size);
int VPP_AllocateMemory(void **shm_handle, unsigned int type,
			unsigned int size, unsigned int align);
unsigned int VPP_SHM_CleanCache(uintptr_t phy_addr, unsigned int size);
int VPP_AllocateMemory_NC(void **shm_handle, unsigned int type,
			unsigned int size, unsigned int align);
int VPP_Memory_for_ta(bool bAllocate, unsigned int *pPhyAddr);

void MV_VPP_CopyFrameBuffer(VBUF_INFO *vbufinfo,mrvl_vpp_dev *vdev,int y,
			int width, int height, int stride, int bpp, int disp_width, int disp_height);
bool MV_VPP_IsRecoveryMode(void) {
	return (IS_VPP_IN_NORMAL_MODE() ? 0 : 1);
}

static void destroy_vbuf_desc(VBUF_INFO *pVBufInfo)
{
	if (pVBufInfo) {
		if (pVBufInfo->m_bufferID != NULL) {
			struct dma_buf *ion_dma_buf = (struct dma_buf *) pVBufInfo->hShm;
			void *alloc_addr = dma_buf_kmap(ion_dma_buf, 0);

			dma_buf_kunmap(ion_dma_buf, 0, alloc_addr);
			dma_buf_put(ion_dma_buf);
		}
		kfree(pVBufInfo);
	}
}

static int make_frame_data(unsigned int iVideo, unsigned int *pStartAddr,
			  unsigned int uiPicH, unsigned int uiLineV, unsigned int uiWidth,
			  unsigned int uiHeight, unsigned int uiPicA, unsigned int uiPicB)
{
	unsigned int *Ptr;
	unsigned int VLines, VRep, HS, HRep, PixVal, VBlock, HBlock, PixelPerWord;
	int Ret = 0;

	if (!pStartAddr) {
		pr_err("invalid input parameters\n");
		return -1;
	}

	/**
	 * make the whole frame data
	 */
	Ptr = pStartAddr;

	if (iVideo)
		PixelPerWord = 2;
	else
		PixelPerWord = 1;

	VBlock = uiLineV;

	for (VLines = 0; VLines < uiHeight; VLines += VBlock) {
		if ((uiHeight - VLines) < VBlock)
			VBlock = uiHeight - VLines;

		for (VRep = 0; VRep < VBlock; VRep++) {
			HBlock = uiPicH * PixelPerWord;

			for (HS = 0; HS < uiWidth; HS += HBlock) {
				if ((VLines / uiLineV +
						HS/(uiPicH * PixelPerWord))
						& 0x1)
					PixVal = uiPicB;
				else
					PixVal = uiPicA;

				if ((uiWidth - HS) < HBlock)
					HBlock = uiWidth - HS;

				for (HRep = 0; HRep < HBlock/PixelPerWord;
						HRep++)
					*Ptr++ = PixVal;
			}
		}
	}
	return Ret;
}

static void build_frames(VBUF_INFO *vbufinfo, void *vbuf,
	int srcfmt, int bit_depth, int x, int y, int width,
	int height, int progressive, int pattern_type, bool IsPatt)
{
	unsigned int datasize;

	/* set other fields of logo frame descriptor */
	if (srcfmt == SRCFMT_YUV422) {
		vbufinfo->m_bytes_per_pixel = 2;
		vbufinfo->m_srcfmt = SRCFMT_YUV422;
		vbufinfo->m_order = ORDER_UYVY;
	} else {
		vbufinfo->m_bytes_per_pixel = 4;
		//MSB(4th) byte is always alpha(0x00) in android buffer
		//use ARGB32 for VPP TA and XRGB32 for fastlogo.ta
		vbufinfo->m_srcfmt = IS_VPP_IN_NORMAL_MODE() ? SRCFMT_ARGB32 : SRCFMT_XRGB32;
	#ifdef VPP_ENABLE_ANDROID_FROMAT_BGRA_8888
		//ABGR_8888 or BGRA_8888
		vbufinfo->m_order = ORDER_BGRA;
	#else
		//RGBX_8888
		vbufinfo->m_order = ORDER_RGBA;
	#endif
	}

	datasize = width * height * vbufinfo->m_bytes_per_pixel;
	vbufinfo->m_content_width = width;
	vbufinfo->m_content_height = height;
	vbufinfo->m_buf_stride = vbufinfo->m_content_width *
					vbufinfo->m_bytes_per_pixel;
	vbufinfo->m_buf_size = (int)datasize;
	vbufinfo->m_flags = 1;
	vbufinfo->m_disp_offset = 0;
	vbufinfo->m_buf_stride = vbufinfo->m_content_width *
					vbufinfo->m_bytes_per_pixel;
	vbufinfo->m_active_width = width;
	vbufinfo->m_active_height = height;
	vbufinfo->m_active_left = x;
	vbufinfo->m_active_top = y;
	vbufinfo->m_content_offset = 0;
	if (IsPatt && vbuf) {
		if (srcfmt == SRCFMT_YUV422)
			make_frame_data(1, vbuf, 32, 36, width,
				height, 0x40604060, 0xb0a0b0a0);
		else
			make_frame_data(0, vbuf, 32, 36, width,
				height, 0xFF000000, 0xFF000000);
	}
}

static int create_new_frame(int width, int height, int progressive,
	int src_fmt, int bit_depth, int pattern_type,
	VBUF_INFO **frame)
{

	VBUF_INFO *vbufinfo = NULL;
	void *base = NULL;
	void *shm_handle = NULL;
	void *shm_handle_vbuf = NULL;
	int result = 0;
	phys_addr_t phy_base;

	struct dma_buf *ion_dma_buf = NULL;

	int datasize = 0;

	vbufinfo  = kzalloc(sizeof(VBUF_INFO), GFP_KERNEL | __GFP_NOWARN);
	if (!vbufinfo)
		return MV_VPP_ENOMEM;

	if (vpp_config_param.enable_frame_buf_copy) {
		if (src_fmt == SRCFMT_YUV422)
			datasize = width * height * 2;
		else
			datasize = width * height * 4;

		result = VPP_AllocateMemory(&shm_handle, VPP_SHM_VIDEO_FB,
				datasize, PAGE_SIZE);
		if (result == MV_VPP_ENOMEM) {
			pr_err("share memory allocation failed\n");
			goto err_free_vbufinfo;
		}

		ion_dma_buf = (struct dma_buf *) shm_handle;
		base = dma_buf_kmap(ion_dma_buf, 0);

		if (IS_ERR_OR_NULL(base)) {
			result = MV_VPP_ENOMEM;
			pr_err("share memory get vitual address failed\n");
			goto err;
		}

		phy_base = VPP_ion_dmabuf_get_phy(ion_dma_buf);
		if (phy_base == 0) {
			result = MV_VPP_ENOMEM;
			pr_err("get physical address failed %x\n", result);
			goto err;
		}

		vbufinfo->hShm	= shm_handle;
		vbufinfo->m_pbuf_start = phy_base;
		vbufinfo->m_bufferID = base;
	}

	build_frames(vbufinfo, base, src_fmt, bit_depth,
			0, 0, width, height,
			progressive, pattern_type, 1);

	//Allocate memory for VBUFINFO
	result = VPP_AllocateMemory(&shm_handle_vbuf, VPP_SHM_VIDEO_FB, sizeof(VPP_VBUF), PAGE_SIZE);
	if (result == MV_VPP_ENOMEM) {
		pr_err("share memory allocation failed\n");
		goto err;
	}

	ion_dma_buf = (struct dma_buf *) shm_handle_vbuf;
	vbufinfo->pVppVbufInfo_virt = dma_buf_kmap(ion_dma_buf, 0);
	if (IS_ERR(vbufinfo->pVppVbufInfo_virt)) {
		result = MV_VPP_ENOMEM;
		pr_err("share memory get vitual address failed\n");
		goto err1;
	}

	vbufinfo->pVppVbufInfo_phy = (void *)VPP_ion_dmabuf_get_phy(ion_dma_buf);
	if (vbufinfo->pVppVbufInfo_phy == NULL) {
		result = MV_VPP_ENOMEM;
		pr_err("get physical address failed %x\n", result);
		goto err1;
	}

	*frame = vbufinfo;
	return MV_VPP_OK;

err1:
	if (shm_handle_vbuf) {
		ion_dma_buf = (struct dma_buf *) shm_handle_vbuf;
		if (vbufinfo->pVppVbufInfo_virt)
			dma_buf_kunmap(ion_dma_buf, 0, base);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}
err:
	if (vpp_config_param.enable_frame_buf_copy)
		if (shm_handle) {
			if (base)
				dma_buf_kunmap(ion_dma_buf, 0, base);
			ion_dma_buf = (struct dma_buf *) shm_handle;
			dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
			dma_buf_put(ion_dma_buf);
	}
err_free_vbufinfo:
	if (vpp_config_param.enable_frame_buf_copy) {
		kfree(vbufinfo);
		*frame = NULL;
	}

	return MV_VPP_ENOMEM;
}

static void MV_VPP_GetOutResolutionSize(int *p_width, int *p_height)
{
	VPP_RESOLUTION_DESCRIPTION  ResDesc;
	int res;

	res = wrap_MV_VPPOBJ_GetResolutionDescription(vpp_config_param.disp_res_id, &ResDesc);
	if (!res) {
		*p_width = ResDesc.uiActiveWidth;
		*p_height = ResDesc.uiActiveHeight;
	} else {
		/* Default values to FB size in case of failure */
        MV_VPP_GetFrameSize(p_width, p_height);
	}
}

static void MV_VPP_GetInputFrameSize(int *width, int *height)
{
	int res_width, res_height;

	MV_VPP_GetFrameSize(width, height);
	MV_VPP_GetOutResolutionSize(&res_width, &res_height);
	if (res_width < res_height)
		swap(*width, *height);
}

int create_global_desc_array(void)
{
	unsigned char uiCnt;
	int Ret = 0;
	int width, height;

	MV_VPP_GetInputFrameSize(&width, &height);

	gDescArray = kzalloc(vpp_config_param.fb_count * sizeof(VBUF_INFO *), GFP_KERNEL | __GFP_NOWARN);
	if (gDescArray == NULL)
		return MV_VPP_ENOMEM;

	for (uiCnt = 0; uiCnt < vpp_config_param.fb_count; uiCnt++) {
		Ret = create_new_frame(width, height, 1,
			(IS_VPP_IN_NORMAL_MODE() ? SRCFMT_ARGB32 : SRCFMT_XRGB32),
			INPUT_BIT_DEPTH_8BIT, FB_PATTERN_CUSTOM,
			&(gDescArray[uiCnt]));
		if (Ret != MV_VPP_OK) {
			pr_err("%s fails\n", __func__);
			goto create_global_desc_array_exit;
		}
	}
	return 0;

create_global_desc_array_exit:
	for (uiCnt = 0; uiCnt < vpp_config_param.fb_count; uiCnt++) {
		if (gDescArray[uiCnt])
			destroy_vbuf_desc(gDescArray[uiCnt]);
	}
	kfree(gDescArray);
	return Ret;
}

void destroy_global_desc_array(void)
{
	unsigned char uiCnt;

	for (uiCnt = 0; uiCnt < vpp_config_param.fb_count; uiCnt++) {
		if (gDescArray[uiCnt])
			destroy_vbuf_desc(gDescArray[uiCnt]);
	}
	memset(gDescArray, 0, vpp_config_param.fb_count * sizeof(VBUF_INFO *));
	kfree(gDescArray);
}

void *MV_VPP_GetFrame(mrvl_vpp_dev *vdev, int x,
			int y, int w, int h, int stride)
{
	int disp_width, disp_height;
	VBUF_INFO *vbufinfo = gDescArray[gDescIndex];
	vbufinfo = gDescArray[gDescIndex];
	MV_VPP_GetOutResolutionSize(&disp_width, &disp_height);
	if (vbufinfo) {
		if (vpp_config_param.enable_frame_buf_copy) {
			MV_VPP_CopyFrameBuffer(vbufinfo, vdev, y, w, h, stride, vbufinfo->m_bytes_per_pixel, disp_width, disp_height);
		} else {
			vbufinfo->m_pbuf_start =  vdev->ionpaddr;
		}

		FlushCache((dma_addr_t)vbufinfo->m_pbuf_start, h*stride);
	}
	gDescIndex++;
	if (gDescIndex == vpp_config_param.fb_count)
		gDescIndex = 0;
	return vbufinfo;
}

void MV_VPP_GetFrameSize(int *pWidth, int *pHeight)
{
	int maxFrameSizeNdx;
	mrvl_frame_size frameSize[] = {
		{1280, 720},
		{1920, 1080}
	};
	maxFrameSizeNdx = ARRAY_SIZE(frameSize);
	vpp_config_param.frame_size_ndx = vpp_config_param.frame_size_ndx >= maxFrameSizeNdx ? 0 : vpp_config_param.frame_size_ndx;

	*pWidth = frameSize[vpp_config_param.frame_size_ndx].width;
	*pHeight = frameSize[vpp_config_param.frame_size_ndx].height;
}

void MV_VPP_ConfigParams(vpp_config_params *param)
{
	memcpy(&vpp_config_param, param, sizeof(vpp_config_param));
}

static void convert_frame_info(VBUF_INFO *pVbufInfo, VPP_VBUF *pVppBuf)
{
	pVppBuf->m_pbuf_start = (unsigned int)pVbufInfo->m_pbuf_start;
	pVppBuf->m_srcfmt = pVbufInfo->m_srcfmt;
	pVppBuf->m_order = pVbufInfo->m_order;
	pVppBuf->m_bytes_per_pixel = pVbufInfo->m_bytes_per_pixel;
	pVppBuf->m_content_offset = pVbufInfo->m_content_offset;
	pVppBuf->m_content_width = pVbufInfo->m_content_width;
	pVppBuf->m_content_height = pVbufInfo->m_content_height;
	pVppBuf->m_buf_stride = pVbufInfo->m_buf_stride;

	pVppBuf->m_active_left = pVbufInfo->m_active_left;
	pVppBuf->m_active_top = pVbufInfo->m_active_top;
	pVppBuf->m_active_width = pVbufInfo->m_active_width;
	pVppBuf->m_active_height = pVbufInfo->m_active_height;
	pVppBuf->m_disp_offset = pVbufInfo->m_disp_offset;
	pVppBuf->m_is_frame_seq = 1;

	pVppBuf->m_is_top_field_first = 0;
	pVppBuf->m_is_repeat_first_field = 0;
	pVppBuf->m_is_progressive_pic = 1;
	pVppBuf->m_pixel_aspect_ratio = 0;
	pVppBuf->m_frame_rate_num = 0;
	pVppBuf->m_frame_rate_den = 0;

	pVppBuf->m_is_compressed = 0;
	pVppBuf->m_luma_left_ofst = 0;
	pVppBuf->m_luma_top_ofst = 0;
	pVppBuf->m_chroma_left_ofst = 0;
	pVppBuf->m_chroma_top_ofst = 0;

	//Indicate the bitdepth of the frame, if 8bit, is 8, if 10bit, is 10
	pVppBuf->m_bits_per_pixel = 8;
	pVppBuf->m_buf_pbuf_start_UV = 0;
	pVppBuf->m_buf_stride_UV = 0;

	pVppBuf->m_flags = pVbufInfo->m_flags;

	pVppBuf->builtinFrame = 0;

	pVppBuf->m_hBD = (unsigned int)0;
	pVppBuf->m_colorprimaries = 0;
}

void MV_VPP_DisplayFrame(mrvl_vpp_dev *vdev, int x,
			int y, int w, int h, int stride)
{
	VBUF_INFO *pVbufInfo = NULL;

	pVbufInfo = MV_VPP_GetFrame(vdev, x, y, w, h, stride);
	if (pVbufInfo) {
		int uiPlaneId = PLANE_GFX1;

		VPP_VBUF *pVppBuf_virt = pVbufInfo->pVppVbufInfo_virt;
		VPP_VBUF *pVppBuf_phy = pVbufInfo->pVppVbufInfo_phy;

		pr_debug("%s:%d: pVppBuf_virt:%p, pVppBuf_phy:%p, x=%x, y=%x\n",
				__func__, __LINE__, pVppBuf_virt, pVppBuf_phy, x, y);
		convert_frame_info(pVbufInfo, pVppBuf_virt);
		pr_debug("%s:%d: pVppBuf_virt:%p, pVppBuf_phy:%p, x=%x, y=%x, "
				"src_fmt:%d, src_order:%d\n", __func__, __LINE__,
				pVppBuf_virt, pVppBuf_phy, x, y,
				pVppBuf_virt->m_srcfmt, pVppBuf_virt->m_order);

		VPP_SHM_CleanCache((uintptr_t) pVppBuf_phy, sizeof(VPP_VBUF));
		if (IS_VPP_IN_NORMAL_MODE()) {
			wrap_MV_VPPOBJ_SetStillPicture(uiPlaneId, pVppBuf_phy);
		} else {
		#ifndef VPP_ENABLE_USE_SET_STILL_PICTURE
			wrap_MV_VPPOBJ_DisplayFrame(uiPlaneId, pVppBuf_phy);
		#else
			wrap_MV_VPPOBJ_SetStillPicture(uiPlaneId, pVppBuf_phy);
		#endif
		}
	}
}

int MV_VPP_SetHdmiTxControl(int enable)
{
	return wrap_MV_VPPOBJ_SetHdmiTxControl(enable);
}

static int VPP_Clock_Init(int vpp_clk_rate)
{
	int res = -1;

	res = VPP_Clock_Set_Rate(vpp_clk_rate);
	if (res < 0) {
		pr_err("Failed to set VPP clock\n");
		return res;
	}

	res = VPP_Clock_Set_Rate_Ext(vpp_clk_rate);
	if (res < 0) {
		pr_err("Failed to set VPP dpi clock\n");
		return res;
	}

	return res;
}

static int VPP_Init_Recovery_fastlogo_ta(void)
{
	VPP_INIT_PARM vpp_init_parm;
	VPP_WIN_ATTR fb_attr;
	VPP_WIN fb_win;
	VPP_WIN disp_win;
	int width;
	int height;
	int disp_width;
	int disp_height;
	int res = 0;
	int planeID=2;
	/* 0 - Magic number, 1- heap handle which is not used FL ta as of now */
	unsigned int fl_init_param[2] = {VPP_CA_INIT_MAGIC_NUM, 0};
	VPP_DISP_OUT_PARAMS pdispParams[MAX_NUM_CPCBS];
	int pixel_clock;
	VPP_DISP_OUT_PARAMS dispParams;
	VPP_RESOLUTION_DESCRIPTION  ResDesc;

	//Allocate memory for TA heap memory manager
	res = VPP_Memory_for_ta(1, &vpp_init_parm.uiShmPA);
	if (res != 0) {
		pr_info("VPP internal memory allocation: Not enough memory!!!!!!!!\n");
		return -ENOMEM;
	}

	//Initialize fastlogo TA
	vpp_init_parm.iHDMIEnable = 1;  //Set zero to disable flushcache in VPP TA
	vpp_init_parm.iVdacEnable = 0;
	vpp_init_parm.uiShmSize=SHM_SHARE_SZ;

	res = wrap_MV_VPP_InitVPPS(TA_UUID_FASTLOGO, fl_init_param);
	if (res != MV_VPP_OK) {
		pr_err("wrap_MV_VPP_Init FAILED E[%d]\n", res);
		goto EXIT;
	}
	/* Retrieve following details from bootloader
	 * 1. dispout param - resid, display id/mode, TGID
	 * 2. Clock - Get pixel clock for resid got from BL.
	 * 3. Res Info - Get resInfo for resid got from BL.
	 */
	wrap_MV_VPPOBJ_GetDispOutParams(pdispParams, MAX_NUM_CPCBS * sizeof(VPP_DISP_OUT_PARAMS));

	/* Override the params from DTS if available */
	if (vpp_config_param.disp_res_id  == -1) {
		dispParams.uiDispId = pdispParams[CPCB_1].uiDispId;
		dispParams.uiDisplayMode = pdispParams[CPCB_1].uiDisplayMode;
		dispParams.uiResId  = pdispParams[CPCB_1].uiResId;
	} else {
		dispParams.uiDispId = vpp_config_param.disp_out_type;
		dispParams.uiDisplayMode = vpp_config_param.disp_out_type == 0 ? VOUT_HDMI :\
                                                                         VOUT_DSI;
		dispParams.uiResId  = vpp_config_param.disp_res_id;
	}

	if (dispParams.uiDispId == VOUT_DSI)
		wrap_MV_VPP_MIPI_Reset(0);

	vpp_config_param.disp_res_id  = dispParams.uiResId;
	dispParams.uiBitDepth = OUTPUT_BIT_DEPTH_8BIT;
	dispParams.uiColorFmt = OUTPUT_COLOR_FMT_RGB888;
	dispParams.iPixelRepeat = 1;

	/* Get and Configure clock here */
	res = wrap_MV_VPPOBJ_GetCPCBOutputPixelClock(vpp_config_param.disp_res_id, &pixel_clock);
	pr_debug("\nresID[%d]clock[%d]\n", vpp_config_param.disp_res_id, pixel_clock);

	/* Convert to Hz */
	pixel_clock *= 1000;

	res = wrap_MV_VPPOBJ_GetResolutionDescription(vpp_config_param.disp_res_id, &ResDesc);
	if (res != MV_VPP_OK) {
		pr_err("wrap_MV_VPPOBJ_GetResolutionDescription FAILED, error: 0x%x\n", res);
		res = MV_DISP_E_CFG;
		goto EXIT_DESTROY;
	}

	vpp_config_param.frame_rate = pixel_clock / (ResDesc.uiHeight * ResDesc.uiWidth);

	/* Disable AutoPush enabled in bootlogo
	 * recovery mode: should be done here
	 * normal mode: Done by ampcore, alllowing bootlogo shown for longer time */
	wrap_DhubEnableAutoPush(false, true, vpp_config_param.frame_rate);

	res = VPP_Clock_Init(pixel_clock);
	if (res) {
		pr_err("VPP %s:%d Failed to set clock\n", __func__, __LINE__);
		return res;
	}

	res = wrap_MV_VPPOBJ_Reset();
	if (res != MV_VPP_OK) {
		pr_err("wrap_MV_VPPOBJ_Reset FAILED, error: 0x%x\n", res);
		res = MV_DISP_E_RST;
		goto EXIT_DESTROY;
	}

	res = wrap_MV_VPPOBJ_Config(NULL, NULL, NULL, NULL);
	if (res != MV_VPP_OK) {
		pr_err("wrap_MV_VPPOBJ_Config FAILED, error: 0x%x\n", res);
		res = MV_DISP_E_CFG;
		goto EXIT_DESTROY;
	}

	res = wrap_MV_VPPOBJ_SetFormat(CPCB_1, &dispParams);
	if (res != MV_VPP_OK) {
		pr_err("%s:%d: wrap_MV_VPPOBJ_SetFormat FAILED, error: 0x%x\n", __func__, __LINE__, res);
		goto EXIT_DESTROY;
	}

	//Get the width and height of FB
	MV_VPP_GetInputFrameSize(&width, &height);

	MV_VPP_GetOutResolutionSize(&disp_width, &disp_height);
	fb_win.x = 0;
	fb_win.y = 0;
	fb_win.width  = width;
	fb_win.height = height;

	fb_attr.bgcolor = 0x801080; // black
	fb_attr.alpha  = 0xFFF;

	disp_win.x = 0;
	disp_win.y = 0;
	disp_win.width  = disp_width;
	disp_win.height = disp_height;

	res = wrap_MV_VPPOBJ_OpenDispWindow(planeID, &disp_win, &fb_attr);
	if (res != MV_VPP_OK) {
		pr_err("%s:%d OpenDispWindow FAILED, error: 0x%x\n", __func__, __LINE__, res);
		goto EXIT_DESTROY;
	}

#ifndef VPP_ENABLE_USE_SET_STILL_PICTURE
	res = wrap_MV_VPPOBJ_SetDisplayMode(planeID, DISP_FRAME);
	if (res != MV_VPP_OK) {
		pr_err("%s:%d SetDisplayMode FAILED, error: 0x%x\n", __func__, __LINE__, res);
		goto EXIT_DESTROY;
	}
#endif //VPP_ENABLE_USE_SET_STILL_PICTURE

	res = wrap_MV_VPPOBJ_SetRefWindow(planeID, &fb_win);
	if (res != MV_VPP_OK) {
		pr_err("%s:%d SetRefWindow FAILED, error: 0x%x\n", __func__, __LINE__, res);
		goto EXIT_DESTROY;
	}

	res = create_global_desc_array();
	if (res != MV_VPP_OK) {
		pr_info("MV_VPP_Init:create_global_desc_array - %d FAILED\n",res);
		goto EXIT_DESTROY;
	}

	//Start ISR kernel thread
	VPP_CreateISRTask();
	pr_info("MV_VPP_Init:Sucess: (libfastlogo.ta)\n");

	return 0;

EXIT_DESTROY:
	wrap_MV_VPPOBJ_Destroy();
EXIT:
	wrap_MV_VPP_DeInit();
	VPP_Memory_for_ta(0, NULL);

	pr_err("MV_VPP_Init:Error: (libfastlogo.ta) - %d\n", res);

	return res;
}

int MV_VPP_Init(void)
{
	int res = 0;

	if (!IS_VPP_IN_NORMAL_MODE()) {
		pr_info("MV_VPP_Init - Normal/Recovery - libfastlogo.ta \n");
		res = avio_sub_module_dhub_init();
		if (res) {
			pr_info("%s: dhub open failed: %x\n", __func__, res);
			return res;
		}
		VPP_EnableDhubInterrupt(true);
		res = VPP_Init_Recovery_fastlogo_ta();
	} else {
		pr_info("MV_VPP_Init - Normal - libvpp.ta\n");
		res = create_global_desc_array();
	}
	return res;
}

void MV_VPP_Deinit(void)
{
	if (!IS_VPP_IN_NORMAL_MODE())
		VPP_EnableDhubInterrupt(false);
	wrap_MV_VPPOBJ_Destroy();
	if (!IS_VPP_IN_NORMAL_MODE())
		VPP_StopISRTask();
	wrap_MV_VPP_DeInit();
	destroy_global_desc_array();
}

int MV_VPP_Config(void) {
	int ret;
	unsigned int vppInitParam[2];
	VPP_WIN fb_win;
	VPP_WIN disp_win;
	VPP_WIN_ATTR attr;

	if (IS_VPP_IN_NORMAL_MODE()) {
		vppInitParam[0] = VPP_CA_INIT_MAGIC_NUM;
		vppInitParam[1] = 0;

		ret = wrap_MV_VPP_InitVPPS(TA_UUID_VPP, vppInitParam);

		if (ret) {
			pr_err("%s:%d InitVPPS FAILED, error: 0x%x\n", __func__, __LINE__, ret);
			return -ENODEV;
		}
	}

	fb_win.x = 0;
	fb_win.y = 0;

	attr.alpha = 0x800;
	attr.bgcolor = 0xe00080;
	attr.globalAlphaFlag = 0x0;

	MV_VPP_GetInputFrameSize(&fb_win.width, &fb_win.height);

	disp_win.x = 0;
	disp_win.y = 0;
	MV_VPP_GetOutResolutionSize(&disp_win.width, &disp_win.height);

	ret = wrap_MV_VPPOBJ_SetRefWindow(PLANE_GFX1, &fb_win);

	if (ret) {
		pr_err("%s:%d: SetRefWindow FAILED, error: 0x%x\n", __func__, __LINE__, ret);
		return -ECOMM;
	}

	ret = wrap_MV_VPPOBJ_ChangeDispWindow(PLANE_GFX1, &disp_win, &attr);

	if (ret) {
		pr_err("%s:%d ChangeDispWindow FAILED, error: 0x%x\n", __func__, __LINE__, ret);
		return -ECOMM;
	}
	pr_debug("%s:%d (exit)\n", __func__, __LINE__);

	return 0;
}
