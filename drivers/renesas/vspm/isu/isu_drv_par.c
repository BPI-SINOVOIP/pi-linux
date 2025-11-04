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

#include "isu_drv_public.h"
#include "isu_drv_local.h"

/******************************************************************************
 * Function:	isu_ins_check_init_parameter
 * Description:	Check initialize parameter
 * Returns:	0/E_ISU_PARA_INPAR
 ******************************************************************************/
long isu_ins_check_init_parameter(struct isu_init_t *param)
{
	/* check pointer */
	if (!param)
		return E_ISU_PARA_INPAR;

	/* check IP number */
	if (param->ip_num < 1 || param->ip_num > ISU_IP_MAX)
		return E_ISU_PARA_INPAR;

	return 0;
}

/******************************************************************************
 * Function:	isu_ins_check_open_parameter
 * Description:	Check open channel parameter
 * Returns:	0/E_ISU_PARA_INPAR
 ******************************************************************************/
long isu_ins_check_open_parameter(struct isu_open_t *param)
{
	/* check pointer */
	if (!param)
		return E_ISU_PARA_INPAR;

	/* check device parameter */
	if (!param->pdev)
		return E_ISU_PARA_INPAR;

	return 0;
}

/******************************************************************************
 * Function:	isu_ins_check_rpf_param
 * Description:	Check source parameter of RPF.
 * Returns:
******************************************************************************/
static long isu_ins_check_rpf_param(struct isu_ch_info *ch_info,
				struct isu_src_t *src_par)
{
	struct isu_rpf_info *rpf_info = &ch_info->rpf_info;
	unsigned char grada_mode_tmp;
	char fmt_yuv=0;

	/* check pointer */
	if (!src_par)
		return E_ISU_PARA_NOIN;

	/* check input addr 32 boundary */
	if ((!src_par->addr) || (src_par->addr % 32))
		return E_ISU_PARA_IN_ADDR;

	/* check stride 32 boundary */
	if (src_par->stride%32)
		return E_ISU_PARA_IN_STRD;

	/* check input format */
	switch (src_par->format) {
	case ISU_ARGB1555:
	case ISU_RGB565:
	case ISU_BGR666:
	case ISU_RGB888:
	case ISU_BGR888:
	case ISU_ARGB8888:
	case ISU_RGBA8888:
	case ISU_ABGR8888:
	case ISU_RAW8:
	case ISU_RAW10:
	case ISU_RAW12:
		break;
	case ISU_YUV422_UYVY:
	case ISU_YUV422_YUY2:
		fmt_yuv=1;
		break;
	case ISU_YUV422_NV16:
	case ISU_YUV420_NV12:
		fmt_yuv=1;
		if((!src_par->addr_c)||(src_par->addr_c%32))
			 return E_ISU_PARA_IN_ADDR;
		if((!src_par->stride_c)||(src_par->stride_c%32))
			return E_ISU_PARA_IN_STRD;
		break;
	default:
		return E_ISU_PARA_IN_FORMAT;
	}

	rpf_info->format = src_par->format;
	rpf_info->addr = src_par->addr;
	rpf_info->addr_c = src_par->addr_c;
	rpf_info->stride = src_par->stride;
	rpf_info->stride_c = src_par->stride_c;

	/* check test date parameter */
	if (src_par->td) {
		rpf_info->src_td1 |= ISU_RPF_TD_USE;
		grada_mode_tmp = (src_par->td->grada_mode) >> 1;
		rpf_info->src_td1 |= ((unsigned int)grada_mode_tmp) << 16;
		grada_mode_tmp = src_par->td->grada_mode & 0x01;
		rpf_info->src_td1 |= ((unsigned int)grada_mode_tmp) << 4;
		if (src_par->td->grada_step) {
			rpf_info->src_td1 |= ((unsigned int)src_par->td->grada_step)&0x0000000F;
		} else {
			rpf_info->src_td2 |= (unsigned int)src_par->td->init_val;
		}
	} else {
		rpf_info->src_td1 = 0;
	}

	/* set data swapping parameter */
	rpf_info->swap = (unsigned int)(src_par->swap);

	/* check basic area */
	if ((src_par->width < 1) || (src_par->width > 4096) || ((fmt_yuv)&&(src_par->width%2)))
		return E_ISU_PARA_IN_WIDTH;
	else {
		rpf_info->width = src_par->width;
	}

	if ((src_par->height < 1) || (src_par->height > 4096) || ((fmt_yuv)&&(src_par->height%2)))
		return E_ISU_PARA_IN_HEIGHT;
	else {
		rpf_info->height = src_par->height;
	}

	/* check alpha selection and color converter parameter */
	if(src_par->alpha){
		if(src_par->alpha->asel == ISU_AEXT_COPY) {
		/* Copy input Alpha value 1bit to all 8bit */
		} else if (src_par->alpha->asel == ISU_AEXT_EXPAN) {
			/* set 8bit transparent-alpha generator */
			rpf_info->rpf_alpha_val |= ISU_RPF_AEXT;
			rpf_info->rpf_alpha_val |= ((unsigned int)src_par->alpha->anum1) << 8;
			rpf_info->rpf_alpha_val |= (unsigned int)src_par->alpha->anum0;
		} else
			return E_ISU_PARA_ALPHA_ASEL;
	} else {
		rpf_info->rpf_alpha_val = 0;
	}
	/* Check whether convert Cb/Cr/UV to offset binary */
	rpf_info->uv_bin |= src_par->uv_conv;

	return 0;
}

/******************************************************************************
 * Function:	isu_ins_check_rs_param
 * Description:	Check module parameter of RS.
 * Returns:
 ******************************************************************************/
static long isu_ins_check_rs_param(struct isu_ch_info *ch_info,
				struct isu_rs_t *rs_param, struct isu_dst_t *dst_par)
{
	struct isu_rpf_info *rpf_info = &ch_info->rpf_info;
	struct isu_rs_info *rs_info = &ch_info->rs_info;

	if (rs_param){
		 /* Check start position after resize */
		if ((rs_param->start_x >= rpf_info->width)||(rs_param->start_y >= rpf_info->height))
			return E_ISU_PARA_RS_START;
		else {
			rs_info->start_x = rs_param->start_x;
			rs_info->start_y = rs_param->start_y;
		}

		/* Check tuning parameter */
		if ((rs_param->tune_x >= ISU_TUNE_MAX)||(rs_param->tune_y >= ISU_TUNE_MAX))
			return E_ISU_PARA_RS_TUNE;
		else {
			rs_info->tune_x = rs_param->tune_x;
			rs_info->tune_y = rs_param->tune_y;
		}

		/* Check output size */
		if ((rs_param->crop_w < 1)||(rs_param->crop_w > 4096))
			return E_ISU_PARA_RS_CROP_WIDTH;
		else if ((dst_par->format ==
					ISU_YUV422_UYVY ||
				dst_par->format ==
					ISU_YUV422_YUY2 ||
				dst_par->format ==
					ISU_YUV422_NV16 ||
				dst_par->format ==
					ISU_YUV420_NV12) &&
				rs_param->crop_w%2)
			return E_ISU_PARA_RS_CROP_WIDTH;
		else
			rs_info->crop_w = rs_param->crop_w;

		if ((rs_param->crop_h < 1) ||(rs_param->crop_h > 4096))
			return E_ISU_PARA_RS_CROP_HEIGHT;
		else if ((dst_par->format ==
					ISU_YUV422_UYVY ||
				dst_par->format ==
					ISU_YUV422_YUY2 ||
				dst_par->format ==
					ISU_YUV422_NV16 ||
				dst_par->format ==
					ISU_YUV420_NV12) &&
				rs_param->crop_h%2)
			return E_ISU_PARA_RS_CROP_HEIGHT;
		else
			rs_info->crop_h = rs_param->crop_h;

		/* Check pad mode */
		switch(rs_param->pad_mode){
		case ISU_PAD_IN:
			break;
		case ISU_PAD_P:
			rs_info->pad_mode = rs_param->pad_mode;
			rs_info->pad_val  = rs_param->pad_val;
			break;
		default:
			return E_ISU_PARA_RS_PAD;
		}
		/* Check ratio scale */
		if((rs_param->x_ratio & 0x0000F000)&&(rs_param->y_ratio & 0x0000F000)) {
			rs_info->x_scale = ((unsigned int)rs_param->x_ratio >> 12) << 16;
			rs_info->x_scale |= ((unsigned int)rs_param->x_ratio);
			rs_info->y_scale = ((unsigned int)rs_param->y_ratio >> 12) << 16;
			rs_info->y_scale |= ((unsigned int)rs_param->y_ratio & 0x00000FFF);
		} else
			return E_ISU_PARA_RS_RATIO;
	} else {
		rs_info->start_x = 0;
		rs_info->start_y = 0;
		rs_info->tune_x = 0;
		rs_info->tune_y = 0;

		if ((rpf_info->width < 1)||(rpf_info->width > 4096))
			return E_ISU_PARA_RS_CROP_WIDTH;
		else if ((dst_par->format ==
					ISU_YUV422_UYVY ||
				dst_par->format ==
					ISU_YUV422_YUY2 ||
				dst_par->format ==
					ISU_YUV422_NV16 ||
				dst_par->format ==
					ISU_YUV420_NV12)&&
				rpf_info->width%2)
			return E_ISU_PARA_RS_CROP_WIDTH;
		else
			rs_info->crop_w = rpf_info->width;

		if ((rpf_info->height < 1) ||(rpf_info->height > 4096))
			return E_ISU_PARA_RS_CROP_HEIGHT;
		else if ((dst_par->format ==
					ISU_YUV422_UYVY ||
				dst_par->format ==
					ISU_YUV422_YUY2 ||
				dst_par->format ==
					ISU_YUV422_NV16 ||
				dst_par->format ==
					ISU_YUV420_NV12)&&
				rpf_info->height%2)
			return E_ISU_PARA_RS_CROP_HEIGHT;
		else
			rs_info->crop_h = rpf_info->height;

		rs_info->x_scale |= ISU_RS_NO_RESIZE;
		rs_info->y_scale |= ISU_RS_NO_RESIZE;
		rs_info->pad_mode=0;
		rs_info->pad_val=0;
	}
	return 0;
}

/******************************************************************************
 * Function:isu_ins_check_connection_module_from_rpf
 * Description: Check connection module parameter from RPF.
 * Returns:
 *	return of isu_ins_check_rpf_param()
 *	return of isu_ins_check_rs_param()
 ******************************************************************************/
static long isu_ins_check_connection_module_from_rpf(struct isu_prv_data *prv,
						struct isu_start_t *param)
{
	struct isu_ch_info *ch_info = &prv->ch_info;
	long ercd;

	/* check RPF parameter */
	if(!(param->dl_hard_addr)){
		ercd = isu_ins_check_rpf_param(ch_info, param->src_par);
		if (ercd)
			return ercd;

		/* check module parameter */
		ercd = isu_ins_check_rs_param(ch_info, param->rs_par, param->dst_par);
		if (ercd)
			return ercd;
	}
	return 0;
}

/******************************************************************************
 * Function:isu_ins_check_wpf_param
 * Description: Check output parameter of WPF.
 * Returns:
 ******************************************************************************/
static long isu_ins_check_wpf_param( struct isu_ch_info *ch_info,
				struct isu_dst_t *dst_par)
{
	struct isu_wpf_info *wpf_info = &ch_info->wpf_info;
	unsigned char i, j;

	/* check pointer */
	if (!dst_par)
		return E_ISU_PARA_NOOUT;

	/* check format parameter */
	if ((!dst_par->addr) || (dst_par->addr % 512))
		return E_ISU_PARA_OUT_ADDR;

	/* check stride 32 boundary */
	if (dst_par->stride%32)
		return E_ISU_PARA_OUT_STRD;

	/* check format parameter */
	switch (dst_par->format) {
	case ISU_ARGB1555:
	case ISU_RGB565:
	case ISU_BGR666:
	case ISU_RGB888:
	case ISU_BGR888:
	case ISU_ARGB8888:
	case ISU_RGBA8888:
	case ISU_ABGR8888:
	case ISU_RAW8:
	case ISU_RAW10:
	case ISU_RAW12:
	case ISU_YUV422_UYVY:
	case ISU_YUV422_YUY2:
		break;
	case ISU_YUV422_NV16:
	case ISU_YUV420_NV12:
		if((!dst_par->addr_c)||(dst_par->addr_c%512))
			return E_ISU_PARA_OUT_ADDR;
		if((!dst_par->stride_c)||(dst_par->stride_c%32))
			return E_ISU_PARA_OUT_STRD;
		break;
	default:
		return E_ISU_PARA_OUT_FORMAT;
	}

	wpf_info->format = dst_par->format;
	wpf_info->addr = dst_par->addr;
	wpf_info->addr_c   = dst_par->addr_c;
	wpf_info->stride   = dst_par->stride;
	wpf_info->stride_c = dst_par->stride_c;

	/* set data swapping parameter */
	wpf_info->swap = (unsigned int)(dst_par->swap);

	for (i = 0; i < ISU_LAYER_NUM; i++) {
		for (j = 0; j < ISU_OFFSET_NUM; j++) {
			wpf_info->k_matrix[i][j] = 0;
			wpf_info->offset[i][j] = 0;
		}
	}
	for (i = 0; i < ISU_LAYER_NUM; i++) {
			wpf_info->clip[i][0] = 0;
			wpf_info->clip[i][1] = 0xFF;
	}

	/* check color space conversion parameter */
	if (dst_par->csc) {
		wpf_info->ccol |= ISU_WPF_CCOL_SEL;  // enable color conversion
		wpf_info->ccol |= ISU_WPF_CCOL_ASE; // enable alpha color conversion
		for (i = 0; i < ISU_LAYER_NUM; i++) {
			for (j = 0; j < ISU_LAYER_NUM; j++) {
				wpf_info->k_matrix[i][j] = dst_par->csc->k_matrix[i][j];
			}
		}
		for (i = 0; i < ISU_LAYER_NUM; i++) {
			for (j = 0; j < ISU_OFFSET_NUM; j++) {
				wpf_info->offset[i][j] = dst_par->csc->offset[i][j]&0xFF;
				wpf_info->clip[i][j] = dst_par->csc->clip[i][j]&0xFF;
			}
		}
	} else {
		/* disable color space conversion */
		wpf_info->ccol = 0;
	}

	/* check alpha selection and color converter parameter */
	if ((dst_par->format == ISU_ARGB1555)||(dst_par->format == ISU_ARGB8888)){
		if (dst_par->alpha){
			switch (dst_par->alpha->asel){
			case ISU_AEXT_COPY:
				break;
			case ISU_AEXT_COMP:
				/* Select alpha output base on a-value and a-thres */
				wpf_info->alpha_asel1 |= ISU_WPF_ASEL;
				wpf_info->alpha_asel1 |= ((unsigned int)dst_par->alpha->athres0);
				break;
			case ISU_AEXT_CONV:
				wpf_info->alpha_asel2 |= ISU_WPF_ASEL;
				wpf_info->alpha_asel2 |= ((unsigned int)dst_par->alpha->athres0) << 8;
				wpf_info->alpha_asel2 |= ((unsigned int)dst_par->alpha->athres1);
				wpf_info->alpha_val |= ((unsigned int)dst_par->alpha->anum2) << 16;
				wpf_info->alpha_val |= ((unsigned int)dst_par->alpha->anum1) << 8;
				wpf_info->alpha_val |= ((unsigned int)dst_par->alpha->anum0);
				break;
			default:
				return E_ISU_PARA_ALPHA_ASEL;
			}
		} else {
			wpf_info->alpha_val = 0;
			wpf_info->alpha_asel1 = 0;
			wpf_info->alpha_asel2 = 0;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:vsp_ins_check_dl_param
 * Description: Check display list parameter.
 * Returns: 0/E_ISU_PARA_DL_ADDR
 ******************************************************************************/
static long isu_ins_check_dl_param(struct isu_ch_info *ch_info,
				unsigned long dl_hard_addr)
{
	if(dl_hard_addr%32)
		return E_ISU_PARA_DL_ADDR;

	ch_info->dl_info = dl_hard_addr;
	return 0;
}
/******************************************************************************
 * Function:isu_ins_check_output_module
 * Description: Check destination module parameter.
 * Returns:
 ******************************************************************************/
static long isu_ins_check_output_module(
struct isu_ch_info *ch_info, struct isu_start_t *param)
{
	long ercd;
	/* check WPF parameter */
	if(!(param->dl_hard_addr)){
		ercd = isu_ins_check_wpf_param(ch_info, param->dst_par);
		if (ercd)
			return ercd;
	} else {
		/* check DL parameter */
		ercd = isu_ins_check_dl_param(ch_info, param->dl_hard_addr);
		if (ercd)
			return ercd;
	}

	return 0;
}

/******************************************************************************
 * Function:isu_ins_check_start_parameter
 * Description: Check vsp_start_t parameter.
 * Returns: 0/E_VSP_PARA_USEMODULE
 *	Return of isu_ins_check_connection_module_from_rpf()
 *	Return of isu_ins_check_output_module()
 *	Return of isu_ins_check_partition()
 ******************************************************************************/
long isu_ins_check_start_parameter(struct isu_prv_data *prv,
				struct isu_start_t *param)
{
	struct isu_ch_info *ch_info = &prv->ch_info;
	long ercd;

	/* check WPF module parameter */
	ercd = isu_ins_check_output_module(ch_info, param);
	if (ercd)
		return ercd;

	/* check connection module parameter (RPF->RS or WPF) */
	ercd = isu_ins_check_connection_module_from_rpf(prv, param);
	if (ercd)
		return ercd;

	return 0;
}
