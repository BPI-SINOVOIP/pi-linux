// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#include <drm/drmP.h>
#include <linux/delay.h>
#include <linux/ion.h>
#include "vpp_defines.h"
#include "avio_type.h"
#include "vpp_vbuf.h"
#include "hal_vpp_wrap.h"
#include "syna_vpp.h"
#include "drm_syna_gem.h"
#include "drm_syna_drv.h"


#define DEFAULT_DEVICE_ROTATION 0
#define MAX_VBUF_INFO 10
#define MAX_ROTATE_BUFFER 3

#define MAX_PLANE_NUM 2

long device_rotate = DEFAULT_DEVICE_ROTATION;
static int in_use_device_rotate = -1;

static VPP_VBUF *vpp_disp_info[MAX_VBUF_INFO];
static ion_phys_addr_t vpp_disp_info_phys_addr[MAX_VBUF_INFO];
static struct dma_buf *vpp_disp_info_dma_buf[MAX_VBUF_INFO];
static int vbuf_info_num;
static int init_vbuf_info;

static struct dma_buf *rotate_buffer[MAX_PLANE_NUM][MAX_ROTATE_BUFFER];
static ion_phys_addr_t rotate_buffer_kernel_addr[MAX_PLANE_NUM][MAX_ROTATE_BUFFER];
static ion_phys_addr_t rotate_buffer_phy_addr[MAX_PLANE_NUM][MAX_ROTATE_BUFFER];

static int primery_frame;
static int overlay_frame;

static ion_phys_addr_t last_primery_addr;
static ion_phys_addr_t last_overlay_addr;

static unsigned long ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (unsigned long)PFN_PHYS(page_to_pfn(page));
}

void syna_vpp_wait_vsync(void)
{
	int ret = 0;
	long use_hw_vsync = syna_debugfs_get_hw_vsync_val();

	struct timespec64 start;
	struct timespec64 end;
	struct timespec64 dts;
	long long delta = 0;
	int frame_num = 0;

	if (use_hw_vsync) {
		if (use_hw_vsync == 2)
			ktime_get_real_ts64(&start);

		ret = wrap_MV_VPP_WaitVsync();

		if (use_hw_vsync == 2) {
			ktime_get_real_ts64(&end);
			dts = timespec64_sub(end, start);
			delta = timespec64_to_ns(&dts) / 1000;
			frame_num++;
			if (frame_num % 100 == 0)
				DRM_ERROR("%s use time %lld us",
						__func__, delta);
		}

		if (ret < 0) {
			DRM_ERROR("%s %d wait_vpp_vsyn() fail as: %d\n",
			       __func__, __LINE__, ret);
		}
	} else {
		usleep_range(16000, 17000);
	}
}

static VOID convert_frame_info(VPP_VBUF *pVppBuf, UINT32 srcfmt, INT32 x,
			       INT32 y, INT32 width, INT32 height,
			       UINT32 m_pbuf_start)
{
	if (srcfmt == SRCFMT_YUV422) {
		pVppBuf->m_bytes_per_pixel = 2;
		pVppBuf->m_srcfmt = SRCFMT_YUV422;
		pVppBuf->m_order = ORDER_UYVY;
	} else {
		pVppBuf->m_bytes_per_pixel = 4;
		pVppBuf->m_srcfmt = srcfmt;
		pVppBuf->m_order = ORDER_BGRA;
	}

	pVppBuf->m_pbuf_start = (UINT32) m_pbuf_start;
	pVppBuf->m_content_offset = 0;
	pVppBuf->m_content_width = width;
	pVppBuf->m_content_height = height;
	pVppBuf->m_buf_stride = width * pVppBuf->m_bytes_per_pixel;
	pVppBuf->m_buf_size = height * pVppBuf->m_buf_stride;

	pVppBuf->m_active_left = x;
	pVppBuf->m_active_top = y;
	pVppBuf->m_active_width = width;
	pVppBuf->m_active_height = height;
	pVppBuf->m_disp_offset = 0;
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

	pVppBuf->m_flags = 1;

	pVppBuf->builtinFrame = 0;

	pVppBuf->m_hBD = (UINT32) 0;
	pVppBuf->m_colorprimaries = 0;
}

static VOID rotate_display_buffer(void *kernel_vir_dst_addr, void *kernel_vir_src_addr, int width, int height) {
	switch (device_rotate) {
	default:
		break;
	case 180:
		{
			int y = 0;

			for (y = 0; y < height; y++) {
				char *dst =
					(char *)kernel_vir_dst_addr + 4 * width * y;
				char *src =
					(char *)kernel_vir_src_addr +
					4 * width * (height - y - 1);
				memcpy((void *)dst, (void *)src, 4 * width);
			}
			break;
		}
	case 90:
		{
			int tmp = width;
			width = height;
			height = tmp;

			break;
		}
	case 270:
		{
			int x = 0, y = 0;
			int tmp = width;

			unsigned long long *src_long =
				(unsigned long long *)kernel_vir_src_addr;
			unsigned long long *dst_long =
				(unsigned long long *)kernel_vir_dst_addr;
			unsigned long long a, b, c, d;

			for (y = 0; y < height; y += 2) {
				unsigned long long *start_y0 =
					src_long + y * width / 2;
				unsigned long long *start_y1 =
					src_long + (y + 1) * width / 2;
				for (x = 0; x < width; x += 2) {
					a = *(start_y0 + x / 2);
					b = *(start_y1 + x / 2);

					c = a >> 32LL;
					a = a & 0x00000000FFFFFFFFLL;

					d = b >> 32LL;
					b = b & 0x00000000FFFFFFFFLL;
					*(dst_long +
					  ((width - 1 - (x)) * height +
					   y) / 2) = (b << 32) | (a);
					*(dst_long +
					  ((width - 1 - (x + 1)) * height +
					   y) / 2) = (d << 32) | (c);
				}
			}

			width = height;
			height = tmp;

			break;
		}
	}
}

static void syna_vpp_init(struct device *dev)
{
	int i;
	int plane;

	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);
	for (i = 0; i < MAX_VBUF_INFO; i++) {
		DRM_DEBUG_DRIVER("Init %d\n", i);
		vpp_disp_info_dma_buf[i] =
		    ion_alloc(sizeof(VPP_VBUF), nonSecurePoolID, ION_CACHED);
		if (IS_ERR(vpp_disp_info_dma_buf[i])) {
			DRM_ERROR("ion alloc VPP_VBUF failed\n");
			return;
		}

		if (dma_buf_begin_cpu_access
		    (vpp_disp_info_dma_buf[i], DMA_BIDIRECTIONAL) != 0) {
			dma_buf_put(vpp_disp_info_dma_buf[i]);
			DRM_ERROR("%s %d ion dma_buf_begin_cpu_access fail\n",
			       __func__, __LINE__);
			return;
		}
		vpp_disp_info[i] =
		    (VPP_VBUF *) dma_buf_kmap(vpp_disp_info_dma_buf[i], 0);
		if (vpp_disp_info[i] == NULL) {
			DRM_ERROR("ion buffer kmap failed.");
			dma_buf_end_cpu_access(vpp_disp_info_dma_buf[i],
					       DMA_BIDIRECTIONAL);
			dma_buf_put(vpp_disp_info_dma_buf[i]);
			return;
		}

		vpp_disp_info_phys_addr[i] =
		    ion_dmabuf_get_phy(vpp_disp_info_dma_buf[i]);

		DRM_DEBUG_DRIVER("Init vpp_disp_info_phys_addr[%d]=%lx\n",
				 i, vpp_disp_info_phys_addr[i]);
	}

	for (plane = 0; plane < MAX_PLANE_NUM; plane++) {
		for (i = 0; i < MAX_ROTATE_BUFFER; i++) {
			DRM_DEBUG_DRIVER("Init %d\n", i);
			rotate_buffer[plane][i] =
			    ion_alloc(SYNA_WIDTH_MAX * SYNA_HEIGHT_MAX * 4, nonSecurePoolID, ION_CACHED);
			if (IS_ERR(rotate_buffer)) {
				DRM_ERROR("ion alloc 8M rotate buffer failed\n");
				return;
			}

			if (dma_buf_begin_cpu_access
			    (rotate_buffer[plane][i], DMA_BIDIRECTIONAL) != 0) {
				dma_buf_put(vpp_disp_info_dma_buf[i]);
				DRM_ERROR("%s:dma_buf_begin_cpu_access fail\n",
				       __func__);
				return;
			}
			rotate_buffer_kernel_addr[plane][i] =
			    (unsigned long)dma_buf_kmap(rotate_buffer[plane][i],
							0);
			if (rotate_buffer_kernel_addr[plane][i] == NULL) {
				DRM_ERROR("ion buffer kmap failed.\n");
				dma_buf_end_cpu_access(rotate_buffer[plane][i],
						       DMA_BIDIRECTIONAL);
				dma_buf_put(rotate_buffer[plane][i]);
				return;
			}

			rotate_buffer_phy_addr[plane][i] =
			    ion_dmabuf_get_phy(rotate_buffer[plane][i]);

			DRM_DEBUG_DRIVER
			    ("Init vpp_disp_info_phys_addr[%d][%d]=%lx\n",
			     plane, i, rotate_buffer_phy_addr[plane][i]);
		}
	}
}

void syna_vpp_exit(void)
{
	int i;
	int plane;

	if (!init_vbuf_info)
		return;

	init_vbuf_info = 0;
	in_use_device_rotate = -1;
	device_rotate = DEFAULT_DEVICE_ROTATION;

	for (i = 0; i < MAX_VBUF_INFO; i++) {
		vpp_disp_info_phys_addr[i] = NULL;

		dma_buf_kunmap(vpp_disp_info_dma_buf[i], 0, vpp_disp_info[i]);
		vpp_disp_info[i] = NULL;

		dma_buf_end_cpu_access(vpp_disp_info_dma_buf[i],
				       DMA_BIDIRECTIONAL);

		dma_buf_put(vpp_disp_info_dma_buf[i]);
		vpp_disp_info_dma_buf[i] = NULL;
	}

	for (plane = 0; plane < MAX_PLANE_NUM; plane++) {
		for (i = 0; i < MAX_ROTATE_BUFFER; i++) {
			dma_buf_kunmap(rotate_buffer[plane][i], 0, (void *)
				       rotate_buffer_kernel_addr[plane][i]);
			dma_buf_end_cpu_access(rotate_buffer[plane][i],
					       DMA_BIDIRECTIONAL);
			dma_buf_put(rotate_buffer[plane][i]);

			rotate_buffer_kernel_addr[plane][i] = NULL;
			rotate_buffer_phy_addr[plane][i] = NULL;
		}
	}
}

bool syna_vpp_clocks_set(struct device *dev,
			 void __iomem *syna_reg, u32 clock_in_mhz,
			 u32 hdisplay, u32 vdisplay)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);

	return true;
}

void syna_vpp_set_updates_enabled(struct device *dev, void __iomem *syna_reg,
				  bool enable)
{
	DRM_DEBUG_DRIVER("Set updates: %s\n", enable ? "enable" : "disable");
	/* nothing to do here */
}

void syna_vpp_set_syncgen_enabled(struct device *dev, void __iomem *syna_reg,
				  bool enable)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);
	dev_info(dev, "Set syncgen: %s\n", enable ? "enable" : "disable");
}

void syna_vpp_set_powerdwn_enabled(struct device *dev, void __iomem *syna_reg,
				   bool enable)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);

	dev_info(dev, "Set powerdwn: %s\n", enable ? "enable" : "disable");
}

void syna_vpp_set_vblank_enabled(struct device *dev, void __iomem *syna_reg,
				 bool enable)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);

	dev_info(dev, "Set vblank: %s\n", enable ? "enable" : "disable");
}

bool syna_vpp_check_and_clear_vblank(struct device *dev,
				     void __iomem *syna_reg)
{
	return true;
}

void syna_vpp_set_plane_enabled(struct device *dev, void __iomem *syna_reg,
				u32 plane, bool enable)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);

	dev_info(dev, "Set plane %u: %s\n",
		 plane, enable ? "enable" : "disable");
}

void syna_vpp_reset_planes(struct device *dev, void __iomem *syna_reg)
{
	dev_info(dev, "Reset planes\n");

	syna_vpp_set_plane_enabled(dev, syna_reg, 0, false);
}

void syna_vpp_set_surface(struct device *dev, void __iomem *syna_reg,
			  u32 plane, u64 address,
			  u32 posx, u32 posy,
			  u32 width, u32 height, u32 stride,
			  u32 format, u32 alpha, bool blend)
{
	struct syna_gem_object *syna_obj = (struct syna_gem_object *)(address);
	ion_phys_addr_t disp_phyaddr;

	int VPP_Format = 0;
	VPP_WIN fb_win;
	int order = ORDER_BGRA;
	void *kernel_vir_src_addr = NULL;
	void *kernel_vir_dst_addr = NULL;

	struct dma_buf *sync_buf = NULL;

	DRM_DEBUG_DRIVER
	    ("Set surface: size=%dx%d stride=%d format=%d address=0x%llx\n",
	     width, height, stride, format, address);

	if (!init_vbuf_info) {
		init_vbuf_info = 1;
		syna_vpp_init(dev);
	}

	switch (format) {
	case DRM_FORMAT_ARGB8888:
		VPP_Format = SRCFMT_ARGB32;
		order = ORDER_BGRA;
		DRM_DEBUG_DRIVER("%s:%d Disp as DRM_FORMAT_ARGB8888\n",
				 __func__, __LINE__);
		break;
	case DRM_FORMAT_ABGR8888:
		VPP_Format = SRCFMT_ARGB32;
		order = ORDER_RGBA;
		DRM_DEBUG_DRIVER("%s:%d Disp as DRM_FORMAT_ABGR8888\n",
				 __func__, __LINE__);
		break;
	case DRM_FORMAT_XRGB8888:
		VPP_Format = SRCFMT_XRGB32;
		order = ORDER_BGRA;
		DRM_DEBUG_DRIVER("%s:%d Disp as DRM_FORMAT_XRGB8888\n",
				 __func__, __LINE__);
		break;
	case DRM_FORMAT_XBGR8888:
		VPP_Format = SRCFMT_XRGB32;
		order = ORDER_RGBA;
		DRM_DEBUG_DRIVER("%s:%d Disp as DRM_FORMAT_XBGR8888\n",
				 __func__, __LINE__);
		break;
	case DRM_FORMAT_NV12:
		DRM_ERROR("Do not support display as NV12\n");
		return;
	default:
		DRM_ERROR("%s:%d Unknown request format=%d\n",
				 __func__, __LINE__, format);
		BUG_ON(1);
		break;
	}

	DRM_DEBUG_DRIVER
	    ("Display frame: planeID=%d x=%x y=%d w=%d, h=%d, phyaddr=%lx\n",
	     plane, posx, posy, width, height, syna_obj->phyaddr);

	if (plane == VPP_PLANE_GFX) {
		if (last_primery_addr != syna_obj->phyaddr) {
			last_primery_addr = syna_obj->phyaddr;
		} else {
			DRM_ERROR("Push the same frame for primery, return\n");
			return;
		}
	} else if (plane == DRM_OVERLAY_PLANE) {
		if (last_overlay_addr != syna_obj->phyaddr) {
			last_overlay_addr = syna_obj->phyaddr;
		} else {
			DRM_ERROR("Push the same frame for overlay, return\n");
			return;
		}
	} else {
		DRM_ERROR("Push frame nn wrong plane\n");
		return;
	}

	/*Have rotate, get a convert target buffer */
	disp_phyaddr = syna_obj->phyaddr;
	sync_buf = syna_obj->dma_buf;

	if (device_rotate != 0 && plane == DRM_OVERLAY_PLANE) {
		overlay_frame++;
		overlay_frame = overlay_frame % MAX_ROTATE_BUFFER;
		disp_phyaddr = rotate_buffer_phy_addr[0][overlay_frame];
		sync_buf = rotate_buffer[0][overlay_frame];
		kernel_vir_dst_addr =
		    (void *)rotate_buffer_kernel_addr[0][overlay_frame];
	} else if (device_rotate != 0 && plane == VPP_PLANE_GFX) {
		primery_frame++;
		primery_frame = primery_frame % MAX_ROTATE_BUFFER;
		disp_phyaddr = rotate_buffer_phy_addr[1][primery_frame];
		sync_buf = rotate_buffer[1][primery_frame];
		kernel_vir_dst_addr =
		    (void *)rotate_buffer_kernel_addr[1][primery_frame];
	}

	kernel_vir_src_addr = syna_obj->kernel_vir_addr;

	if (device_rotate != 0)
		rotate_display_buffer(kernel_vir_dst_addr, kernel_vir_src_addr, width, height);


	if (plane == DRM_OVERLAY_PLANE) {
		convert_frame_info(vpp_disp_info[vbuf_info_num], SRCFMT_ARGB32,
				   0, 0, width, height, disp_phyaddr);
		vpp_disp_info[vbuf_info_num]->m_order = ORDER_ARGB;
		DRM_DEBUG_DRIVER
		    ("Hack for PIP: Always use SRCFMT_ARGB32+ORDER_ARGB\n");
	} else {
		convert_frame_info(vpp_disp_info[vbuf_info_num], VPP_Format, 0, 0,
				   width, height, disp_phyaddr);
		vpp_disp_info[vbuf_info_num]->m_order = order;
	}


	if (in_use_device_rotate != device_rotate) {
		DRM_DEBUG_DRIVER
		    ("[DRM] device rotate is change!! pre:%d update:%ld\n",
		     in_use_device_rotate, device_rotate);
		in_use_device_rotate = device_rotate;
		fb_win.x = 0;
		fb_win.y = 0;
		fb_win.width = width;
		fb_win.height = height;
		wrap_MV_VPPOBJ_SetRefWindow(plane, &fb_win);
	}
	wrap_MV_VPPOBJ_SetStillPicture(plane, (void *)
				       vpp_disp_info_phys_addr[vbuf_info_num]);

	vbuf_info_num = (vbuf_info_num + 1) % MAX_VBUF_INFO;
}

void syna_vpp_mode_set(struct device *dev, void __iomem *syna_reg,
		       u32 h_display, u32 v_display,
		       u32 hbps, u32 ht, u32 has,
		       u32 hlbs, u32 hfps, u32 hrbs,
		       u32 vbps, u32 vt, u32 vas,
		       u32 vtbs, u32 vfps, u32 vbbs, bool nhsync, bool nvsync)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);

	dev_info(dev, "Set mode: %dx%d\n", h_display, v_display);
	dev_info(dev, " ht: %d hbps %d has %d hlbs %d hfps %d hrbs %d\n",
		 ht, hbps, has, hlbs, hfps, hrbs);
	dev_info(dev, " vt: %d vbps %d vas %d vtbs %d vfps %d vbbs %d\n",
		 vt, vbps, vas, vtbs, vfps, vbbs);
}
