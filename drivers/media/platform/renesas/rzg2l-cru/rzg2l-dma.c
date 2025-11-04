// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Renesas RZ/G2L CRU
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 *
 * Based on the rcar_vin driver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>

#include <media/videobuf2-dma-contig.h>

#include "rzg2l-cru.h"
#include <media/rzg2l-sensor-settings.h>

static int retry_count;
module_param(retry_count, int, 0644);
static int auto_recovery = 1;
module_param(auto_recovery, int, 0644);

/* CRU Control Register */
#define CRUnCTRL_VINSEL(x)		((x) << 0)
#define CRUnCTRL_ATH			BIT(1)

/* CRU Interrupt Enable Register */
#define CRUnIE_FOE			BIT(0)
#define CRUnIE_SLVEE			BIT(1)
#define CRUnIE_DECEE			BIT(2)
#define CRUnIE_FEOVWE			BIT(3)
#define CRUnIE_SFE			BIT(16)
#define CRUnIE_EFE			BIT(17)
#define CRUnIE_SIE			BIT(18)
#define CRUnIE_WIE			BIT(19)
#define CRUnIE_CEE			BIT(20)

/* CRU Interrupt Enable 2 Register */
#define CRUnIE2_FSE(x)			BIT((x) * 3)
#define CRUnIE2_FEE(x)			BIT((x) * 3 + 1)
#define CRUnIE2_SIE(x)			BIT((x) * 3 + 2)
#define CRUnIE2_VDADRxWE(x)		GENMASK(16 + (x), 17)

/* CRU Interrupt Status Register */
#define CRUnINTS_FOS			BIT(0)
#define CRUnINTS_SLVES			BIT(1)
#define CRUnINTS_DECES			BIT(2)
#define CRUnINTS_FEOVWS			BIT(3)
#define CRUnINTS_SFS			BIT(16)
#define CRUnINTS_EFS			BIT(17)
#define CRUnINTS_SIS			BIT(18)
#define CRUnINTS_WIS			BIT(19)
#define CRUnINTS_CES			BIT(20)

/* CRU Interrupt Status 2 Register */
#define CRUnINTS2_VDADRxWS_MASK		GENMASK(20, 17)
#define CRUnINTS2_FSS(x)		BIT((x) * 3)
#define CRUnINTS2_FES(x)		BIT((x) * 3 + 1)
#define CRUnINTS2_SIS(x)		BIT((x) * 3 + 2)

/* CRU Reset Register */
#define CRUnRST_VRESETN			BIT(0)

/* CRU General Read/Write Register */
#define CRUnCOM_COMMON(x)		((x) << 0)

/* Memory Bank Enable Register for CRU Image Data */
#define AMnMBVALID_MBVALID(x)		GENMASK(x, 0)

/* Memory Bank Status Register for CRU Image Data */
#define AMnMBS_MBSTS			0x7

/* AXI-VD Bus Master Transfer Setting Register */
#define AMnAXIATTR_AXILEN		GENMASK(3, 0)

/* AXI Master FIFO Pointer Register for CRU Image Data */
#define AMnFIFOPNTR_FIFOWPNTR		GENMASK(7, 0)
#define AMnFIFOPNTR_FIFORPNTR_Y		GENMASK(23, 16)
#define AMnFIFOPNTR_FIFORPNTR_UV	GENMASK(31, 24)
#define AMnFIFOPNTR_FIFOWPNTR_B0	GENMASK(7, 0)
#define AMnFIFOPNTR_FIFOWPNTR_B1	GENMASK(15, 8)
#define AMnFIFOPNTR_FIFORPNTR_B0	GENMASK(23, 16)
#define AMnFIFOPNTR_FIFORPNTR_B1	GENMASK(31, 24)

/* AXI Master Transfer Stop Register for CRU Image Data */
#define AMnAXISTP_AXI_STOP		BIT(0)

/* AXI Master Transfer Stop Status Register for CRU Image Data */
#define AMnAXISTPACK_AXI_STOP_ACK	BIT(0)

/* Image Stride Setting Register */
#define AMnIS_IS(x)			((x) << 7)

/* CRU Image Processing Enable Register */
#define ICnEN_ICEN			BIT(0)

/* CRU SVC Number Register */
#define ICnSVCNUM_SVCNUM(x)		((x) - 1)

/* CRU VC Select Register */
#define ICnSVC_SVC0(x)			(x)
#define ICnSVC_SVC1(x)			((x) << 4)
#define ICnSVC_SVC2(x)			((x) << 8)
#define ICnSVC_SVC3(x)			((x) << 12)
#define ICnSVC_SVC_MASK			0xF

/* CRU Image Processing Main Control Register */
#define ICnMC_ICTHR			BIT(0)
#define ICnMC_DECTHR			BIT(1)
#define ICnMC_CLPTHR			BIT(2)
#define ICnMC_CSCTHR			BIT(5)
#define ICnMC_LUTTHR			(0 << 6)
#define ICnMC_CLP_NOY_CLPUV		(0 << 12)
#define ICnMC_CLP_YUV			(1 << 12)
#define ICnMC_CLP_NOY_CLPUV_128		(2 << 12)
#define ICnMC_CLP_NOCLP			(3 << 12)
#define ICnMC_IBINSEL_STRAIGHT		(0 << 14)
#define ICnMC_IBINSEL_COMP		BIT(14)
#define ICnMC_INF_YUV8_422		(0x1E << 16)
#define ICnMC_INF_YUV10_422		(0x1F << 16)
#define ICnMC_INF_RGB444		(0x20 << 16)
#define ICnMC_INF_RGB565		(0x22 << 16)
#define ICnMC_INF_RGB666		(0x23 << 16)
#define ICnMC_INF_RGB888		(0x24 << 16)
#define ICnMC_INF_RAW8			(0x2A << 16)
#define ICnMC_INF_RAW10			(0x2B << 16)
#define ICnMC_INF_RAW12			(0x2C << 16)
#define ICnMC_INF_RAW14			(0x2D << 16)
#define ICnMC_INF_RAW16			(0x2E << 16)
#define ICnMC_INF_USER			(0x30 << 16)
#define ICnMC_VCSEL(x)			((x) << 22)
#define ICnMC_INF_MASK			GENMASK(21, 16)

/* CRU Parallel I/F Control Register */
#define ICnPIFC_PINF_UYVY8_BT656	0x0
#define ICnPIFC_PINF_UYVY10_BT656	0x1
#define ICnPIFC_PINF_YUYV16		0x2
#define ICnPIFC_PINF_YVYU16		0x3
#define ICnPIFC_PINF_UYVY8		0x4
#define ICnPIFC_PINF_VYUY8		0x5
#define ICnPIFC_PINF_YUYV8		0x6
#define ICnPIFC_PINF_YVYU8		0x7
#define ICnPIFC_PINF_UYVY10		0x8
#define ICnPIFC_PINF_VYUY10		0x9
#define ICnPIFC_PINF_YUYV10		0xA
#define ICnPIFC_PINF_YVYU10		0xB
#define ICnPIFC_PINF_RAW16		0xC
#define ICnPIFC_ITL_PROGRESSIVE		(0 << 8)
#define ICnPIFC_ITL_INTERLACED		(1 << 8)
#define ICnPIFC_ITL_INTERLACED_TB	(1 << 8)
#define ICnPIFC_ITL_INTERLACED_BT	(5 << 8)
#define ICnPIFC_EC			BIT(12)
#define ICnPIFC_VSPOL_HIGH		(0 << 13)
#define ICnPIFC_VSPOL_LOW		BIT(13)
#define ICnPIFC_ENPOL_HIGH		(0 << 14)
#define ICnPIFC_ENPOL_LOW		BIT(14)

/* CRU Module Status Register */
#define ICnMS_CA(x)			BIT((x) * 4)
#define ICnMS_AV(x)			BIT((x) * 4 + 1)
#define ICnMS_IA(x)			BIT((x) * 4 + 2)

/* CRU Data Output Mode Register */
#define ICnDMR_RGBMODE_RGB24		(0 << 0)
#define ICnDMR_RGBMODE_XRGB32		(1 << 0)
#define ICnDMR_RGBMODE_ABGR32		(2 << 0)
#define ICnDMR_RGBMODE_ARGB32		(3 << 0)
#define ICnDMR_YCMODE_YUYV		(0 << 4)
#define ICnDMR_YCMODE_UYVY		(1 << 4)
#define ICnDMR_YCMODE_NV16		(2 << 4)
#define ICnDMR_YCMODE_GREY		(3 << 4)

/* CRU Test Image Generation Control 1 Register */
#define ICnTICTRL1_TIEN			BIT(0)
#define ICnTICTRL1_TIMODE		BIT(1)
#define ICnTICTRL1_TIPTNY1(x)		((x) << 4)
#define ICnTICTRL1_TIPTNU1(x)		((x) << 8)
#define ICnTICTRL1_TIPTNV1(x)		((x) << 12)

/* CRU Test Image Generation Control 2 Register */
#define ICnTICTRL2_TIPTNY2(x)		((x) << 0)
#define ICnTICTRL2_TIPTNU2(x)		((x) << 8)
#define ICnTICTRL2_TIPTNV2(x)		((x) << 16)

/* CRU Test Image Size Setting 1 Register */
#define ICnTISIZE1_TIPPL(x)		((x) << 0)

/* CRU Test Image Size Setting 2 Register */
#define ICnTISIZE2_TIN(x)		((x) << 0)
#define ICnTISIZE2_TIM(x)		((x) << 16)

static bool test_pattern;
module_param(test_pattern, bool, 0644);
MODULE_PARM_DESC(test_pattern,
		 "Enable/Disable test pattern generation for Debugging ");

struct rzg2l_cru_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

/**
 * struct image_conv_settings - register settings for rzg2l image converter
 */
struct image_conv_settings {
	int SLPrC, ELPrC, SPPrC, EPPrC;
};

#ifdef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
/*
 * @brief	Calculate image converter settings from pixel area settings for sensor
 * @param	[out]cv pointer to image converter settings
 * @param	[in]cv pointer to pixel area settings
 */
static void pixel_area_to_image_conv_settings(struct image_conv_settings *cv, const struct rzg2l_sensor_settings *a)
{
	cv->SLPrC = a->pixel_area.vertical.dummy_0 + a->pixel_area.vertical.ignored_area_of_effective_pixel_0 + a->pixel_area.vertical.effective_margin_for_color_processing_0 + 1;
	cv->ELPrC = cv->SLPrC + a->pixel_area.vertical.recording_pixel_area - 1;
	cv->SPPrC = a->pixel_area.horizontal.dummy_0 + a->pixel_area.horizontal.ignored_area_of_effective_pixel_0 + a->pixel_area.horizontal.effective_margin_for_color_processing_0 + 1;
	cv->EPPrC = cv->SPPrC + a->pixel_area.horizontal.recording_pixel_area - 1;
//	printk("pixel_area_to_image_conv_settings: %d %d %d %d\n", cv->SLPrC, cv->ELPrC, cv->SPPrC, cv->EPPrC);
}
#endif

static int sensor_stop_try[CRU_CHANNEL_MAX];
static int prev_slot[CRU_CHANNEL_MAX];
static int frame_skip[CRU_CHANNEL_MAX];
static u32 amnmbxaddrl[CRU_CHANNEL_MAX][HW_BUFFER_MAX];
static u32 amnmbxaddrh[CRU_CHANNEL_MAX][HW_BUFFER_MAX];

#define to_buf_list(vb2_buffer) (&container_of(vb2_buffer, \
						struct rzg2l_cru_buffer, \
						vb)->list)

/*
 * @brief	Calculate image size multiplied by exposure
 * @param	[in] cru pointer to cru structure
 * @return  image size in bytes
 */
static u32 rzg2l_cru_get_sizeimage(struct rzg2l_cru_dev *cru)
{
	return cru->format.sizeimage * cru->exposure;
}

static void rzg2l_cru_write(struct rzg2l_cru_dev *cru, u32 offset, u32 value)
{
	iowrite32(value, cru->base + cru->info->regs[offset].offset);
}

static u32 rzg2l_cru_read(struct rzg2l_cru_dev *cru, u32 offset)
{
	return ioread32(cru->base + cru->info->regs[offset].offset);
}

static void rzg2l_cru_set_mb(struct rzg2l_cru_dev *cru,
			     u32 bank, u32 low, u32 high)
{
	iowrite32(low,
		  cru->base + cru->info->regs[AMnMB1ADDRL].offset + (bank) * 8);
	iowrite32(high,
		  cru->base + cru->info->regs[AMnMB1ADDRH].offset + (bank) * 8);
#ifndef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
	{
		dma_addr_t addr = (((dma_addr_t)high << 32) | low) + rzg2l_cru_get_sizeimage(cru);

		iowrite32(lower_32_bits(addr),
			  cru->base + cru->info->regs[AMnIVT0ADDRL].offset + (bank + 1) * 8);
		iowrite32(upper_32_bits(addr),
			  cru->base + cru->info->regs[AMnIVT0ADDRH].offset + (bank + 1) * 8);
	}
#endif
}

static void rzg2l_cru_get_mb(struct rzg2l_cru_dev *cru,
			     u32 bank, u32 *low, u32 *high)
{
	*low = ioread32(cru->base +
			cru->info->regs[AMnMB1ADDRL].offset + bank * 8);
	*high = ioread32(cru->base +
			 cru->info->regs[AMnMB1ADDRH].offset + bank * 8);
}

/* Need to hold qlock before calling */
static void return_all_buffers(struct rzg2l_cru_dev *cru,
			       enum vb2_buffer_state state)
{
	struct rzg2l_cru_buffer *buf, *node;
	int i;

	for (i = 0; i < cru->num_buf; i++) {
		if (cru->queue_buf[i]) {
			vb2_buffer_done(&cru->queue_buf[i]->vb2_buf,
					state);
			cru->queue_buf[i] = NULL;
		}
	}

	list_for_each_entry_safe(buf, node, &cru->buf_list, list) {
		vb2_buffer_done(&buf->vb.vb2_buf, state);
		list_del(&buf->list);
	}
}

static int rzg2l_cru_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			    unsigned int *nplanes, unsigned int sizes[],
			    struct device *alloc_devs[])

{
	struct rzg2l_cru_dev *cru = vb2_get_drv_priv(vq);

	/* Make sure the image size is large enough. */
	if (*nplanes)
		return sizes[0] < rzg2l_cru_get_sizeimage(cru) ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = rzg2l_cru_get_sizeimage(cru);

	return 0;
};

static int rzg2l_cru_buffer_prepare(struct vb2_buffer *vb)
{
	struct rzg2l_cru_dev *cru = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size = rzg2l_cru_get_sizeimage(cru);

	if (vb2_plane_size(vb, 0) < size) {
		cru_err(cru, "buffer too small (%lu < %lu)\n",
			vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(vb, 0, size);

	return 0;
}

static void rzg2l_cru_buffer_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct rzg2l_cru_dev *cru = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long flags;

	spin_lock_irqsave(&cru->qlock, flags);

	list_add_tail(to_buf_list(vbuf), &cru->buf_list);

	spin_unlock_irqrestore(&cru->qlock, flags);
}

static int rzg2l_cru_mc_validate_format(struct rzg2l_cru_dev *cru,
					struct v4l2_subdev *sd,
					struct media_pad *pad)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};

	fmt.pad = pad->index;
	if (v4l2_subdev_call(sd, pad, get_fmt, NULL, &fmt))
		return -EPIPE;

	if (!cru->is_csi) {
		switch (fmt.format.code) {
		case MEDIA_BUS_FMT_UYVY8_2X8:
		case MEDIA_BUS_FMT_VYUY8_2X8:
		case MEDIA_BUS_FMT_YUYV8_2X8:
		case MEDIA_BUS_FMT_YVYU8_2X8:
		case MEDIA_BUS_FMT_UYVY10_2X10:
		case MEDIA_BUS_FMT_VYUY10_2X10:
		case MEDIA_BUS_FMT_YUYV10_2X10:
		case MEDIA_BUS_FMT_YVYU10_2X10:
		case MEDIA_BUS_FMT_YUYV8_1X16:
		case MEDIA_BUS_FMT_YVYU8_1X16:
		case MEDIA_BUS_FMT_SBGGR16_1X16:
		case MEDIA_BUS_FMT_SGBRG16_1X16:
		case MEDIA_BUS_FMT_SGRBG16_1X16:
		case MEDIA_BUS_FMT_SRGGB16_1X16:
			cru->mbus_code = fmt.format.code;
			break;
		default:
			return -EPIPE;
		}

		switch (fmt.format.field) {
		case V4L2_FIELD_NONE:
		case V4L2_FIELD_INTERLACED_TB:
		case V4L2_FIELD_INTERLACED_BT:
		case V4L2_FIELD_INTERLACED:
			/* Supported natively */
			break;
		case V4L2_FIELD_ALTERNATE:
			fmt.format.height *= 2;
			break;
		default:
			return -EPIPE;
		}
	} else {
		switch (fmt.format.code) {
		case MEDIA_BUS_FMT_UYVY8_2X8:
		case MEDIA_BUS_FMT_UYVY10_2X10:
		case MEDIA_BUS_FMT_RGB444_1X12:
		case MEDIA_BUS_FMT_RGB565_2X8_LE:
		case MEDIA_BUS_FMT_RGB666_1X18:
		case MEDIA_BUS_FMT_RGB888_1X24:
		case MEDIA_BUS_FMT_SRGGB8_1X8:
		case MEDIA_BUS_FMT_SGRBG8_1X8:
		case MEDIA_BUS_FMT_SGBRG8_1X8:
		case MEDIA_BUS_FMT_SBGGR8_1X8:
		case MEDIA_BUS_FMT_SRGGB10_1X10:
		case MEDIA_BUS_FMT_SGRBG10_1X10:
		case MEDIA_BUS_FMT_SGBRG10_1X10:
		case MEDIA_BUS_FMT_SBGGR10_1X10:
		case MEDIA_BUS_FMT_SRGGB12_1X12:
		case MEDIA_BUS_FMT_SGRBG12_1X12:
		case MEDIA_BUS_FMT_SGBRG12_1X12:
		case MEDIA_BUS_FMT_SBGGR12_1X12:
		case MEDIA_BUS_FMT_SRGGB14_1X14:
		case MEDIA_BUS_FMT_SGRBG14_1X14:
		case MEDIA_BUS_FMT_SGBRG14_1X14:
		case MEDIA_BUS_FMT_SBGGR14_1X14:
		case MEDIA_BUS_FMT_SRGGB16_1X16:
		case MEDIA_BUS_FMT_SGRBG16_1X16:
		case MEDIA_BUS_FMT_SGBRG16_1X16:
		case MEDIA_BUS_FMT_SBGGR16_1X16:
			cru->mbus_code = fmt.format.code;
			break;
		default:
			return -EPIPE;
		}
	}

	if (fmt.format.width != cru->format.width ||
	    fmt.format.height != cru->format.height ||
	    fmt.format.code != cru->mbus_code)
		return -EPIPE;

	return 0;
}

static void rzg2l_cru_set_slot_addr(struct rzg2l_cru_dev *cru,
				    int slot, dma_addr_t addr)
{
	const struct rzg2l_cru_video_format *fmt;
	int offsetx, offsety;
	dma_addr_t offset;

	fmt = rzg2l_cru_format_from_pixel(cru->format.pixelformat);

	/*
	 * There is no HW support for composition do the beast we can
	 * by modifying the buffer offset
	 */
	offsetx = cru->compose.left * fmt->bpp / (fmt->bpp_denominator_minus1 + 1);
	offsety = cru->compose.top * cru->format.bytesperline;
	offset = addr + (offsetx + offsety) * cru->exposure;

	/*
	 * The address needs to be 512 bytes aligned. Driver should never accept
	 * settings that do not satisfy this in the first place...
	 */
	if (WARN_ON((offsetx | offsety | offset) & HW_BUFFER_MASK))
		return;

	/* Currently, we just use the buffer in 32 bits address */
	rzg2l_cru_set_mb(cru, slot, lower_32_bits(offset),
			 upper_32_bits(offset));
}

/*
 * Moves a buffer from the queue to the HW slot. If no buffer is
 * available use the scratch buffer. The scratch buffer is never
 * returned to userspace, its only function is to enable the capture
 * loop to keep running.
 */
static void rzg2l_cru_fill_hw_slot(struct rzg2l_cru_dev *cru, int slot)
{
	struct rzg2l_cru_buffer *buf;
	struct vb2_v4l2_buffer *vbuf;
	dma_addr_t phys_addr;

	/* A already populated slot shall never be overwritten. */
	if (WARN_ON(cru->queue_buf[slot] != NULL))
		return;

	// cru_dbg(cru, "Filling HW slot: %d\n", slot);

	if (list_empty(&cru->buf_list)) {
		cru_dbg(cru, "No buf in queue. Use scratch %#llx for slot[%d]", cru->scratch_phys, slot);
		cru->queue_buf[slot] = NULL;
		phys_addr = cru->scratch_phys;
	} else {
		/* Keep track of buffer we give to HW */
		buf = list_entry(cru->buf_list.next,
				 struct rzg2l_cru_buffer, list);
		vbuf = &buf->vb;
		list_del_init(to_buf_list(vbuf));
		cru->queue_buf[slot] = vbuf;
		cru_dbg(cru, "Filling HW slot[%d] = vbuf idx#%d", slot, vbuf->vb2_buf.index);

		/* Setup DMA */
		phys_addr = vb2_dma_contig_plane_dma_addr(&vbuf->vb2_buf, 0);
	}

	rzg2l_cru_set_slot_addr(cru, slot, phys_addr);

	rzg2l_cru_get_mb(cru, slot, &amnmbxaddrl[cru->id][slot],
			 &amnmbxaddrh[cru->id][slot]);
	cru->last_filled_slot = slot;
}

static int rzg2l_cru_initialize_axi(struct rzg2l_cru_dev *cru)
{
	int slot;
	u32 reg;

	/* Set image data memory banks.
	 * Currently, we will use maximum address.
	 */
	rzg2l_cru_write(cru, AMnMBVALID, AMnMBVALID_MBVALID(cru->num_buf - 1));

	if (cru->retry_thread) {
		for (slot = 0; slot < cru->num_buf; slot++)
			rzg2l_cru_set_mb(cru, slot, amnmbxaddrl[cru->id][slot],
					 amnmbxaddrh[cru->id][slot]);
	} else {
		for (slot = 0; slot < cru->num_buf; slot++)
			rzg2l_cru_fill_hw_slot(cru, slot);
	}

	if (cru->info->type == RZ_CRU_V2H) {
		u32 stride;

		stride = cru->format.bytesperline;
		if ((stride % 128) == 0) {
			stride = stride / 128;
			rzg2l_cru_write(cru, AMnIS, AMnIS_IS(stride));
		} else {
			cru_err(cru,
				"Bytesperline must be multiple of 128 bytes\n");
			return -EINVAL;
		}
	}

	/* Set AXI burst max length to recommended setting */
	reg = rzg2l_cru_read(cru, AMnAXIATTR) & ~AMnAXIATTR_AXILEN;
	rzg2l_cru_write(cru, AMnAXIATTR, reg | 0xF);

	return 0;
}

static void rzg2l_cru_csi2_setup(struct rzg2l_cru_dev *cru)
{
	u32 icnmc, icnmc_regs;

	if (cru->info->type == RZ_CRU_V2H) {
		rzg2l_cru_write(cru, ICnSVCNUM, ICnSVCNUM_SVCNUM(cru->exposure));
		rzg2l_cru_write(cru, AMnSVCAOFL, cru->format.sizeimage);
		rzg2l_cru_write(cru, AMnSVCAOFH, 0);
	}
	/*
	 * Input interface
	 */
	switch (cru->mbus_code) {
	case MEDIA_BUS_FMT_UYVY8_2X8:
		icnmc = ICnMC_INF_YUV8_422;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_UYVY10_2X10:
		icnmc = ICnMC_INF_YUV10_422;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_RGB444_1X12:
		icnmc = ICnMC_INF_RGB444;
		cru->input_fmt = RGB;
		break;
	case MEDIA_BUS_FMT_RGB565_2X8_LE:
		icnmc = ICnMC_INF_RGB565;
		cru->input_fmt = RGB;
		break;
	case MEDIA_BUS_FMT_RGB666_1X18:
		icnmc = ICnMC_INF_RGB666;
		cru->input_fmt = RGB;
		break;
	case MEDIA_BUS_FMT_RGB888_1X24:
		icnmc = ICnMC_INF_RGB888;
		cru->input_fmt = RGB;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		icnmc = ICnMC_INF_RAW8;
		cru->input_fmt = BAYER_RAW;
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		icnmc = ICnMC_INF_RAW10;
		cru->input_fmt = BAYER_RAW;
		break;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		icnmc = ICnMC_INF_RAW12;
		cru->input_fmt = BAYER_RAW;
		break;
	case MEDIA_BUS_FMT_SRGGB14_1X14:
	case MEDIA_BUS_FMT_SGRBG14_1X14:
	case MEDIA_BUS_FMT_SGBRG14_1X14:
	case MEDIA_BUS_FMT_SBGGR14_1X14:
		icnmc = ICnMC_INF_RAW14;
		cru->input_fmt = BAYER_RAW;
		break;
	case MEDIA_BUS_FMT_SRGGB16_1X16:
	case MEDIA_BUS_FMT_SGRBG16_1X16:
	case MEDIA_BUS_FMT_SGBRG16_1X16:
	case MEDIA_BUS_FMT_SBGGR16_1X16:
		icnmc = ICnMC_INF_RAW16;
		cru->input_fmt = BAYER_RAW;
		break;
	default:
		cru->input_fmt = USER_DEFINED;
		icnmc = ICnMC_INF_USER;
		break;
	}

	icnmc_regs = (cru->info->type == RZ_CRU_G2L) ? ICnMC : ICnIPMC_C0;

	icnmc |= (rzg2l_cru_read(cru, icnmc_regs) & ~ICnMC_INF_MASK);

	/* Set virtual channel CSI2 */
	if (cru->info->type == RZ_CRU_G2L)
		icnmc |= ICnMC_VCSEL(cru->group->csi.channel);
	else
		rzg2l_cru_write(cru, ICnSVC, cru->sensor_settings.is_hdr ?
			ICnSVC_SVC0(1) | ICnSVC_SVC1(0) | ICnSVC_SVC2(2) | ICnSVC_SVC3(3) :
			ICnSVC_SVC0(0) | ICnSVC_SVC1(1) | ICnSVC_SVC2(2) | ICnSVC_SVC3(3));

	rzg2l_cru_write(cru, icnmc_regs, icnmc);
	if (cru->info->type != RZ_CRU_G2L && cru->exposure == 2)
		rzg2l_cru_write(cru, ICnIPMC_C1, icnmc);
}

static void rzg2l_cru_parallel_setup(struct rzg2l_cru_dev *cru)
{
	u32 icnpifc;

	switch (cru->format.field) {
	case V4L2_FIELD_INTERLACED:
		/* Default to TB */
		icnpifc = ICnPIFC_ITL_INTERLACED;
		break;
	case V4L2_FIELD_INTERLACED_TB:
		icnpifc = ICnPIFC_ITL_INTERLACED_TB;
		break;
	case V4L2_FIELD_INTERLACED_BT:
		icnpifc = ICnPIFC_ITL_INTERLACED_BT;
		break;
	case V4L2_FIELD_NONE:
		icnpifc = ICnPIFC_ITL_PROGRESSIVE;
		break;
	default:
		icnpifc = ICnPIFC_ITL_INTERLACED;
		break;
	}

	/*
	 * Input interface
	 */
	switch (cru->mbus_code) {
	case MEDIA_BUS_FMT_UYVY8_2X8:
		/* BT.656 8bit YCbCr422 or BT.601 8bit YCbCr422 */
		if (cru->parallel->mbus_type == V4L2_MBUS_BT656)
			icnpifc |= ICnPIFC_PINF_UYVY8_BT656;
		else
			icnpifc |= ICnPIFC_PINF_UYVY8;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_UYVY10_2X10:
		/* BT.656 10bit YCbCr422 or BT.601 10bit YCbCr422 */
		if (cru->parallel->mbus_type == V4L2_MBUS_BT656)
			icnpifc |= ICnPIFC_PINF_UYVY10_BT656;
		else
			icnpifc |= ICnPIFC_PINF_UYVY10;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_YUYV8_1X16:
		icnpifc |= ICnPIFC_PINF_YUYV16;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_YVYU8_1X16:
		icnpifc |= ICnPIFC_PINF_YVYU16;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_VYUY8_2X8:
		icnpifc |= ICnPIFC_PINF_VYUY8;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_YUYV8_2X8:
		icnpifc |= ICnPIFC_PINF_YUYV8;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_YVYU8_2X8:
		icnpifc |= ICnPIFC_PINF_YVYU8;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_VYUY10_2X10:
		icnpifc |= ICnPIFC_PINF_VYUY10;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_YUYV10_2X10:
		icnpifc |= ICnPIFC_PINF_YUYV10;
		cru->input_fmt = YUV;
		break;
	case MEDIA_BUS_FMT_YVYU10_2X10:
		icnpifc |= ICnPIFC_PINF_YVYU10;
		cru->input_fmt = YUV;
		break;
	default:
		cru->input_fmt = USER_DEFINED;
		icnpifc |= ICnPIFC_PINF_RAW16;
		break;
	}

	/* Hsync Signal Polarity Select */
	if (cru->parallel->mbus_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW)
		icnpifc |= ICnPIFC_ENPOL_LOW;

	/* Vsync Signal Polarity Select */
	if (cru->parallel->mbus_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW)
		icnpifc |= ICnPIFC_VSPOL_LOW;

	/* Set field and input data */
	rzg2l_cru_write(cru, ICnPIFC, icnpifc);
}

static int rzg2l_cru_initialize_image_conv(struct rzg2l_cru_dev *cru)
{
	u32 icndmr, icnmc_regs, icnmc_value;

	/* Currently we do not support:
	 * - Frame subsampling
	 * - LUT conversion
	 * - Clipping
	 */

	if (test_pattern) {
		/* Pattern Generator (for Debugging) is a test image (YUV only).
		 * It is used as an input by setting following registers.
		 *
		 * TODO
		 *
		 * Currently we just support only RED monochrome pattern.
		 * Consider to support more patterns.
		 */
		rzg2l_cru_write(cru, ICnTICTRL1,
				ICnTICTRL1_TIEN | ICnTICTRL1_TIMODE);
		rzg2l_cru_write(cru, ICnTICTRL2, ICnTICTRL2_TIPTNY2(0x52) |
						 ICnTICTRL2_TIPTNU2(0x5A) |
						 ICnTICTRL2_TIPTNV2(0xF0));

		rzg2l_cru_write(cru, ICnTISIZE1,
				ICnTISIZE1_TIPPL(cru->format.width));
		rzg2l_cru_write(cru, ICnTISIZE2,
				ICnTISIZE2_TIN(cru->format.height));

		cru->input_fmt = YUV;
	} else {
		if (cru->is_csi)
			rzg2l_cru_csi2_setup(cru);
		else
			rzg2l_cru_parallel_setup(cru);
	}

	/* Output format */
	switch (cru->format.pixelformat) {
	case V4L2_PIX_FMT_YUYV:
		icndmr = ICnDMR_YCMODE_YUYV;
		cru->output_fmt = YUV;
		break;
	case V4L2_PIX_FMT_UYVY:
		icndmr = ICnDMR_YCMODE_UYVY;
		cru->output_fmt = YUV;
		break;
	case V4L2_PIX_FMT_GREY:
		icndmr = ICnDMR_YCMODE_GREY;
		cru->output_fmt = YUV;
		break;
	case V4L2_PIX_FMT_NV16:
		icndmr = ICnDMR_YCMODE_NV16;
		cru->output_fmt = YUV;
		rzg2l_cru_write(cru, AMnUVAOFL,
			ALIGN(cru->format.width * cru->format.height, 0x200));
		break;
	case V4L2_PIX_FMT_BGR24:
		icndmr = ICnDMR_RGBMODE_RGB24;
		cru->output_fmt = RGB;
		break;
	case V4L2_PIX_FMT_XBGR32:
		icndmr = ICnDMR_RGBMODE_XRGB32;
		cru->output_fmt = RGB;
		break;
	case V4L2_PIX_FMT_ABGR32:
		icndmr = ICnDMR_RGBMODE_ABGR32;
		cru->output_fmt = RGB;
		break;
	case V4L2_PIX_FMT_ARGB32:
		icndmr = ICnDMR_RGBMODE_ARGB32;
		cru->output_fmt = RGB;
		break;
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG14P:
	case V4L2_PIX_FMT_SGRBG14P:
	case V4L2_PIX_FMT_SRGGB14P:
	case V4L2_PIX_FMT_SBGGR14P:
	case V4L2_PIX_FMT_SGBRG16:
	case V4L2_PIX_FMT_SGRBG16:
	case V4L2_PIX_FMT_SRGGB16:
	case V4L2_PIX_FMT_SBGGR16:
		icndmr = 0;
		cru->output_fmt = BAYER_RAW;
		break;
	default:
		cru_err(cru, "Invalid pixelformat (0x%x)\n",
			cru->format.pixelformat);
		return -EINVAL;
	}

	icnmc_regs = (cru->info->type == RZ_CRU_G2L) ? ICnMC : ICnIPMC_C0;
	icnmc_value = rzg2l_cru_read(cru, icnmc_regs);

	icnmc_value |= ICnMC_CSCTHR;
	icnmc_value &= ~ICnMC_CLPTHR;

	if (cru->info->type == RZ_CRU_V2H) {
		rzg2l_cru_write(cru, ICnDTVP, 0x00000000); // no header, no footer
#ifdef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
		rzg2l_cru_write(cru, ICnNIPDT_C0L, 0x00000000);
		rzg2l_cru_write(cru, ICnNIPDT_C0H, 0x00000000);
		if (cru->exposure == 2) {
			rzg2l_cru_write(cru, ICnNIPDT_C1L, 0x00000000);
			rzg2l_cru_write(cru, ICnNIPDT_C1H, 0x00000000);
		}
		{
			struct image_conv_settings ic;

			pixel_area_to_image_conv_settings(&ic, &cru->sensor_settings);
			rzg2l_cru_write(cru, ICnSLPrC_C0, ic.SLPrC);
			if (cru->exposure == 2)
				rzg2l_cru_write(cru, ICnSLPrC_C1, ic.SLPrC);
			rzg2l_cru_write(cru, ICnELPrC_C0, ic.ELPrC);
			if (cru->exposure == 2)
				rzg2l_cru_write(cru, ICnELPrC_C1, ic.ELPrC);
			rzg2l_cru_write(cru, ICnSPPrC_C0, ic.SPPrC);
			if (cru->exposure == 2)
				rzg2l_cru_write(cru, ICnSPPrC_C1, ic.SPPrC);
			rzg2l_cru_write(cru, ICnEPPrC_C0, ic.EPPrC);
			if (cru->exposure == 2)
				rzg2l_cru_write(cru, ICnEPPrC_C1, ic.EPPrC);
		}
#else
		{
			int dtype = rzg2l_cru_format_from_pixel(cru->format.pixelformat)->dtype;
			uint32_t nipdt_l = dtype < 32 ? (1 << dtype) : 0;
			uint32_t nipdt_h = dtype >= 32 ? (1 << (dtype - 32)) : 0;
#ifdef CONFIG_VIDEO_RZG2L_CRU_EMBEDDED_DATA
			nipdt_l = 1 << 0x12;
#endif
			rzg2l_cru_write(cru, ICnNIPDT_C0L, nipdt_l);
			rzg2l_cru_write(cru, ICnNIPDT_C0H, nipdt_h);
			if (cru->exposure == 2) {
				rzg2l_cru_write(cru, ICnNIPDT_C1L, nipdt_l);
				rzg2l_cru_write(cru, ICnNIPDT_C1H, nipdt_h);
			}
		}
#endif
	}

	/*
	 * CRU can perform colorspace conversion: YUV <=> RGB.
	 * If other formats, do bypass mode.
	 */
	if (cru->output_fmt == cru->input_fmt)
		icnmc_value |= ICnMC_CSCTHR;
	else if (((cru->output_fmt == YUV) && (cru->input_fmt == RGB)) ||
		 ((cru->output_fmt == RGB) && (cru->input_fmt == YUV)))
		icnmc_value &= ~ICnMC_CSCTHR;
	else {
		cru_err(cru, "Not support color space conversion for (0x%x)\n",
			cru->format.pixelformat);
		return -ENOEXEC;
	}
	rzg2l_cru_write(cru, icnmc_regs, icnmc_value);
	if (cru->info->type != RZ_CRU_G2L && cru->exposure == 2)
		rzg2l_cru_write(cru, ICnIPMC_C1, icnmc_value);

	/* Set output data format */
	rzg2l_cru_write(cru, ICnDMR, icndmr);

	return 0;
}

static int rzg2l_cru_start(struct rzg2l_cru_dev *cru, struct v4l2_subdev *sd)
{
	int ret;
	u32 value;

	/* Select a video input */
	if (cru->is_csi) {
		/* Initializing DPHY */
		ret = rzg2l_cru_init_csi_dphy(sd, cru->sensor_settings.mbps);
		if (ret)
			return ret;

		value = CRUnCTRL_VINSEL(0);

	} else
		value = CRUnCTRL_VINSEL(1);

	/* If RZ/V2N, set Image Converter to Full Bypass */
#ifndef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
	if ((cru->info->type == RZ_CRU_V2H) && !test_pattern)
		value |= CRUnCTRL_ATH;
#endif
	rzg2l_cru_write(cru, CRUnCTRL, value);

	/* Cancel the software reset for image processing block */
	rzg2l_cru_write(cru, CRUnRST, CRUnRST_VRESETN);

	/* Disable and clear the interrupt before using */
	if (cru->info->type == RZ_CRU_G2L) {
		rzg2l_cru_write(cru, CRUnIE, 0);
		rzg2l_cru_write(cru, CRUnINTS, 0x1F000F);
	} else {
		rzg2l_cru_write(cru, CRUnIE1, 0);
		rzg2l_cru_write(cru, CRUnIE2, 0);
		rzg2l_cru_write(cru, CRUnINTS1, 0x80000F0F);
		rzg2l_cru_write(cru, CRUnINTS2, 0x031F0FFF);
		rzg2l_cru_write(cru, AMnAXIATTR, 0x0000015f);
		rzg2l_cru_write(cru, AMnFIFO, auto_recovery ? 0 : 1); // enable auto recovery
	}

	/* Initialize the AXI master */
	ret = rzg2l_cru_initialize_axi(cru);
	if (ret)
		return ret;

	/* Initialize image convert */
	ret = rzg2l_cru_initialize_image_conv(cru);
	if (ret)
		return ret;

	/* Enable interrupt */
	if (cru->info->type == RZ_CRU_G2L)
		rzg2l_cru_write(cru, CRUnIE, CRUnIE_EFE);
	else {
		rzg2l_cru_write(cru, CRUnIE1, CRUnIE_FOE | CRUnIE_SLVEE | CRUnIE_DECEE);
		/* Frame End IRQ + AXI-VD Bus Write Completion IRQ 1 */
		rzg2l_cru_write(cru, CRUnIE2, CRUnIE2_FEE(cru->exposure - 1)
#ifndef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
		| CRUnIE2_VDADRxWE(1) | CRUnIE2_VDADRxWE(2) | CRUnIE2_VDADRxWE(3) | CRUnIE2_VDADRxWE(4)
#endif
		);
	}

	/* Enable AXI master & image_conv reception */
	rzg2l_cru_write(cru, ICnEN, ICnEN_ICEN);

	return 0;
}

static int rzg2l_cru_set_stream(struct rzg2l_cru_dev *cru, int on)
{
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	int ret, i;
	unsigned long flags;

	pad = media_pad_remote_pad_first(&cru->pad);
	if (!pad)
		return -EPIPE;

	sd = media_entity_to_v4l2_subdev(pad->entity);

	if (!on) {
		video_device_pipeline_stop(&cru->vdev);
		return v4l2_subdev_call(sd, video, s_stream, 0);
	}

	ret = rzg2l_cru_mc_validate_format(cru, sd, pad);
	if (ret)
		return ret;

	ret = video_device_pipeline_alloc_start(&cru->vdev);
	if (ret)
		return ret;

	spin_lock_irqsave(&cru->qlock, flags);

	ret = rzg2l_cru_start(cru, sd);

	for (i = 0; i < cru->num_buf; i++) {
		rzg2l_cru_get_mb(cru, i, &amnmbxaddrl[cru->id][i],
				 &amnmbxaddrh[cru->id][i]);
	}

	spin_unlock_irqrestore(&cru->qlock, flags);

	if (ret)
		return ret;

	ret = v4l2_subdev_call(sd, video, s_stream, 1);
	if (ret == -ENOIOCTLCMD)
		ret = 0;
	if (ret)
		video_device_pipeline_stop(&cru->vdev);

	return ret;
}
static void rzg2l_cru_stop(struct rzg2l_cru_dev *cru)
{
	int retries = 0;
	unsigned long flags;
	u32 icnms;

	spin_lock_irqsave(&cru->qlock, flags);
	if (!(cru->is_csi)) {
		/* Enable IRQ to detect frame start reception */
		rzg2l_cru_write(cru, CRUnIE, CRUnIE_SFE);

		/* Wait for streaming to stop */
		while (retries++ < 5) {
			spin_unlock_irqrestore(&cru->qlock, flags);
			msleep(100);
			spin_lock_irqsave(&cru->qlock, flags);
		}
	}

	/* Disable and clear the interrupt before using */
	if (cru->info->type == RZ_CRU_G2L) {
		rzg2l_cru_write(cru, CRUnIE, 0);
		rzg2l_cru_write(cru, CRUnINTS, 0x1F000F);
	} else {
		rzg2l_cru_write(cru, CRUnIE1, 0);
		rzg2l_cru_write(cru, CRUnIE2, 0);
		rzg2l_cru_write(cru, CRUnINTS1, 0x80000F0F);
		rzg2l_cru_write(cru, CRUnINTS2, 0x031F0FFF);
	}

	/* Wait until frame processing being stopped */
	for (retries = 10; retries > 0; retries--) {
		icnms = rzg2l_cru_read(cru, ICnMS) & (ICnMS_IA(0) | ICnMS_IA(1) | ICnMS_IA(2) | ICnMS_IA(3));
		if (!icnms)
			break;

		udelay(500);
	};

	/* TODO
	 * It is bad if can not stop reception from Sensor.
	 * Should reset system here.
	 */
	if (icnms || sensor_stop_try[cru->id] > 5)
		cru_err(cru, "Failed stop HW, something is seriously broken\n");

	cru->state = STOPPED;

	/* Stop the test pattern generator if using */
	if (test_pattern)
		rzg2l_cru_write(cru, ICnTICTRL1, 0);

	/* Stop AXI bus */
	rzg2l_cru_write(cru, AMnAXISTP, AMnAXISTP_AXI_STOP);

	/* Wait until the AXI bus stop */
	for (retries = 5; retries > 0; retries--) {
		if (rzg2l_cru_read(cru, AMnAXISTPACK) &
						AMnAXISTPACK_AXI_STOP_ACK)
			break;

		udelay(10);
	};

	/* Notify that AXI bus can not stop here */
	if (!retries)
		cru_err(cru, "Failed to stop AXI bus\n");

	/* Cancel the AXI bus stop request */
	rzg2l_cru_write(cru, AMnAXISTP, 0);

	/* Set reset state */
	reset_control_assert(cru->rstc.aresetn);

	/* Resets the image processing module */
	rzg2l_cru_write(cru, CRUnRST, 0);

	reset_control_assert(cru->rstc.presetn);

	spin_unlock_irqrestore(&cru->qlock, flags);
}

static void rzg2l_cru_stop_streaming(struct vb2_queue *vq)
{
	struct rzg2l_cru_dev *cru = vb2_get_drv_priv(vq);

	cru->state = STOPPING;

	if (cru->retry_thread)
		kthread_stop(cru->retry_thread);

	/* Stop the operation of image conversion */
	rzg2l_cru_write(cru, ICnEN, 0);

	rzg2l_cru_set_stream(cru, 0);

	rzg2l_cru_stop(cru);

	/* Release all active buffers */
	return_all_buffers(cru, VB2_BUF_STATE_ERROR);

	/* Free scratch buffer */
	dma_free_coherent(cru->dev, rzg2l_cru_get_sizeimage(cru), cru->scratch,
			  cru->scratch_phys);
}

static int retry_streaming_func(void *data)
{
	struct rzg2l_cru_dev *cru = (struct rzg2l_cru_dev *) data;
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	int ret;
	int retry = 0;
	int i;

	pad = media_pad_remote_pad_first(&cru->pad);
	if (!pad)
		return -EPIPE;
	sd = media_entity_to_v4l2_subdev(pad->entity);

	while (retry < retry_count) {
		for (i = 0; i < 10; i++) {
			if (cru->state == RUNNING)
				goto retry_done;

			msleep(20);
		}

		/* Stop CRU reception */
		rzg2l_cru_write(cru, ICnEN, 0);
		v4l2_subdev_call(sd, video, s_stream, 0);
		rzg2l_cru_stop(cru);
		pm_runtime_put(cru->dev);
		cru->pm_got = false;

		msleep(20);

		cru->state = STARTING;

		pm_runtime_get_sync(cru->dev);
		cru->pm_got = true;

		/* Release reset state */
		reset_control_deassert(cru->rstc.presetn);
		reset_control_deassert(cru->rstc.aresetn);

		msleep(20);

		ret = rzg2l_cru_start(cru, sd);
		if (ret)
			goto retry_done;

		ret = v4l2_subdev_call(sd, video, s_stream, 1);
		if (ret == -ENOIOCTLCMD)
			ret = 0;
		if (ret)
			goto retry_done;

		retry++;

		cru_info(cru, "CRU retry init: %d times", retry);
	}

	if (retry_count > 0)
		cru_err(cru, "Please retry due to no input signal after %d retries",
			retry);

retry_done:
	cru->retry_thread = NULL;

	return 0;
}

static int rzg2l_cru_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct rzg2l_cru_dev *cru = vb2_get_drv_priv(vq);
	unsigned long flags;
	int ret;

	/* Release reset state */
	reset_control_deassert(cru->rstc.presetn);
	reset_control_deassert(cru->rstc.aresetn);

	sensor_stop_try[cru->id] = 0;
	frame_skip[cru->id] = 0;

	/* Allocate scratch buffer. */
	cru->scratch = dma_alloc_coherent(cru->dev, rzg2l_cru_get_sizeimage(cru),
					  &cru->scratch_phys, GFP_KERNEL);
	if (!cru->scratch) {
		spin_lock_irqsave(&cru->qlock, flags);
		return_all_buffers(cru, VB2_BUF_STATE_QUEUED);
		spin_unlock_irqrestore(&cru->qlock, flags);
		cru_err(cru, "Failed to allocate scratch buffer\n");
		return -ENOMEM;
	}

	cru->sequence = 0;

	ret = rzg2l_cru_set_stream(cru, 1);
	if (ret) {
		spin_lock_irqsave(&cru->qlock, flags);
		return_all_buffers(cru, VB2_BUF_STATE_QUEUED);
		spin_unlock_irqrestore(&cru->qlock, flags);
		goto out;
	}

	cru->state = STARTING;


	/* Initialize value of previous memory bank slot before streaming */
	prev_slot[cru->id] = -1;

	/*
	 * Workaround to start a thread to restart CRU processing flow
	 * if there is no input to CRU while using MIPI CSI2.
	 */
	if (cru->is_csi && !test_pattern) {
		cru->retry_thread = kthread_create(retry_streaming_func, cru,
						   "CRU retry thread");
		if (IS_ERR(cru->retry_thread)) {
			ret = PTR_ERR(cru->retry_thread);
			cru->retry_thread = NULL;
			goto out;
		}

		wake_up_process(cru->retry_thread);
	}

	cru_dbg(cru, "Starting to capture\n");

out:
	if (ret)
		dma_free_coherent(cru->dev, rzg2l_cru_get_sizeimage(cru), cru->scratch,
				  cru->scratch_phys);

	return ret;
}

void rzg2l_cru_resume_start_streaming(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rzg2l_cru_dev *cru = container_of(dwork,
						 struct rzg2l_cru_dev,
						 rzg2l_cru_resume);
	unsigned long flags;
	int ret;

	ret = rzg2l_cru_set_stream(cru, 1);
	if (ret) {
		dev_warn(cru->dev, "Warning at streaming when resuming.\n");
		spin_lock_irqsave(&cru->qlock, flags);
		return_all_buffers(cru, VB2_BUF_STATE_ERROR);
		spin_unlock_irqrestore(&cru->qlock, flags);
	}

	spin_lock_irqsave(&cru->qlock, flags);
	cru->sequence = 0;
	spin_unlock_irqrestore(&cru->qlock, flags);

	cru->suspend = false;
	wake_up(&cru->setup_wait);
}

void rzg2l_cru_suspend_stop_streaming(struct rzg2l_cru_dev *cru)
{
	unsigned long flags;
	int retries = 0;

	spin_lock_irqsave(&cru->qlock, flags);

	/* Disable and clear the interrupt */
	rzg2l_cru_write(cru, CRUnIE, 0);
	rzg2l_cru_write(cru, CRUnINTS, 0x1F000F);

	/* Stop the operation of image conversion */
	rzg2l_cru_write(cru, ICnEN, 0);

	/* Stop AXI bus */
	rzg2l_cru_write(cru, AMnAXISTP, AMnAXISTP_AXI_STOP);

	/* Wait until the AXI bus stop */
	for (retries = 5; retries > 0; retries--) {
		if (rzg2l_cru_read(cru, AMnAXISTPACK) &
						AMnAXISTPACK_AXI_STOP_ACK)
			break;

		udelay(10);
	};

	/* Cancel the AXI bus stop request */
	rzg2l_cru_write(cru, AMnAXISTP, 0);

	/* Release all active buffers */
	return_all_buffers(cru, VB2_BUF_STATE_ERROR);

	spin_unlock_irqrestore(&cru->qlock, flags);

	rzg2l_cru_set_stream(cru, 0);

	cru->suspend = true;
}

static const struct vb2_ops rzg2l_cru_qops = {
	.queue_setup		= rzg2l_cru_queue_setup,
	.buf_prepare		= rzg2l_cru_buffer_prepare,
	.buf_queue		= rzg2l_cru_buffer_queue,
	.start_streaming	= rzg2l_cru_start_streaming,
	.stop_streaming		= rzg2l_cru_stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

/**
 * @brief	Set callback functions which trigger during CRU operations
 * @param	struct rzg2l_cru_dev *cru
 * @param struct rzg2l_cru_callbacks *ops
 * @return 0 on success
 */
int rzg2l_cru_set_callbacks(struct rzg2l_cru_dev *cru, const struct rzg2l_cru_callbacks *ops)
{
	if (ops) {
		cru->callbacks = *ops;
	} else {
		/* If NULL, unregister all callbacks */
		memset(&cru->callbacks, 0, sizeof(struct rzg2l_cru_callbacks));
	}
	return 0;
}
EXPORT_SYMBOL_GPL(rzg2l_cru_set_callbacks);

#ifdef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
static irqreturn_t rzv2h_cru_irq(int irq, void *data)
{
	struct rzg2l_cru_dev *cru = data;
	unsigned long flags;
	u32 irq_status, i, slot;
	unsigned int handled = 0;
	dma_addr_t phys_addr;
	bool write_complete = false;

	spin_lock_irqsave(&cru->qlock, flags);

	irq_status = rzg2l_cru_read(cru, CRUnINTS2);
	if (!(irq_status))
		goto done;

	handled = 1;

	rzg2l_cru_write(cru, CRUnINTS2, irq_status);

	/* Nothing to do if capture status is 'STOPPED' */
	if (cru->state == STOPPED) {
		cru_dbg(cru, "IRQ while state stopped\n");
		goto done;
	}

	phys_addr = rzg2l_cru_read(cru, AMnMADRSL);
	phys_addr |= (((unsigned long) rzg2l_cru_read(cru, AMnMADRSH)) << 32);

	/* Check current HW slot based on current MB address */
	slot = prev_slot[cru->id];
	for (i = 0; i < cru->num_buf; i++) {
		dma_addr_t tmp;

		slot++;
		if (slot >= cru->num_buf)
			slot = 0;

		tmp = amnmbxaddrh[cru->id][slot];
		tmp = (tmp << 32) | amnmbxaddrl[cru->id][slot];

		tmp = phys_addr - tmp;
		if (((long) tmp) && (tmp <= rzg2l_cru_get_sizeimage(cru))) {
			write_complete = true;
			break;
		}
	}
	prev_slot[cru->id] = slot;

	/* Prepare for capture and update state */
	if (!write_complete) {
		// use last slot
		cru_info(cru, "Slot[%d] doesn't have proper output, but continue with it anyway", slot);
	}

	/*
	 * To hand buffers back in a known order to userspace start
	 * to capture first from slot 0.
	 */
	if (cru->state == STARTING) {
		if (cru->is_frame_skip) {
			if (frame_skip[cru->id] < CRU_FRAME_SKIP) {
				cru_dbg(cru, "Skipping %d frame\n",
					frame_skip[cru->id]);
				frame_skip[cru->id]++;
				goto done;
			}
		}

		cru_dbg(cru, "Capture start synced!\n");
		cru->state = RUNNING;
	}

	if (slot != prev_slot[cru->id]) {
		/* Update value of previous memory bank slot */
		prev_slot[cru->id] = slot;
	} else {
		/*
		* AXI-Bus congestion maybe occurred.
		* Set auto recovery mode to clear all FIFOs
		* and resume transmission.
		*/
		rzg2l_cru_write(cru, AMnFIFO, 0);

		cru_dbg(cru, "Dropping frame %u with CRU channel %d\n", cru->sequence, cru->id);
		goto done;
	}

	/* Capture frame */
	if (cru->queue_buf[slot]) {
		cru->queue_buf[slot]->field = cru->format.field;
		cru->queue_buf[slot]->sequence = cru->sequence;
		cru->queue_buf[slot]->vb2_buf.timestamp = ktime_get_ns();
		vb2_buffer_done(&cru->queue_buf[slot]->vb2_buf,
				VB2_BUF_STATE_DONE);
		/* Call IVC callback if it exists */
		if (cru->callbacks.frame_end)
			cru->callbacks.frame_end(cru->ctx_id, cru->queue_buf[slot]->vb2_buf.timestamp, 0);
		cru->queue_buf[slot] = NULL;
	} else {
		/* Scratch buffer was used, dropping frame. */
		cru_dbg(cru, "Dropping frame %u\n", cru->sequence);
	}

	cru->sequence++;

	/* Prepare for next frame */
	rzg2l_cru_fill_hw_slot(cru, slot);

done:
	spin_unlock_irqrestore(&cru->qlock, flags);

	return IRQ_RETVAL(handled);
}
#else
static irqreturn_t rzv2h_cru_vsd_addr_wend_irq(int irq, void *data)
{
	unsigned int handled = 0;
	struct rzg2l_cru_dev *cru = data;
	unsigned long flags;
	u32 irq_status, i, slot;

	spin_lock_irqsave(&cru->qlock, flags);

	irq_status = rzg2l_cru_read(cru, CRUnINTS2);
	if (!(irq_status))
		goto done;

	for (i = 0; i < cru->num_buf; ++i) {
		if (irq_status & (0x00020000 << i)) {
			rzg2l_cru_write(cru, CRUnINTS2, (0x00020000 << i));
			break;
		}
	}
	if (i >= cru->num_buf) {
		cru_info(cru, "Slot[%d] doesn't have proper output, but continue with it anyway", slot);
		goto done;
	}
	prev_slot[cru->id] = slot;
	slot = i;

	/*
	 * To hand buffers back in a known order to userspace start
	 * to capture first from slot 0.
	 */
	if (cru->state == STARTING) {
		if (cru->is_frame_skip) {
			if (frame_skip[cru->id] < CRU_FRAME_SKIP) {
				cru_dbg(cru, "Skipping %d frame\n",
					frame_skip[cru->id]);
				frame_skip[cru->id]++;
				goto done;
			}
		}

		cru_dbg(cru, "Capture start synced!\n");
		cru->state = RUNNING;
	}

	/* Capture frame */
	if (cru->queue_buf[slot]) {
		cru->queue_buf[slot]->field = cru->format.field;
		cru->queue_buf[slot]->sequence = cru->sequence;
		cru->queue_buf[slot]->vb2_buf.timestamp = ktime_get_ns();
		vb2_buffer_done(&cru->queue_buf[slot]->vb2_buf,
				VB2_BUF_STATE_DONE);
		/* Call IVC callback if it exists */
		if (cru->callbacks.frame_end)
			cru->callbacks.frame_end(cru->ctx_id, cru->queue_buf[slot]->vb2_buf.timestamp, 0);
		cru->queue_buf[slot] = NULL;
	} else {
		/* Scratch buffer was used, dropping frame. */
		cru_dbg(cru, "Dropping frame %u\n", cru->sequence);
	}

	cru->sequence++;

	/* Prepare for next frame */
	rzg2l_cru_fill_hw_slot(cru, slot);
	handled = 1;

done:
	spin_unlock_irqrestore(&cru->qlock, flags);
	return IRQ_RETVAL(handled);
}
#endif

#ifdef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
static irqreturn_t rzg2l_cru_irq(int irq, void *data)
{
	struct rzg2l_cru_dev *cru = data;
	int slot;
	unsigned long flags;
	u32 irq_status;
	u32 amnmbs;
	unsigned int handled = 0;

	spin_lock_irqsave(&cru->qlock, flags);

	irq_status = rzg2l_cru_read(cru, CRUnINTS);
	if (!irq_status)
		goto done;

	handled = 1;

	rzg2l_cru_write(cru, CRUnINTS, rzg2l_cru_read(cru, CRUnINTS));

	/* Nothing to do if capture status is 'STOPPED' */
	if (cru->state == STOPPED) {
		cru_dbg(cru, "IRQ while state stopped\n");
		goto done;
	}

	/* Increase stop retries if capture status is 'STOPPING' */
	if (cru->state == STOPPING) {
		cru_dbg(cru, "IRQ while state stopping\n");
		if (irq_status & CRUnINTS_SFS)
			sensor_stop_try[cru->id]++;

		goto done;
	}

	/* Prepare for capture and update state */
	amnmbs = rzg2l_cru_read(cru, AMnMBS);
	slot = amnmbs & AMnMBS_MBSTS;

	/*
	 * AMnMBS.MBSTS indicates the destination of Memory Bank (MB).
	 * Recalculate to get the current transfer complete MB.
	 */
	if (slot == 0)
		slot = cru->num_buf - 1;
	else
		slot--;

	/*
	 * To hand buffers back in a known order to userspace start
	 * to capture first from slot 0.
	 */
	if (cru->state == STARTING) {
		if (cru->is_frame_skip) {
			if (frame_skip[cru->id] < CRU_FRAME_SKIP) {
				cru_dbg(cru, "Skipping %d frame\n",
					frame_skip[cru->id]);
				frame_skip[cru->id]++;
				goto done;
			}
		} else {
			if (slot != 0) {
				cru_dbg(cru, "Starting sync slot: %d\n", slot);
				goto done;
			}
		}

		cru_dbg(cru, "Capture start synced!\n");
		cru->state = RUNNING;
	}

	if (slot != prev_slot[cru->id]) {
		/* Update value of previous memory bank slot */
		prev_slot[cru->id] = slot;
	} else {
		/*
		 * AXI-Bus congestion maybe occurred.
		 * Set auto recovery mode to clear all FIFOs
		 * and resume transmission.
		 */
		rzg2l_cru_write(cru, AMnFIFO, 0);

		cru_dbg(cru, "Dropping frame %u\n", cru->sequence);
		goto done;
	}

	/* Capture frame */
	if (cru->queue_buf[slot]) {
		cru->queue_buf[slot]->field = cru->format.field;
		cru->queue_buf[slot]->sequence = cru->sequence;
		cru->queue_buf[slot]->vb2_buf.timestamp = ktime_get_ns();
		vb2_buffer_done(&cru->queue_buf[slot]->vb2_buf,
				VB2_BUF_STATE_DONE);
		cru->queue_buf[slot] = NULL;
	} else {
		/* Scratch buffer was used, dropping frame. */
		cru_dbg(cru, "Dropping frame %u\n", cru->sequence);
	}

	cru->sequence++;

	/* Prepare for next frame */
	rzg2l_cru_fill_hw_slot(cru, slot);

done:
	spin_unlock_irqrestore(&cru->qlock, flags);

	return IRQ_RETVAL(handled);
}
#endif

static irqreturn_t rzv2n_cru_err_irq(int irq, void *data)
{
	struct rzg2l_cru_dev *cru = data;
	u32 irq_status;
	unsigned int handled = 0;

	irq_status = rzg2l_cru_read(cru, CRUnINTS1);
	if (!(irq_status))
		goto done;

	handled = 1;

	if (irq_status & (CRUnINTS_FOS | CRUnINTS_SLVES | CRUnINTS_DECES)) {
		int slot = cru->last_filled_slot;

		cru_err(cru, "axi_mst_err_int: 0x%08x, slot=%d\n", irq_status, slot);
		if (!auto_recovery) {
			if (cru->queue_buf[slot]) {
				cru->queue_buf[slot]->field = cru->format.field;
				cru->queue_buf[slot]->sequence = cru->sequence;
				cru->queue_buf[slot]->vb2_buf.timestamp = ktime_get_ns();
				vb2_buffer_done(&cru->queue_buf[slot]->vb2_buf,
						VB2_BUF_STATE_DONE);
				/* Call IVC callback if it exists */
				if (cru->callbacks.frame_end)
					cru->callbacks.frame_end(cru->ctx_id, cru->queue_buf[slot]->vb2_buf.timestamp, 1);
				cru->queue_buf[slot] = NULL;
				rzg2l_cru_fill_hw_slot(cru, slot);
			} else {
				cru_err(cru, "axi_mst_err_int: no queue buf\n");
			}
		}
	}

	rzg2l_cru_write(cru, CRUnINTS1, irq_status);

done:
	return IRQ_RETVAL(handled);
}

void rzg2l_cru_dma_unregister(struct rzg2l_cru_dev *cru)
{
	mutex_destroy(&cru->lock);

	v4l2_device_unregister(&cru->v4l2_dev);
}

int rzg2l_cru_dma_register(struct rzg2l_cru_dev *cru, const int *irqs)
{
	struct vb2_queue *q = &cru->queue;
	int i, ret;

	/* Initialize the top-level structure */
	ret = v4l2_device_register(cru->dev, &cru->v4l2_dev);
	if (ret)
		return ret;

	mutex_init(&cru->lock);
	INIT_LIST_HEAD(&cru->buf_list);

	spin_lock_init(&cru->qlock);

	cru->state = STOPPED;
	cru->suspend = false;
	init_waitqueue_head(&cru->setup_wait);

	for (i = 0; i < HW_BUFFER_MAX; i++)
		cru->queue_buf[i] = NULL;

	/* buffer queue */
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_READ | VB2_DMABUF | VB2_USERPTR;
	q->lock = &cru->lock;
	q->drv_priv = cru;
	q->buf_struct_size = sizeof(struct rzg2l_cru_buffer);
	q->ops = &rzg2l_cru_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 4;
	q->dev = cru->dev;

	ret = vb2_queue_init(q);
	if (ret < 0) {
		cru_err(cru, "failed to initialize VB2 queue\n");
		goto error;
	}

	/* IRQ */
#ifdef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
	ret = devm_request_irq(cru->dev, irqs[CRU_IRQ_IMAGE_CONV_INT], (cru->info->type == RZ_CRU_G2L) ?
			       rzg2l_cru_irq : rzv2h_cru_irq,
			       IRQF_SHARED, dev_name(cru->dev), cru);
	if (ret) {
		cru_err(cru, "failed to request image conv_int irq\n");
		goto error;
	}
#else
	ret = devm_request_irq(cru->dev, irqs[CRU_IRQ_CRU_VSD_ADDR_WEND], rzv2h_cru_vsd_addr_wend_irq,
			       IRQF_SHARED, dev_name(cru->dev), cru);
	if (ret) {
		cru_err(cru, "failed to request cru_vd_addr_wend irq\n");
		goto error;
	}
#endif

	ret = devm_request_irq(cru->dev, irqs[CRU_IRQ_AXI_MST_ERR_INT], rzv2n_cru_err_irq,
			       IRQF_SHARED, dev_name(cru->dev), cru);
	if (ret) {
		cru_err(cru, "failed to request axi_mst_err_int irq\n");
		goto error;
	}

	return 0;
error:
	rzg2l_cru_dma_unregister(cru);

	return ret;
}
