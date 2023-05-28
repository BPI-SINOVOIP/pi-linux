// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner sun50i deinterlacer driver
 *
 * Copyright (C) 2020 Jernej Skrabec <jernej.skrabec@siol.net>
 *
 * Based on vim2m driver.
 */

#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>

#include "sun50i-di.h"

#define FLAG_SIZE (DEINTERLACE_MAX_WIDTH * DEINTERLACE_MAX_HEIGHT / 4)

static u32 deinterlace_formats[] = {
	V4L2_PIX_FMT_NV12,
	V4L2_PIX_FMT_NV21,
	V4L2_PIX_FMT_YUV420,
	V4L2_PIX_FMT_NV16,
	V4L2_PIX_FMT_NV61,
	V4L2_PIX_FMT_YUV422P
};

static inline u32 deinterlace_read(struct deinterlace_dev *dev, u32 reg)
{
	return readl(dev->base + reg);
}

static inline void deinterlace_write(struct deinterlace_dev *dev,
				     u32 reg, u32 value)
{
	writel(value, dev->base + reg);
}

static inline void deinterlace_set_bits(struct deinterlace_dev *dev,
					u32 reg, u32 bits)
{
	u32 val = readl(dev->base + reg);

	val |= bits;

	writel(val, dev->base + reg);
}

static inline void deinterlace_clr_set_bits(struct deinterlace_dev *dev,
					    u32 reg, u32 clr, u32 set)
{
	u32 val = readl(dev->base + reg);

	val &= ~clr;
	val |= set;

	writel(val, dev->base + reg);
}

static void deinterlace_device_run(void *priv)
{
	u32 width, height, reg, msk, pitch[3], offset[2], fmt;
	dma_addr_t buf, prev, curr, next, addr[4][3];
	struct deinterlace_ctx *ctx = priv;
	struct deinterlace_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *src, *dst;
	unsigned int val;
	bool motion;

	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	v4l2_m2m_buf_copy_metadata(src, dst, true);

	fmt = ctx->src_fmt.pixelformat;

	deinterlace_write(dev, DEINTERLACE_IN_FLAG_ADDR, ctx->flag1_buf_dma);
	deinterlace_write(dev, DEINTERLACE_OUT_FLAG_ADDR, ctx->flag2_buf_dma);
	deinterlace_write(dev, DEINTERLACE_FLAG_ADDRH, 0);
	deinterlace_write(dev, DEINTERLACE_FLAG_PITCH, 0x200);
	if (device_iommu_mapped(dev->dev))
		deinterlace_set_bits(dev, DEINTERLACE_CTRL,
				     DEINTERLACE_CTRL_IOMMU_EN);

	width = ctx->src_fmt.width;
	height = ctx->src_fmt.height;

	reg = DEINTERLACE_SIZE_WIDTH(width);
	reg |= DEINTERLACE_SIZE_HEIGHT(height);
	deinterlace_write(dev, DEINTERLACE_SIZE, reg);

	switch (fmt) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		reg = DEINTERLACE_FORMAT_YUV420SP;
		break;
	case V4L2_PIX_FMT_YUV420:
		reg = DEINTERLACE_FORMAT_YUV420P;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		reg = DEINTERLACE_FORMAT_YUV422SP;
		break;
	case V4L2_PIX_FMT_YUV422P:
		reg = DEINTERLACE_FORMAT_YUV422P;
		break;
	}
	deinterlace_write(dev, DEINTERLACE_FORMAT, reg);

	pitch[0] = ctx->src_fmt.bytesperline;
	switch (fmt) {
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YUV422P:
		pitch[1] = pitch[0] / 2;
		pitch[2] = pitch[1];
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		pitch[1] = pitch[0];
		pitch[2] = 0;
		break;
	}

	deinterlace_write(dev, DEINTERLACE_IN_PITCH0, pitch[0] * 2);
	deinterlace_write(dev, DEINTERLACE_IN_PITCH1, pitch[1] * 2);
	deinterlace_write(dev, DEINTERLACE_IN_PITCH2, pitch[2] * 2);
	deinterlace_write(dev, DEINTERLACE_OUT_PITCH0, pitch[0]);
	deinterlace_write(dev, DEINTERLACE_OUT_PITCH1, pitch[1]);
	deinterlace_write(dev, DEINTERLACE_OUT_PITCH2, pitch[2]);

	offset[0] = pitch[0] * height;
	switch (fmt) {
	case V4L2_PIX_FMT_YUV420:
		offset[1] = offset[0] + offset[0] / 4;
		break;
	case V4L2_PIX_FMT_YUV422P:
		offset[1] = offset[0] + offset[0] / 2;
		break;
	default:
		offset[1] = 0;
		break;
	}

	buf = vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0);
	next = buf;
	if (ctx->prev[0])
		buf = vb2_dma_contig_plane_dma_addr(&ctx->prev[0]->vb2_buf, 0);
	curr = buf;
	if (ctx->prev[1])
		buf = vb2_dma_contig_plane_dma_addr(&ctx->prev[1]->vb2_buf, 0);
	prev = buf;

	if (ctx->first_field == 0) {
		if (ctx->field == 0) {
			addr[0][0] = prev;
			addr[0][1] = prev + offset[0];
			addr[0][2] = prev + offset[1];
			addr[1][0] = prev + pitch[0];
			addr[1][1] = prev + offset[0] + pitch[1];
			addr[1][2] = prev + offset[1] + pitch[2];
			addr[2][0] = curr;
			addr[2][1] = curr + offset[0];
			addr[2][2] = curr + offset[1];
			addr[3][0] = curr + pitch[0];
			addr[3][1] = curr + offset[0] + pitch[1];
			addr[3][2] = curr + offset[1] + pitch[2];
		} else {
			addr[0][0] = prev + pitch[0];
			addr[0][1] = prev + offset[0] + pitch[1];
			addr[0][2] = prev + offset[1] + pitch[2];
			addr[1][0] = curr;
			addr[1][1] = curr + offset[0];
			addr[1][2] = curr + offset[1];
			addr[2][0] = curr + pitch[0];
			addr[2][1] = curr + offset[0] + pitch[1];
			addr[2][2] = curr + offset[1] + pitch[2];
			addr[3][0] = next;
			addr[3][1] = next + offset[0];
			addr[3][2] = next + offset[1];
		}
	} else {
		if (ctx->field == 0) {
			addr[0][0] = prev;
			addr[0][1] = prev + offset[0];
			addr[0][2] = prev + offset[1];
			addr[1][0] = curr + pitch[0];
			addr[1][1] = curr + offset[0] + pitch[1];
			addr[1][2] = curr + offset[1] + pitch[2];
			addr[2][0] = curr;
			addr[2][1] = curr + offset[0];
			addr[2][2] = curr + offset[1];
			addr[3][0] = next + pitch[0];
			addr[3][1] = next + offset[0] + pitch[1];
			addr[3][2] = next + offset[1] + pitch[2];
		} else {
			addr[0][0] = prev + pitch[0];
			addr[0][1] = prev + offset[0] + pitch[1];
			addr[0][2] = prev + offset[1] + pitch[2];
			addr[1][0] = prev;
			addr[1][1] = prev + offset[0];
			addr[1][2] = prev + offset[1];
			addr[2][0] = curr + pitch[0];
			addr[2][1] = curr + offset[0] + pitch[1];
			addr[2][2] = curr + offset[1] + pitch[2];
			addr[3][0] = curr;
			addr[3][1] = curr + offset[0];
			addr[3][2] = curr + offset[1];
		}
	}

	deinterlace_write(dev, DEINTERLACE_IN0_ADDR0, addr[0][0]);
	deinterlace_write(dev, DEINTERLACE_IN0_ADDR1, addr[0][1]);
	deinterlace_write(dev, DEINTERLACE_IN0_ADDR2, addr[0][2]);

	deinterlace_write(dev, DEINTERLACE_IN1_ADDR0, addr[1][0]);
	deinterlace_write(dev, DEINTERLACE_IN1_ADDR1, addr[1][1]);
	deinterlace_write(dev, DEINTERLACE_IN1_ADDR2, addr[1][2]);

	deinterlace_write(dev, DEINTERLACE_IN2_ADDR0, addr[2][0]);
	deinterlace_write(dev, DEINTERLACE_IN2_ADDR1, addr[2][1]);
	deinterlace_write(dev, DEINTERLACE_IN2_ADDR2, addr[2][2]);

	deinterlace_write(dev, DEINTERLACE_IN3_ADDR0, addr[3][0]);
	deinterlace_write(dev, DEINTERLACE_IN3_ADDR1, addr[3][1]);
	deinterlace_write(dev, DEINTERLACE_IN3_ADDR2, addr[3][2]);

	buf = vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0);
	deinterlace_write(dev, DEINTERLACE_OUT_ADDR0, buf);
	deinterlace_write(dev, DEINTERLACE_OUT_ADDR1, buf + offset[0]);
	deinterlace_write(dev, DEINTERLACE_OUT_ADDR2, buf + offset[1]);

	if (ctx->first_field == 0)
		val = 4;
	else
		val = 5;

	reg = DEINTERLACE_INTP_PARAM0_LUMA_CUR_FAC_MODE(val);
	reg |= DEINTERLACE_INTP_PARAM0_CHROMA_CUR_FAC_MODE(val);
	msk = DEINTERLACE_INTP_PARAM0_LUMA_CUR_FAC_MODE_MSK;
	msk |= DEINTERLACE_INTP_PARAM0_CHROMA_CUR_FAC_MODE_MSK;
	deinterlace_clr_set_bits(dev, DEINTERLACE_INTP_PARAM0, msk, reg);

	reg = DEINTERLACE_POLAR_FIELD(ctx->field);
	deinterlace_write(dev, DEINTERLACE_POLAR, reg);

	motion = ctx->prev[0] && ctx->prev[1];
	reg = DEINTERLACE_MODE_DEINT_LUMA;
	if (motion)
		reg |= DEINTERLACE_MODE_MOTION_EN;
	reg |= DEINTERLACE_MODE_INTP_EN;
	reg |= DEINTERLACE_MODE_AUTO_UPD_MODE(ctx->first_field);
	reg |= DEINTERLACE_MODE_DEINT_CHROMA;
	if (!motion)
		reg |= DEINTERLACE_MODE_FIELD_MODE;
	deinterlace_write(dev, DEINTERLACE_MODE, reg);

	deinterlace_set_bits(dev, DEINTERLACE_INT_CTRL,
			     DEINTERLACE_INT_EN);

	deinterlace_set_bits(dev, DEINTERLACE_CTRL,
			     DEINTERLACE_CTRL_START);
}

static int deinterlace_job_ready(void *priv)
{
	struct deinterlace_ctx *ctx = priv;

	return v4l2_m2m_num_src_bufs_ready(ctx->fh.m2m_ctx) >= 1 &&
	       v4l2_m2m_num_dst_bufs_ready(ctx->fh.m2m_ctx) >= 2;
}

static void deinterlace_job_abort(void *priv)
{
	struct deinterlace_ctx *ctx = priv;

	/* Will cancel the transaction in the next interrupt handler */
	ctx->aborting = 1;
}

static irqreturn_t deinterlace_irq(int irq, void *data)
{
	struct deinterlace_dev *dev = data;
	struct vb2_v4l2_buffer *src, *dst;
	struct deinterlace_ctx *ctx;
	unsigned int val;

	ctx = v4l2_m2m_get_curr_priv(dev->m2m_dev);
	if (!ctx) {
		v4l2_err(&dev->v4l2_dev,
			 "Instance released before the end of transaction\n");
		return IRQ_NONE;
	}

	val = deinterlace_read(dev, DEINTERLACE_STATUS);
	if (!(val & DEINTERLACE_STATUS_FINISHED))
		return IRQ_NONE;

	deinterlace_write(dev, DEINTERLACE_INT_CTRL, 0);
	deinterlace_set_bits(dev, DEINTERLACE_STATUS,
			     DEINTERLACE_STATUS_FINISHED);
	deinterlace_clr_set_bits(dev, DEINTERLACE_CTRL,
				 DEINTERLACE_CTRL_START, 0);

	dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	v4l2_m2m_buf_done(dst, VB2_BUF_STATE_DONE);

	if (ctx->field != ctx->first_field || ctx->aborting) {
		ctx->field = ctx->first_field;

		src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		if (ctx->prev[1])
			v4l2_m2m_buf_done(ctx->prev[1], VB2_BUF_STATE_DONE);
		ctx->prev[1] = ctx->prev[0];
		ctx->prev[0] = src;

		v4l2_m2m_job_finish(ctx->dev->m2m_dev, ctx->fh.m2m_ctx);
	} else {
		ctx->field = !ctx->first_field;
		deinterlace_device_run(ctx);
	}

	return IRQ_HANDLED;
}

static void deinterlace_init(struct deinterlace_dev *dev)
{
	u32 reg;

	deinterlace_write(dev, DEINTERLACE_OUT_PATH, 0);

	reg = DEINTERLACE_MD_PARAM0_MIN_LUMA_TH(4);
	reg |= DEINTERLACE_MD_PARAM0_MAX_LUMA_TH(12);
	reg |= DEINTERLACE_MD_PARAM0_AVG_LUMA_SHIFT(6);
	reg |= DEINTERLACE_MD_PARAM0_TH_SHIFT(1);
	deinterlace_write(dev, DEINTERLACE_MD_PARAM0, reg);

	reg = DEINTERLACE_MD_PARAM1_MOV_FAC_NONEDGE(2);
	deinterlace_write(dev, DEINTERLACE_MD_PARAM1, reg);

	reg = DEINTERLACE_MD_PARAM2_CHROMA_SPATIAL_TH(128);
	reg |= DEINTERLACE_MD_PARAM2_CHROMA_DIFF_TH(5);
	reg |= DEINTERLACE_MD_PARAM2_PIX_STATIC_TH(3);
	deinterlace_write(dev, DEINTERLACE_MD_PARAM2, reg);

	reg = DEINTERLACE_INTP_PARAM0_ANGLE_LIMIT(20);
	reg |= DEINTERLACE_INTP_PARAM0_ANGLE_CONST_TH(5);
	reg |= DEINTERLACE_INTP_PARAM0_LUMA_CUR_FAC_MODE(1);
	reg |= DEINTERLACE_INTP_PARAM0_CHROMA_CUR_FAC_MODE(1);
	deinterlace_write(dev, DEINTERLACE_INTP_PARAM0, reg);

	reg = DEINTERLACE_MD_CH_PARAM_BLEND_MODE(1);
	reg |= DEINTERLACE_MD_CH_PARAM_FONT_PRO_EN;
	reg |= DEINTERLACE_MD_CH_PARAM_FONT_PRO_TH(48);
	reg |= DEINTERLACE_MD_CH_PARAM_FONT_PRO_FAC(4);
	deinterlace_write(dev, DEINTERLACE_MD_CH_PARAM, reg);

	reg = DEINTERLACE_INTP_PARAM1_A(4);
	reg |= DEINTERLACE_INTP_PARAM1_EN;
	reg |= DEINTERLACE_INTP_PARAM1_C(10);
	reg |= DEINTERLACE_INTP_PARAM1_CMAX(64);
	reg |= DEINTERLACE_INTP_PARAM1_MAXRAT(2);
	deinterlace_write(dev, DEINTERLACE_INTP_PARAM1, reg);

	/* only 32-bit addresses are supported, so high bits are always 0 */
	deinterlace_write(dev, DEINTERLACE_IN0_ADDRH, 0);
	deinterlace_write(dev, DEINTERLACE_IN1_ADDRH, 0);
	deinterlace_write(dev, DEINTERLACE_IN2_ADDRH, 0);
	deinterlace_write(dev, DEINTERLACE_IN3_ADDRH, 0);
	deinterlace_write(dev, DEINTERLACE_OUT_ADDRH, 0);
}

static inline struct deinterlace_ctx *deinterlace_file2ctx(struct file *file)
{
	return container_of(file->private_data, struct deinterlace_ctx, fh);
}

static bool deinterlace_check_format(u32 pixelformat)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(deinterlace_formats); i++)
		if (deinterlace_formats[i] == pixelformat)
			return true;

	return false;
}

static void deinterlace_prepare_format(struct v4l2_pix_format *pix_fmt)
{
	unsigned int bytesperline = pix_fmt->bytesperline;
	unsigned int height = pix_fmt->height;
	unsigned int width = pix_fmt->width;
	unsigned int sizeimage;

	width = clamp(width, DEINTERLACE_MIN_WIDTH,
		      DEINTERLACE_MAX_WIDTH);
	height = clamp(height, DEINTERLACE_MIN_HEIGHT,
		       DEINTERLACE_MAX_HEIGHT);

	/* try to respect userspace wishes about pitch */
	bytesperline = ALIGN(bytesperline, 2);
	if (bytesperline < ALIGN(width, 2))
		bytesperline = ALIGN(width, 2);

	/* luma */
	sizeimage = bytesperline * height;
	/* chroma */
	switch (pix_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_YUV420:
		sizeimage += bytesperline * height / 2;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_YUV422P:
		sizeimage += bytesperline * height;
		break;
	}

	if (pix_fmt->sizeimage < sizeimage)
		pix_fmt->sizeimage = sizeimage;

	pix_fmt->width = width;
	pix_fmt->height = height;
	pix_fmt->bytesperline = bytesperline;
}

static int deinterlace_querycap(struct file *file, void *priv,
				struct v4l2_capability *cap)
{
	strscpy(cap->driver, DEINTERLACE_NAME, sizeof(cap->driver));
	strscpy(cap->card, DEINTERLACE_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", DEINTERLACE_NAME);

	return 0;
}

static int deinterlace_enum_fmt(struct file *file, void *priv,
				struct v4l2_fmtdesc *f)
{
	if (f->index < ARRAY_SIZE(deinterlace_formats)) {
		f->pixelformat = deinterlace_formats[f->index];

		return 0;
	}

	return -EINVAL;
}

static int deinterlace_enum_framesizes(struct file *file, void *priv,
				       struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index != 0)
		return -EINVAL;

	if (!deinterlace_check_format(fsize->pixel_format))
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = DEINTERLACE_MIN_WIDTH;
	fsize->stepwise.min_height = DEINTERLACE_MIN_HEIGHT;
	fsize->stepwise.max_width = DEINTERLACE_MAX_WIDTH;
	fsize->stepwise.max_height = DEINTERLACE_MAX_HEIGHT;
	fsize->stepwise.step_width = 2;

	switch (fsize->pixel_format) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_YUV420:
		fsize->stepwise.step_height = 2;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_YUV422P:
		fsize->stepwise.step_height = 1;
		break;
	}

	return 0;
}

static int deinterlace_set_cap_format(struct deinterlace_ctx *ctx,
				      struct v4l2_pix_format *f)
{
	if (!deinterlace_check_format(ctx->src_fmt.pixelformat))
		return -EINVAL;

	f->pixelformat = ctx->src_fmt.pixelformat;
	f->field = V4L2_FIELD_NONE;
	f->width = ctx->src_fmt.width;
	f->height = ctx->src_fmt.height;

	deinterlace_prepare_format(f);

	return 0;
}

static int deinterlace_g_fmt_vid_cap(struct file *file, void *priv,
				     struct v4l2_format *f)
{
	struct deinterlace_ctx *ctx = deinterlace_file2ctx(file);

	f->fmt.pix = ctx->dst_fmt;

	return 0;
}

static int deinterlace_g_fmt_vid_out(struct file *file, void *priv,
				     struct v4l2_format *f)
{
	struct deinterlace_ctx *ctx = deinterlace_file2ctx(file);

	f->fmt.pix = ctx->src_fmt;

	return 0;
}

static int deinterlace_try_fmt_vid_cap(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct deinterlace_ctx *ctx = deinterlace_file2ctx(file);

	return deinterlace_set_cap_format(ctx, &f->fmt.pix);
}

static int deinterlace_try_fmt_vid_out(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	if (!deinterlace_check_format(f->fmt.pix.pixelformat))
		f->fmt.pix.pixelformat = deinterlace_formats[0];

	if (f->fmt.pix.field != V4L2_FIELD_INTERLACED_TB &&
	    f->fmt.pix.field != V4L2_FIELD_INTERLACED_BT &&
	    f->fmt.pix.field != V4L2_FIELD_INTERLACED)
		f->fmt.pix.field = V4L2_FIELD_INTERLACED;

	deinterlace_prepare_format(&f->fmt.pix);

	return 0;
}

static int deinterlace_s_fmt_vid_cap(struct file *file, void *priv,
				     struct v4l2_format *f)
{
	struct deinterlace_ctx *ctx = deinterlace_file2ctx(file);
	struct vb2_queue *vq;
	int ret;

	ret = deinterlace_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq))
		return -EBUSY;

	ctx->dst_fmt = f->fmt.pix;

	return 0;
}

static int deinterlace_s_fmt_vid_out(struct file *file, void *priv,
				     struct v4l2_format *f)
{
	struct deinterlace_ctx *ctx = deinterlace_file2ctx(file);
	struct vb2_queue *vq;
	int ret;

	ret = deinterlace_try_fmt_vid_out(file, priv, f);
	if (ret)
		return ret;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq))
		return -EBUSY;

	/*
	 * Capture queue has to be also checked, because format and size
	 * depends on output format and size.
	 */
	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (vb2_is_busy(vq))
		return -EBUSY;

	ctx->src_fmt = f->fmt.pix;

	/* Propagate colorspace information to capture. */
	ctx->dst_fmt.colorspace = f->fmt.pix.colorspace;
	ctx->dst_fmt.xfer_func = f->fmt.pix.xfer_func;
	ctx->dst_fmt.ycbcr_enc = f->fmt.pix.ycbcr_enc;
	ctx->dst_fmt.quantization = f->fmt.pix.quantization;

	return deinterlace_set_cap_format(ctx, &ctx->dst_fmt);
}

static const struct v4l2_ioctl_ops deinterlace_ioctl_ops = {
	.vidioc_querycap		= deinterlace_querycap,

	.vidioc_enum_framesizes		= deinterlace_enum_framesizes,

	.vidioc_enum_fmt_vid_cap	= deinterlace_enum_fmt,
	.vidioc_g_fmt_vid_cap		= deinterlace_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= deinterlace_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= deinterlace_s_fmt_vid_cap,

	.vidioc_enum_fmt_vid_out	= deinterlace_enum_fmt,
	.vidioc_g_fmt_vid_out		= deinterlace_g_fmt_vid_out,
	.vidioc_try_fmt_vid_out		= deinterlace_try_fmt_vid_out,
	.vidioc_s_fmt_vid_out		= deinterlace_s_fmt_vid_out,

	.vidioc_reqbufs			= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf		= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf			= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf			= v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf		= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs		= v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf			= v4l2_m2m_ioctl_expbuf,

	.vidioc_streamon		= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff		= v4l2_m2m_ioctl_streamoff,
};

static int deinterlace_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				   unsigned int *nplanes, unsigned int sizes[],
				   struct device *alloc_devs[])
{
	struct deinterlace_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix_fmt;

	if (V4L2_TYPE_IS_OUTPUT(vq->type))
		pix_fmt = &ctx->src_fmt;
	else
		pix_fmt = &ctx->dst_fmt;

	if (*nplanes) {
		if (sizes[0] < pix_fmt->sizeimage)
			return -EINVAL;
	} else {
		sizes[0] = pix_fmt->sizeimage;
		*nplanes = 1;
	}

	return 0;
}

static int deinterlace_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct deinterlace_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix_fmt;

	if (V4L2_TYPE_IS_OUTPUT(vq->type))
		pix_fmt = &ctx->src_fmt;
	else
		pix_fmt = &ctx->dst_fmt;

	if (vb2_plane_size(vb, 0) < pix_fmt->sizeimage)
		return -EINVAL;

	vb2_set_plane_payload(vb, 0, pix_fmt->sizeimage);

	return 0;
}

static void deinterlace_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct deinterlace_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static void deinterlace_queue_cleanup(struct vb2_queue *vq, u32 state)
{
	struct deinterlace_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vbuf;

	do {
		if (V4L2_TYPE_IS_OUTPUT(vq->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

		if (vbuf)
			v4l2_m2m_buf_done(vbuf, state);
	} while (vbuf);

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		if (ctx->prev[0])
			v4l2_m2m_buf_done(ctx->prev[0], state);
		if (ctx->prev[1])
			v4l2_m2m_buf_done(ctx->prev[1], state);
	}
}

static int deinterlace_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct deinterlace_ctx *ctx = vb2_get_drv_priv(vq);
	struct device *dev = ctx->dev->dev;
	int ret;

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "Failed to enable module\n");

			goto err_runtime_get;
		}

		ctx->first_field =
			ctx->src_fmt.field == V4L2_FIELD_INTERLACED_BT;
		ctx->field = ctx->first_field;

		ctx->prev[0] = NULL;
		ctx->prev[1] = NULL;
		ctx->aborting = 0;

		ctx->flag1_buf = dma_alloc_coherent(dev, FLAG_SIZE,
						    &ctx->flag1_buf_dma,
						    GFP_KERNEL);
		if (!ctx->flag1_buf) {
			ret = -ENOMEM;

			goto err_no_mem1;
		}

		ctx->flag2_buf = dma_alloc_coherent(dev, FLAG_SIZE,
						    &ctx->flag2_buf_dma,
						    GFP_KERNEL);
		if (!ctx->flag2_buf) {
			ret = -ENOMEM;

			goto err_no_mem2;
		}
	}

	return 0;

err_no_mem2:
	dma_free_coherent(dev, FLAG_SIZE, ctx->flag1_buf,
			  ctx->flag1_buf_dma);
err_no_mem1:
	pm_runtime_put(dev);
err_runtime_get:
	deinterlace_queue_cleanup(vq, VB2_BUF_STATE_QUEUED);

	return ret;
}

static void deinterlace_stop_streaming(struct vb2_queue *vq)
{
	struct deinterlace_ctx *ctx = vb2_get_drv_priv(vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		struct device *dev = ctx->dev->dev;

		dma_free_coherent(dev, FLAG_SIZE, ctx->flag1_buf,
				  ctx->flag1_buf_dma);
		dma_free_coherent(dev, FLAG_SIZE, ctx->flag2_buf,
				  ctx->flag2_buf_dma);

		pm_runtime_put(dev);
	}

	deinterlace_queue_cleanup(vq, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops deinterlace_qops = {
	.queue_setup		= deinterlace_queue_setup,
	.buf_prepare		= deinterlace_buf_prepare,
	.buf_queue		= deinterlace_buf_queue,
	.start_streaming	= deinterlace_start_streaming,
	.stop_streaming		= deinterlace_stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

static int deinterlace_queue_init(void *priv, struct vb2_queue *src_vq,
				  struct vb2_queue *dst_vq)
{
	struct deinterlace_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->min_buffers_needed = 1;
	src_vq->ops = &deinterlace_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->dev->dev_mutex;
	src_vq->dev = ctx->dev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->min_buffers_needed = 2;
	dst_vq->ops = &deinterlace_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->dev->dev_mutex;
	dst_vq->dev = ctx->dev->dev;

	ret = vb2_queue_init(dst_vq);
	if (ret)
		return ret;

	return 0;
}

static int deinterlace_open(struct file *file)
{
	struct deinterlace_dev *dev = video_drvdata(file);
	struct deinterlace_ctx *ctx = NULL;
	int ret;

	if (mutex_lock_interruptible(&dev->dev_mutex))
		return -ERESTARTSYS;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		mutex_unlock(&dev->dev_mutex);
		return -ENOMEM;
	}

	/* default output format */
	ctx->src_fmt.pixelformat = deinterlace_formats[0];
	ctx->src_fmt.field = V4L2_FIELD_INTERLACED;
	ctx->src_fmt.width = 640;
	ctx->src_fmt.height = 480;
	deinterlace_prepare_format(&ctx->src_fmt);

	/* default capture format */
	ctx->dst_fmt.pixelformat = deinterlace_formats[0];
	ctx->dst_fmt.field = V4L2_FIELD_NONE;
	ctx->dst_fmt.width = 640;
	ctx->dst_fmt.height = 480;
	deinterlace_prepare_format(&ctx->dst_fmt);

	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	ctx->dev = dev;

	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx,
					    &deinterlace_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_free;
	}

	v4l2_fh_add(&ctx->fh);

	mutex_unlock(&dev->dev_mutex);

	return 0;

err_free:
	kfree(ctx);
	mutex_unlock(&dev->dev_mutex);

	return ret;
}

static int deinterlace_release(struct file *file)
{
	struct deinterlace_dev *dev = video_drvdata(file);
	struct deinterlace_ctx *ctx = container_of(file->private_data,
						   struct deinterlace_ctx, fh);

	mutex_lock(&dev->dev_mutex);

	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);

	kfree(ctx);

	mutex_unlock(&dev->dev_mutex);

	return 0;
}

static const struct v4l2_file_operations deinterlace_fops = {
	.owner		= THIS_MODULE,
	.open		= deinterlace_open,
	.release	= deinterlace_release,
	.poll		= v4l2_m2m_fop_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= v4l2_m2m_fop_mmap,
};

static const struct video_device deinterlace_video_device = {
	.name		= DEINTERLACE_NAME,
	.vfl_dir	= VFL_DIR_M2M,
	.fops		= &deinterlace_fops,
	.ioctl_ops	= &deinterlace_ioctl_ops,
	.minor		= -1,
	.release	= video_device_release_empty,
	.device_caps	= V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING,
};

static const struct v4l2_m2m_ops deinterlace_m2m_ops = {
	.device_run	= deinterlace_device_run,
	.job_ready	= deinterlace_job_ready,
	.job_abort	= deinterlace_job_abort,
};

static int deinterlace_probe(struct platform_device *pdev)
{
	struct deinterlace_dev *dev;
	struct video_device *vfd;
	struct resource *res;
	int irq, ret;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->vfd = deinterlace_video_device;
	dev->dev = &pdev->dev;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0)
		return irq;

	ret = devm_request_irq(dev->dev, irq, deinterlace_irq,
			       0, dev_name(dev->dev), dev);
	if (ret) {
		dev_err(dev->dev, "Failed to request IRQ\n");

		return ret;
	}

	ret = of_dma_configure(dev->dev, dev->dev->of_node, true);
	if (ret)
		return ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dev->base))
		return PTR_ERR(dev->base);

	dev->bus_clk = devm_clk_get(dev->dev, "bus");
	if (IS_ERR(dev->bus_clk)) {
		dev_err(dev->dev, "Failed to get bus clock\n");

		return PTR_ERR(dev->bus_clk);
	}

	dev->mod_clk = devm_clk_get(dev->dev, "mod");
	if (IS_ERR(dev->mod_clk)) {
		dev_err(dev->dev, "Failed to get mod clock\n");

		return PTR_ERR(dev->mod_clk);
	}

	dev->ram_clk = devm_clk_get(dev->dev, "ram");
	if (IS_ERR(dev->ram_clk)) {
		dev_err(dev->dev, "Failed to get ram clock\n");

		return PTR_ERR(dev->ram_clk);
	}

	dev->rstc = devm_reset_control_get(dev->dev, NULL);
	if (IS_ERR(dev->rstc)) {
		dev_err(dev->dev, "Failed to get reset control\n");

		return PTR_ERR(dev->rstc);
	}

	mutex_init(&dev->dev_mutex);

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret) {
		dev_err(dev->dev, "Failed to register V4L2 device\n");

		return ret;
	}

	vfd = &dev->vfd;
	vfd->lock = &dev->dev_mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;

	snprintf(vfd->name, sizeof(vfd->name), "%s",
		 deinterlace_video_device.name);
	video_set_drvdata(vfd, dev);

	ret = video_register_device(vfd, VFL_TYPE_VIDEO, 0);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "Failed to register video device\n");

		goto err_v4l2;
	}

	v4l2_info(&dev->v4l2_dev,
		  "Device registered as /dev/video%d\n", vfd->num);

	dev->m2m_dev = v4l2_m2m_init(&deinterlace_m2m_ops);
	if (IS_ERR(dev->m2m_dev)) {
		v4l2_err(&dev->v4l2_dev,
			 "Failed to initialize V4L2 M2M device\n");
		ret = PTR_ERR(dev->m2m_dev);

		goto err_video;
	}

	platform_set_drvdata(pdev, dev);

	pm_runtime_enable(dev->dev);

	return 0;

err_video:
	video_unregister_device(&dev->vfd);
err_v4l2:
	v4l2_device_unregister(&dev->v4l2_dev);

	return ret;
}

static int deinterlace_remove(struct platform_device *pdev)
{
	struct deinterlace_dev *dev = platform_get_drvdata(pdev);

	v4l2_m2m_release(dev->m2m_dev);
	video_unregister_device(&dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);

	pm_runtime_force_suspend(&pdev->dev);

	return 0;
}

static int deinterlace_runtime_resume(struct device *device)
{
	struct deinterlace_dev *dev = dev_get_drvdata(device);
	int ret;

	ret = clk_prepare_enable(dev->bus_clk);
	if (ret) {
		dev_err(dev->dev, "Failed to enable bus clock\n");

		return ret;
	}

	ret = clk_prepare_enable(dev->mod_clk);
	if (ret) {
		dev_err(dev->dev, "Failed to enable mod clock\n");

		goto err_bus_clk;
	}

	ret = clk_prepare_enable(dev->ram_clk);
	if (ret) {
		dev_err(dev->dev, "Failed to enable ram clock\n");

		goto err_mod_clk;
	}

	ret = reset_control_deassert(dev->rstc);
	if (ret) {
		dev_err(dev->dev, "Failed to apply reset\n");

		goto err_ram_clk;
	}

	deinterlace_init(dev);

	return 0;

err_ram_clk:
	clk_disable_unprepare(dev->ram_clk);
err_mod_clk:
	clk_disable_unprepare(dev->mod_clk);
err_bus_clk:
	clk_disable_unprepare(dev->bus_clk);

	return ret;
}

static int deinterlace_runtime_suspend(struct device *device)
{
	struct deinterlace_dev *dev = dev_get_drvdata(device);

	reset_control_assert(dev->rstc);

	clk_disable_unprepare(dev->ram_clk);
	clk_disable_unprepare(dev->mod_clk);
	clk_disable_unprepare(dev->bus_clk);

	return 0;
}

static const struct of_device_id deinterlace_dt_match[] = {
	{ .compatible = "allwinner,sun50i-h6-deinterlace" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, deinterlace_dt_match);

static const struct dev_pm_ops deinterlace_pm_ops = {
	.runtime_resume		= deinterlace_runtime_resume,
	.runtime_suspend	= deinterlace_runtime_suspend,
};

static struct platform_driver deinterlace_driver = {
	.probe		= deinterlace_probe,
	.remove		= deinterlace_remove,
	.driver		= {
		.name		= DEINTERLACE_NAME,
		.of_match_table	= deinterlace_dt_match,
		.pm		= &deinterlace_pm_ops,
	},
};
module_platform_driver(deinterlace_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jernej Skrabec <jernej.skrabec@siol.net>");
MODULE_DESCRIPTION("Allwinner Deinterlace driver");
