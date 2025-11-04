/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2015-2024 Renesas Electronics Corporation
 *
 * License        Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * GPLv2:
 * If you wish to use this file under the terms of GPL, following terms are
 * effective.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */ /*************************************************************************/

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "fdp_drv_public.h"
#include "fdp_drv_local.h"
#include "fdp_drv_hw.h"

#include "fdp_drv_tbl.h"

/******************************************************************************
 * Function:		fdp_ins_check_seq_param
 * Description:	Check sequence parameter.
 * Returns:		0/E_FDP_PARA_INHEIGHT/E_FDP_PARA_INWIDTH
 *	E_FDP_PARA_SEQMODE/E_FDP_PARA_INTERPOLATED/E_FDP_PARA_TELECINEMODE
 ******************************************************************************/
static long fdp_ins_check_seq_param(
	struct fdp_obj_t *obj,
	struct fdp_fproc_t *fproc_par,
	struct fdp_seq_t *seq_par)
{
	struct vspm_fdp_proc_info *proc_info = obj->proc_info;

	/* sequence mode */
	switch (seq_par->seq_mode) {
	case FDP_SEQ_PROG:
		/* check input pucture vertical size */
		if (seq_par->in_height < 32 ||
		    seq_par->in_height > 8190 ||
		    (seq_par->in_height & 0x1))
			return E_FDP_PARA_INHEIGHT;

		/* set progressive mode */
		obj->ctrl_mode =
			FD1_CTL_OPMODE_PROGRESSIVE |
			FD1_CTL_OPMODE_NO_INTERRUPT;
		obj->rpf_format =
			FD1_RPF_FORMAT_CIPM_PRG;

		/* set still mask address index */
		proc_info->stlmsk_idx = 0;
		break;
	case FDP_SEQ_INTER:
	case FDP_SEQ_INTER_2D:
		/* check input pucture vertical size */
		if (seq_par->in_height < 16 ||
		    seq_par->in_height > 4095)
			return E_FDP_PARA_INHEIGHT;

		/* set interlace mode */
		obj->ctrl_mode =
			FD1_CTL_OPMODE_INTERLACE |
			FD1_CTL_OPMODE_NO_INTERRUPT;
		obj->rpf_format =
			FD1_RPF_FORMAT_CIPM_INT;

		/* check current field parity parameter */
		if (fproc_par->current_field == FDP_CF_TOP)
			obj->rpf_format |= FD1_RPF_FORMAT_CF_TOP;
		else if (fproc_par->current_field == FDP_CF_BOTTOM)
			obj->rpf_format |= FD1_RPF_FORMAT_CF_BOTTOM;
		else
			return E_FDP_PARA_CF;

		/* set still mask address index */
		proc_info->stlmsk_idx = fproc_par->current_field;
		break;
	default:
		return E_FDP_PARA_SEQMODE;
	}

	/* check input picture horizontal size */
	if (seq_par->in_width < 32 ||
	    seq_par->in_width > 8190 ||
	    (seq_par->in_width & 0x1))
		return E_FDP_PARA_INWIDTH;

	/* check telecine detect mode */
	switch (seq_par->telecine_mode) {
	case FDP_TC_OFF:
	case FDP_TC_FORCED_PULL_DOWN:
		break;
	case FDP_TC_INTERPOLATED_LINE:
		if (seq_par->seq_mode == FDP_SEQ_INTER) {
			if (fproc_par->interpolated_line != FDP_DIM_PREV &&
			    fproc_par->interpolated_line != FDP_DIM_NEXT) {
				return E_FDP_PARA_INTERPOLATED;
			}
		}
		break;
	default:
		return E_FDP_PARA_TELECINEMODE;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_check_pic_param
 * Description:	Check picture information parameter.
 * Returns:		0/E_FDP_PARA_INPIC/E_FDP_PARA_CHROMA
 ******************************************************************************/
static long fdp_ins_check_pic_param(
	struct fdp_obj_t *obj, struct fdp_pic_t *in_pic)
{
	/* check pointer */
	if (!in_pic)
		return E_FDP_PARA_INPIC;

	/* check input format */
	switch (in_pic->chroma_format) {
	case FDP_YUV420:	/* supported TL conversion */
		obj->rpf_format |= FD1_RPF_FORMAT_YUV420_NV12;
		break;
	case FDP_YUV420_PLANAR:
		obj->rpf_format |= FD1_RPF_FORMAT_YUV420_YV12;
		break;
	case FDP_YUV420_NV21:	/* supported TL conversion */
		obj->rpf_format |= FD1_RPF_FORMAT_YUV420_NV21;
		break;
	case FDP_YUV422_NV16:
		obj->rpf_format |= FD1_RPF_FORMAT_YUV422_NV16;
		break;
	case FDP_YUV422_YUY2:
		obj->rpf_format |= FD1_RPF_FORMAT_YUV422_YUY2;
		break;
	case FDP_YUV422_UYVY:
		obj->rpf_format |= FD1_RPF_FORMAT_YUV422_UYVY;
		break;
	case FDP_YUV422_PLANAR:
		obj->rpf_format |= FD1_RPF_FORMAT_YUV422_YV16;
		break;
	case FDP_YUV444_PLANAR:
		obj->rpf_format |= FD1_RPF_FORMAT_YUV444_PLANAR;
		break;
	default:
		return E_FDP_PARA_CHROMA;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_check_pic_param_for_pulldown
 * Description:	Check picture information parameter.
 * Returns:		0/E_FDP_PARA_PICWIDTH/E_FDP_PARA_PICHEIGHT
 *	E_FDP_PARA_PROGSEQ/E_FDP_PARA_REPEATTOP/E_FDP_PARA_PICSTRUCT
 ******************************************************************************/
static long fdp_ins_check_pic_param_for_pulldown(
	struct fdp_pic_t *in_pic, struct fdp_seq_t *seq_par)
{
	/* check horizontal size */
	if (in_pic->width != seq_par->in_width)
		return E_FDP_PARA_PICWIDTH;

	/* check vertical size */
	if (in_pic->height != seq_par->in_height)
		return E_FDP_PARA_PICHEIGHT;

	/* check decode information progressive_sequence */
	if (in_pic->progressive_sequence == 0) {
		if (seq_par->seq_mode == FDP_SEQ_PROG)
			return E_FDP_PARA_PROGSEQ;

		/* check decode information picture_structure */
		switch (in_pic->picture_structure) {
		case 0:
			return E_FDP_PARA_REPEATTOP;
		case 1:
		case 2:
			if (in_pic->progressive_frame == 1)
				return E_FDP_PARA_REPEATTOP;
		case 3:
			break;
		default:
			return E_FDP_PARA_PICSTRUCT;
		}
	} else if (in_pic->progressive_sequence == 1) {
		if (seq_par->seq_mode != FDP_SEQ_PROG)
			return E_FDP_PARA_PROGSEQ;

		/* check decode information picture_structure */
		switch (in_pic->picture_structure) {
		case 0:
		case 1:
		case 2:
			return E_FDP_PARA_REPEATTOP;
		case 3:
			if (in_pic->repeat_first_field == 0 &&
			    in_pic->top_field_first == 1)
				return E_FDP_PARA_REPEATTOP;
			break;
		default:
			return E_FDP_PARA_PICSTRUCT;
		}
	} else {
		return E_FDP_PARA_PROGSEQ;
	}

	/* check decode information repeat_first_field */
	if (in_pic->repeat_first_field != 0 &&
	    in_pic->repeat_first_field != 1)
		return E_FDP_PARA_REPEATTOP;

	/* check decode information top_field_first */
	if (in_pic->top_field_first != 0 &&
	    in_pic->top_field_first != 1)
		return E_FDP_PARA_REPEATTOP;

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_check_dstbuf_param
 * Description:	Check destination parameter.
 * Returns:		0/E_FDP_PARA_OUTBUF
 *	E_FDP_PARA_DST_ADDR/E_FDP_PARA_DST_ADDR_C0/E_FDP_PARA_DST_ADDR_C1
 *	E_FDP_PARA_DST_STRIDE/E_FDP_PARA_DST_STRIDE_C
 ******************************************************************************/
static long fdp_ins_check_dstbuf_param(
	struct fdp_fproc_t *fproc_par, struct fdp_seq_t *seq_par)
{
	struct fdp_imgbuf_t *buf = fproc_par->out_buf;

	unsigned short stride = seq_par->in_width << 1;
	unsigned short stride_c = seq_par->in_width;

	/* check pointer */
	if (!buf)
		return E_FDP_PARA_OUTBUF;

	switch (fproc_par->out_format) {
	/* planar */
	case FDP_YUV420_PLANAR:
	case FDP_YUV422_PLANAR:
		stride_c >>= 1;
		/* check c1 address */
		if (buf->addr_c1 == 0)
			return E_FDP_PARA_DST_ADDR_C1;

		/* check c0 address */
		if (buf->addr_c0 == 0)
			return E_FDP_PARA_DST_ADDR_C0;

		/* check chroma stride */
		if (buf->stride_c < stride_c)
			return E_FDP_PARA_DST_STRIDE_C;

		stride >>= 1;

		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_DST_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_DST_STRIDE;
		break;
	case FDP_YUV444_PLANAR:
		/* check c1 address */
		if (buf->addr_c1 == 0)
			return E_FDP_PARA_DST_ADDR_C1;

		/* check c0 address */
		if (buf->addr_c0 == 0)
			return E_FDP_PARA_DST_ADDR_C0;

		/* check chroma stride */
		if (buf->stride_c < stride_c)
			return E_FDP_PARA_DST_STRIDE_C;

		stride >>= 1;

		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_DST_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_DST_STRIDE;
		break;
	/* semi-planar */
	case FDP_YUV420:
	case FDP_YUV420_NV21:
	case FDP_YUV422_NV16:
		/* check c0 address */
		if (buf->addr_c0 == 0)
			return E_FDP_PARA_DST_ADDR_C0;

		/* check chroma stride */
		if (buf->stride_c < stride_c)
			return E_FDP_PARA_DST_STRIDE_C;

		stride >>= 1;

		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_DST_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_DST_STRIDE;
		break;
	/* interleaved */
	case FDP_YUV422_YUY2:
	case FDP_YUV422_UYVY:
		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_DST_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_DST_STRIDE;
		break;
	default:
		break;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_check_refbuf_param
 * Description:	Check reference buffer parameter.
 * Returns:		0/E_FDP_PARA_REFBUF
 *	E_FDP_PARA_BUFREFRD1/E_FDP_PARA_SRC_ADDR/E_FDP_PARA_SRC_ADDR_C0
 *	E_FDP_PARA_SRC_ADDR_C1/E_FDP_PARA_SRC_STRIDE/E_FDP_PARA_SRC_STRIDE_C
 ******************************************************************************/
static long fdp_ins_check_refbuf_param(
	struct fdp_obj_t *obj,
	struct fdp_fproc_t *fproc_par,
	struct fdp_seq_t *seq_par)
{
	struct fdp_refbuf_t *ref = fproc_par->ref_buf;
	struct fdp_imgbuf_t *buf;

	unsigned short stride = seq_par->in_width << 1;
	unsigned short stride_c = seq_par->in_width;

	obj->rpf0_addr_y = 0;
	obj->rpf2_addr_y = 0;

	/* check reference pointer */
	if (!ref)
		return E_FDP_PARA_REFBUF;

	/* check current field buffer */
	buf = ref->cur_buf;
	if (!buf)
		return E_FDP_PARA_BUFREFRD1;

	/* check format */
	switch (fproc_par->in_pic->chroma_format) {
	/* planar */
	case FDP_YUV420_PLANAR:
	case FDP_YUV422_PLANAR:
		stride_c >>= 1;
		/* check c1 address */
		if (buf->addr_c1 == 0)
			return E_FDP_PARA_SRC_ADDR_C1;

		/* check c0 address */
		if (buf->addr_c0 == 0)
			return E_FDP_PARA_SRC_ADDR_C0;

		/* check chroma stride */
		if (buf->stride_c < stride_c)
			return E_FDP_PARA_SRC_STRIDE_C;

		stride >>= 1;

		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_SRC_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_SRC_STRIDE;
		break;
	case FDP_YUV444_PLANAR:
		/* check c1 address */
		if (buf->addr_c1 == 0)
			return E_FDP_PARA_SRC_ADDR_C1;

		/* check c0 address */
		if (buf->addr_c0 == 0)
			return E_FDP_PARA_SRC_ADDR_C0;

		/* check chroma stride */
		if (buf->stride_c < stride_c)
			return E_FDP_PARA_SRC_STRIDE_C;

		stride >>= 1;

		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_SRC_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_SRC_STRIDE;
		break;
	/* semi-planar */
	case FDP_YUV420:
	case FDP_YUV420_NV21:
	case FDP_YUV422_NV16:
		/* check c0 address */
		if (buf->addr_c0 == 0)
			return E_FDP_PARA_SRC_ADDR_C0;

		/* check chroma stride */
		if (buf->stride_c < stride_c)
			return E_FDP_PARA_SRC_STRIDE_C;

		stride >>= 1;

		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_SRC_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_SRC_STRIDE;
		break;
	/* interleaved */
	case FDP_YUV422_YUY2:
	case FDP_YUV422_UYVY:
		/* check luma address */
		if (buf->addr == 0)
			return E_FDP_PARA_SRC_ADDR;

		/* check luma stride */
		if (buf->stride < stride)
			return E_FDP_PARA_SRC_STRIDE;
		break;
	default:
		break;
	}

	/* get previous address */
	if (ref->prev_buf)
		obj->rpf0_addr_y = ref->prev_buf->addr;

	/* get next address */
	if (ref->next_buf)
		obj->rpf2_addr_y = ref->next_buf->addr;

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_check_fcp_param
 * Description:	Check frame compression processer parameter.
 * Returns:		0/E_FDP_PARA_OUTFORMAT/E_FDP_PARA_DST_ADDR
 *	E_FDP_PARA_DST_ADDR_C0/E_FDP_PARA_DST_ADDR_C1
 *	E_FDP_PARA_CHROMA/E_FDP_PARA_SRC_STRIDE/E_FDP_PARA_SRC_STRIDE_C
 *	E_FDP_PARA_FCNL/E_FDP_PARA_FCP_POS/E_FDP_PARA_FCP_STRIDE
 *	E_FDP_PARA_BA_REF/E_FDP_PARA_TLEN/E_FDP_PARA_BA_ANC
 ******************************************************************************/
static long fdp_ins_check_fcp_param(
	struct fdp_obj_t *obj, struct fdp_fproc_t *fproc_par)
{
	struct fcp_info_t *fcp = fproc_par->fcp_par;
	struct fdp_imgbuf_t *buf;
	unsigned short fcp_stride;

	obj->fcp_ref_addr_y0 = 0;
	obj->fcp_ref_addr_y2 = 0;
	obj->fcp_anc_addr_y0 = 0;
	obj->fcp_anc_addr_y2 = 0;

	if (fcp) {
		/* check FCNL compress parameter */
		if (fcp->fcnl == FCP_FCNL_ENABLE) {
			/* check destination format */
			if (fproc_par->out_format != FDP_YUV420_PLANAR &&
			    fproc_par->out_format != FDP_YUV422_PLANAR &&
			    fproc_par->out_format != FDP_YUV444_PLANAR)
				return E_FDP_PARA_OUTFORMAT;

			buf = fproc_par->out_buf;
			/* check destination address */
			if (buf->addr & 0xff)
				return E_FDP_PARA_DST_ADDR;

			if (buf->addr_c0 & 0xff)
				return E_FDP_PARA_DST_ADDR_C0;

			if (buf->addr_c1 & 0xff)
				return E_FDP_PARA_DST_ADDR_C1;

			/* check destination strude */
			if (buf->stride & 0xff)
				return E_FDP_PARA_DST_STRIDE;

			if (buf->stride_c & 0xff)
				return E_FDP_PARA_DST_STRIDE_C;

			/* enable FCNL compress */
			obj->wpf_format |= FD1_WPF_FORMAT_FCNL_ENABLE;
			obj->wpf_swap =
				FD1_WPF_SWAP_SSWAP | FD1_WPF_SWAP_OSWAP_FCP;
		} else if (fcp->fcnl == FCP_FCNL_DISABLE) {
			/* disable FCNL compress */
			obj->wpf_swap = FD1_WPF_SWAP_SSWAP | FD1_WPF_SWAP_OSWAP;
		} else {
			return E_FDP_PARA_FCNL;
		}

		/* check TL conversion parameter */
		if (fcp->tlen == FCP_TL_ENABLE) {
			/* check position */
			if (fcp->pos_y > 8189 ||
			    fcp->pos_c > 4094)
				return E_FDP_PARA_FCP_POS;

			/* check stride */
			fcp_stride = fcp->stride_div16 << 4;
			if (fcp_stride & (fcp_stride - 1))
				return E_FDP_PARA_FCP_STRIDE;

			if (fcp_stride < 128 || fcp_stride > 8192)
				return E_FDP_PARA_FCP_STRIDE;

			/* check current address of reference */
			if (fcp->ba_ref_cur_y == 0 ||
			    fcp->ba_ref_cur_c == 0 ||
			    (fcp->ba_ref_cur_y & 0x3fff) ||
			    (fcp->ba_ref_cur_c & 0x3fff))
				return E_FDP_PARA_BA_REF;

			/* get previous address of reference */
			obj->fcp_ref_addr_y0 = fcp->ba_ref_prev_y;

			/* get next address of reference */
			obj->fcp_ref_addr_y2 = fcp->ba_ref_next_y;

			/* check source format */
			if (fproc_par->in_pic->chroma_format != FDP_YUV420 &&
			    fproc_par->in_pic->chroma_format != FDP_YUV420_NV21)
				return E_FDP_PARA_CHROMA;

			buf = fproc_par->ref_buf->cur_buf;
			/* check source stride */
			if (buf->stride != fcp_stride)
				return E_FDP_PARA_SRC_STRIDE;

			if (buf->stride_c != fcp_stride)
				return E_FDP_PARA_SRC_STRIDE_C;

			obj->ipc_lmem = FD1_IPC_LMEM_PNUM_FCP;
		} else if (fcp->tlen == FCP_TL_DISABLE) {
			/* disable TL conversion */
			obj->ipc_lmem = FD1_IPC_LMEM_PNUM;
		} else {
			return E_FDP_PARA_TLEN;
		}

		/* check FCL decompress parameter */
		/* check current address of ancillary */
		if ((fcp->ba_anc_cur_y & 0x7f) ||
		    (fcp->ba_anc_cur_c & 0x7f))
			return E_FDP_PARA_BA_ANC;

		/* get previous address of ancillary */
		obj->fcp_anc_addr_y0 = fcp->ba_anc_prev_y;

		/* get next address of ancillary */
		obj->fcp_anc_addr_y2 = fcp->ba_anc_next_y;
	} else {
		/* disable FCNL compress */
		obj->wpf_swap = FD1_WPF_SWAP_SSWAP | FD1_WPF_SWAP_OSWAP;

		/* disable TL conversion */
		obj->ipc_lmem = FD1_IPC_LMEM_PNUM;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_check_aux_buffer_param
 * Description:	Check auxiliary buffer parameter.
 * Returns:		0/E_FDP_PARA_BUFREFRD0/E_FDP_PARA_BUFREFRD2
 *	E_FDP_PARA_BA_REF/E_FDP_PARA_BA_ANC
 ******************************************************************************/
static long fdp_ins_check_aux_buffer_param(
	struct fdp_obj_t *obj,
	struct fdp_fproc_t *fproc_par,
	struct fdp_seq_t *seq_par)
{
	struct fcp_info_t *fcp = fproc_par->fcp_par;

	if (seq_par->seq_mode == FDP_SEQ_INTER) {
		if (seq_par->telecine_mode == FDP_TC_OFF) {
			if (obj->rpf0_addr_y == 0 ||
			    obj->rpf2_addr_y == 0) {
				/* reject previous and next field buffer */
				obj->ctrl_chact &=
					~(FD1_CTL_CHACT_PRE_READ |
					  FD1_CTL_CHACT_NEX_READ);

				/* change fixed 2D de-interlacing */
				obj->ipc_mode =
					FD1_IPC_MODE_DLI |
					FD1_IPC_MODE_DIM_FIXED_2D;
			}
		} else if (seq_par->telecine_mode == FDP_TC_FORCED_PULL_DOWN) {
			if (obj->rpf0_addr_y == 0 &&
			    obj->rpf2_addr_y != 0) {
				/* copy address */
				obj->rpf0_addr_y = obj->rpf2_addr_y;
				obj->fcp_ref_addr_y0 = obj->fcp_ref_addr_y2;
				obj->fcp_anc_addr_y0 = obj->fcp_anc_addr_y2;
			} else if (
				(obj->rpf0_addr_y != 0) &&
				(obj->rpf2_addr_y == 0)) {
				/* copy address */
				obj->rpf2_addr_y = obj->rpf0_addr_y;
				obj->fcp_ref_addr_y2 = obj->fcp_ref_addr_y0;
				obj->fcp_anc_addr_y2 = obj->fcp_anc_addr_y0;
			}
		}

		/* check previous field buffer */
		if (obj->ctrl_chact & FD1_CTL_CHACT_PRE_READ) {
			/* check source address */
			if (obj->rpf0_addr_y == 0)
				return E_FDP_PARA_BUFREFRD2;

			if (fcp) {
				if (fcp->tlen == FCP_TL_ENABLE) {
					/* check reference base address */
					if (obj->fcp_ref_addr_y0 == 0 ||
					    (obj->fcp_ref_addr_y0 & 0x3fff))
						return E_FDP_PARA_BA_REF;
				}

				/* check ancillary address */
				if (obj->fcp_anc_addr_y0 & 0x7f)
					return E_FDP_PARA_BA_ANC;
			}
		}

		/* check next field buffer */
		if (obj->ctrl_chact & FD1_CTL_CHACT_NEX_READ) {
			/* check source address */
			if (obj->rpf2_addr_y == 0)
				return E_FDP_PARA_BUFREFRD0;

			if (fcp) {
				if (fcp->tlen == FCP_TL_ENABLE) {
					/* check reference base address */
					if (obj->fcp_ref_addr_y2 == 0 ||
					    (obj->fcp_ref_addr_y2 & 0x3fff))
						return E_FDP_PARA_BA_REF;
				}

				/* check ancillary address */
				if (obj->fcp_anc_addr_y2 & 0x7f)
					return E_FDP_PARA_BA_ANC;
			}
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_set_chact
 * Description:	Set channel activation register value.
 *vReturns:		void
 ******************************************************************************/
static void fdp_ins_set_chact(
	struct fdp_obj_t *obj,
	struct fdp_fproc_t *fproc_par,
	struct fdp_seq_t *seq_par,
	unsigned char seq_cnt)
{
	/* set default */
	obj->ctrl_chact = FD1_CTL_CHACT_WRITE | FD1_CTL_CHACT_CUR_READ;
	obj->ipc_mode = FD1_IPC_MODE_DLI;

	if (seq_par->seq_mode == FDP_SEQ_INTER) {
		if (seq_par->telecine_mode == FDP_TC_FORCED_PULL_DOWN) {
			if (fproc_par->current_field !=
					fproc_par->in_pic->top_field_first) {
				/* add next field read */
				obj->ctrl_chact |=
					FD1_CTL_CHACT_NEX_READ;
				obj->ipc_mode |=
					FD1_IPC_MODE_DIM_NEXT_FIELD;
			} else {	/* FDP_CF_BOTTOM */
				/* add previous field read */
				obj->ctrl_chact |=
					FD1_CTL_CHACT_PRE_READ;
				obj->ipc_mode |=
					FD1_IPC_MODE_DIM_PREVIOUS_FIELD;
			}
		} else if (seq_par->telecine_mode ==
				FDP_TC_INTERPOLATED_LINE) {
			if (fproc_par->interpolated_line == FDP_DIM_NEXT) {
				/* add next field read */
				obj->ctrl_chact |=
					FD1_CTL_CHACT_NEX_READ;
				obj->ipc_mode |=
					FD1_IPC_MODE_DIM_NEXT_FIELD;
			} else {	/* FDP_DIM_PREV */
				/* add previous field read */
				obj->ctrl_chact |=
					FD1_CTL_CHACT_PRE_READ;
				obj->ipc_mode |=
					FD1_IPC_MODE_DIM_PREVIOUS_FIELD;
			}
		} else {	/* FDP_TC_OFF */
			if (seq_cnt == 0 || seq_cnt == 1) {
				obj->ctrl_chact |=
					FD1_CTL_CHACT_SMSK_WRITE |
					FD1_CTL_CHACT_PRE_READ |
					FD1_CTL_CHACT_NEX_READ;
			} else {
				obj->ctrl_chact |=
					FD1_CTL_CHACT_SMSK_WRITE |
					FD1_CTL_CHACT_SMSK_READ |
					FD1_CTL_CHACT_PRE_READ |
					FD1_CTL_CHACT_NEX_READ;
			}
			/* Adaptive 2D/3D de-interlacing */
			obj->ipc_mode |=
				FD1_IPC_MODE_DIM_ADAPTIVE_2D_3D;
		}
	} else {	/* FDP_SEQ_PROG or FDP_SEQ_INTER_2D */
		/* Fixed 2D de-interlacing */
		obj->ipc_mode |= FD1_IPC_MODE_DIM_FIXED_2D;
	}
}

/******************************************************************************
 * Function:		fdp_ins_check_fproc_param
 * Description:	Check processing parameter.
 * Returns:		0/E_FDP_PARA_FPROCPAR/E_FDP_PARA_SEQPAR
 *	E_FDP_PARA_STLMSK_ADDR/E_FDP_PARA_LASTSTART/E_FDP_PARA_CF
 *	E_FDP_PARA_OUTFORMAT
 *	return of fdp_ins_check_seq_param()
 *	return of fdp_ins_check_pic_param()
 *	return of fdp_ins_check_pic_param_for_pulldown()
 *	return of fdp_ins_check_dstbuf_param()
 *	return of fdp_ins_check_refbuf_param()
 *	return of fdp_ins_check_fcp_param()
 *	return of fdp_ins_check_aux_buffer_param()
 ******************************************************************************/
static long fdp_ins_check_fproc_param(
	struct fdp_obj_t *obj, struct fdp_fproc_t *fproc_par)
{
	struct vspm_fdp_proc_info *proc_info = obj->proc_info;
	struct fdp_seq_t *seq_par;
	unsigned char seq_cnt;
	long ercd;

	if (!fproc_par)
		return E_FDP_PARA_FPROCPAR;

	/* select sequence parameter */
	if (fproc_par->seq_par) {
		/* set current sequence */
		seq_par = fproc_par->seq_par;

		/* set sequence counter */
		seq_cnt = 0;	/* 1st seq */
	} else {
		if (proc_info->set_prev_flag == 0)
			return E_FDP_PARA_SEQPAR;

		/* set previous sequence */
		seq_par = &proc_info->seq_par;

		/* set sequence counter */
		if (proc_info->seq_cnt == 0)
			seq_cnt = 1;	/* 2nd seq. */
		else
			seq_cnt = 2;	/* 3rd or more seq */
	}

	/* check sequence parameter */
	ercd = fdp_ins_check_seq_param(obj, fproc_par, seq_par);
	if (ercd)
		return ercd;

	/* check picture information parameter */
	ercd = fdp_ins_check_pic_param(obj, fproc_par->in_pic);
	if (ercd)
		return ercd;

	if (seq_par->telecine_mode == FDP_TC_FORCED_PULL_DOWN) {
		ercd = fdp_ins_check_pic_param_for_pulldown(
			fproc_par->in_pic, seq_par);
		if (ercd)
			return ercd;
	}

	/* check last sequence indicator parameter */
	if (fproc_par->last_seq_indicator != 0 &&
	    fproc_par->last_seq_indicator != 1)
		return E_FDP_PARA_LASTSTART;

	fdp_ins_set_chact(obj, fproc_par, seq_par, seq_cnt);

	/* check still mask address parameter */
	if ((obj->ctrl_chact & FD1_CTL_CHACT_SMSK_WRITE) ||
	    (obj->ctrl_chact & FD1_CTL_CHACT_SMSK_READ)) {
		if (proc_info->stlmsk_addr[0] == 0 ||
		    proc_info->stlmsk_addr[1] == 0)
			return E_FDP_PARA_STLMSK_ADDR;
	}

	/* check destination picture format parameter */
	switch (fproc_par->out_format) {
	case FDP_YUV420:
		obj->wpf_format = FD1_WPF_FORMAT_YUV420_NV12;
		break;
	case FDP_YUV420_PLANAR:	/* supports FCNL compression */
		obj->wpf_format = FD1_WPF_FORMAT_YUV420_YV12;
		break;
	case FDP_YUV420_NV21:
		obj->wpf_format = FD1_WPF_FORMAT_YUV420_NV21;
		break;
	case FDP_YUV422_NV16:
		obj->wpf_format = FD1_WPF_FORMAT_YUV422_NV16;
		break;
	case FDP_YUV422_YUY2:
		obj->wpf_format = FD1_WPF_FORMAT_YUV422_YUY2;
		break;
	case FDP_YUV422_UYVY:
		obj->wpf_format = FD1_WPF_FORMAT_YUV422_UYVY;
		break;
	case FDP_YUV422_PLANAR:	/* supports FCNL compression */
		obj->wpf_format = FD1_RPF_FORMAT_YUV422_YV16;
		break;
	case FDP_YUV444_PLANAR:	/* supports FCNL compression */
		obj->wpf_format = FD1_RPF_FORMAT_YUV444_PLANAR;
		break;
	default:
		return E_FDP_PARA_OUTFORMAT;
	}

	/* check destination buffer parameter */
	ercd = fdp_ins_check_dstbuf_param(fproc_par, seq_par);
	if (ercd)
		return ercd;

	/* check reference buffer parameter */
	ercd = fdp_ins_check_refbuf_param(obj, fproc_par, seq_par);
	if (ercd)
		return ercd;

	/* check frame compression parameter */
	ercd = fdp_ins_check_fcp_param(obj, fproc_par);
	if (ercd)
		return ercd;

	/* check auxiliary buffer address parameter */
	ercd = fdp_ins_check_aux_buffer_param(obj, fproc_par, seq_par);
	if (ercd)
		return ercd;

	/* update sequence counter */
	proc_info->seq_cnt = seq_cnt;

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_update_proc_info
 * Description:	Update processing information.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_update_proc_info(
	struct fdp_obj_t *obj, struct fdp_fproc_t *fproc_par)
{
	struct vspm_fdp_proc_info *proc_info = obj->proc_info;

	if (fproc_par->seq_par) {
		/* copy current sequence parameter */
		memcpy(
			&proc_info->seq_par,
			fproc_par->seq_par,
			sizeof(struct fdp_seq_t));
		proc_info->set_prev_flag = 1;
	}

	/* set picture ID */
	proc_info->status.picid = fproc_par->in_pic->picid;
}

/******************************************************************************
 * Function:		fdp_ins_check_start_param
 * Description:	Check fdp_start_t parameter.
 * Returns:		0/E_FDP_PARA_STARTPAR/E_FDP_PARA_FDPGO
 *	return of fdp_ins_check_fproc_param()
 ******************************************************************************/
long fdp_ins_check_start_param(
	struct fdp_obj_t *obj, struct fdp_start_t *start_par)
{
	long ercd;

	/* check pointer */
	if (!start_par)
		return E_FDP_PARA_STARTPAR;

	/* check frame processing request */
	if (start_par->fdpgo == FDP_GO) {
		/* check frame processing parameter */
		ercd = fdp_ins_check_fproc_param(obj, start_par->fproc_par);
		if (ercd)
			return ercd;

		/* update processing information */
		fdp_ins_update_proc_info(obj, start_par->fproc_par);
	} else if (start_par->fdpgo == FDP_NOGO) {
		/* no check */
	} else {
		return E_FDP_PARA_FDPGO;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_set_ctrl_reg
 * Description:	Set control register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_set_ctrl_reg(struct fdp_obj_t *obj)
{
	/* operation mode register */
	fdp_write_reg(obj->ctrl_mode, P_FDP, FD1_CTL_OPMODE);

	/* chennel activation register */
	fdp_write_reg(obj->ctrl_chact, P_FDP, FD1_CTL_CHACT);
}

/******************************************************************************
 * Function:		fdp_ins_set_rpf_reg
 * Description:	Set RPF register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_set_rpf_reg(struct fdp_obj_t *obj, struct fdp_refbuf_t *ref)
{
	struct vspm_fdp_proc_info *proc_info = obj->proc_info;
	struct fdp_seq_t *seq_par =
		&proc_info->seq_par; /* use private parameter */
	struct fdp_imgbuf_t *cur = ref->cur_buf;

	unsigned int reg_data;

	/* picture size register */
	reg_data =
		((((unsigned int)(seq_par->in_width)) << 16) |
		  ((unsigned int)(seq_par->in_height)));
	fdp_write_reg(reg_data, P_FDP, FD1_RPF_SIZE);

	/* picture format register */
	fdp_write_reg(obj->rpf_format, P_FDP, FD1_RPF_FORMAT);

	/* picture stride register */
	reg_data =
		((((unsigned int)(cur->stride)) << 16) |
		  ((unsigned int)(cur->stride_c)));
	fdp_write_reg(reg_data, P_FDP, FD1_RPF_PSTRIDE);

	/* RPF0 source component-Y address register */
	fdp_write_reg(obj->rpf0_addr_y, P_FDP, FD1_RPF0_ADDR_Y);

	/* RPF1 source component-Y address register */
	fdp_write_reg(cur->addr, P_FDP, FD1_RPF1_ADDR_Y);

	/* RPF1 source component-C0 address register */
	fdp_write_reg(cur->addr_c0, P_FDP, FD1_RPF1_ADDR_C0);

	/* RPF1 source component-C1 address register */
	fdp_write_reg(cur->addr_c1, P_FDP, FD1_RPF1_ADDR_C1);

	/* RPF2 source component-Y address register */
	fdp_write_reg(obj->rpf2_addr_y, P_FDP, FD1_RPF2_ADDR_Y);

	/* still mask address register */
	reg_data = proc_info->stlmsk_addr[proc_info->stlmsk_idx];
	fdp_write_reg(reg_data, P_FDP, FD1_RPF_SMSK_ADDR);

	/* data swap register */
	fdp_write_reg(FD1_RPF_SWAP_ISWAP, P_FDP, FD1_RPF_SWAP);
}

/******************************************************************************
 * Function:		fdp_ins_set_wpf_reg
 * Description:	Set WPF register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_set_wpf_reg(struct fdp_obj_t *obj, struct fdp_imgbuf_t *buf)
{
	unsigned int reg_data;

	/* picture format register */
	fdp_write_reg(obj->wpf_format, P_FDP, FD1_WPF_FORMAT);

	/* picture stride register */
	reg_data =
		((((unsigned int)(buf->stride)) << 16) |
		  ((unsigned int)(buf->stride_c)));
	fdp_write_reg(reg_data, P_FDP, FD1_WPF_PSTRIDE);

	/* component-Y address register */
	fdp_write_reg(buf->addr, P_FDP, FD1_WPF_ADDR_Y);

	/* component-C0 address register */
	fdp_write_reg(buf->addr_c0, P_FDP, FD1_WPF_ADDR_C0);

	/* component-C1 address register */
	fdp_write_reg(buf->addr_c1, P_FDP, FD1_WPF_ADDR_C1);

	/* data swap register */
	fdp_write_reg(obj->wpf_swap, P_FDP, FD1_WPF_SWAP);
}

/******************************************************************************
 * Function:		fdp_ins_set_ipc_reg
 * Description:	Set IPC register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_set_ipc_reg(
	struct fdp_obj_t *obj, struct fdp_ipc_t *ipc)
{
	struct fdp_seq_t *seq_par =
		&obj->proc_info->seq_par; /* use private parameter */
	unsigned int reg_data;

	/* IPC mode register */
	fdp_write_reg(obj->ipc_mode, P_FDP, FD1_IPC_MODE);

	/* Comb detection parameter register */
	if (seq_par->seq_mode == FDP_SEQ_INTER && ipc) {
		reg_data =
			(((unsigned int)ipc->cmb_ofst) << 16) |
			(((unsigned int)ipc->cmb_max) << 8) |
			((unsigned int)ipc->cmb_gard);
	} else {
		/* default setting */
		reg_data =
			FD1_IPC_COMB_DET_CMB_OFST |
			FD1_IPC_COMB_DET_CMB_MAX |
			FD1_IPC_COMB_DET_CMB_GRAD;
	}
	fdp_write_reg(reg_data, P_FDP, FD1_IPC_COMB_DET);

	/* sensor control register0 */
	fdp_write_reg(
		FD1_SENSOR_CTL0_FRM_LVTH |
		FD1_SENSOR_CTL0_FLD_LVTH |
		FD1_SENSOR_CTL0_FD_EN,
		P_FDP,
		FD1_SENSOR_CTL0);

	/* sensor control register1 */
	fdp_write_reg(
		FD1_SENSOR_CTL1_XS |
		FD1_SENSOR_CTL1_YS,
		P_FDP,
		FD1_SENSOR_CTL1);

	/* sensor control register2 */
	reg_data = ((unsigned int)(seq_par->in_width - 1)) << 16;
	if (seq_par->seq_mode == FDP_SEQ_PROG) {
		/* progressive mode */
		reg_data |= (unsigned int)(seq_par->in_height - 1);
	} else {
		/* interlace mode */
		reg_data |= (unsigned int)(seq_par->in_height * 2 - 1);
	}
	fdp_write_reg(reg_data, P_FDP, FD1_SENSOR_CTL2);

	/* sensor control register3 */
	reg_data = ((unsigned int)(seq_par->in_width / 3)) << 16;
	reg_data |= (unsigned int)(2 * seq_par->in_width / 3);
	fdp_write_reg(reg_data, P_FDP, FD1_SENSOR_CTL3);

	/* line memory pixel number register */
	fdp_write_reg(obj->ipc_lmem, P_FDP, FD1_IPC_LMEM);
}

/******************************************************************************
 * Function:		fdp_ins_set_fcp_reg
 * Description:	Set FCP register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_set_fcp_reg(
	struct fdp_obj_t *obj, struct fcp_info_t *fcp)
{
	unsigned int reg_data;

	if (fcp) {
		/* TL conversion control register */
		if (fcp->tlen == FCP_TL_ENABLE) {
			/* ECR1417 and ECR1469 */
			reg_data = fdp_read_reg(P_FCP, FD1_FCP_VCR);
			if (reg_data == 0x00000101) {
				/* RIIF, RSIF and DCMP reset */
				fdp_write_reg(
					FD1_FCP_RST_RIIF |
					FD1_FCP_RST_RSIF |
					FD1_FCP_RST_DCMP,
					P_FCP, FD1_FCP_RST);
			}

			reg_data =
				FD1_FCP_TL_CTRL_TLEN |
				(((unsigned int)fcp->pos_c) << 16) |
				((unsigned int)fcp->pos_y);
		} else {
			reg_data = 0;
		}
		fdp_write_reg(reg_data, P_FCP, FD1_FCP_TL_CTRL);

		/* picture information register */
		reg_data = (unsigned int)fcp->stride_div16;
		fdp_write_reg(reg_data, P_FCP, FD1_FCP_PICINFO1);

		/* Base address of reference information register */
		fdp_write_reg(
			obj->fcp_ref_addr_y0, P_FCP, FD1_BA_REF_Y0);
		fdp_write_reg(
			fcp->ba_ref_cur_y, P_FCP, FD1_BA_REF_Y1);
		fdp_write_reg(
			obj->fcp_ref_addr_y2, P_FCP, FD1_BA_REF_Y2);
		fdp_write_reg(
			fcp->ba_ref_cur_c, P_FCP, FD1_BA_REF_C);

		/* Base address of ancillary information register */
		fdp_write_reg(
			obj->fcp_anc_addr_y0, P_FCP, FD1_BA_ANC_Y0);
		fdp_write_reg(
			fcp->ba_anc_cur_y, P_FCP, FD1_BA_ANC_Y1);
		fdp_write_reg(
			obj->fcp_anc_addr_y2, P_FCP, FD1_BA_ANC_Y2);
		fdp_write_reg(
			fcp->ba_anc_cur_c, P_FCP, FD1_BA_ANC_C);
	} else {
		/* TL conversion control register */
		fdp_write_reg(0, P_FCP, FD1_FCP_TL_CTRL);

		/* Base address of ancillary information register */
		fdp_write_reg(0, P_FCP, FD1_BA_ANC_Y0);
		fdp_write_reg(0, P_FCP, FD1_BA_ANC_Y1);
		fdp_write_reg(0, P_FCP, FD1_BA_ANC_Y2);
		fdp_write_reg(0, P_FCP, FD1_BA_ANC_C);
	}
}

/******************************************************************************
 * Function:		fdp_ins_start_processing
 * Description:	Start FDP processing.
 * Returns:		void
 ******************************************************************************/
void fdp_ins_start_processing(
	struct fdp_obj_t *obj, struct fdp_start_t *start_par)
{
	/* clear interrupt status */
	fdp_write_reg(0, P_FDP, FD1_CTL_IRQSTA);

	/* enable interrupt */
	fdp_write_reg(FD1_CTL_IRQ_FRAME_END, P_FDP, FD1_CTL_IRQENB);

	if (start_par->fdpgo == FDP_GO) {
		/* start set register sequence */
		fdp_rewrite_reg(
			0x1, ~FD1_CTL_CMD_STRCMD_MSK, P_FDP, FD1_CTL_CMD);

		/* set control register */
		fdp_ins_set_ctrl_reg(obj);

		/* set RPF register */
		fdp_ins_set_rpf_reg(obj, start_par->fproc_par->ref_buf);

		/* set WPF register */
		fdp_ins_set_wpf_reg(obj, start_par->fproc_par->out_buf);

		/* set IPC register */
		fdp_ins_set_ipc_reg(obj, start_par->fproc_par->ipc_par);

		/* set FCPF register */
		fdp_ins_set_fcp_reg(obj, start_par->fproc_par->fcp_par);

		/* set register finished */
		fdp_write_reg(0x1, P_FDP, FD1_CTL_REGEND);

		/* trigger to execute FDP process */
		fdp_write_reg(0x1, P_FDP, FD1_CTL_SGCMD);

		/* end set register sequence */
		fdp_rewrite_reg(
			0x0, ~FD1_CTL_CMD_STRCMD_MSK, P_FDP, FD1_CTL_CMD);

		/* negate trigger */
		fdp_write_reg(0x0, P_FDP, FD1_CTL_SGCMD);
	} else {	/* FDP_NOGO */
		/* set force interrupt notify */
		fdp_write_reg(
			FD1_CTL_IRQ_FRAME_END, P_FDP, FD1_CTL_IRQFSET);
	}
}

/******************************************************************************
 * Function:		fdp_ins_stop_processing
 * Description:	Abort FDP processing.
 * Returns:		void
 ******************************************************************************/
void fdp_ins_stop_processing(struct fdp_obj_t *obj)
{
	unsigned int status;
	unsigned int loop_cnt;

	/* disable interrupt */
	fdp_write_reg(0, P_FDP, FD1_CTL_IRQENB);

	/* clear interrupt status */
	fdp_write_reg(0, P_FDP, FD1_CTL_IRQSTA);

	/* dummy read */
	fdp_read_reg(P_FDP, FD1_CTL_IRQSTA);
	fdp_read_reg(P_FDP, FD1_CTL_IRQSTA);

	/* init loop counter */
	loop_cnt = FDP_STATUS_LOOP_CNT;

	/* read status register of FCP */
	status = fdp_read_reg(P_FCP, FD1_FCP_STA);

	if (status & FD1_FCP_STA_ACT) {
		/* reset */
		fdp_write_reg(FD1_FCP_RST_SOFTRST, P_FCP, FD1_FCP_RST);

		/* waiting reset process */
		do {
			/* sleep */
			msleep(FDP_STATUS_LOOP_TIME);

			/* read status register of FCP */
			status = fdp_read_reg(P_FCP, FD1_FCP_STA);
		} while ((status & FD1_FCP_STA_ACT) &&
			(--loop_cnt > 0));

		if (loop_cnt == 0) {
			APRINT("%s: happen to timeout after reset of FCP!!\n",
			       __func__);
		}
	}

	/* init loop counter */
	loop_cnt = FDP_STATUS_LOOP_CNT;

	/* read status register of FDP */
	status = fdp_read_reg(P_FDP, FD1_CTL_STATUS);

	if (status & FD1_CTL_STATUS_BSY) {
		/* software reset */
		fdp_write_reg(FD1_CTL_SRESET_SRST, P_FDP, FD1_CTL_SRESET);

		/* waiting reset process */
		do {
			/* sleep */
			msleep(FDP_STATUS_LOOP_TIME);

			/* read status register of FDP */
			status = fdp_read_reg(P_FDP, FD1_CTL_STATUS);
		} while ((status & FD1_CTL_STATUS_BSY) &&
			(--loop_cnt > 0));

		if (loop_cnt == 0) {
			APRINT("%s: happen to timeout after reset of FDP!!\n",
			       __func__);
		}
	}

	/* disable callback function */
	obj->cb_info.fdp_cb2 = NULL;

	/* update status */
	obj->status = FDP_STAT_READY;
}

/******************************************************************************
 * Function:		fdp_ins_wait_processing
 * Description:	Waiting FDP processing.
 * Returns:		void
 ******************************************************************************/
void fdp_ins_wait_processing(struct fdp_obj_t *obj)
{
	unsigned int loop_cnt = FDP_STATUS_LOOP_CNT;
	struct fdp_cb_info_t cb_info;

	do {
		/* sleep */
		msleep(FDP_STATUS_LOOP_TIME);
	} while ((obj->status == FDP_STAT_RUN) && (--loop_cnt > 0));

	if (loop_cnt == 0) {
		APRINT("%s: happen to timeout!!\n", __func__);

		/* save callback information */
		cb_info = obj->cb_info;

		/* update status */
		obj->status = FDP_STAT_READY;

		/* execute callback2 */
		if (cb_info.fdp_cb2) {
			cb_info.fdp_cb2(
				0, R_VSPM_DRIVER_ERR, cb_info.userdata2);
		}
	}
}

/******************************************************************************
 * Function:		fdp_ins_software_reset
 * Description:	Software reset of FDP and FCP.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_software_reset(struct fdp_obj_t *obj)
{
	/* Forced stop the FDP processing */
	fdp_ins_stop_processing(obj);
}

/******************************************************************************
 * Function:		fdp_int_get_process_result
 * Description:	Get processing result
 * Returns:		void
 ******************************************************************************/
static void fdp_int_get_process_result(struct fdp_obj_t *obj)
{
	struct vspm_fdp_proc_info *proc_info = obj->proc_info;

	unsigned int *sensor;
	unsigned int i;

	/* status update */
	fdp_write_reg(0x1, P_FDP, FD1_CTL_REGEND);
	fdp_write_reg(0x1, P_FDP, FD1_CTL_SGCMD);
	fdp_write_reg(0x0, P_FDP, FD1_CTL_SGCMD);

	/* read vcycle */
	proc_info->status.vcycle =
		fdp_read_reg(P_FDP, FD1_CTL_VCYCLE_STAT);

	/* read sensor */
	sensor = &proc_info->status.sensor[0];
	for (i = FD1_SENSOR_0; i <= FD1_SENSOR_17; i += 4)
		*sensor++ = fdp_read_reg(P_FDP, i);
}

/******************************************************************************
 * Function:		fdp_int_hdr
 * Description:	Interrupt handler (sub routine)
 * Returns:		void
 ******************************************************************************/
static void fdp_int_hdr(struct fdp_obj_t *obj)
{
	struct fdp_cb_info_t cb_info;

	/* check parameter */
	if (!obj)
		return;

	if (obj->status == FDP_STAT_RUN) {
		/* get processing result */
		fdp_int_get_process_result(obj);

		/* save callback information */
		cb_info = obj->cb_info;

		/* update status */
		obj->status = FDP_STAT_READY;

		/* execute callback2 */
		if (cb_info.fdp_cb2) {
			cb_info.fdp_cb2(
				0, R_VSPM_OK, cb_info.userdata2);
		}
	}
}

/******************************************************************************
 * Function:		fdp_ins_init_cmn_reg
 * Description:	Initialize common register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_init_cmn_reg(struct fdp_obj_t *obj)
{
	/* operation mode register */
	fdp_rewrite_reg(
		FD1_CTL_OPMODE_NO_INTERRUPT,
		~FD1_CTL_OPMODE_VIMD_MSK,
		P_FDP,
		FD1_CTL_OPMODE);

	/* clock control register */
	fdp_write_reg(FD1_CTL_CLK_CTRL_CSTP_N, P_FDP, FD1_CTL_CLKCTRL);
}

/******************************************************************************
 * Function:		fdp_ins_init_ipc_reg
 * Description:	Initialize IPC register.
 * Returns:		void
 ******************************************************************************/
static void fdp_ins_init_ipc_reg(struct fdp_obj_t *obj)
{
	/* Still mask threshold register */
	fdp_write_reg(
		FD1_IPC_SMSK_THRESH_FSM0 |
		FD1_IPC_SMSK_THRESH_SMSK_TH,
		P_FDP,
		FD1_IPC_SMSK_THRESH);

	/* Comb detection parameter register */
	fdp_write_reg(
		FD1_IPC_COMB_DET_CMB_OFST |
		FD1_IPC_COMB_DET_CMB_MAX |
		FD1_IPC_COMB_DET_CMB_GRAD,
		P_FDP,
		FD1_IPC_COMB_DET);

	/* Motion decision parameter register */
	fdp_write_reg(
		FD1_IPC_MOTDEC_MOV_COEF |
		FD1_IPC_MOTDEC_STL_COEF,
		P_FDP,
		FD1_IPC_MOTDEC);

	/* DLI blend parameter register */
	fdp_write_reg(
		FD1_IPC_DLI_BLEND_BLD_GRAD |
		FD1_IPC_DLI_BLEND_BLD_MAX |
		FD1_IPC_DLI_BLEND_BLD_OFST,
		P_FDP,
		FD1_IPC_DLI_BLEND);

	/* DLI horizontal frequency gain register */
	fdp_write_reg(
		FD1_IPC_DLI_HGAIN_HG_GRAD |
		FD1_IPC_DLI_HGAIN_HG_OFST |
		FD1_IPC_DLI_HGAIN_HG_MAX,
		P_FDP,
		FD1_IPC_DLI_HGAIN);

	/* DLI suppression parameter register */
	fdp_write_reg(
		FD1_IPC_DLI_SPRS_SPRS_GRAD |
		FD1_IPC_DLI_SPRS_SPRS_OFST |
		FD1_IPC_DLI_SPRS_SPRS_MAX,
		P_FDP,
		FD1_IPC_DLI_SPRS);

	/* DLI angle parameter register */
	fdp_write_reg(
		FD1_IPC_DLI_ANGLE_ASEL45 |
		FD1_IPC_DLI_ANGLE_ASEL22 |
		FD1_IPC_DLI_ANGLE_ASEL15,
		P_FDP,
		FD1_IPC_DLI_ANGLE);

	/* DLI isolated pixel parameter register */
	fdp_write_reg(
		FD1_IPC_DLI_ISOPIX_MAX45 |
		FD1_IPC_DLI_ISOPIX_GRAD45 |
		FD1_IPC_DLI_ISOPIX_MAX22 |
		FD1_IPC_DLI_ISOPIX_GRAD22,
		P_FDP,
		FD1_IPC_DLI_ISOPIX0);
	fdp_write_reg(
		FD1_IPC_DLI_ISOPIX_MAX15 |
		FD1_IPC_DLI_ISOPIX_GRAD15,
		P_FDP,
		FD1_IPC_DLI_ISOPIX1);

	/* IPC sensor threshold register */
	fdp_write_reg(
		FD1_IPC_SENSOR_VFQ_TH |
		FD1_IPC_SENSOR_HFQ_TH |
		FD1_IPC_SENSOR_DIF_TH |
		FD1_IPC_SENSOR_SAD_TH,
		P_FDP,
		FD1_IPC_SENSOR_TH0);
	fdp_write_reg(
		FD1_IPC_SENSOR_DETECTOR_SEL |
		FD1_IPC_SENSOR_COMB_TH |
		FD1_IPC_SENSOR_FREQ_TH,
		P_FDP,
		FD1_IPC_SENSOR_TH1);

	/* look-up table */
	fdp_write_reg_lut_tbl(
		fdp_tbl_dif_adj[obj->lut_tbl_idx],
		P_FDP,
		FD1_DIF_ADJ_OFFSET);
	fdp_write_reg_lut_tbl(
		fdp_tbl_sad_adj[obj->lut_tbl_idx],
		P_FDP,
		FD1_SAD_ADJ_OFFSET);
	fdp_write_reg_lut_tbl(
		fdp_tbl_bld_gain[obj->lut_tbl_idx],
		P_FDP,
		FD1_BLD_GAIN_OFFSET);
	fdp_write_reg_lut_tbl(
		fdp_tbl_dif_gain[obj->lut_tbl_idx],
		P_FDP,
		FD1_DIF_GAIN_OFFSET);
	fdp_write_reg_lut_tbl(
		fdp_tbl_mdet[obj->lut_tbl_idx],
		P_FDP,
		FD1_MDET_OFFSET);
}

/******************************************************************************
 * Function:		fdp_ins_get_resource
 * Description:	Get FDP resource.
 * Returns:		0/E_FDP_INVALID_PARAM
 ******************************************************************************/
long fdp_ins_get_resource(struct fdp_obj_t *obj)
{
	struct device_node *np = obj->pdev->dev.of_node;

	/* read LUT table index */
	of_property_read_u32(
		np,
		"renesas,#lut_table_index",
		&obj->lut_tbl_idx);
	if (obj->lut_tbl_idx >= FDP_LUT_TBL_MAX)
		return E_FDP_INVALID_PARAM;

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_init_reg
 * Description:	Initialize FDP register
 * Returns:		0/E_FDP_DEF_REG
 ******************************************************************************/
long fdp_ins_init_reg(struct fdp_obj_t *obj)
{
	struct platform_device *pdev = obj->pdev;
	struct resource *res;

	/* get an I/O memory resource of FDP */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		EPRINT("%s: failed to get resource of FDP!!\n", __func__);
		return E_FDP_DEF_REG;
	}

	/* remap an I/O memory of FDP */
	obj->fdp_reg = ioremap(res->start, resource_size(res));
	if (!obj->fdp_reg) {
		EPRINT("%s: failed to ioremap of FDP!!\n", __func__);
		return E_FDP_DEF_REG;
	}

	/* get an I/O memory resource for FCP */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		EPRINT("%s: failed to get resource of FCP!!\n", __func__);
		iounmap(obj->fdp_reg);
		return E_FDP_DEF_REG;
	}

	/* remap and I/O memory of FCP */
	obj->fcp_reg = ioremap(res->start, resource_size(res));
	if (!obj->fcp_reg) {
		EPRINT("%s: failed to ioremap of FCP!!\n", __func__);
		iounmap(obj->fdp_reg);
		return E_FDP_DEF_REG;
	}

	/* software reset */
	(void)fdp_ins_software_reset(obj);

	/* initialize FDP register */
	fdp_ins_init_cmn_reg(obj);
	fdp_ins_init_ipc_reg(obj);

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_quit_reg
 * Description:	Finalize FDP register
 * Returns:		0
 ******************************************************************************/
long fdp_ins_quit_reg(struct fdp_obj_t *obj)
{
	/* unmap an I/O register of FCP */
	if (obj->fcp_reg) {
		iounmap(obj->fcp_reg);
		obj->fcp_reg = NULL;
	}

	/* unmap an I/O register of FDP */
	if (obj->fdp_reg) {
		iounmap(obj->fdp_reg);
		obj->fdp_reg = NULL;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_ih
 * Description:	Interrupt handler
 * Returns:		IRQ_HANDLED
 ******************************************************************************/
static irqreturn_t fdp_ins_ih(int irq, void *dev)
{
	struct fdp_obj_t *obj = (struct fdp_obj_t *)dev;

	unsigned int status;

	/* interrupt status read */
	status = fdp_read_reg(P_FDP, FD1_CTL_IRQSTA);

	/* FDP end status */
	if (status & FD1_CTL_IRQ_FRAME_END) {
		/* disable interrupt */
		fdp_write_reg(0, P_FDP, FD1_CTL_IRQENB);

		/* clear interrupt status */
		fdp_write_reg(0, P_FDP, FD1_CTL_IRQSTA);

		fdp_int_hdr(obj);
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * Function:		fdp_reg_inth
 * Description:	Registry interrupt handler
 * Returns:		0/E_FDP_DEF_INH
 ******************************************************************************/
long fdp_reg_inth(struct fdp_obj_t *obj)
{
	struct platform_device *pdev = obj->pdev;
	int ercd;

	/* get irq information from platform */
	obj->irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!obj->irq) {
		EPRINT("%s: failed to get IRQ resource!!\n", __func__);
		return E_FDP_DEF_INH;
	}

	/* registory interrupt handler */
	ercd = request_irq(
		obj->irq->start,
		fdp_ins_ih,
		IRQF_SHARED,
		dev_name(&pdev->dev),
		obj);
	if (ercd) {
		EPRINT("%s: failed to request irq!! ercd=%d, irq=%d\n",
		       __func__, ercd, (int)obj->irq->start);
		return E_FDP_DEF_INH;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_free_inth
 * Description:	Unregistry interrupt handler
 * Returns:		0
 ******************************************************************************/
long fdp_free_inth(struct fdp_obj_t *obj)
{
	/* registory interrupt handler */
	if (obj->irq) {
		free_irq(obj->irq->start, obj);
		obj->irq = NULL;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_enable_clock
 * Description:	Enable clock supply
 * Returns:		0/E_FDP_NO_CLK
 ******************************************************************************/
long fdp_ins_enable_clock(struct fdp_obj_t *obj)
{
	struct platform_device *pdev = obj->pdev;
	struct device *dev = &pdev->dev;

	int ercd;

	/* wake up device */
	ercd = pm_runtime_get_sync(dev);
	if (ercd < 0) {
		EPRINT("%s: failed to pm_runtime_get_sync!! ercd=%d\n",
		       __func__, ercd);
		return E_FDP_NO_CLK;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_disable_clock
 * Description:	Disable clock supply
 * Returns:		0
 ******************************************************************************/
long fdp_ins_disable_clock(struct fdp_obj_t *obj)
{
	struct platform_device *pdev = obj->pdev;

	/* mark deice as idle */
	pm_runtime_put_sync(&pdev->dev);

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_allocate_memory
 * Description:	Allocate memory.
 * Returns:		0/E_FDP_NO_MEM
 ******************************************************************************/
long fdp_ins_allocate_memory(struct fdp_obj_t **obj)
{
	/* allocate memory */
	*obj = kzalloc(sizeof(struct fdp_obj_t), GFP_KERNEL);
	if (!(*obj)) {
		EPRINT("%s: failed to allocate memory!!\n", __func__);
		return E_FDP_NO_MEM;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_ins_release_memory
 * Description:	Release memory.
 * Returns:		0
 ******************************************************************************/
long fdp_ins_release_memory(struct fdp_obj_t *obj)
{
	/* release memory */
	kfree(obj);

	return 0;
}

/******************************************************************************
 * Function:		fdp_write_reg_lut_tbl
 * Description:	Write LUT table to register
 * Returns:		void
 ******************************************************************************/
void fdp_write_reg_lut_tbl(
	const unsigned char *data, void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *dst_addr;
	unsigned int src_data;

	int i;

	dst_addr = (unsigned int __iomem *)base;
	dst_addr += (offset >> 2);

	for (i = 0; i < 256; i++) {
		src_data = (unsigned int)(*data++);
		iowrite32(src_data, dst_addr);
		dst_addr++;
	}
}

/******************************************************************************
 * Function:		fdp_rewrite_reg
 * Description:	Rewrite to register
 * Returns:		void
 ******************************************************************************/
inline void fdp_rewrite_reg(
	unsigned int data,
	unsigned int mask,
	void __iomem *base,
	unsigned int offset)
{
	unsigned int __iomem *reg_addr;
	unsigned int reg_data;

	reg_addr = (unsigned int __iomem *)base;
	reg_addr += (offset >> 2);

	reg_data = ioread32(reg_addr);
	reg_data = (reg_data & mask) | data;

	iowrite32(reg_data, reg_addr);
}

/******************************************************************************
 * Function:		fdp_write_reg
 * Description:	Write to register
 * Returns:		void
 ******************************************************************************/
inline void fdp_write_reg(
	unsigned int data, void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *reg =
		(unsigned int __iomem *)base;
	reg += (offset >> 2);
	iowrite32(data, reg);
}

/******************************************************************************
 * Function:		fdp_read_reg
 * Description:	Read from register
 * Returns:		read value from register.
 ******************************************************************************/
inline unsigned int fdp_read_reg(void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *reg =
		(unsigned int __iomem *)base;
	reg += (offset >> 2);
	return ioread32(reg);
}
