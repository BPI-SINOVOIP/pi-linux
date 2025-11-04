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

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"

#include "vsp_drv_public.h"
#include "vsp_drv_local.h"

/* BRU module route table */
static const unsigned int vsp_tbl_bru_route[VSP_BRU_IN_MAX] = {
	VSP_DPR_ROUTE_BRU0,
	VSP_DPR_ROUTE_BRU1,
	VSP_DPR_ROUTE_BRU2,
	VSP_DPR_ROUTE_BRU3,
	VSP_DPR_ROUTE_BRU4,
};

/******************************************************************************
 * Function:		vsp_ins_check_init_parameter
 * Description:	Check initialize parameter
 * Returns:		0/E_VSP_PARA_INPAR
 ******************************************************************************/
long vsp_ins_check_init_parameter(struct vsp_init_t *param)
{
	/* check pointer */
	if (!param)
		return E_VSP_PARA_INPAR;

	/* check IP number */
	if (param->ip_num < 1 || param->ip_num > VSP_IP_MAX)
		return E_VSP_PARA_INPAR;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_open_parameter
 * Description:	Check open channel parameter
 * Returns:		0/E_VSP_PARA_INPAR
 ******************************************************************************/
long vsp_ins_check_open_parameter(struct vsp_open_t *param)
{
	/* check pointer */
	if (!param)
		return E_VSP_PARA_INPAR;

	/* check device parameter */
	if (!param->pdev)
		return E_VSP_PARA_INPAR;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_get_dpr_route
 * Description:	Get route value of module
 * Returns:		route value for DPR.
 ******************************************************************************/
static unsigned int vsp_ins_get_dpr_route(
	struct vsp_ch_info *ch_info, unsigned long connect, unsigned char fxa)
{
	unsigned int route;

	/* set fixed alpha output value */
	route = ((unsigned int)fxa) << 16;

	/* get target node value */
	switch (connect) {
	case 0:
		route |= VSP_DPR_ROUTE_WPF0;
		ch_info->wpf_cnt++;
		break;
	case VSP_SRU_USE:
		route |= VSP_DPR_ROUTE_SRU;
		break;
	case VSP_UDS_USE:
		route |= VSP_DPR_ROUTE_UDS;
		break;
	case VSP_LUT_USE:
		route |= VSP_DPR_ROUTE_LUT;
		break;
	case VSP_CLU_USE:
		route |= VSP_DPR_ROUTE_CLU;
		break;
	case VSP_HST_USE:
		route |= VSP_DPR_ROUTE_HST;
		break;
	case VSP_HSI_USE:
		route |= VSP_DPR_ROUTE_HSI;
		break;
	case VSP_SHP_USE:
		route |= VSP_DPR_ROUTE_SHP;
		break;
	case VSP_BRU_USE:
		if (ch_info->bru_cnt < VSP_BRU_IN_MAX) {
			route |= vsp_tbl_bru_route[ch_info->bru_cnt];
		} else {
			/* BRU channel empty */
			route = VSP_DPR_ROUTE_NOT_USE;
		}
		ch_info->bru_cnt++;
		break;
	case VSP_BRS_USE:
		if (ch_info->brs_cnt < 1) {
			route |= VSP_DPR_ROUTE_BRS0;
		} else if (ch_info->brs_cnt < 2) {
			route |= VSP_DPR_ROUTE_BRS1;
		} else {
			/* BRS channel empty */
			route = VSP_DPR_ROUTE_NOT_USE;
		}
		ch_info->brs_cnt++;
		break;
	default:
		route = VSP_DPR_ROUTE_NOT_USE;
		break;
	}

	ch_info->next_module = connect;

	return route;
}

/******************************************************************************
 * Function:		vsp_ins_get_dpr_smppt
 * Description:	Get sampling point value of module
 * Returns:		sampling point for DPR.
 ******************************************************************************/
static unsigned int vsp_ins_get_dpr_smppt(
	struct vsp_ch_info *ch_info, unsigned int sampling)
{
	unsigned int smppt = 0;

	/* get target node index */
	switch (sampling) {
	case VSP_SMPPT_SRC1:
	case VSP_SMPPT_SRC2:
	case VSP_SMPPT_SRC3:
	case VSP_SMPPT_SRC4:
	case VSP_SMPPT_SRC5:
		if (sampling < ch_info->src_cnt)
			smppt |= ch_info->src_info[sampling].rpf_ch;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_SRU:
		if (ch_info->reserved_module & VSP_SRU_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_UDS:
		if (ch_info->reserved_module & VSP_UDS_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_LUT:
		if (ch_info->reserved_module & VSP_LUT_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_BRU:
		if (ch_info->reserved_module & VSP_BRU_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_CLU:
		if (ch_info->reserved_module & VSP_CLU_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_HST:
		if (ch_info->reserved_module & VSP_HST_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_HSI:
		if (ch_info->reserved_module & VSP_HSI_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	case VSP_SMPPT_SHP:
		if (ch_info->reserved_module & VSP_SHP_USE)
			smppt |= sampling;
		else
			smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	default:
		smppt = VSP_DPR_SMPPT_NOT_USE;
		break;
	}

	return smppt;
}

/******************************************************************************
 * Function:		vsp_ins_get_passband_bwidth
 * Description:	Get passband of VI6_UDS_PASS_BWIDTH.
 * Returns:		Passband value for UDS.
 ******************************************************************************/
static unsigned int vsp_ins_get_passband_bwidth(unsigned short ratio)
{
	unsigned int bwidth;

	if (ratio & VSP_UDS_SCALE_MANT) {
		if (ratio < 0x4000)
			bwidth = 0x40000;
		else if (ratio < 0x8000)
			bwidth = 0x80000;
		else
			bwidth = 0x100000;

		bwidth = bwidth / ratio;
	} else {
		bwidth = 64;
	}

	return bwidth;
}

/******************************************************************************
 * Function:		vsp_ins_check_input_color_space_of_bru
 * Description:	Check input color space of bru.
 * Returns:		0/E_VSP_PARA_BRU_INHSV/E_VSP_PARA_BRU_INCOLOR
 ******************************************************************************/
static long vsp_ins_check_input_color_space_of_bru(struct vsp_ch_info *ch_info)
{
	struct vsp_src_info *src_info;
	unsigned char base_color;
	unsigned char i;

	if (ch_info->src_cnt > 0) {
		src_info = &ch_info->src_info[0];

		/* set base color space */
		base_color = src_info->color;

		for (i = 0; i < ch_info->src_cnt; i++) {
			if (src_info->color == VSP_COLOR_HSV)
				return E_VSP_PARA_BRU_INHSV;

			if (src_info->color != base_color)
				return E_VSP_PARA_BRU_INCOLOR;

			src_info++;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_input_color_space_of_brs
 * Description:	Check input color space of brs.
 * Returns:		0/E_VSP_PARA_BRS_INHSV/E_VSP_PARA_BRS_INCOLOR
 ******************************************************************************/
static long vsp_ins_check_input_color_space_of_brs(struct vsp_ch_info *ch_info)
{
	struct vsp_src_info *src_info;
	unsigned char base_color;
	unsigned char i;

	if (ch_info->src_cnt > 0) {
		src_info = &ch_info->src_info[0];

		/* set base color space */
		base_color = src_info->color;

		for (i = 0; i < ch_info->src_cnt; i++) {
			if (src_info->color == VSP_COLOR_HSV)
				return E_VSP_PARA_BRS_INHSV;

			if (src_info->color != base_color)
				return E_VSP_PARA_BRS_INCOLOR;

			src_info++;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_master_layer
 * Description:	Check master layer.
 * Returns:		0/E_VSP_PARA_NOINPUT/E_VSP_PARA_NOPARENT
 *	E_VSP_PARA_VIR_XPOSI/E_VSP_PARA_IN_XPOSI/E_VSP_PARA_VIR_YPOSI
 *	E_VSP_PARA_IN_YPOSI
 ******************************************************************************/
static long vsp_ins_check_master_layer(struct vsp_ch_info *ch_info)
{
	struct vsp_src_info *src_info;
	unsigned char master_idx;
	unsigned char master_cnt = 0;
	unsigned int srcrpf = 0;

	unsigned char color = VSP_COLOR_NO;

	unsigned char i;

	/* check input source counter */
	if (ch_info->src_cnt == 0) {
		/* no input source */
		return E_VSP_PARA_NOINPUT;
	}

	/* search and increment master counter */
	src_info = &ch_info->src_info[0];
	for (i = 0; i < ch_info->src_cnt; i++) {
		if (src_info->master == VSP_LAYER_PARENT) {
			master_idx = i;
			master_cnt++;
		}

		/* set WPFn_SRCRPF register value */
		srcrpf |= src_info->master << (src_info->rpf_ch * 2);

		/* set color space */
		if (src_info->color != VSP_COLOR_NO)
			color = src_info->color;

		src_info++;
	}

	/* check master counter */
	if (master_cnt != 1) {
		/* no master or over masters */
		return E_VSP_PARA_NOPARENT;
	}

	/* check position parameter */
	src_info = &ch_info->src_info[0];
	for (i = 0; i < ch_info->src_cnt; i++) {
		if (i != master_idx) {
			if (src_info->x_position >=
					ch_info->src_info[master_idx].width) {
				if (src_info->rpf_ch == 14 ||
				    src_info->rpf_ch == 12)
					return E_VSP_PARA_VIR_XPOSI;
				else
					return E_VSP_PARA_IN_XPOSI;
			}

			if (src_info->y_position >=
					ch_info->src_info[master_idx].height) {
				if (src_info->rpf_ch == 14 ||
				    src_info->rpf_ch == 12)
					return E_VSP_PARA_VIR_YPOSI;
				else
					return E_VSP_PARA_IN_YPOSI;
			}
		}

		src_info++;
	}

	/* set master layer */
	ch_info->src_idx = master_idx;
	ch_info->src_info[master_idx].master = srcrpf;
	ch_info->src_info[master_idx].color = color;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_rpf_format
 * Description:	Check format of RPF.
 * Returns:		0/E_VSP_PARA_IN_ADR/E_VSP_PARA_IN_ADRC0
 *	E_VSP_PARA_IN_ADRC1/E_VSP_PARA_IN_WIDTH/E_VSP_PARA_IN_WIDTHEX
 *	E_VSP_PARA_IN_XOFFSET/E_VSP_PARA_IN_HEIGHT/E_VSP_PARA_IN_HEIGHTEX
 *	E_VSP_PARA_IN_YOFFSET/E_VSP_PARA_IN_FORMAT
 ******************************************************************************/
static long vsp_ins_check_rpf_format(
	struct vsp_rpf_info *rpf_info, struct vsp_src_t *src_par)
{
	unsigned int x_offset = (unsigned int)src_par->x_offset;
	unsigned int x_offset_c = (unsigned int)src_par->x_offset;
	unsigned int y_offset = (unsigned int)src_par->y_offset;
	unsigned int y_offset_c = (unsigned int)src_par->y_offset;
	unsigned int stride = (unsigned int)src_par->stride;
	unsigned int stride_c = (unsigned int)src_par->stride_c;
	unsigned int temp;

	if (src_par->vir == VSP_NO_VIR) {
		/* check RGB(Y) address pointer */
		if (src_par->addr == 0)
			return E_VSP_PARA_IN_ADR;

		switch (src_par->format) {
		case VSP_IN_RGB332:
		case VSP_IN_XRGB4444:
		case VSP_IN_RGBX4444:
		case VSP_IN_XRGB1555:
		case VSP_IN_RGBX5551:
		case VSP_IN_RGB565:
		case VSP_IN_AXRGB86666:
		case VSP_IN_RGBXA66668:
		case VSP_IN_XRGBA66668:
		case VSP_IN_ARGBX86666:
		case VSP_IN_AXXXRGB82666:
		case VSP_IN_XXXRGBA26668:
		case VSP_IN_ARGBXXX86662:
		case VSP_IN_RGBXXXA66628:
		case VSP_IN_XRGB6666:
		case VSP_IN_RGBX6666:
		case VSP_IN_XXXRGB2666:
		case VSP_IN_RGBXXX6662:
		case VSP_IN_ARGB8888:
		case VSP_IN_RGBA8888:
		case VSP_IN_RGB888:
		case VSP_IN_XXRGB7666:
		case VSP_IN_XRGB14666:
		case VSP_IN_BGR888:
		case VSP_IN_ARGB4444:
		case VSP_IN_RGBA4444:
		case VSP_IN_ARGB1555:
		case VSP_IN_RGBA5551:
		case VSP_IN_ABGR4444:
		case VSP_IN_BGRA4444:
		case VSP_IN_ABGR1555:
		case VSP_IN_BGRA5551:
		case VSP_IN_XXXBGR2666:
		case VSP_IN_ABGR8888:
		case VSP_IN_XRGB16565:
		case VSP_IN_RGB_CLUT_DATA:
		case VSP_IN_YUV_CLUT_DATA:
			temp = (unsigned int)
				((src_par->format & 0x0f00) >> 8);

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				(x_offset * temp);

			rpf_info->val_addr_c0 = 0;
			rpf_info->val_addr_c1 = 0;
			break;
		/* YUV interleaved */
		case VSP_IN_YUV420_INTERLEAVED:
			/* check height */
			if (src_par->height & 0x1)
				return E_VSP_PARA_IN_HEIGHT;

			/* check height_ex */
			if (src_par->height_ex & 0x1)
				return E_VSP_PARA_IN_HEIGHTEX;

			/* check y_offset */
			if (src_par->y_offset & 0x1)
				return E_VSP_PARA_IN_YOFFSET;

			x_offset <<= 1;
			y_offset >>= 1;

			/* check width */
			if (src_par->width & 0x1)
				return E_VSP_PARA_IN_WIDTH;

			/* check width_ex */
			if (src_par->width_ex & 0x1)
				return E_VSP_PARA_IN_WIDTHEX;

			/* check x_offset */
			if (src_par->x_offset & 0x1)
				return E_VSP_PARA_IN_XOFFSET;

			x_offset_c >>= 1;

			temp = (y_offset * stride) +
				(x_offset + x_offset_c + x_offset_c);

			rpf_info->val_addr_y = src_par->addr + temp;

			rpf_info->val_addr_c0 = 0;
			rpf_info->val_addr_c1 = 0;
			break;
		case VSP_IN_YUV422_INTERLEAVED0:
		case VSP_IN_YUV422_INT0_YUY2:
		case VSP_IN_YUV422_INT0_YVYU:
		case VSP_IN_YUV422_INTERLEAVED1:
			/* check width */
			if (src_par->width & 0x1)
				return E_VSP_PARA_IN_WIDTH;

			/* check width_ex */
			if (src_par->width_ex & 0x1)
				return E_VSP_PARA_IN_WIDTHEX;

			/* check x_offset */
			if (src_par->x_offset & 0x1)
				return E_VSP_PARA_IN_XOFFSET;

			x_offset_c >>= 1;

			temp = (y_offset * stride) +
				(x_offset + x_offset_c + x_offset_c);

			rpf_info->val_addr_y = src_par->addr + temp;

			rpf_info->val_addr_c0 = 0;
			rpf_info->val_addr_c1 = 0;
			break;
		case VSP_IN_YUV444_INTERLEAVED:

			temp = (y_offset * stride) +
				(x_offset + x_offset_c + x_offset_c);

			rpf_info->val_addr_y = src_par->addr + temp;

			rpf_info->val_addr_c0 = 0;
			rpf_info->val_addr_c1 = 0;
			break;
		/* YUV semi planer */
		case VSP_IN_YUV420_SEMI_PLANAR:
		case VSP_IN_YUV420_SEMI_NV21:
			/* check height */
			if (src_par->height & 0x1)
				return E_VSP_PARA_IN_HEIGHT;

			/* check height_ex */
			if (src_par->height_ex & 0x1)
				return E_VSP_PARA_IN_HEIGHTEX;

			/* check y_offset */
			if (src_par->y_offset & 0x1)
				return E_VSP_PARA_IN_YOFFSET;

			y_offset_c >>= 1;

			/* check width */
			if (src_par->width & 0x1)
				return E_VSP_PARA_IN_WIDTH;

			/* check width_ex */
			if (src_par->width_ex & 0x1)
				return E_VSP_PARA_IN_WIDTHEX;

			/* check x_offset */
			if (src_par->x_offset & 0x1)
				return E_VSP_PARA_IN_XOFFSET;

			x_offset_c >>= 1;

			/* check Cb address pointer */
			if (src_par->addr_c0 == 0)
				return E_VSP_PARA_IN_ADRC0;

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				x_offset;

			temp = (y_offset_c * stride_c) +
				(x_offset_c + x_offset_c);

			rpf_info->val_addr_c0 = src_par->addr_c0 + temp;
			rpf_info->val_addr_c1 = 0;
			break;
		case VSP_IN_YUV422_SEMI_PLANAR:
		case VSP_IN_YUV422_SEMI_NV61:
			/* check width */
			if (src_par->width & 0x1)
				return E_VSP_PARA_IN_WIDTH;

			/* check width_ex */
			if (src_par->width_ex & 0x1)
				return E_VSP_PARA_IN_WIDTHEX;

			/* check x_offset */
			if (src_par->x_offset & 0x1)
				return E_VSP_PARA_IN_XOFFSET;

			x_offset_c >>= 1;

			/* check Cb address pointer */
			if (src_par->addr_c0 == 0)
				return E_VSP_PARA_IN_ADRC0;

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				x_offset;

			temp = (y_offset_c * stride_c) +
				(x_offset_c + x_offset_c);

			rpf_info->val_addr_c0 = src_par->addr_c0 + temp;
			rpf_info->val_addr_c1 = 0;
			break;
		case VSP_IN_YUV444_SEMI_PLANAR:
			/* check Cb address pointer */
			if (src_par->addr_c0 == 0)
				return E_VSP_PARA_IN_ADRC0;

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				x_offset;

			temp = (y_offset_c * stride_c) +
				(x_offset_c + x_offset_c);

			rpf_info->val_addr_c0 = src_par->addr_c0 + temp;
			rpf_info->val_addr_c1 = 0;
			break;
		/* YUV planar */
		case VSP_IN_YUV420_PLANAR:
			/* check height */
			if (src_par->height & 0x1)
				return E_VSP_PARA_IN_HEIGHT;

			/* check height_ex */
			if (src_par->height_ex & 0x1)
				return E_VSP_PARA_IN_HEIGHTEX;

			/* check y_offset */
			if (src_par->y_offset & 0x1)
				return E_VSP_PARA_IN_YOFFSET;

			y_offset_c >>= 1;

			/* check width */
			if (src_par->width & 0x1)
				return E_VSP_PARA_IN_WIDTH;

			/* check width_ex */
			if (src_par->width_ex & 0x1)
				return E_VSP_PARA_IN_WIDTHEX;

			/* check x_offset */
			if (src_par->x_offset & 0x1)
				return E_VSP_PARA_IN_XOFFSET;

			x_offset_c >>= 1;

			/* check CbCr address pointer */
			if (src_par->addr_c0 == 0)
				return E_VSP_PARA_IN_ADRC0;

			if (src_par->addr_c1 == 0)
				return E_VSP_PARA_IN_ADRC1;

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				x_offset;

			temp = (y_offset_c * stride_c) + x_offset_c;

			rpf_info->val_addr_c0 = src_par->addr_c0 + temp;
			rpf_info->val_addr_c1 = src_par->addr_c1 + temp;
			break;
		case VSP_IN_YUV422_PLANAR:
			/* check width */
			if (src_par->width & 0x1)
				return E_VSP_PARA_IN_WIDTH;

			/* check width_ex */
			if (src_par->width_ex & 0x1)
				return E_VSP_PARA_IN_WIDTHEX;

			/* check x_offset */
			if (src_par->x_offset & 0x1)
				return E_VSP_PARA_IN_XOFFSET;

			x_offset_c >>= 1;

			/* check CbCr address pointer */
			if (src_par->addr_c0 == 0)
				return E_VSP_PARA_IN_ADRC0;

			if (src_par->addr_c1 == 0)
				return E_VSP_PARA_IN_ADRC1;

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				x_offset;

			temp = (y_offset_c * stride_c) + x_offset_c;

			rpf_info->val_addr_c0 = src_par->addr_c0 + temp;
			rpf_info->val_addr_c1 = src_par->addr_c1 + temp;
			break;
		case VSP_IN_YUV444_PLANAR:
			/* check CbCr address pointer */
			if (src_par->addr_c0 == 0)
				return E_VSP_PARA_IN_ADRC0;

			if (src_par->addr_c1 == 0)
				return E_VSP_PARA_IN_ADRC1;

			/* set address */
			rpf_info->val_addr_y =
				src_par->addr +
				(y_offset * stride) +
				x_offset;

			temp = (y_offset_c * stride_c) + x_offset_c;

			rpf_info->val_addr_c0 = src_par->addr_c0 + temp;
			rpf_info->val_addr_c1 = src_par->addr_c1 + temp;
			break;
		default:
			return E_VSP_PARA_IN_FORMAT;
		}
	} else {	/* src_par->vir == VSP_VIR */
		if (src_par->format != VSP_IN_ARGB8888 &&
		    src_par->format != VSP_IN_YUV444_SEMI_PLANAR) {
			return E_VSP_PARA_IN_FORMAT;
		}

		rpf_info->val_addr_y = 0;
		rpf_info->val_addr_c0 = 0;
		rpf_info->val_addr_c1 = 0;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_rpf_clut_param
 * Description:	Check clut parameter of RPF.
 * Returns:		0/E_VSP_PARA_OSD_CLUT/E_VSP_PARA_OSD_SIZE
 ******************************************************************************/
static long vsp_ins_check_rpf_clut_param(
	struct vsp_rpf_info *rpf_info, struct vsp_src_t *src_par)
{
	struct vsp_dl_t *clut = src_par->clut;

	if (src_par->format == VSP_IN_RGB_CLUT_DATA ||
	    src_par->format == VSP_IN_YUV_CLUT_DATA) {
		if (clut) {
			/* check display list pointer */
			if (clut->hard_addr == 0 ||
			    !clut->virt_addr)
				return E_VSP_PARA_OSD_CLUT;

			/* check size */
			if (clut->tbl_num < 1 ||
			    clut->tbl_num > 256)
				return E_VSP_PARA_OSD_SIZE;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_ckey_unit_param
 * Description:	Check color key unit parameter of RPF.
 * Returns:		0/E_VSP_PARA_ALPHA_CKEY
 ******************************************************************************/
static long vsp_ins_check_ckey_unit_param(
	struct vsp_rpf_info *rpf_info, struct vsp_ckey_unit_t *ckey)
{
	/* initialize */
	rpf_info->val_ckey_ctrl = 0;
	rpf_info->val_ckey_set[0] = 0;
	rpf_info->val_ckey_set[1] = 0;

	if (ckey) {
		switch (ckey->mode) {
		case VSP_CKEY_THROUGH:
			/* ckey unit is disable */
			break;
		case VSP_CKEY_TRANS_COLOR2:
			rpf_info->val_ckey_ctrl |= VSP_RPF_CKEY_CTRL_SAPE1;
			rpf_info->val_ckey_ctrl |= VSP_RPF_CKEY_CTRL_SAPE0;
			break;
		case VSP_CKEY_TRANS_COLOR1:
			rpf_info->val_ckey_ctrl |= VSP_RPF_CKEY_CTRL_SAPE0;
			break;
		case VSP_CKEY_MATCHED_COLOR:
			rpf_info->val_ckey_ctrl |= VSP_RPF_CKEY_CTRL_CV;
			break;
		case VSP_CKEY_LUMA_THRESHOLD:
			rpf_info->val_ckey_ctrl |= VSP_RPF_CKEY_CTRL_LTH;
			break;
		default:
			return E_VSP_PARA_ALPHA_CKEY;
		}
		rpf_info->val_ckey_set[0] = (unsigned int)ckey->color1;
		rpf_info->val_ckey_set[1] = (unsigned int)ckey->color2;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_irop_unit_param
 * Description:	Check irop unit parameter of RPF.
 * Returns:		0/E_VSP_PARA_ALPHA_BSEL/E_VSP_PARA_ALPHA_MSKEN
 *	E_VSP_PARA_ALPHA_IROP
 ******************************************************************************/
static long vsp_ins_check_irop_unit_param(
	struct vsp_rpf_info *rpf_info,
	struct vsp_alpha_unit_t *alpha,
	unsigned char *use_alpha_plane)
{
	struct vsp_irop_unit_t *irop = alpha->irop;

	/* initialize */
	rpf_info->val_mskctrl = 0;
	rpf_info->val_mskset[0] = 0;
	rpf_info->val_mskset[1] = 0;

	if (irop) {
		if (alpha->asel == VSP_ALPHA_NUM1 ||
		    alpha->asel == VSP_ALPHA_NUM3) {
			/* check 1bit mask-alpha generator */
			if (irop->ref_sel == VSP_MSKEN_ALPHA) {
				if (irop->bit_sel == VSP_ALPHA_8BIT)
					*use_alpha_plane = 1;
				else if (irop->bit_sel == VSP_ALPHA_1BIT)
					*use_alpha_plane = 8;
				else
					return E_VSP_PARA_ALPHA_BSEL;

				rpf_info->val_alpha_sel |=
					((unsigned int)irop->bit_sel) << 23;

				if (irop->op_mode == VSP_IROP_NOP ||
				    irop->op_mode == VSP_IROP_CLEAR ||
				    irop->op_mode == VSP_IROP_INVERT ||
				    irop->op_mode == VSP_IROP_SET) {
					/* don't use external memory */
					*use_alpha_plane = 0;
				}
			} else if (irop->ref_sel == VSP_MSKEN_COLOR) {
				rpf_info->val_mskctrl = (unsigned int)
					(VSP_RPF_MSKCTRL_EN | irop->comp_color);
			} else {
				return E_VSP_PARA_ALPHA_MSKEN;
			}
			rpf_info->val_mskset[0] =
				(unsigned int)irop->irop_color0;
			rpf_info->val_mskset[1] =
				(unsigned int)irop->irop_color1;

			/* check irop unit parameter */
			if (irop->op_mode >= VSP_IROP_MAX)
				return E_VSP_PARA_ALPHA_IROP;

			rpf_info->val_alpha_sel |=
				((unsigned int)irop->op_mode) << 24;
		} else if ((alpha->asel == VSP_ALPHA_NUM2) ||
			(alpha->asel == VSP_ALPHA_NUM4)) {
			/* check 1bit mask-alpha generator */
			if (irop->ref_sel == VSP_MSKEN_ALPHA) {
				if (irop->op_mode != VSP_IROP_NOP)
					return E_VSP_PARA_ALPHA_IROP;
			} else if (irop->ref_sel == VSP_MSKEN_COLOR) {
				rpf_info->val_mskctrl = (unsigned int)
					(VSP_RPF_MSKCTRL_EN | irop->comp_color);
			} else {
				return E_VSP_PARA_ALPHA_MSKEN;
			}
			rpf_info->val_mskset[0] =
				(unsigned int)irop->irop_color0;
			rpf_info->val_mskset[1] =
				(unsigned int)irop->irop_color1;

			/* check irop unit parameter */
			if (irop->op_mode >= VSP_IROP_MAX)
				return E_VSP_PARA_ALPHA_IROP;

			rpf_info->val_alpha_sel |=
				((unsigned int)irop->op_mode) << 24;
		} else if (alpha->asel == VSP_ALPHA_NUM5) {
			/* check irop unit parameter */
			if (irop->op_mode != VSP_IROP_NOP)
				return E_VSP_PARA_ALPHA_IROP;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_mult_unit_param
 * Description:	Check multiple unit parameter of RPF.
 * Returns:		0/E_VSP_PARA_ALPHA_MULT
 ******************************************************************************/
static long vsp_ins_check_mult_unit_param(
	struct vsp_rpf_info *rpf_info,
	struct vsp_src_info *src_info,
	struct vsp_mult_unit_t *mult)
{
	/* initialize */
	rpf_info->val_mult_alph = 0;

	if (mult) {
		if (mult->a_mmd != VSP_MULT_THROUGH &&
		    mult->a_mmd != VSP_MULT_RATIO)
			return E_VSP_PARA_ALPHA_MULT;

		if (mult->p_mmd != VSP_MULT_THROUGH &&
		    mult->p_mmd != VSP_MULT_RATIO &&
		    mult->p_mmd != VSP_MULT_ALPHA &&
		    mult->p_mmd != VSP_MULT_RATIO_ALPHA)
			return E_VSP_PARA_ALPHA_MULT;

		if (src_info->color == VSP_COLOR_YUV) {
			if (mult->a_mmd != VSP_MULT_THROUGH ||
			    mult->p_mmd != VSP_MULT_THROUGH)
				return E_VSP_PARA_ALPHA_MULT;
		}

		rpf_info->val_mult_alph =
			(((unsigned int)mult->a_mmd) << 12) |
			(((unsigned int)mult->p_mmd) << 8) |
			(unsigned int)mult->ratio;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_alpha_blend_param
 * Description:	Check alpha blend parameter of RPF.
 * Returns:		0/E_VSP_PARA_IN_ALPHA/E_VSP_PARA_ALPHA_ASEL
 *	E_VSP_PARA_ALPHA_AEXT/E_VSP_PARA_ALPHA_ADR/E_VSP_PARA_IN_XOFFSET
 *	return of vsp_ins_check_ckey_unit_param()
 *	return of vsp_ins_check_irop_unit_param()
 *	return of vsp_ins_check_mult_unit_param()
 ******************************************************************************/
static long vsp_ins_check_alpha_blend_param(
	struct vsp_rpf_info *rpf_info,
	struct vsp_src_info *src_info,
	struct vsp_src_t *src_par)
{
	struct vsp_alpha_unit_t *alpha = src_par->alpha;

	unsigned char use_alpha_plane = 0;
	long ercd;

	/* check pointer */
	if (!alpha)
		return E_VSP_PARA_IN_ALPHA;

	/* initialise value */
	rpf_info->val_mskctrl = 0;
	rpf_info->val_mskset[0] = 0;
	rpf_info->val_mskset[1] = 0;

	/* check virtual input parameter */
	if (src_par->vir == VSP_NO_VIR) {
		/* check color key unit parameter */
		ercd = vsp_ins_check_ckey_unit_param(rpf_info, alpha->ckey);
		if (ercd)
			return ercd;

		if (alpha->asel == VSP_ALPHA_NUM5) {
			rpf_info->val_vrtcol =
				((unsigned int)alpha->afix) << 24;
		}
	} else {	/* src_par->vir == VSP_VIR */
		if (alpha->asel != VSP_ALPHA_NUM5)
			return E_VSP_PARA_ALPHA_ASEL;
	}

	/* check alpha blend parameter */
	rpf_info->val_alpha_sel = ((unsigned int)alpha->asel) << 28;
	if (alpha->asel == VSP_ALPHA_NUM1) {
		/* check 8bit transparent-alpha generator */
		if (alpha->aext != VSP_AEXT_EXPAN &&
		    alpha->aext != VSP_AEXT_COPY &&
		    alpha->aext != VSP_AEXT_EXPAN_MAX) {
			return E_VSP_PARA_ALPHA_AEXT;
		}

		/* set 8bit transparent-alpha generator */
		rpf_info->val_alpha_sel |= ((unsigned int)alpha->aext) << 18;
	} else if (alpha->asel == VSP_ALPHA_NUM2) {
		use_alpha_plane = 1;
	} else if (alpha->asel == VSP_ALPHA_NUM3) {
		/* set 8bit transparent-alpha generator */
		rpf_info->val_alpha_sel |= ((unsigned int)alpha->anum1) << 8;
		rpf_info->val_alpha_sel |=  (unsigned int)alpha->anum0;
	} else if (alpha->asel == VSP_ALPHA_NUM4) {
		use_alpha_plane = 8;

		/* set 8bit transparent-alpha generator */
		rpf_info->val_alpha_sel |= ((unsigned int)alpha->anum1) << 8;
		rpf_info->val_alpha_sel |=  (unsigned int)alpha->anum0;
	} else if (alpha->asel == VSP_ALPHA_NUM5) {
		/* no error */
	} else {
		return E_VSP_PARA_ALPHA_ASEL;
	};

	/* check 1bit-mask generator and irop unit parameter */
	ercd = vsp_ins_check_irop_unit_param(rpf_info, alpha, &use_alpha_plane);
	if (ercd)
		return ercd;

	if (use_alpha_plane) {
		/* check alpha plane buffer pointer */
		if (alpha->addr_a == 0)
			return E_VSP_PARA_ALPHA_ADR;

		/* check x_offset parameter */
		if (src_par->x_offset % use_alpha_plane)
			return E_VSP_PARA_IN_XOFFSET;

		/* set address and stride value */
		rpf_info->val_addr_ai =
			alpha->addr_a +
			((unsigned int)alpha->stride_a) *
			((unsigned int)src_par->y_offset) +
			(unsigned int)(src_par->x_offset / use_alpha_plane);
		rpf_info->val_astride = (unsigned int)alpha->stride_a;

		/* update swap value */
		rpf_info->val_dswap |= ((unsigned int)alpha->swap) << 8;
	} else {
		/* disable reading external memory */
		rpf_info->val_addr_ai = 0;
		rpf_info->val_astride = 0;
	}

	/* check multiple unit parameter */
	ercd = vsp_ins_check_mult_unit_param(rpf_info, src_info, alpha->mult);
	if (ercd)
		return ercd;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_rpf_param
 * Description:	Check source parameter of RPF.
 * Returns:		0/E_VSP_PARA_NOIN/E_VSP_PARA_IN_ITURBT
 *	E_VSP_PARA_IN_CLRCNG/E_VSP_PARA_IN_CSC/E_VSP_PARA_IN_VIR
 *	E_VSP_PARA_IN_CIPM/E_VSP_PARA_IN_CEXT/E_VSP_PARA_IN_WIDTH
 *	E_VSP_PARA_IN_HEIGHT/E_VSP_PARA_IN_WIDTHEX/E_VSP_PARA_IN_HEIGHTEX
 *	E_VSP_PARA_IN_PWD/E_VSP_PARA_IN_CONNECT
 *	return of vsp_ins_check_rpf_format()
 *	return of vsp_ins_check_rpf_clut_param()
 *	return of vsp_ins_check_alpha_blend_param()
 ******************************************************************************/
static long vsp_ins_check_rpf_param(
	struct vsp_ch_info *ch_info,
	struct vsp_rpf_info *rpf_info,
	struct vsp_src_t *src_par)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_part_info *part_info = &ch_info->part_info;
	unsigned int width, height;
	long ercd;

	/* initialise */
	rpf_info->val_infmt = 0;
	rpf_info->val_vrtcol = 0;

	/* check pointer */
	if (!src_par)
		return E_VSP_PARA_NOIN;

	/* check virtual parameter */
	if (src_par->vir == VSP_NO_VIR) {
		/* check color space conversion parameter */
		if (src_par->csc == VSP_CSC_OFF) {
			/* no error */
		} else if (src_par->csc == VSP_CSC_ON) {
			rpf_info->val_infmt |= VSP_RPF_INFMT_CSC;

			if (src_par->iturbt == VSP_ITURBT_601) {
				/* no error */
				/* dummy line */
			} else if (src_par->iturbt == VSP_ITURBT_709) {
				rpf_info->val_infmt |= VSP_RPF_INFMT_RDTM1;
			} else {
				return E_VSP_PARA_IN_ITURBT;
			}

			if (src_par->clrcng == VSP_ITU_COLOR) {
				/* no error */
				/* dummy line */
			} else if (src_par->clrcng == VSP_FULL_COLOR) {
				rpf_info->val_infmt |= VSP_RPF_INFMT_RDTM0;
			} else {
				return E_VSP_PARA_IN_CLRCNG;
			}
		} else {
			return E_VSP_PARA_IN_CSC;
		}
	} else if (src_par->vir == VSP_VIR) {
		rpf_info->val_infmt |= VSP_RPF_INFMT_VIR;
		rpf_info->val_vrtcol = (unsigned int)src_par->vircolor;

		/* check color space conversion parameter */
		if (src_par->csc != VSP_CSC_OFF)
			return E_VSP_PARA_IN_CSC;
	} else {
		return E_VSP_PARA_IN_VIR;
	}

	/* check horizontal chrominance interpolation method parameter */
	if (src_par->cipm != VSP_CIPM_0_HOLD &&
	    src_par->cipm != VSP_CIPM_BI_LINEAR)
		return E_VSP_PARA_IN_CIPM;
	rpf_info->val_infmt |= ((unsigned int)src_par->cipm) << 16;

	/* check lower-bit color data extension method parameter */
	if (src_par->cext != VSP_CEXT_EXPAN &&
	    src_par->cext != VSP_CEXT_COPY &&
	    src_par->cext != VSP_CEXT_EXPAN_MAX)
		return E_VSP_PARA_IN_CEXT;
	rpf_info->val_infmt |= ((unsigned int)src_par->cext) << 12;

	/* check format parameter */
	ercd = vsp_ins_check_rpf_format(rpf_info, src_par);
	if (ercd)
		return ercd;

	rpf_info->val_infmt |=
		(unsigned int)(src_par->format & VSP_RPF_INFMT_MSK);

	/* set color space */
	if (((src_par->format & 0x40) == 0x40) ^
		 (src_par->csc == VSP_CSC_ON))
		src_info->color = VSP_COLOR_YUV;
	else
		src_info->color = VSP_COLOR_RGB;

	/* set data swapping parameter */
	rpf_info->val_dswap = (unsigned int)(src_par->swap);

	/* check basic area */
	if (src_par->width < 1 || src_par->width > 8190)
		return E_VSP_PARA_IN_WIDTH;

	if (src_par->height < 1 || src_par->height > 8190)
		return E_VSP_PARA_IN_HEIGHT;

	rpf_info->val_bsize = ((unsigned int)src_par->width << 16);
	rpf_info->val_bsize |= (unsigned int)src_par->height;

	/* check extended area */
	if (src_par->width_ex == 0) {
		width = (unsigned int)src_par->width;
	} else {
		if (src_par->width_ex < src_par->width ||
		    src_par->width_ex > 8190)
			return E_VSP_PARA_IN_WIDTHEX;
		width = (unsigned int)src_par->width_ex;
	}

	if (src_par->height_ex == 0) {
		height = (unsigned int)src_par->height;
	} else {
		if (src_par->height_ex < src_par->height ||
		    src_par->height_ex > 8190)
			return E_VSP_PARA_IN_HEIGHTEX;
		height = (unsigned int)src_par->height_ex;
	}
	rpf_info->val_esize = (width << 16) | height;

	src_info->width = width;
	src_info->height = height;

	/* check source RPF register */
	if (src_par->pwd == VSP_LAYER_PARENT) {
		width = 0;
		height = 0;
	} else if (src_par->pwd == VSP_LAYER_CHILD) {
		width = (unsigned int)src_par->x_position;
		height = (unsigned int)src_par->y_position;
	} else {
		return E_VSP_PARA_IN_PWD;
	}
	rpf_info->val_loc = (width << 16) | height;

	src_info->master = (unsigned int)src_par->pwd;
	src_info->x_position = width;
	src_info->y_position = height;

	/* check input color look up table parameter */
	ercd = vsp_ins_check_rpf_clut_param(rpf_info, src_par);
	if (ercd)
		return ercd;

	/* check alpha blend and color converter parameter */
	ercd = vsp_ins_check_alpha_blend_param(
		rpf_info, src_info, src_par);
	if (ercd)
		return ercd;

	/* check connect parameter */
	if (src_par->connect & ~VSP_RPF_USABLE_DPR)
		return E_VSP_PARA_IN_CONNECT;

	/* get DPR value */
	rpf_info->val_dpr =
		vsp_ins_get_dpr_route(ch_info, src_par->connect, 0);
	if (rpf_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_IN_CONNECT;

	/* save address */
	part_info->rpf_addr_y = rpf_info->val_addr_y;
	part_info->rpf_addr_c0 = rpf_info->val_addr_c0;
	part_info->rpf_addr_c1 = rpf_info->val_addr_c1;
	part_info->rpf_addr_ai = rpf_info->val_addr_ai;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_wpf_format
 * Description:	Check format of WPF.
 * Returns:		0/E_VSP_PARA_OUT_ADR/E_VSP_PARA_OUT_ADRC0
 *	E_VSP_PARA_OUT_ADRC1/E_VSP_PARA_OUT_WIDTH/E_VSP_PARA_OUT_XOFFSET
 *	E_VSP_PARA_OUT_HEIGHT/E_VSP_PARA_OUT_YOFFSET/E_VSP_PARA_OUT_FORMAT
 ******************************************************************************/
static long vsp_ins_check_wpf_format(
	struct vsp_ch_info *ch_info, struct vsp_dst_t *dst_par)
{
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;
	unsigned int x_offset = (unsigned int)dst_par->x_offset;
	unsigned int x_offset_c = (unsigned int)dst_par->x_offset;
	unsigned int y_offset = (unsigned int)dst_par->y_offset;
	unsigned int y_offset_c = (unsigned int)dst_par->y_offset;
	unsigned int stride = (unsigned int)dst_par->stride;
	unsigned int stride_c = (unsigned int)dst_par->stride_c;
	unsigned int temp;

	/* check RGB(Y) address pointer */
	if (dst_par->addr == 0)
		return E_VSP_PARA_OUT_ADR;

	switch (dst_par->format) {
	case VSP_OUT_RGB332:
	case VSP_OUT_XRGB4444:
	case VSP_OUT_RGBX4444:
	case VSP_OUT_XRGB1555:
	case VSP_OUT_RGBX5551:
	case VSP_OUT_RGB565:
	case VSP_OUT_PXRGB86666:
	case VSP_OUT_RGBXP66668:
	case VSP_OUT_XRGBP66668:
	case VSP_OUT_PRGBX86666:
	case VSP_OUT_PXXXRGB82666:
	case VSP_OUT_XXXRGBP26668:
	case VSP_OUT_PRGBXXX86662:
	case VSP_OUT_RGBXXXP66628:
	case VSP_OUT_XRGB6666:
	case VSP_OUT_RGBX6666:
	case VSP_OUT_XXXRGB2666:
	case VSP_OUT_RGBXXX6662:
	case VSP_OUT_PRGB8888:
	case VSP_OUT_RGBP8888:
	case VSP_OUT_RGB888:
	case VSP_OUT_XXRGB7666:
	case VSP_OUT_XRGB14666:
	case VSP_OUT_BGR888:
	case VSP_OUT_PRGB4444:
	case VSP_OUT_RGBP4444:
	case VSP_OUT_PRGB1555:
	case VSP_OUT_RGBP5551:
	case VSP_OUT_PBGR4444:
	case VSP_OUT_BGRP4444:
	case VSP_OUT_PBGR1555:
	case VSP_OUT_BGRP5551:
	case VSP_OUT_XXXBGR2666:
	case VSP_OUT_PBGR8888:
	case VSP_OUT_XRGB16565:
		temp = (unsigned int)((dst_par->format & 0x0f00) >> 8);

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			(x_offset * temp);

		wpf_info->val_addr_c0 = 0;
		wpf_info->val_addr_c1 = 0;
		break;
	/* YUV interleaved */
	case VSP_OUT_YUV420_INTERLEAVED:
		/* check height */
		if (dst_par->height & 0x1)
			return E_VSP_PARA_OUT_HEIGHT;

		/* check y_offset */
		if (dst_par->y_offset & 0x1)
			return E_VSP_PARA_OUT_YOFFSET;

		x_offset <<= 1;
		y_offset >>= 1;

		/* check width */
		if (dst_par->width & 0x1)
			return E_VSP_PARA_OUT_WIDTH;

		/* check x_offset */
		if (dst_par->x_offset & 0x1)
			return E_VSP_PARA_OUT_XOFFSET;

		x_offset_c >>= 1;

		temp = (y_offset * stride) +
			(x_offset + x_offset_c + x_offset_c);

		wpf_info->val_addr_y = dst_par->addr + temp;

		wpf_info->val_addr_c0 = 0;
		wpf_info->val_addr_c1 = 0;
		break;
	case VSP_OUT_YUV422_INTERLEAVED0:
	case VSP_OUT_YUV422_INT0_YUY2:
	case VSP_OUT_YUV422_INT0_YVYU:
	case VSP_OUT_YUV422_INTERLEAVED1:
		/* check width */
		if (dst_par->width & 0x1)
			return E_VSP_PARA_OUT_WIDTH;

		/* check x_offset */
		if (dst_par->x_offset & 0x1)
			return E_VSP_PARA_OUT_XOFFSET;

		x_offset_c >>= 1;

		temp = (y_offset * stride) +
			(x_offset + x_offset_c + x_offset_c);

		wpf_info->val_addr_y = dst_par->addr + temp;

		wpf_info->val_addr_c0 = 0;
		wpf_info->val_addr_c1 = 0;
		break;
	case VSP_OUT_YUV444_INTERLEAVED:

		temp = (y_offset * stride) +
			(x_offset + x_offset_c + x_offset_c);

		wpf_info->val_addr_y = dst_par->addr + temp;

		wpf_info->val_addr_c0 = 0;
		wpf_info->val_addr_c1 = 0;
		break;
	/* YUV semi planer */
	case VSP_OUT_YUV420_SEMI_PLANAR:
	case VSP_OUT_YUV420_SEMI_NV21:
		/* check height */
		if (dst_par->height & 0x1)
			return E_VSP_PARA_OUT_HEIGHT;

		/* check y_offset */
		if (dst_par->y_offset & 0x1)
			return E_VSP_PARA_OUT_YOFFSET;

		y_offset_c >>= 1;

		/* check width */
		if (dst_par->width & 0x1)
			return E_VSP_PARA_OUT_WIDTH;

		/* check x_offset */
		if (dst_par->x_offset & 0x1)
			return E_VSP_PARA_OUT_XOFFSET;

		x_offset_c >>= 1;

		/* check Cb address pointer */
		if (dst_par->addr_c0 == 0)
			return E_VSP_PARA_OUT_ADRC0;

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			x_offset;

		temp = (y_offset_c * stride_c) + (x_offset_c + x_offset_c);

		wpf_info->val_addr_c0 = dst_par->addr_c0 + temp;
		wpf_info->val_addr_c1 = 0;
		break;
	case VSP_OUT_YUV422_SEMI_PLANAR:
	case VSP_OUT_YUV422_SEMI_NV61:
		/* check width */
		if (dst_par->width & 0x1)
			return E_VSP_PARA_OUT_WIDTH;

		/* check x_offset */
		if (dst_par->x_offset & 0x1)
			return E_VSP_PARA_OUT_XOFFSET;

		x_offset_c >>= 1;

		/* check Cb address pointer */
		if (dst_par->addr_c0 == 0)
			return E_VSP_PARA_OUT_ADRC0;

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			x_offset;

		temp = (y_offset_c * stride_c) + (x_offset_c + x_offset_c);

		wpf_info->val_addr_c0 = dst_par->addr_c0 + temp;
		wpf_info->val_addr_c1 = 0;
		break;
	case VSP_OUT_YUV444_SEMI_PLANAR:
		/* check Cb address pointer */
		if (dst_par->addr_c0 == 0)
			return E_VSP_PARA_OUT_ADRC0;

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			x_offset;

		temp = (y_offset_c * stride_c) + (x_offset_c + x_offset_c);

		wpf_info->val_addr_c0 = dst_par->addr_c0 + temp;
		wpf_info->val_addr_c1 = 0;
		break;
	/* YUV planar */
	case VSP_OUT_YUV420_PLANAR:
		/* check height */
		if (dst_par->height & 0x1)
			return E_VSP_PARA_OUT_HEIGHT;

		/* check y_offset */
		if (dst_par->y_offset & 0x1)
			return E_VSP_PARA_OUT_YOFFSET;

		y_offset_c >>= 1;

		/* check width */
		if (dst_par->width & 0x1)
			return E_VSP_PARA_OUT_WIDTH;

		/* check x_offset */
		if (dst_par->x_offset & 0x1)
			return E_VSP_PARA_OUT_XOFFSET;

		x_offset_c >>= 1;

		/* check CbCr address pointer */
		if (dst_par->addr_c0 == 0)
			return E_VSP_PARA_OUT_ADRC0;

		if (dst_par->addr_c1 == 0)
			return E_VSP_PARA_OUT_ADRC1;

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			x_offset;

		temp = (y_offset_c * stride_c) + x_offset_c;

		wpf_info->val_addr_c0 = dst_par->addr_c0 + temp;
		wpf_info->val_addr_c1 = dst_par->addr_c1 + temp;
		break;
	case VSP_OUT_YUV422_PLANAR:
		/* check width */
		if (dst_par->width & 0x1)
			return E_VSP_PARA_OUT_WIDTH;

		/* check x_offset */
		if (dst_par->x_offset & 0x1)
			return E_VSP_PARA_OUT_XOFFSET;

		x_offset_c >>= 1;

		if (dst_par->addr_c0 == 0)
			return E_VSP_PARA_OUT_ADRC0;

		if (dst_par->addr_c1 == 0)
			return E_VSP_PARA_OUT_ADRC1;

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			x_offset;

		temp = (y_offset_c * stride_c) + x_offset_c;

		wpf_info->val_addr_c0 = dst_par->addr_c0 + temp;
		wpf_info->val_addr_c1 = dst_par->addr_c1 + temp;
		break;
	case VSP_OUT_YUV444_PLANAR:
		/* check CbCr address pointer */
		if (dst_par->addr_c0 == 0)
			return E_VSP_PARA_OUT_ADRC0;

		if (dst_par->addr_c1 == 0)
			return E_VSP_PARA_OUT_ADRC1;

		/* set address */
		wpf_info->val_addr_y =
			dst_par->addr +
			(y_offset * stride) +
			x_offset;

		temp = (y_offset_c * stride_c) + x_offset_c;

		wpf_info->val_addr_c0 = dst_par->addr_c0 + temp;
		wpf_info->val_addr_c1 = dst_par->addr_c1 + temp;
		break;
	default:
		return E_VSP_PARA_OUT_FORMAT;
	}

	/* set format parameter */
	wpf_info->val_outfmt =
		(unsigned int)(dst_par->format & VSP_WPF_OUTFMT_MSK);

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_wpf_rotation
 * Description:	Check rotation parameter of WPF.
 * Returns:		0/E_VSP_PARA_OUT_XCOFFSET/E_VSP_PARA_OUT_WIDTH
 *	E_VSP_PARA_OUT_XCLIP/E_VSP_PARA_OUT_YCOFFSET/E_VSP_PARA_OUT_HEIGHT
 *	E_VSP_PARA_OUT_ROTATION
 ******************************************************************************/
static long vsp_ins_check_wpf_rotation(
	struct vsp_ch_info *ch_info, struct vsp_dst_t *dst_par)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	unsigned int width;
	unsigned int height;

	unsigned int x_coffset;
	unsigned int y_coffset;

	/* check rotation parameter */
	if (dst_par->rotation <= VSP_ROT_180) {
		/* set image size */
		width = (unsigned int)dst_par->width;
		height = (unsigned int)dst_par->height;

		/* set clipping offset */
		x_coffset = (unsigned int)dst_par->x_coffset;
		y_coffset = (unsigned int)dst_par->y_coffset;

		/* check clipping paremeter */
		if (x_coffset > 255)
			return E_VSP_PARA_OUT_XCOFFSET;

		if (width == 0)
			return E_VSP_PARA_OUT_WIDTH;

		if (x_coffset + width > src_info->width)
			return E_VSP_PARA_OUT_XCLIP;

		if (y_coffset > 255)
			return E_VSP_PARA_OUT_YCOFFSET;

		if (height == 0)
			return E_VSP_PARA_OUT_HEIGHT;

		if (y_coffset + height > src_info->height)
			return E_VSP_PARA_OUT_YCLIP;
	} else if (dst_par->rotation <= VSP_ROT_270) {
		/* interchange image size */
		width = (unsigned int)dst_par->height;
		height = (unsigned int)dst_par->width;

		/* interchange clipping offset */
		x_coffset = (unsigned int)dst_par->y_coffset;
		y_coffset = (unsigned int)dst_par->x_coffset;

		/* check clipping paremeter */
		if (x_coffset > 255)
			return E_VSP_PARA_OUT_YCOFFSET;

		if (width == 0)
			return E_VSP_PARA_OUT_HEIGHT;

		if (x_coffset + width > src_info->width)
			return E_VSP_PARA_OUT_YCLIP;

		if (y_coffset > 255)
			return E_VSP_PARA_OUT_XCOFFSET;

		if (height == 0)
			return E_VSP_PARA_OUT_WIDTH;

		if (y_coffset + height > src_info->height)
			return E_VSP_PARA_OUT_XCLIP;
	} else {
		return E_VSP_PARA_OUT_ROTATION;
	}

	/* set rotation parameter */
	wpf_info->val_outfmt |= ((unsigned int)dst_par->rotation) << 16;

	/* set clipping parameter */
	wpf_info->val_hszclip = VSP_WPF_HSZCLIP_HCEN;
	wpf_info->val_hszclip |= (x_coffset << 16) | width;

	wpf_info->val_vszclip = VSP_WPF_VSZCLIP_VCEN;
	wpf_info->val_vszclip |= (y_coffset << 16) | height;

	/* check partition */
	if (dst_par->rotation > VSP_ROT_V_FLIP) {
		/* set partition flag */
		part_info->div_flag = 1;
	}

	/* decide division size of partition */
	part_info->div_size *= part_info->div_flag;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_wpf_fcnl
 * Description:	Check FCNL compression parameter of WPF.
 * Returns:		0/E_VSP_PARA_OUT_STRIDE_C/E_VSP_PARA_OUT_ADRC0
 *	E_VSP_PARA_OUT_ADRC1/E_VSP_PARA_OUT_STRIDE_Y/E_VSP_PARA_OUT_ADR
 *	E_VSP_PARA_OUT_FORMAT/E_VSP_PARA_OUT_ROTATION/E_VSP_PARA_OUT_SWAP
 *	E_VSP_PARA_FCNL
 ******************************************************************************/
static long vsp_ins_check_wpf_fcnl(
	struct vsp_ch_info *ch_info, struct vsp_dst_t *dst_par)
{
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;
	struct fcp_info_t *fcp = dst_par->fcp;

	if (fcp) {
		if (fcp->fcnl == FCP_FCNL_ENABLE) {
			switch (dst_par->rotation) {
			case VSP_ROT_OFF:
			case VSP_ROT_V_FLIP:
				/* check stride and address */
				if (dst_par->stride_c & 0xff)
					return E_VSP_PARA_OUT_STRIDE_C;

				if (wpf_info->val_addr_c0 & 0xff)
					return E_VSP_PARA_OUT_ADRC0;

				if (wpf_info->val_addr_c1 & 0xff)
					return E_VSP_PARA_OUT_ADRC1;

				/* check stride and address */
				if (dst_par->stride & 0xff)
					return E_VSP_PARA_OUT_STRIDE_Y;

				if (wpf_info->val_addr_y & 0xff)
					return E_VSP_PARA_OUT_ADR;

				/* check format */
				if (dst_par->format !=
						VSP_OUT_PRGB8888 &&
					dst_par->format !=
						VSP_OUT_YUV422_INT0_YUY2 &&
					dst_par->format !=
						VSP_OUT_YUV444_PLANAR &&
					dst_par->format !=
						VSP_OUT_YUV422_PLANAR &&
					dst_par->format !=
						VSP_OUT_YUV420_PLANAR)
					return E_VSP_PARA_OUT_FORMAT;
				break;
			case VSP_ROT_H_FLIP:
			case VSP_ROT_180:
				/* check stride and address */
				if (dst_par->stride & 0xff)
					return E_VSP_PARA_OUT_STRIDE_Y;

				if (wpf_info->val_addr_y & 0xff)
					return E_VSP_PARA_OUT_ADR;

				/* check format */
				if (dst_par->format !=
						VSP_OUT_PRGB8888 &&
					dst_par->format !=
						VSP_OUT_YUV422_INT0_YUY2 &&
					dst_par->format !=
						VSP_OUT_YUV444_PLANAR &&
					dst_par->format !=
						VSP_OUT_YUV422_PLANAR &&
					dst_par->format !=
						VSP_OUT_YUV420_PLANAR)
					return E_VSP_PARA_OUT_FORMAT;
				break;
			case VSP_ROT_90:
			case VSP_ROT_90_V_FLIP:
			case VSP_ROT_90_H_FLIP:
			case VSP_ROT_270:
				/* check stride and address */
				if (dst_par->stride & 0xff)
					return E_VSP_PARA_OUT_STRIDE_Y;

				if (wpf_info->val_addr_y & 0xff)
					return E_VSP_PARA_OUT_ADR;

				/* check format */
				if (dst_par->format != VSP_OUT_PRGB8888)
					return E_VSP_PARA_OUT_FORMAT;
				break;
			default:
				return E_VSP_PARA_OUT_ROTATION;
			}

			/* check swap */
			if (dst_par->swap != VSP_SWAP_LL)
				return E_VSP_PARA_OUT_SWAP;

			/* enable FCNL compression */
			wpf_info->val_outfmt |= VSP_WPF_OUTFMT_FCNL;
		} else if (fcp->fcnl == FCP_FCNL_DISABLE) {
			/* no error */
		} else {
			return E_VSP_PARA_FCNL;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_recalculate_wpf_addr
 * Description:	Recalculate buffer address of WPF.
 * Returns:		0
 ******************************************************************************/
static long vsp_ins_recalculate_wpf_addr(
	struct vsp_ch_info *ch_info, struct vsp_dst_t *dst_par)
{
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	unsigned int temp_y;
	unsigned int temp_c;

	if (dst_par->rotation == VSP_ROT_90 ||
	    dst_par->rotation == VSP_ROT_90_V_FLIP) {
		if (wpf_info->val_outfmt & VSP_WPF_OUTFMT_FCNL) {
			/* enable FCNL compression */
			temp_y = vsp_ins_get_bpp_luma(
				dst_par->format, dst_par->width);
			temp_y = VSP_ROUND_UP(temp_y, 256) * 256;
			temp_y -= vsp_ins_get_bpp_luma(dst_par->format, 16);

			temp_c = vsp_ins_get_bpp_chroma(
				dst_par->format, dst_par->width);
			temp_c = VSP_ROUND_UP(temp_c, 256) * 256;
			temp_c -= vsp_ins_get_bpp_chroma(dst_par->format, 16);
		} else {
			/* disable FCNL compression */
			if (dst_par->width > 16) {
				temp_y = vsp_ins_get_bpp_luma(
					dst_par->format, dst_par->width - 16);
				temp_c = vsp_ins_get_bpp_chroma(
					dst_par->format, dst_par->width - 16);
			} else {
				temp_y = 0;
				temp_c = 0;
			}
		}

		wpf_info->val_addr_y += temp_y;
		if (wpf_info->val_addr_c0)
			wpf_info->val_addr_c0 += temp_c;
		if (wpf_info->val_addr_c1)
			wpf_info->val_addr_c1 += temp_c;
	} else if ((dst_par->rotation == VSP_ROT_H_FLIP) ||
		(dst_par->rotation == VSP_ROT_180)) {
		if (part_info->div_flag != 0) {
			if (wpf_info->val_outfmt & VSP_WPF_OUTFMT_FCNL) {
				/* enable FCNL compression */
				temp_y =
					(dst_par->width - 1) /
					part_info->div_size;
				temp_y *= vsp_ins_get_bpp_luma(
					dst_par->format, part_info->div_size);

				temp_c =
					(dst_par->width - 1) /
					part_info->div_size;
				temp_c *= vsp_ins_get_bpp_chroma(
					dst_par->format, part_info->div_size);
			} else {
				/* disable FCNL compression */
				if (dst_par->width > part_info->div_size) {
					temp_y = vsp_ins_get_bpp_luma(
						dst_par->format,
						dst_par->width -
							part_info->div_size);
					temp_c = vsp_ins_get_bpp_chroma(
						dst_par->format,
						dst_par->width -
							part_info->div_size);
				} else {
					temp_y = 0;
					temp_c = 0;
				}
			}

			wpf_info->val_addr_y += temp_y;
			if (wpf_info->val_addr_c0)
				wpf_info->val_addr_c0 += temp_c;
			if (wpf_info->val_addr_c1)
				wpf_info->val_addr_c1 += temp_c;
		}
	}

	if (dst_par->rotation & 0x1) {
		temp_y = vsp_ins_get_line_luma(
			dst_par->format, dst_par->height);
		temp_y = (temp_y - 1) * (unsigned int)dst_par->stride;
		wpf_info->val_addr_y += temp_y;

		temp_c = vsp_ins_get_line_chroma(
			dst_par->format, dst_par->height);
		temp_c = (temp_c - 1) * (unsigned int)dst_par->stride_c;
		if (wpf_info->val_addr_c0)
			wpf_info->val_addr_c0 += temp_c;
		if (wpf_info->val_addr_c1)
			wpf_info->val_addr_c1 += temp_c;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_wpf_param
 * Description:	Check output parameter of WPF.
 * Returns:		0/E_VSP_PARA_OUTPAR/E_VSP_PARA_OUT_INHSV
 *	E_VSP_PARA_OUT_PXA/E_VSP_PARA_OUT_DITH/E_VSP_PARA_OUT_ITURBT
 *	E_VSP_PARA_OUT_CLRCNG/E_VSP_PARA_OUT_CSC/E_VSP_PARA_OUT_NOTCOLOR
 *	E_VSP_PARA_OUT_CBRM/E_VSP_PARA_OUT_ABRM/E_VSP_PARA_OUT_CLMD
 *	return of vsp_ins_check_wpf_format()
 *	return of vsp_ins_check_wpf_rotation()
 *	return of vsp_ins_check_wpf_fcnl()
 *	return of vsp_ins_recalculate_wpf_addr()
 ******************************************************************************/
static long vsp_ins_check_wpf_param(
	struct vsp_ch_info *ch_info, struct vsp_dst_t *dst_par)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	unsigned char exp_color;
	long ercd;

	/* check pointer */
	if (!dst_par)
		return E_VSP_PARA_OUTPAR;

	/* check input color space */
	if (src_info->color == VSP_COLOR_HSV)
		return E_VSP_PARA_OUT_INHSV;

	/* set source RPF parameter */
	wpf_info->val_srcrpf = src_info->master;

	/* check format parameter */
	ercd = vsp_ins_check_wpf_format(ch_info, dst_par);
	if (ercd)
		return ercd;

	/* check rotation parameter */
	ercd = vsp_ins_check_wpf_rotation(ch_info, dst_par);
	if (ercd)
		return ercd;

	/* check FCNL compression parameter */
	ercd = vsp_ins_check_wpf_fcnl(ch_info, dst_par);
	if (ercd)
		return ercd;

	/* recalculate address */
	ercd = vsp_ins_recalculate_wpf_addr(ch_info, dst_par);
	if (ercd)
		return ercd;

	/* check PAD data parameter */
	if (dst_par->pxa == VSP_PAD_P)
		wpf_info->val_outfmt |= ((unsigned int)(dst_par->pad)) << 24;
	else if (dst_par->pxa == VSP_PAD_IN)
		wpf_info->val_outfmt |= VSP_WPF_OUTFMT_PXA;
	else
		return E_VSP_PARA_OUT_PXA;

	/* check dithering parameter */
	if (dst_par->dith == VSP_DITH_OFF) {
		/* disable dithering */
		/* no error */
	} else if (dst_par->dith == VSP_DITH_COLOR_REDUCTION) {
		/* enable color reduction dither */
		wpf_info->val_outfmt |= VSP_WPF_OUTFMT_DITH;
	} else if (dst_par->dith == VSP_DITH_ORDERED_DITHER) {
		/* enable ordered dither */
		wpf_info->val_outfmt |= VSP_WPF_OUTFMT_ODE;
	} else {
		return E_VSP_PARA_OUT_DITH;
	}

	/* check color space conversion parameter */
	if (dst_par->csc == VSP_CSC_OFF) {
		/* disable color space conversion */
		/* no error */
	} else if (dst_par->csc == VSP_CSC_ON) {
		wpf_info->val_outfmt |= VSP_WPF_OUTFMT_CSC;

		if (dst_par->iturbt == VSP_ITURBT_601) {
			/* ITU-R BT601 compliant */
			/* no error */
		} else if (dst_par->iturbt == VSP_ITURBT_709) {
			/* ITU-R BT709 compliant */
			wpf_info->val_outfmt |= VSP_WPF_OUTFMT_WRTM1;
		} else {
			return E_VSP_PARA_OUT_ITURBT;
		}

		if (dst_par->clrcng == VSP_ITU_COLOR) {
			/* ITU-R rule conversion */
			/* no error */
		} else if (dst_par->clrcng == VSP_FULL_COLOR) {
			/* Full scale conversion */
			wpf_info->val_outfmt |= VSP_WPF_OUTFMT_WRTM0;
		} else {
			return E_VSP_PARA_OUT_CLRCNG;
		}
	} else {
		return E_VSP_PARA_OUT_CSC;
	}

	/* set expectation color space */
	if (((dst_par->format & 0x40) == 0x40) ^
		 (dst_par->csc == VSP_CSC_ON))
		exp_color = VSP_COLOR_YUV;
	else
		exp_color = VSP_COLOR_RGB;

	/* check expectation color space */
	if (src_info->color != VSP_COLOR_NO) {
		if (src_info->color != exp_color)
			return E_VSP_PARA_OUT_NOTCOLOR;
	}

	/* check rounding control parameter */
	if (dst_par->cbrm != VSP_CSC_ROUND_DOWN &&
	    dst_par->cbrm != VSP_CSC_ROUND_OFF)
		return E_VSP_PARA_OUT_CBRM;

	if (dst_par->abrm != VSP_CONVERSION_ROUNDDOWN &&
	    dst_par->abrm != VSP_CONVERSION_ROUNDING &&
	    dst_par->abrm != VSP_CONVERSION_THRESHOLD)
		return E_VSP_PARA_OUT_ABRM;

	if (dst_par->clmd != VSP_CLMD_NO &&
	    dst_par->clmd != VSP_CLMD_MODE1 &&
	    dst_par->clmd != VSP_CLMD_MODE2)
		return E_VSP_PARA_OUT_CLMD;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_sru_param
 * Description:	Check module parameter of SRU.
 * Returns:		0/E_VSP_PARA_NOSRU/E_VSP_PARA_SRU_INHSV
 *	E_VSP_PARA_SRU_MODE/E_VSP_PARA_SRU_WIDTH/E_VSP_PARA_SRU_HEIGHT
 *	E_VSP_PARA_SRU_PARAM/E_VSP_PARA_SRU_ENSCL/E_VSP_PARA_SRU_CONNECT
 ******************************************************************************/
static long vsp_ins_check_sru_param(
	struct vsp_ch_info *ch_info, struct vsp_sru_t *sru_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_sru_info *sru_info = &ch_info->sru_info;

	/* check pointer */
	if (!sru_param)
		return E_VSP_PARA_NOSRU;

	/* check input color space */
	if (src_info->color == VSP_COLOR_HSV)
		return E_VSP_PARA_SRU_INHSV;

	/* check input image size */
	if (src_info->width < 4)
		return E_VSP_PARA_SRU_WIDTH;

	if (src_info->height < 4)
		return E_VSP_PARA_SRU_HEIGHT;

	/* judge SRU module first flag */
	if (part_info->div_flag == 0)
		part_info->sru_first_flag = 1;
	else
		part_info->margin += 2;

	/* check mode */
	if (sru_param->mode == VSP_SRU_MODE1) {
		/* set partition flag */
		part_info->div_flag = 1;

		/* set margin size */
		if (part_info->margin < 2)
			part_info->margin = 2;
	} else if (sru_param->mode == VSP_SRU_MODE2) {
		src_info->width <<= 1;
		src_info->height <<= 1;

		/* set partition flag */
		part_info->div_flag = 2;

		/* set margin size */
		if (part_info->margin < 4)
			part_info->margin = 4;
		else
			part_info->margin <<= 1;
	} else {
		return E_VSP_PARA_SRU_MODE;
	}

	/* check output image size */
	if (src_info->height > 8190)
		return E_VSP_PARA_SRU_HEIGHT;

	/* check param */
	if (sru_param->param & (~(VSP_SRU_RCR | VSP_SRU_GY | VSP_SRU_BCB)))
		return E_VSP_PARA_SRU_PARAM;

	/* check enscl */
	if (sru_param->enscl >= VSP_SCL_LEVEL_MAX)
		return E_VSP_PARA_SRU_ENSCL;

	/* check connect parameter */
	if (sru_param->connect & ~VSP_SRU_USABLE_DPR)
		return E_VSP_PARA_SRU_CONNECT;

	/* check connect parameter */
	sru_info->val_dpr = vsp_ins_get_dpr_route(
		ch_info, sru_param->connect, sru_param->fxa);
	if (sru_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_SRU_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_uds_param
 * Description:	Check module parameter of UDS.
 * Returns:		0/E_VSP_PARA_NOUDS/E_VSP_PARA_UDS_INWIDTH
 *	E_VSP_PARA_UDS_INHEIGHT/E_VSP_PARA_UDS_XRATIO/E_VSP_PARA_UDS_INWIDTH
 *	E_VSP_PARA_UDS_YRATIO/E_VSP_PARA_UDS_AMD/E_VSP_PARA_UDS_CLIP/
 *	E_VSP_PARA_UDS_ALPHA/E_VSP_PARA_UDS_COMP/E_VSP_PARA_UDS_CONNECT
 ******************************************************************************/
static long vsp_ins_check_uds_param(
	struct vsp_ch_info *ch_info, struct vsp_uds_t *uds_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_uds_info *uds_info = &ch_info->uds_info;

	/* initialise */
	uds_info->val_ctrl = 0;
	uds_info->val_pass = 0;
	uds_info->val_clip = 0;
	uds_info->val_hphase = 0;
	uds_info->val_hszclip = 0;

	/* check pointer */
	if (!uds_param)
		return E_VSP_PARA_NOUDS;

	/* check input image size */
	if (src_info->width < 4)
		return E_VSP_PARA_UDS_INWIDTH;

	if (src_info->height < 4)
		return E_VSP_PARA_UDS_INHEIGHT;

	/* check scaling factor parameter */
	if (uds_param->x_ratio < VSP_UDS_SCALE_16_1) {
		return E_VSP_PARA_UDS_XRATIO;
	} else if (uds_param->x_ratio < VSP_UDS_SCALE_8_1) {
		/* set partition flag */
		part_info->div_flag = 8;
		part_info->margin <<= 4;
	} else if (uds_param->x_ratio < VSP_UDS_SCALE_4_1) {
		/* set partition flag */
		part_info->div_flag = 4;
		part_info->margin <<= 3;
	} else if (uds_param->x_ratio < VSP_UDS_SCALE_2_1) {
		/* set partition flag */
		part_info->div_flag = 2;
		part_info->margin <<= 2;
	} else if (uds_param->x_ratio < VSP_UDS_SCALE_1_1) {
		/* set partition flag */
		part_info->div_flag = 1;
		part_info->margin <<= 1;
	} else {
		/* set partition flag */
		part_info->div_flag = 1;
	}

	if (uds_param->y_ratio < VSP_UDS_SCALE_16_1)
		return E_VSP_PARA_UDS_YRATIO;

	/* check pixel count at scale-up parameter */
	if (uds_param->amd == VSP_AMD_NO) {
		/* calculate after scale */
		src_info->width = VSP_UDS_SCALE_AMD0(
			src_info->width, uds_param->x_ratio);
		src_info->height = VSP_UDS_SCALE_AMD0(
			src_info->height, uds_param->y_ratio);

	} else if (uds_param->amd == VSP_AMD) {
		uds_info->val_ctrl |= VSP_UDS_CTRL_AMD;

		/* calculate after scale */
		src_info->width = VSP_UDS_SCALE_AMD1(
			src_info->width, uds_param->x_ratio);
		src_info->height = VSP_UDS_SCALE_AMD1(
			src_info->height, uds_param->y_ratio);
	} else {
		return E_VSP_PARA_UDS_AMD;
	}

	/* check output image size */
	if (src_info->height > 8190)
		src_info->height = 8190;

	/* set clipping size */
	uds_info->val_clip = ((src_info->width << 16) | src_info->height);

	/* calculate passband parameter */
	uds_info->val_pass =
		vsp_ins_get_passband_bwidth(uds_param->x_ratio) << 16;
	uds_info->val_pass |= vsp_ins_get_passband_bwidth(uds_param->y_ratio);

	/* check scale-up/down of alpha plane parameter */
	if (uds_param->alpha == VSP_ALPHA_OFF) {
		/* no error */
	} else if (uds_param->alpha == VSP_ALPHA_ON) {
		uds_info->val_ctrl |= VSP_UDS_CTRL_AON;

		/* alpha output threshold comparison parameter */
		if (uds_param->clip == VSP_CLIP_OFF) {
			/* no error */
			/* dummy line */
		} else if (uds_param->clip == VSP_CLIP_ON) {
			uds_info->val_ctrl |= VSP_UDS_CTRL_ATHON;
		} else {
			return E_VSP_PARA_UDS_CLIP;
		}
	} else {
		return E_VSP_PARA_UDS_ALPHA;
	}

	if (uds_param->complement == VSP_COMPLEMENT_BIL) {
		/* no error */
	} else if (uds_param->complement == VSP_COMPLEMENT_NN) {
		/* when under quarter, cannot use nearest neighbor */
		if (uds_param->x_ratio > VSP_UDS_SCALE_1_4 ||
		    uds_param->y_ratio > VSP_UDS_SCALE_1_4)
			return E_VSP_PARA_UDS_COMP;

		uds_info->val_ctrl |= VSP_UDS_CTRL_NN;
	} else if (uds_param->complement == VSP_COMPLEMENT_BC) {
		/* when alpha scale up/down is performed, can't use multi tap */
		if (uds_param->alpha == VSP_ALPHA_ON)
			return E_VSP_PARA_UDS_COMP;

		uds_info->val_ctrl |= VSP_UDS_CTRL_BC;
	} else {
		return E_VSP_PARA_UDS_COMP;
	}

	/* check connect parameter */
	if (uds_param->connect & ~VSP_UDS_USABLE_DPR)
		return E_VSP_PARA_UDS_CONNECT;

	/* check connect parameter */
	uds_info->val_dpr =
		vsp_ins_get_dpr_route(ch_info, uds_param->connect, 0);
	if (uds_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_UDS_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_lut_param
 * Description:	Check module parameter of LUT.
 * Returns:		0/E_VSP_PARA_NOLUT/E_VSP_PARA_LUT_ADR
 *	E_VSP_PARA_LUT_SIZE/E_VSP_PARA_LUT_CONNECT
 ******************************************************************************/
static long vsp_ins_check_lut_param(
	struct vsp_ch_info *ch_info, struct vsp_lut_t *lut_param)
{
	struct vsp_lut_info *lut_info = &ch_info->lut_info;
	struct vsp_dl_t *lut = &lut_param->lut;

	/* check pointer */
	if (!lut_param)
		return E_VSP_PARA_NOLUT;

	/* check display list address */
	if (lut->hard_addr == 0)
		return E_VSP_PARA_LUT_ADR;

	/* check display list size */
	if (lut->tbl_num < 1 || lut->tbl_num > 256)
		return E_VSP_PARA_LUT_SIZE;

	/* check connect parameter */
	if (lut_param->connect & ~VSP_LUT_USABLE_DPR)
		return E_VSP_PARA_LUT_CONNECT;

	/* check connect parameter */
	lut_info->val_dpr = vsp_ins_get_dpr_route(
		ch_info, lut_param->connect, lut_param->fxa);
	if (lut_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_LUT_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_clu_param
 * Description:	Check module parameter of CLU.
 * Returns:		0/E_VSP_PARA_NOCLU/E_VSP_PARA_CLU_ADR
 *	E_VSP_PARA_CLU_SIZE/E_VSP_PARA_CLU_MODE/E_VSP_PARA_CLU_CONNECT
 ******************************************************************************/
static long vsp_ins_check_clu_param(
	struct vsp_ch_info *ch_info, struct vsp_clu_t *clu_param)
{
	struct vsp_clu_info *clu_info = &ch_info->clu_info;
	struct vsp_dl_t *clu = &clu_param->clu;

	/* initialise */
	clu_info->val_ctrl = 0;

	/* check pointer */
	if (!clu_param)
		return E_VSP_PARA_NOCLU;

	/* check display list address */
	if (clu->hard_addr == 0)
		return E_VSP_PARA_CLU_ADR;

	/* check mode */
	switch (clu_param->mode) {
	case VSP_CLU_MODE_3D_AUTO:
		/* enable Automatic table Address Increment */
		clu_info->val_ctrl |= VSP_CLU_CTRL_AAI;

		/* check display list size */
		if (clu->tbl_num < 1 || clu->tbl_num > 4913)
			return E_VSP_PARA_CLU_SIZE;

		clu_info->val_ctrl |=
			(VSP_CLU_CTRL_MVS | VSP_CLU_CTRL_3D | VSP_CLU_CTRL_EN);
		break;
	case VSP_CLU_MODE_3D:
		/* check display list size */
		if (clu->tbl_num < 2 || clu->tbl_num > 9826)
			return E_VSP_PARA_CLU_SIZE;

		if (clu->tbl_num % 2)
			return E_VSP_PARA_CLU_SIZE;

		clu_info->val_ctrl |=
			(VSP_CLU_CTRL_MVS | VSP_CLU_CTRL_3D | VSP_CLU_CTRL_EN);
		break;
	case VSP_CLU_MODE_2D_AUTO:
		/* enable Automatic table Address Increment */
		clu_info->val_ctrl |= VSP_CLU_CTRL_AAI;

		/* check display list size */
		if (clu->tbl_num < 1 || clu->tbl_num > 289)
			return E_VSP_PARA_CLU_SIZE;

		clu_info->val_ctrl |=
			(VSP_CLU_CTRL_MVS | VSP_CLU_CTRL_2D | VSP_CLU_CTRL_EN);
		break;
	case VSP_CLU_MODE_2D:
		/* check display list size */
		if (clu->tbl_num < 2 || clu->tbl_num > 578)
			return E_VSP_PARA_CLU_SIZE;

		if (clu->tbl_num % 2)
			return E_VSP_PARA_CLU_SIZE;

		clu_info->val_ctrl |=
			(VSP_CLU_CTRL_MVS | VSP_CLU_CTRL_2D | VSP_CLU_CTRL_EN);
		break;
	default:
		return E_VSP_PARA_CLU_MODE;
	}

	/* check connect parameter */
	if (clu_param->connect & ~VSP_CLU_USABLE_DPR)
		return E_VSP_PARA_CLU_CONNECT;

	/* check connect parameter */
	clu_info->val_dpr = vsp_ins_get_dpr_route(
		ch_info, clu_param->connect, clu_param->fxa);
	if (clu_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_CLU_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_hst_param
 * Description:	Check module parameter of HST.
 * Returns:		0/E_VSP_PARA_NOHST/E_VSP_PARA_HST_NOTRGB
 *	E_VSP_PARA_HST_CONNECT
 ******************************************************************************/
static long vsp_ins_check_hst_param(
	struct vsp_ch_info *ch_info, struct vsp_hst_t *hst_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_hst_info *hst_info = &ch_info->hst_info;

	/* check pointer */
	if (!hst_param)
		return E_VSP_PARA_NOHST;

	/* check input color space */
	if (src_info->color != VSP_COLOR_RGB)
		return E_VSP_PARA_HST_NOTRGB;

	/* check connect parameter */
	if (hst_param->connect & ~VSP_HST_USABLE_DPR)
		return E_VSP_PARA_HST_CONNECT;

	/* check connect parameter */
	hst_info->val_dpr = vsp_ins_get_dpr_route(
		ch_info, hst_param->connect, hst_param->fxa);
	if (hst_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_HST_CONNECT;

	/* change color space */
	src_info->color = VSP_COLOR_HSV;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_hsi_param
 * Description:	Check module parameter of HSI.
 * Returns:		0/E_VSP_PARA_NOHSI/E_VSP_PARA_HSI_NOTHSV
 *	E_VSP_PARA_HSI_CONNECT
 ******************************************************************************/
static long vsp_ins_check_hsi_param(
	struct vsp_ch_info *ch_info, struct vsp_hsi_t *hsi_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_hsi_info *hsi_info = &ch_info->hsi_info;

	/* check pointer */
	if (!hsi_param)
		return E_VSP_PARA_NOHSI;

	/* check input color space */
	if (src_info->color != VSP_COLOR_HSV)
		return E_VSP_PARA_HSI_NOTHSV;

	/* check connect parameter */
	if (hsi_param->connect & ~VSP_HSI_USABLE_DPR)
		return E_VSP_PARA_HSI_CONNECT;

	/* check connect parameter */
	hsi_info->val_dpr = vsp_ins_get_dpr_route(
		ch_info, hsi_param->connect, hsi_param->fxa);
	if (hsi_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_HSI_CONNECT;

	/* change color space */
	src_info->color = VSP_COLOR_RGB;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_blend_virtual_param
 * Description:	Check virtual parameter of BRU.
 * Returns:		0/E_VSP_PARA_VIR_ADR/E_VSP_PARA_VIR_WIDTH
 *				E_VSP_PARA_VIR_HEIGHT/E_VSP_PARA_VIR_PWD
 ******************************************************************************/
static long vsp_ins_check_blend_virtual_param(
	struct vsp_ch_info *ch_info, struct vsp_bld_vir_t *vir_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_bru_info *bru_info = &ch_info->bru_info;
	unsigned int x_pos, y_pos;

	/* check pointer */
	if (!vir_param)
		return E_VSP_PARA_VIR_ADR;

	/* check width */
	if (vir_param->width < 1 || vir_param->width > 8190)
		return E_VSP_PARA_VIR_WIDTH;

	/* check height */
	if (vir_param->height < 1 || vir_param->height > 8190)
		return E_VSP_PARA_VIR_HEIGHT;

	bru_info->val_vir_size = ((unsigned int)vir_param->width) << 16;
	bru_info->val_vir_size |= (unsigned int)vir_param->height;
	bru_info->val_vir_color = (unsigned int)vir_param->color;

	/* check pwd */
	if (vir_param->pwd == VSP_LAYER_PARENT) {
		x_pos = 0;
		y_pos = 0;
	} else if (vir_param->pwd == VSP_LAYER_CHILD) {
		x_pos = (unsigned int)vir_param->x_position;
		y_pos = (unsigned int)vir_param->y_position;
	} else {
		return E_VSP_PARA_VIR_PWD;
	}
	bru_info->val_vir_loc = (x_pos << 16) | y_pos;

	src_info->color = VSP_COLOR_NO;
	src_info->master = (unsigned int)vir_param->pwd;
	src_info->width = (unsigned int)vir_param->width;
	src_info->height = (unsigned int)vir_param->height;
	src_info->x_position = x_pos;
	src_info->y_position = y_pos;

	/* update source counter */
	src_info->rpf_ch = 14;		/* virtual source in BRU */
	ch_info->src_idx++;
	ch_info->src_cnt++;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_brs_virtual_param
 * Description:	Check virtual parameter of BRS.
 * Returns:		0/E_VSP_PARA_VIR_ADR/E_VSP_PARA_VIR_WIDTH
 *				E_VSP_PARA_VIR_HEIGHT/E_VSP_PARA_VIR_PWD
 ******************************************************************************/
static long vsp_ins_check_brs_virtual_param(
	struct vsp_ch_info *ch_info, struct vsp_bld_vir_t *vir_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_brs_info *brs_info = &ch_info->brs_info;
	unsigned int x_pos, y_pos;

	/* check pointer */
	if (!vir_param)
		return E_VSP_PARA_VIR_ADR;

	/* check width */
	if (vir_param->width < 1 || vir_param->width > 8190)
		return E_VSP_PARA_VIR_WIDTH;

	/* check height */
	if (vir_param->height < 1 || vir_param->height > 8190)
		return E_VSP_PARA_VIR_HEIGHT;

	brs_info->val_vir_size = ((unsigned int)vir_param->width) << 16;
	brs_info->val_vir_size |= (unsigned int)vir_param->height;
	brs_info->val_vir_color = (unsigned int)vir_param->color;

	/* check pwd */
	if (vir_param->pwd == VSP_LAYER_PARENT) {
		x_pos = 0;
		y_pos = 0;
	} else if (vir_param->pwd == VSP_LAYER_CHILD) {
		x_pos = (unsigned int)vir_param->x_position;
		y_pos = (unsigned int)vir_param->y_position;
	} else {
		return E_VSP_PARA_VIR_PWD;
	}
	brs_info->val_vir_loc = (x_pos << 16) | y_pos;

	src_info->color = VSP_COLOR_NO;
	src_info->master = (unsigned int)vir_param->pwd;
	src_info->width = (unsigned int)vir_param->width;
	src_info->height = (unsigned int)vir_param->height;
	src_info->x_position = x_pos;
	src_info->y_position = y_pos;

	/* update source counter */
	src_info->rpf_ch = 12;		/* virtual source in BRS */
	ch_info->src_idx++;
	ch_info->src_cnt++;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_blend_control_param
 * Description:	Check blend control parameter of BRU.
 * Returns:		0/E_VSP_PARA_BLEND_RBC/E_VSP_PARA_BLEND_CROP
 *				E_VSP_PARA_BLEND_AROP/E_VSP_PARA_BLEND_FORM
 *				E_VSP_PARA_BLEND_COEFX/E_VSP_PARA_BLEND_COEFY
 *				E_VSP_PARA_BLEND_AFORM/E_VSP_PARA_BLEND_ACOEFX
 *				E_VSP_PARA_BLEND_ACOEFY
 ******************************************************************************/
static long vsp_ins_check_blend_control_param(
	unsigned int *bru_ctrl,
	unsigned int *bru_bld,
	unsigned char dst_layer,
	unsigned char src_layer,
	struct vsp_bld_ctrl_t *ctrl_param)
{
	/* initialise */
	if (dst_layer != VSP_LAY_NO)
		*bru_ctrl = ((unsigned int)(dst_layer - 1)) << 20;
	else
		*bru_ctrl = 0;

	*bru_bld = 0;

	if (ctrl_param && src_layer != VSP_LAY_NO) {
		/* check rbc */
		if (ctrl_param->rbc != VSP_RBC_ROP &&
		    ctrl_param->rbc != VSP_RBC_BLEND)
			return E_VSP_PARA_BLEND_RBC;

		/* check crop */
		if (ctrl_param->crop >= VSP_IROP_MAX)
			return E_VSP_PARA_BLEND_CROP;

		/* check arop */
		if (ctrl_param->arop >= VSP_IROP_MAX)
			return E_VSP_PARA_BLEND_AROP;

		/* check blend_formula */
		if (ctrl_param->blend_formula != VSP_FORM_BLEND0 &&
		    ctrl_param->blend_formula != VSP_FORM_BLEND1)
			return E_VSP_PARA_BLEND_FORM;

		/* check blend_corfx */
		if (ctrl_param->blend_coefx >= VSP_COEFFICIENT_BLENDX_MAX)
			return E_VSP_PARA_BLEND_COEFX;

		/* check blend_corfy */
		if (ctrl_param->blend_coefy >= VSP_COEFFICIENT_BLENDY_MAX)
			return E_VSP_PARA_BLEND_COEFY;

		/* check aformula */
		if (ctrl_param->aformula != VSP_FORM_ALPHA0 &&
		    ctrl_param->aformula != VSP_FORM_ALPHA1)
			return E_VSP_PARA_BLEND_AFORM;

		/* check acoefx */
		if (ctrl_param->acoefx >= VSP_COEFFICIENT_ALPHAX_MAX)
			return E_VSP_PARA_BLEND_ACOEFX;

		/* check acoefy */
		if (ctrl_param->acoefy >= VSP_COEFFICIENT_ALPHAY_MAX)
			return E_VSP_PARA_BLEND_ACOEFY;

		*bru_ctrl |= ((unsigned int)ctrl_param->rbc) << 31;
		*bru_ctrl |= ((unsigned int)(src_layer - 1)) << 16;
		*bru_ctrl |= ((unsigned int)ctrl_param->crop) << 4;
		*bru_ctrl |=  (unsigned int)ctrl_param->arop;

		*bru_bld |= ((unsigned int)ctrl_param->blend_formula) << 31;
		*bru_bld |= ((unsigned int)ctrl_param->blend_coefx) << 28;
		*bru_bld |= ((unsigned int)ctrl_param->blend_coefy) << 24;
		*bru_bld |= ((unsigned int)ctrl_param->aformula) << 23;
		*bru_bld |= ((unsigned int)ctrl_param->acoefx) << 20;
		*bru_bld |= ((unsigned int)ctrl_param->acoefy) << 16;
		*bru_bld |= ((unsigned int)ctrl_param->acoefx_fix) << 8;
		*bru_bld |=  (unsigned int)ctrl_param->acoefy_fix;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_rop_unit_param
 * Description:	Check raster operation unit parameter of BRU.
 * Returns:		0/E_VSP_PARA_ROP_CROP/E_VSP_PARA_ROP_AROP
 ******************************************************************************/
static long vsp_ins_check_rop_unit_param(
	unsigned int *bru_rop,
	unsigned char layer,
	struct vsp_bld_rop_t *rop_param)
{
	/* initialise */
	if (layer != VSP_LAY_NO)
		*bru_rop = ((unsigned int)(layer - 1)) << 20;
	else
		*bru_rop = 0;

	if (rop_param) {
		/* check crop */
		if (rop_param->crop >= VSP_IROP_MAX)
			return E_VSP_PARA_ROP_CROP;

		/* check arop */
		if (rop_param->arop >= VSP_IROP_MAX)
			return E_VSP_PARA_ROP_AROP;

		*bru_rop |= ((unsigned int)rop_param->crop) << 4;
		*bru_rop |=  (unsigned int)rop_param->arop;
	};

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_bru_param
 * Description:	Check module parameter of BRU.
 * Returns:		0/E_VSP_PARA_NOBRU/E_VSP_PARA_BRU_LAYORDER
 *	E_VSP_PARA_BRU_ADIV/E_VSP_PARA_BRU_DITH_BPP/E_VSP_PARA_BRU_DITH_MODE
 *	E_VSP_PARA_BRU_CONNECT
 *	return of vsp_ins_check_input_color_space_of_bru()
 *	return of vsp_ins_check_blend_virtual_param()
 *	return of vsp_ins_check_blend_control_param()
 ******************************************************************************/
static long vsp_ins_check_bru_param(
	struct vsp_ch_info *ch_info, struct vsp_bru_t *bru_param)
{
	struct vsp_bru_info *bru_info = &ch_info->bru_info;

	unsigned long order_tmp;
	unsigned long order_bit;
	unsigned char layer[VSP_BROP_MAX];

	struct vsp_bld_dither_t **dith_unit;

	unsigned char i;
	long ercd;

	/* initialise */
	bru_info->val_inctrl = 0;

	bru_info->val_vir_loc = 0;
	bru_info->val_vir_color = 0;
	bru_info->val_vir_size = 0;

	/* check pointer */
	if (!bru_param)
		return E_VSP_PARA_NOBRU;

	/* check input color space */
	ercd = vsp_ins_check_input_color_space_of_bru(ch_info);
	if (ercd)
		return ercd;

	/* check lay_order */
	order_tmp = bru_param->lay_order;
	order_bit = 0;
	for (i = 0; i < VSP_BROP_MAX; i++) {
		layer[i] = (unsigned char)(order_tmp & 0xf);
		order_bit |= (0x00000001UL << layer[i]); /* layer max 15 */

		order_tmp >>= 4;
	}
	order_bit >>= 1;

	if (ch_info->bru_cnt < 5) {
		if ((order_bit & 0xf) !=
				((0x00000001UL << ch_info->bru_cnt) - 1))
			return E_VSP_PARA_BRU_LAYORDER;
	} else {
		if ((order_bit & 0x2f) != 0x2f)
			return E_VSP_PARA_BRU_LAYORDER;
	}

	if (layer[VSP_BROP_DST_A] == VSP_LAY_NO)
		return E_VSP_PARA_BRU_LAYORDER;

	/* check adiv */
	if (bru_param->adiv != VSP_DIVISION_OFF &&
	    bru_param->adiv != VSP_DIVISION_ON)
		return E_VSP_PARA_BRU_ADIV;

	bru_info->val_inctrl = ((unsigned int)bru_param->adiv) << 28;

	/* set dithering parameter */
	dith_unit = &bru_param->dither_unit[0];
	for (i = 0; i < ch_info->bru_cnt; i++) {
		if (!(*dith_unit)) {
			/* disable dither unit */
			/* no check */
		} else {
			/* enable dither unit */
			if ((*dith_unit)->mode == VSP_DITH_COLOR_REDUCTION) {
				if (((*dith_unit)->bpp != VSP_DITH_OFF) &&
				    ((*dith_unit)->bpp != VSP_DITH_18BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_16BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_15BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_12BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_8BPP)) {
					return E_VSP_PARA_BRU_DITH_BPP;
				}
				/* set color reduction dither */
				bru_info->val_inctrl |= (0x1 << (i + 16));
			} else if ((*dith_unit)->mode ==
					VSP_DITH_ORDERED_DITHER) {
				if ((*dith_unit)->bpp != VSP_DITH_18BPP)
					return E_VSP_PARA_BRU_DITH_BPP;

				/* set ordered dither */
				if (i != 4) {
					bru_info->val_inctrl |=
						(0x8 << (i * 4));
				} else {
					bru_info->val_inctrl |= (0x8 << 24);
				}
			} else {
				return E_VSP_PARA_BRU_DITH_MODE;
			}

			/* set number of color after dithering */
			if (i != 4) {
				bru_info->val_inctrl |=
					(((unsigned int)
						(*dith_unit)->bpp) << (i * 4));
			} else {
				bru_info->val_inctrl |=
					(((unsigned int)
						(*dith_unit)->bpp) << 24);
			}
		}

		dith_unit++;
	}

	/* check blend virtual parameter */
	if (order_bit & 0x10) {
		ercd = vsp_ins_check_blend_virtual_param(
			ch_info, bru_param->blend_virtual);
		if (ercd)
			return ercd;
	}

	/* check blend control UNIT A parameter */
	ercd = vsp_ins_check_blend_control_param(
		&bru_info->val_ctrl[0],
		&bru_info->val_bld[0],
		layer[VSP_BROP_DST_A],
		layer[VSP_BROP_SRC_A],
		bru_param->blend_unit_a
	);
	if (ercd)
		return ercd;

	/* check blend ROP UNIT parameter */
	ercd = vsp_ins_check_rop_unit_param(
		&bru_info->val_rop,
		layer[VSP_BROP_DST_R],
		bru_param->rop_unit
	);
	if (ercd)
		return ercd;

	/* check blend control UNIT B parameter */
	if (layer[VSP_BROP_DST_R] != VSP_LAY_NO) {
		ercd = vsp_ins_check_blend_control_param(
			&bru_info->val_ctrl[1],
			&bru_info->val_bld[1],
			1,
			1,
			bru_param->blend_unit_b
		);
		if (ercd)
			return ercd;
	} else {
		bru_info->val_ctrl[1] = 0;
		bru_info->val_bld[1] = 0;
	}

	/* check blend control UNIT C parameter */
	ercd = vsp_ins_check_blend_control_param(
		&bru_info->val_ctrl[2],
		&bru_info->val_bld[2],
		VSP_LAY_NO,
		layer[VSP_BROP_SRC_C],
		bru_param->blend_unit_c
	);
	if (ercd)
		return ercd;

	/* check blend control UNIT D parameter */
	ercd = vsp_ins_check_blend_control_param(
		&bru_info->val_ctrl[3],
		&bru_info->val_bld[3],
		VSP_LAY_NO,
		layer[VSP_BROP_SRC_D],
		bru_param->blend_unit_d
	);
	if (ercd)
		return ercd;

	/* check blend control UNIT E parameter */
	ercd = vsp_ins_check_blend_control_param(
		&bru_info->val_ctrl[4],
		&bru_info->val_bld[4],
		VSP_LAY_NO,
		layer[VSP_BROP_SRC_E],
		bru_param->blend_unit_e
	);
	if (ercd)
		return ercd;

	/* check connect parameter */
	if (bru_param->connect & ~VSP_BRU_USABLE_DPR)
		return E_VSP_PARA_BRU_CONNECT;

	/* check connect parameter */
	bru_info->val_dpr =
		vsp_ins_get_dpr_route(ch_info, bru_param->connect, 0);
	if (bru_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_BRU_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_brs_param
 * Description:	Check module parameter of BRS.
 * Returns:		0/E_VSP_PARA_NOBRS/E_VSP_PARA_BRS_LAYORDER
 *	E_VSP_PARA_BRS_ADIV/E_VSP_PARA_BRS_DITH_BPP/E_VSP_PARA_BRS_DITH_MODE
 *	E_VSP_PARA_BRS_CONNECT
 *	return of vsp_ins_check_input_color_space_of_brs()
 *	return of vsp_ins_check_brs_virtual_param()
 *	return of vsp_ins_check_blend_control_param()
 ******************************************************************************/
static long vsp_ins_check_brs_param(
	struct vsp_ch_info *ch_info, struct vsp_brs_t *brs_param)
{
	struct vsp_brs_info *brs_info = &ch_info->brs_info;

	unsigned long order_tmp;
	unsigned long order_bit;
	unsigned char layer[VSP_BRS_MAX];

	struct vsp_bld_dither_t **dith_unit;

	unsigned char i;
	long ercd;

	/* initialise */
	brs_info->val_inctrl = 0;

	brs_info->val_vir_loc = 0;
	brs_info->val_vir_color = 0;
	brs_info->val_vir_size = 0;

	/* check pointer */
	if (!brs_param)
		return E_VSP_PARA_NOBRS;

	/* check input color space */
	ercd = vsp_ins_check_input_color_space_of_brs(ch_info);
	if (ercd)
		return ercd;

	/* check lay_order */
	order_tmp = brs_param->lay_order;
	order_bit = 0;
	for (i = 0; i < VSP_BRS_MAX; i++) {
		layer[i] = (unsigned char)(order_tmp & 0xf);
		order_bit |= (0x00000001UL << layer[i]); /* layer max 15 */

		order_tmp >>= 4;
	}
	order_bit >>= 1;

	if ((order_bit & 0xf) !=
			((0x00000001UL << ch_info->brs_cnt) - 1))
		return E_VSP_PARA_BRS_LAYORDER;

	if (layer[VSP_BRS_DST_A] == VSP_LAY_NO)
		return E_VSP_PARA_BRS_LAYORDER;

	/* check adiv */
	if (brs_param->adiv != VSP_DIVISION_OFF &&
	    brs_param->adiv != VSP_DIVISION_ON)
		return E_VSP_PARA_BRS_ADIV;

	brs_info->val_inctrl = ((unsigned int)brs_param->adiv) << 28;

	/* set dithering parameter */
	dith_unit = &brs_param->dither_unit[0];
	for (i = 0; i < ch_info->brs_cnt; i++) {
		if (!(*dith_unit)) {
			/* disable dither unit */
			/* no check */
		} else {
			/* enable dither unit */
			if ((*dith_unit)->mode == VSP_DITH_COLOR_REDUCTION) {
				if (((*dith_unit)->bpp != VSP_DITH_OFF) &&
				    ((*dith_unit)->bpp != VSP_DITH_18BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_16BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_15BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_12BPP) &&
				    ((*dith_unit)->bpp != VSP_DITH_8BPP)) {
					return E_VSP_PARA_BRS_DITH_BPP;
				}
				/* set color reduction dither */
				brs_info->val_inctrl |= (0x1 << (i + 16));
			} else if ((*dith_unit)->mode ==
					VSP_DITH_ORDERED_DITHER) {
				if ((*dith_unit)->bpp != VSP_DITH_18BPP)
					return E_VSP_PARA_BRS_DITH_BPP;

				/* set ordered dither */
				brs_info->val_inctrl |=
					(0x8 << (i * 4));
			} else {
				return E_VSP_PARA_BRS_DITH_MODE;
			}

			/* set number of color after dithering */
			brs_info->val_inctrl |=
				(((unsigned int)
					(*dith_unit)->bpp) << (i * 4));
		}

		dith_unit++;
	}

	/* check blend virtual parameter */
	if (order_bit & 0x10) {
		ercd = vsp_ins_check_brs_virtual_param(
			ch_info, brs_param->blend_virtual);
		if (ercd)
			return ercd;
	}

	/* check blend control UNIT A parameter */
	ercd = vsp_ins_check_blend_control_param(
		&brs_info->val_ctrl[0],
		&brs_info->val_bld[0],
		layer[VSP_BRS_DST_A],
		layer[VSP_BRS_SRC_A],
		brs_param->blend_unit_a
	);
	if (ercd)
		return ercd;

	/* check blend control UNIT B parameter */
	ercd = vsp_ins_check_blend_control_param(
		&brs_info->val_ctrl[1],
		&brs_info->val_bld[1],
		VSP_LAY_NO,
		layer[VSP_BRS_SRC_B],
		brs_param->blend_unit_b
	);
	if (ercd)
		return ercd;

	/* check connect parameter */
	if (brs_param->connect & ~VSP_BRS_USABLE_DPR)
		return E_VSP_PARA_BRS_CONNECT;

	/* check connect parameter */
	brs_info->val_dpr =
		vsp_ins_get_dpr_route(ch_info, brs_param->connect, 0);
	if (brs_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_BRS_CONNECT;

	/* add BRSSEL */
	brs_info->val_dpr |= VSP_DPR_ROUTE_BRSSEL;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_hgo_param
 * Description:	Check module parameter of HGO.
 * Returns:		0/E_VSP_PARA_NOHGO/E_VSP_PARA_HGO_ADR
 *	E_VSP_PARA_HGO_WIDTH/E_VSP_PARA_HGO_HEIGHT/E_VSP_PARA_HGO_XOFFSET
 *	E_VSP_PARA_HGO_YOFFSET/E_VSP_PARA_HGO_BINMODE/E_VSP_PARA_HGO_MAXRGB
 *	E_VSP_PARA_HGO_STEP/E_VSP_PARA_HGO_XSKIP/E_VSP_PARA_HGO_YSKIP
 *	E_VSP_PARA_HGO_SMMPT
 ******************************************************************************/
static long vsp_ins_check_hgo_param(
	struct vsp_ch_info *ch_info, struct vsp_hgo_t *hgo_param)
{
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_hgo_info *hgo_info = &ch_info->hgo_info;

	if (!hgo_param)
		return E_VSP_PARA_NOHGO;

	/* check address pointer */
	if (hgo_param->hard_addr == 0 ||
	    !hgo_param->virt_addr)
		return E_VSP_PARA_HGO_ADR;

	if ((hgo_param->hard_addr & 0xff) ||
	    (((unsigned long)hgo_param->virt_addr) & 0xff))
		return E_VSP_PARA_HGO_ADR;

	hgo_info->virt_addr = (unsigned int *)hgo_param->virt_addr;
	hgo_info->val_hard_addr = hgo_param->hard_addr;

	/* check detection window area */
	if (hgo_param->width < 1 || hgo_param->width > 8190)
		return E_VSP_PARA_HGO_WIDTH;

	if (hgo_param->height < 1 || hgo_param->height > 8190)
		return E_VSP_PARA_HGO_HEIGHT;

	if ((hgo_param->width + hgo_param->x_offset) > 8190)
		return E_VSP_PARA_HGO_XOFFSET;

	if ((hgo_param->height + hgo_param->y_offset) > 8190)
		return E_VSP_PARA_HGO_YOFFSET;

	hgo_info->val_offset = ((unsigned int)hgo_param->x_offset) << 16;
	hgo_info->val_offset |= (unsigned int)hgo_param->y_offset;

	hgo_info->val_size = ((unsigned int)hgo_param->width) << 16;
	hgo_info->val_size |= (unsigned int)hgo_param->height;

	/* check binary mode */
	if (hgo_param->binary_mode != VSP_STRAIGHT_BINARY &&
	    hgo_param->binary_mode != VSP_OFFSET_BINARY)
		return E_VSP_PARA_HGO_BINMODE;
	hgo_info->val_mode = (unsigned int)hgo_param->binary_mode;

	/* check maxrgb mode */
	if (hgo_param->maxrgb_mode != VSP_MAXRGB_OFF &&
	    hgo_param->maxrgb_mode != VSP_MAXRGB_ON)
		return E_VSP_PARA_HGO_MAXRGB;
	hgo_info->val_mode |= (unsigned int)hgo_param->maxrgb_mode;

	/* check step mode */
	if (hgo_param->step_mode != VSP_STEP_64 &&
	    hgo_param->step_mode != VSP_STEP_256)
		return E_VSP_PARA_HGO_STEP;
	hgo_info->val_mode |= ((unsigned int)hgo_param->step_mode) << 8;

	/* check skip mode */
	if (hgo_param->x_skip != VSP_SKIP_OFF &&
	    hgo_param->x_skip != VSP_SKIP_1_2 &&
	    hgo_param->x_skip != VSP_SKIP_1_4)
		return E_VSP_PARA_HGO_XSKIP;
	hgo_info->val_mode |= ((unsigned int)hgo_param->x_skip) << 2;

	if (hgo_param->y_skip != VSP_SKIP_OFF &&
	    hgo_param->y_skip != VSP_SKIP_1_2 &&
	    hgo_param->y_skip != VSP_SKIP_1_4)
		return E_VSP_PARA_HGO_YSKIP;
	hgo_info->val_mode |= (unsigned int)hgo_param->y_skip;

	/* set register reset (1st partition) */
	hgo_info->val_regrst = VSP_HGO_REGRST_RCPART | VSP_HGO_REGRST_RCLEA;

	/* check sampling point */
	hgo_info->val_dpr = vsp_ins_get_dpr_smppt(
		ch_info, (unsigned int)hgo_param->sampling);
	if (hgo_info->val_dpr == VSP_DPR_SMPPT_NOT_USE)
		return E_VSP_PARA_HGO_SMMPT;

	/* save sampling point */
	part_info->hgo_smppt = hgo_info->val_dpr;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_hue_area_param
 * Description:	Check hue area parameter of HGT.
 * Returns:		0/E_VSP_PARA_HGT_AREA
 ******************************************************************************/
static long vsp_ins_check_hue_area_param(struct vsp_hue_area_t *hue_area)
{
	unsigned char val = hue_area[0].upper;
	int i;

	for (i = 1; i < 6; i++) {
		if (val > hue_area[i].lower)
			return E_VSP_PARA_HGT_AREA;

		if (hue_area[i].lower > hue_area[i].upper)
			return E_VSP_PARA_HGT_AREA;

		val = hue_area[i].upper;
	}

	if (hue_area[0].upper < hue_area[0].lower &&
	    hue_area[0].lower < hue_area[5].upper) {
		return E_VSP_PARA_HGT_AREA;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_hgt_param
 * Description:	Check module parameter of HGT.
 * Returns:		0/E_VSP_PARA_NOHGT/E_VSP_PARA_HGT_ADR
 *	E_VSP_PARA_HGT_WIDTH/E_VSP_PARA_HGT_HEIGHT/E_VSP_PARA_HGT_XOFFSET
 *	E_VSP_PARA_HGT_YOFFSET/E_VSP_PARA_HGO_XSKIP/E_VSP_PARA_HGO_YSKIP
 *	E_VSP_PARA_HGT_SMMPT
 *	return of vsp_ins_check_hue_area_param()
 ******************************************************************************/
static long vsp_ins_check_hgt_param(
	struct vsp_ch_info *ch_info, struct vsp_hgt_t *hgt_param)
{
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_hgt_info *hgt_info = &ch_info->hgt_info;

	long ercd;

	if (!hgt_param)
		return E_VSP_PARA_NOHGT;

	/* check address pointer */
	if (hgt_param->hard_addr == 0 ||
	    !hgt_param->virt_addr)
		return E_VSP_PARA_HGT_ADR;

	if ((hgt_param->hard_addr & 0xff) ||
	    (((unsigned long)hgt_param->virt_addr) & 0xff))
		return E_VSP_PARA_HGT_ADR;

	hgt_info->virt_addr = (unsigned int *)hgt_param->virt_addr;
	hgt_info->val_hard_addr = hgt_param->hard_addr;

	/* check detection window area */
	if (hgt_param->width < 1 || hgt_param->width > 8190)
		return E_VSP_PARA_HGT_WIDTH;

	if (hgt_param->height < 1 || hgt_param->height > 8190)
		return E_VSP_PARA_HGT_HEIGHT;

	if ((hgt_param->width + hgt_param->x_offset) > 8190)
		return E_VSP_PARA_HGT_XOFFSET;

	if ((hgt_param->height + hgt_param->y_offset) > 8190)
		return E_VSP_PARA_HGT_YOFFSET;

	hgt_info->val_offset = ((unsigned int)hgt_param->x_offset) << 16;
	hgt_info->val_offset |= (unsigned int)hgt_param->y_offset;

	hgt_info->val_size = ((unsigned int)hgt_param->width) << 16;
	hgt_info->val_size |= (unsigned int)hgt_param->height;

	/* check hue area */
	ercd = vsp_ins_check_hue_area_param(&hgt_param->area[0]);
	if (ercd)
		return ercd;

	/* check skip mode */
	if (hgt_param->x_skip != VSP_SKIP_OFF &&
	    hgt_param->x_skip != VSP_SKIP_1_2 &&
	    hgt_param->x_skip != VSP_SKIP_1_4) {
		return E_VSP_PARA_HGO_XSKIP;
	}

	if (hgt_param->y_skip != VSP_SKIP_OFF &&
	    hgt_param->y_skip != VSP_SKIP_1_2 &&
	    hgt_param->y_skip != VSP_SKIP_1_4) {
		return E_VSP_PARA_HGO_YSKIP;
	}

	/* set register reset (1st partition) */
	hgt_info->val_regrst = VSP_HGT_REGRST_RCPART | VSP_HGT_REGRST_RCLEA;

	/* check sampling point */
	hgt_info->val_dpr = vsp_ins_get_dpr_smppt(
		ch_info, (unsigned int)hgt_param->sampling);
	if (hgt_info->val_dpr == VSP_DPR_SMPPT_NOT_USE)
		return E_VSP_PARA_HGT_SMMPT;

	/* save sampling point */
	part_info->hgt_smppt = hgt_info->val_dpr;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_shp_param
 * Description:	Check module parameter of SHP.
 * Returns:		0/E_VSP_PARA_NOSHP/E_VSP_PARA_SHP_CONNECT
 ******************************************************************************/
static long vsp_ins_check_shp_param(
	struct vsp_ch_info *ch_info, struct vsp_shp_t *shp_param)
{
	struct vsp_src_info *src_info = &ch_info->src_info[ch_info->src_idx];
	struct vsp_shp_info *shp_info = &ch_info->shp_info;

	/* check pointer */
	if (!shp_param)
		return E_VSP_PARA_NOSHP;

	/* check input color space */
	if (src_info->color != VSP_COLOR_YUV)
		return E_VSP_PARA_SHP_INYUV;

	/* check image size */
	if (src_info->width < 4)
		return E_VSP_PARA_SHP_WIDTH;

	if (src_info->height < 4)
		return E_VSP_PARA_SHP_HEIGHT;

	/* check mode */
	if (shp_param->mode != VSP_SHP_SHARP &&
	    shp_param->mode != VSP_SHP_UNSHARP) {
		return E_VSP_PARA_SHP_MODE;
	}

	/* check connect parameter */
	if (shp_param->connect & ~VSP_SHP_USABLE_DPR)
		return E_VSP_PARA_SHP_CONNECT;

	/* check connect parameter */
	shp_info->val_dpr = vsp_ins_get_dpr_route(
		ch_info, shp_param->connect, shp_param->fxa);
	if (shp_info->val_dpr == VSP_DPR_ROUTE_NOT_USE)
		return E_VSP_PARA_SHP_CONNECT;

	if (ch_info->part_info.margin < 4)
		ch_info->part_info.margin = 4;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_dl_param
 * Description:	Check display list parameter.
 * Returns:		0/E_VSP_PARA_DL_ADR/E_VSP_PARA_DL_SIZE
 ******************************************************************************/
static long vsp_ins_check_dl_param(
	struct vsp_ch_info *ch_info, struct vsp_dl_t *dl_param)
{
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	/* check address */
	if (dl_param->hard_addr == 0 ||
	    !dl_param->virt_addr)
		return E_VSP_PARA_DL_ADR;

	/* check size */
	if ((dl_param->tbl_num <
			((VSP_DL_HEAD_SIZE + VSP_DL_BODY_SIZE) >> 3)) ||
		dl_param->tbl_num > 16383)
		return E_VSP_PARA_DL_SIZE;

	wpf_info->val_dl_addr = dl_param->hard_addr;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_module_param
 * Description:	Check module parameter.
 * Returns:		0/E_VSP_PARA_CONNECT
 *	return of each of modules parameter function.
 ******************************************************************************/
static long vsp_ins_check_module_param(
	struct vsp_ch_info *ch_info, struct vsp_ctrl_t *ctrl_param)
{
	unsigned long processing_module;

	long ercd;

	do {
		processing_module = ch_info->next_module;

		/* check duplicate */
		if (processing_module != VSP_BRU_USE &&
		    processing_module != VSP_BRS_USE) {
			if (ch_info->reserved_module & processing_module)
				return E_VSP_PARA_CONNECT;
		}

		switch (processing_module) {
		case VSP_SRU_USE:
			/* check sru parameter */
			ercd = vsp_ins_check_sru_param(
				ch_info, ctrl_param->sru);
			if (ercd)
				return ercd;
			break;
		case VSP_UDS_USE:
			/* check uds parameter */
			ercd = vsp_ins_check_uds_param(
				ch_info, ctrl_param->uds);
			if (ercd)
				return ercd;
			break;
		case VSP_LUT_USE:
			/* check lut parameter */
			ercd = vsp_ins_check_lut_param(
				ch_info, ctrl_param->lut);
			if (ercd)
				return ercd;
			break;
		case VSP_CLU_USE:
			/* check clu parameter */
			ercd = vsp_ins_check_clu_param(
				ch_info, ctrl_param->clu);
			if (ercd)
				return ercd;
			break;
		case VSP_HST_USE:
			/* check hst parameter */
			ercd = vsp_ins_check_hst_param(
				ch_info, ctrl_param->hst);
			if (ercd)
				return ercd;
			break;
		case VSP_HSI_USE:
			/* check hsi parameter */
			ercd = vsp_ins_check_hsi_param(
				ch_info, ctrl_param->hsi);
			if (ercd)
				return ercd;
			break;
		case VSP_SHP_USE:
			/* check shp parameter */
			ercd = vsp_ins_check_shp_param(
				ch_info, ctrl_param->shp);
			if (ercd)
				return ercd;
			break;
		case VSP_BRU_USE:
		default:	/* WPF */
			processing_module = 0;
			break;
		}

		ch_info->reserved_module |= processing_module;
	} while (processing_module);

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_connection_module_from_rpf
 * Description:	Check connection module parameter from RPF.
 * Returns:		0/E_VSP_PARA_RPFNUM/E_VSP_PARA_CTRLPAR
 *	E_VSP_PARA_RPFORDER/E_VSP_PARA_CONNECT
 *	return of vsp_ins_check_rpf_param()
 *	return of vsp_ins_check_module_param()
 ******************************************************************************/
static long vsp_ins_check_connection_module_from_rpf(
	struct vsp_prv_data *prv, struct vsp_start_t *param)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];

	unsigned char rpf_ch;
	unsigned char i;

	unsigned long order = param->rpf_order;
	unsigned long module;

	long ercd;

	/* check RPF number */
	if (param->rpf_num > VSP_RPF_MAX)
		return E_VSP_PARA_RPFNUM;

	/* check module parameter pointer */
	if (!param->ctrl_par)
		return E_VSP_PARA_CTRLPAR;

	for (i = 0; i < param->rpf_num; i++) {
		/* inisialise */
		ch_info->next_module = 0;

		/* get RPF channel */
		rpf_ch = (unsigned char)(order & 0xf);
		module = (0x00000001UL << rpf_ch);

		/* check valid RPF channel */
		if (((unsigned long)prv->rdata.usable_rpf & module) != module)
			return E_VSP_PARA_RPFORDER;

		ch_info->reserved_rpf |= module;

		/* check RPF parameter */
		ercd = vsp_ins_check_rpf_param(
			ch_info, &ch_info->rpf_info[rpf_ch], param->src_par[i]);
		if (ercd)
			return ercd;

		/* check module parameter */
		ercd = vsp_ins_check_module_param(ch_info, param->ctrl_par);
		if (ercd)
			return ercd;

		/* update source counter */
		ch_info->src_info[ch_info->src_idx].rpf_ch = rpf_ch;
		ch_info->src_idx++;
		ch_info->src_cnt++;

		order >>= 4;
	}

	/* check connection */
	if (ch_info->wpf_cnt > 0 && ch_info->bru_cnt > 0)
		return E_VSP_PARA_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_connection_module_from_bru
 * Description:	Check connection module parameter from BRU or BRS.
 * Returns:		0/E_VSP_PARA_CONNECT
 *	return of vsp_ins_check_brs_param()
 *	return of vsp_ins_check_bru_param()
 *	return of vsp_ins_check_master_layer()
 *	return of vsp_ins_check_module_param()
 ******************************************************************************/
static long vsp_ins_check_connection_module_from_bru(
	struct vsp_ch_info *ch_info, struct vsp_start_t *param)
{
	long ercd;

	/* initialise */
	ch_info->next_module = 0;

	/* check module parameter pointer */
	/* already checked */

	if (param->use_module & VSP_BRS_USE) {
		/* check BRS parameter */
		ercd = vsp_ins_check_brs_param(ch_info, param->ctrl_par->brs);
		if (ercd)
			return ercd;

		ch_info->reserved_module |= VSP_BRS_USE;
	}

	if (param->use_module & VSP_BRU_USE) {
		/* check BRU parameter */
		ercd = vsp_ins_check_bru_param(ch_info, param->ctrl_par->bru);
		if (ercd)
			return ercd;

		ch_info->reserved_module |= VSP_BRU_USE;
	}

	/* check master layer */
	ercd = vsp_ins_check_master_layer(ch_info);
	if (ercd)
		return ercd;

	/* check module parameter */
	ercd = vsp_ins_check_module_param(ch_info, param->ctrl_par);
	if (ercd)
		return ercd;

	/* check connection */
	if (ch_info->wpf_cnt != 1)
		return E_VSP_PARA_CONNECT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_independent_module
 * Description:	Check independent module parameter.
 * Returns:		0
 *	return of vsp_ins_check_hgo_param()
 *	return of vsp_ins_check_hgt_param()
 ******************************************************************************/
static long vsp_ins_check_independent_module(
	struct vsp_ch_info *ch_info, struct vsp_start_t *param)
{
	long ercd;

	/* check module parameter pointer */
	/* already checked */

	/* check HGO parameter */
	if (param->use_module & VSP_HGO_USE) {
		ercd = vsp_ins_check_hgo_param(ch_info, param->ctrl_par->hgo);
		if (ercd)
			return ercd;

		ch_info->reserved_module |= VSP_HGO_USE;
	}

	/* check HGT parameter */
	if (param->use_module & VSP_HGT_USE) {
		ercd = vsp_ins_check_hgt_param(ch_info, param->ctrl_par->hgt);
		if (ercd)
			return ercd;

		ch_info->reserved_module |= VSP_HGT_USE;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_output_module
 * Description:	Check destination module parameter.
 * Returns:		0
 *	return of vsp_ins_check_wpf_param()
 *	return of vsp_ins_check_dl_param()
 ******************************************************************************/
static long vsp_ins_check_output_module(
	struct vsp_ch_info *ch_info, struct vsp_start_t *param)
{
	long ercd;

	/* check WPF parameter */
	ercd = vsp_ins_check_wpf_param(ch_info, param->dst_par);
	if (ercd)
		return ercd;

	/* check DL parameter */
	ercd = vsp_ins_check_dl_param(ch_info, &param->dl_par);
	if (ercd)
		return ercd;

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_partition
 * Description:	Check partition parameter.
 * Returns:		0/E_VSP_PARA_IN_WIDTHEX/E_VSP_PARA_IN_HEIGHTEX
 *	E_VSP_PARA_ALPHA_ASEL/E_VSP_PARA_OUT_XCOFFSET/E_VSP_PARA_OUT_YCOFFSET
 *	E_VSP_PARA_DL_SIZE/E_VSP_PARA_CONNECT
 ******************************************************************************/
static long vsp_ins_check_partition(
	struct vsp_ch_info *ch_info, struct vsp_start_t *st_par)
{
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_src_t *src_par = st_par->src_par[0];	/* input source 0 */
	struct vsp_dst_t *dst_par = st_par->dst_par;
	struct vsp_uds_t *uds_par = st_par->ctrl_par->uds;
	struct vsp_dl_t *dl_par = &st_par->dl_par;

	unsigned int div_cnt;
	unsigned int tbl_num;

	if (part_info->div_flag != 0) {
		/* check extended area of RPF */
		if (src_par->width_ex != 0)
			return E_VSP_PARA_IN_WIDTHEX;

		if (src_par->height_ex != 0)
			return E_VSP_PARA_IN_HEIGHTEX;

		/* check 1bit alpha plane of RPF */
		if (src_par->alpha->asel == VSP_ALPHA_NUM4)
			return E_VSP_PARA_ALPHA_ASEL;

		/* check clipping offset of WPF */
		if (dst_par->x_coffset != 0)
			return E_VSP_PARA_OUT_XCOFFSET;

		if (dst_par->y_coffset != 0)
			return E_VSP_PARA_OUT_YCOFFSET;

		/* check display list size */
		div_cnt = VSP_ROUND_UP(
			(ch_info->wpf_info.val_hszclip & 0x1fff),
			part_info->div_size);

		tbl_num = VSP_DL_HEAD_SIZE + VSP_DL_BODY_SIZE;
		tbl_num +=
			(VSP_DL_HEAD_SIZE + VSP_DL_PART_SIZE) *
			(div_cnt - 1);
		tbl_num >>= 3;

		if (dl_par->tbl_num < tbl_num)
			return E_VSP_PARA_DL_SIZE;

		/* check connection */
		if (st_par->use_module & VSP_UDS_USE) {
			if (part_info->sru_first_flag == 1) {
				if (uds_par->x_ratio > VSP_UDS_SCALE_1_1)
					return E_VSP_PARA_CONNECT;
			}
		}

		/* check margin */
		if (part_info->margin < VSP_PART_MARGIN)
			part_info->margin = VSP_PART_MARGIN;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_check_start_parameter
 * Description:	Check vsp_start_t parameter.
 * Returns:		0/E_VSP_PARA_USEMODULE
 *	return of vsp_ins_check_connection_module_from_rpf()
 *	return of vsp_ins_check_connection_module_from_bru()
 *	return of vsp_ins_check_independent_module()
 *	return of vsp_ins_check_output_module()
 *	return of vsp_ins_check_partition()
 ******************************************************************************/
long vsp_ins_check_start_parameter(
	struct vsp_prv_data *prv, struct vsp_start_t *param)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];
	long ercd;

	/* initialise */
	ch_info->wpf_cnt = 0;
	ch_info->bru_cnt = 0;
	ch_info->brs_cnt = 0;

	ch_info->reserved_rpf = 0;
	ch_info->reserved_module = 0;

	memset(ch_info->src_info, 0, sizeof(ch_info->src_info));
	ch_info->src_idx = 0;
	ch_info->src_cnt = 0;

	memset(&ch_info->part_info, 0, sizeof(ch_info->part_info));
	ch_info->part_info.div_size = VSP_PART_SIZE;
	ch_info->part_info.margin = 1;

	/* check connection module parameter (RPF->BRU or WPF) */
	ercd = vsp_ins_check_connection_module_from_rpf(prv, param);
	if (ercd)
		return ercd;

	/* check connection module parameter (BRU->WPF) */
	ercd = vsp_ins_check_connection_module_from_bru(ch_info, param);
	if (ercd)
		return ercd;

	/* check independent module parameter (HGO, HGT) */
	ercd = vsp_ins_check_independent_module(ch_info, param);
	if (ercd)
		return ercd;

	/* check use_module parameter */
	if (param->use_module != ch_info->reserved_module)
		return E_VSP_PARA_USEMODULE;

	/* check valid WPF channel */
	if (((unsigned long)prv->rdata.usable_module &
			ch_info->reserved_module) != ch_info->reserved_module)
		return E_VSP_PARA_USEMODULE;

	/* check WPF module parameter */
	ercd = vsp_ins_check_output_module(ch_info, param);
	if (ercd)
		return ercd;

	/* check partition */
	ercd = vsp_ins_check_partition(ch_info, param);
	if (ercd)
		return ercd;

	return 0;
}
