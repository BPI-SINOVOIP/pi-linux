/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
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
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "vsp_drv_public.h"
#include "vsp_drv_local.h"

/* RPF register offset table */
static const unsigned short vsp_tbl_rpf_reg_offset[VSP_RPF_MAX][VSP_REG_MAX] = {
	{	/* RPF0 */
		VSP_RPF0_OFFSET,		/* CTRL */
		VSP_RPF0_CLUT_OFFSET,	/* CLUT TABLE */
		VSP_DPR_RPF0_ROUTE		/* ROUTE */
	},
	{	/* RPF1 */
		VSP_RPF1_OFFSET,		/* CTRL */
		VSP_RPF1_CLUT_OFFSET,	/* CLUT TABLE */
		VSP_DPR_RPF1_ROUTE		/* ROUTE */
	},
	{	/* RPF2 */
		VSP_RPF2_OFFSET,		/* CTRL */
		VSP_RPF2_CLUT_OFFSET,	/* CLUT TABLE */
		VSP_DPR_RPF2_ROUTE		/* ROUTE */
	},
	{	/* RPF3 */
		VSP_RPF3_OFFSET,		/* CTRL */
		VSP_RPF3_CLUT_OFFSET,	/* CLUT TABLE */
		VSP_DPR_RPF3_ROUTE		/* ROUTE */
	},
	{	/* RPF4 */
		VSP_RPF4_OFFSET,		/* CTRL */
		VSP_RPF4_CLUT_OFFSET,	/* CLUT TABLE */
		VSP_DPR_RPF4_ROUTE		/* ROUTE */
	},
};

/* SRU parameter table */
static const unsigned int vsp_tbl_sru_param[VSP_SCL_LEVEL_MAX][3] = {
/*   CTRL0,      CTRL1       CTRL2 */
	{0x01000400, 0x000007FF, 0x001828FF},	/* level1(weak) */
	{0x01000400, 0x000007FF, 0x000810FF},	/* level2 */
	{0x01800500, 0x000007FF, 0x00243CFF},	/* level3 */
	{0x01800500, 0x000007FF, 0x000C1BFF},	/* level4 */
	{0x01FF0600, 0x000007FF, 0x003050FF},	/* level5 */
	{0x01FF0600, 0x000007FF, 0x001024FF},	/* level6(strong) */
};

/******************************************************************************
 * Function:		vsp_write_reg
 * Description:	Write to register
 * Returns:		void
 ******************************************************************************/
inline void vsp_write_reg(
	unsigned int data, void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *reg =
		(unsigned int __iomem *)base;
	reg += (offset >> 2);
	iowrite32(data, reg);
}

/******************************************************************************
 * Function:		vsp_read_reg
 * Description:	Read from register
 * Returns:		void
 ******************************************************************************/
inline unsigned int vsp_read_reg(void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *reg =
		(unsigned int __iomem *)base;
	reg += (offset >> 2);
	return ioread32(reg);
}

/******************************************************************************
 * Function:		dlwrite32
 * Description:	Set register offset and data to display list
 * Returns:		void
 ******************************************************************************/
inline void dlwrite32(
	unsigned int **data, unsigned int reg_offset, unsigned int reg_data)
{
	*(*data)++ = reg_offset;
	*(*data)++ = reg_data;
}

/******************************************************************************
 * Function:		dlrewrite32_lut
 * Description:	Rewirte CLUT table for LUT/RPF to display list.
 * Returns:		void
 ******************************************************************************/
static void dlrewrite32_lut(struct vsp_dl_t *clut, unsigned int reg_offset)
{
	unsigned int *data = (unsigned int *)(clut->virt_addr);
	unsigned short i;

	/* rewrite register address */
	for (i = 0; i < clut->tbl_num; i++) {
		*data = reg_offset;
		data += 2;

		reg_offset += 4;
	}
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_dpr
 * Description:	Set DPR register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_dpr(
	struct vsp_dl_head_info *head, struct vsp_prv_data *prv)
{
	struct vsp_res_data *rdata = &prv->rdata;
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* RPF0 routing register */
	if (rdata->usable_rpf & VSP_RPF0_USE)
		dlwrite32(&body, VSP_DPR_RPF0_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* RPF1 routing register */
	if (rdata->usable_rpf & VSP_RPF1_USE)
		dlwrite32(&body, VSP_DPR_RPF1_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* RPF2 routing register */
	if (rdata->usable_rpf & VSP_RPF2_USE)
		dlwrite32(&body, VSP_DPR_RPF2_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* RPF3 routing register */
	if (rdata->usable_rpf & VSP_RPF3_USE)
		dlwrite32(&body, VSP_DPR_RPF3_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* RPF4 routing register */
	if (rdata->usable_rpf & VSP_RPF4_USE)
		dlwrite32(&body, VSP_DPR_RPF4_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* super-resolution */
	if (rdata->usable_module & VSP_SRU_USE)
		dlwrite32(&body, VSP_DPR_SRU_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* up down scaler */
	if (rdata->usable_module & VSP_UDS_USE)
		dlwrite32(&body, VSP_DPR_UDS_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* look up table */
	if (rdata->usable_module & VSP_LUT_USE)
		dlwrite32(&body, VSP_DPR_LUT_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* CLU */
	if (rdata->usable_module & VSP_CLU_USE)
		dlwrite32(&body, VSP_DPR_CLU_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* hue saturation value transform */
	if (rdata->usable_module & VSP_HST_USE)
		dlwrite32(&body, VSP_DPR_HST_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* hue saturation value inverse transform */
	if (rdata->usable_module & VSP_HSI_USE)
		dlwrite32(&body, VSP_DPR_HSI_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* blend ROP */
	if (rdata->usable_module & VSP_BRU_USE)
		dlwrite32(&body, VSP_DPR_BRU_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* blend ROP sub */
	if (rdata->usable_module & VSP_BRS_USE)
		dlwrite32(&body, VSP_DPR_BRS_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* histogram generator-one dimension */
	if (rdata->usable_module & VSP_HGO_USE)
		dlwrite32(&body, VSP_DPR_HGO_SMPPT, VSP_DPR_SMPPT_NOT_USE);

	/* histogram generator-two dimension */
	if (rdata->usable_module & VSP_HGT_USE)
		dlwrite32(&body, VSP_DPR_HGT_SMPPT, VSP_DPR_SMPPT_NOT_USE);

	/* sharpness */
	if (rdata->usable_module & VSP_SHP_USE)
		dlwrite32(&body, VSP_DPR_SHP_ROUTE, VSP_DPR_ROUTE_NOT_USE);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_rpf
 * Description:	Set RPF register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_rpf(
	struct vsp_dl_head_info *head,
	struct vsp_ch_info *ch_info,
	unsigned char rpf_ch,
	struct vsp_src_t *param)
{
	struct vsp_rpf_info *rpf_info = &ch_info->rpf_info[rpf_ch];

	unsigned int reg_offset;
	unsigned int reg_temp;
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* set control register offset */
	reg_offset =
		(unsigned int)vsp_tbl_rpf_reg_offset[rpf_ch][VSP_REG_CTRL];

	/* basic read size register */
	VSP_DL_WRITE(VSP_RPF_SRC_BSIZE, rpf_info->val_bsize);

	/* extended read size register */
	VSP_DL_WRITE(VSP_RPF_SRC_ESIZE, rpf_info->val_esize);

	/* input format register */
	VSP_DL_WRITE(VSP_RPF_INFMT, rpf_info->val_infmt);

	/* data swapping register */
	VSP_DL_WRITE(VSP_RPF_DSWAP, rpf_info->val_dswap);

	/* display location register */
	VSP_DL_WRITE(VSP_RPF_LOC, rpf_info->val_loc);

	/* alpha plane selection control register */
	VSP_DL_WRITE(VSP_RPF_ALPH_SEL, rpf_info->val_alpha_sel);

	/* virtual plane color information register */
	VSP_DL_WRITE(VSP_RPF_VRTCOL_SET, rpf_info->val_vrtcol);

	/* mask control register */
	VSP_DL_WRITE(VSP_RPF_MSKCTRL, rpf_info->val_mskctrl);
	VSP_DL_WRITE(VSP_RPF_MSKSET0, rpf_info->val_mskset[0]);
	VSP_DL_WRITE(VSP_RPF_MSKSET1, rpf_info->val_mskset[1]);

	/* color keying control register */
	VSP_DL_WRITE(VSP_RPF_CKEY_CTRL, rpf_info->val_ckey_ctrl);
	VSP_DL_WRITE(VSP_RPF_CKEY_SET0, rpf_info->val_ckey_set[0]);
	VSP_DL_WRITE(VSP_RPF_CKEY_SET1, rpf_info->val_ckey_set[1]);

	/* source picture memory stride setting register */
	reg_temp = ((unsigned int)param->stride) << 16;
	reg_temp |= (unsigned int)param->stride_c;
	VSP_DL_WRITE(VSP_RPF_SRCM_PSTRIDE, reg_temp);
	VSP_DL_WRITE(VSP_RPF_SRCM_ASTRIDE, rpf_info->val_astride);

	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_Y, rpf_info->val_addr_y);
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_C0, rpf_info->val_addr_c0);
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_C1, rpf_info->val_addr_c1);
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_AI, rpf_info->val_addr_ai);

	/* multi alpha control register */
	VSP_DL_WRITE(VSP_RPF_MULT_ALPHA, rpf_info->val_mult_alph);

	/* set lookup table register offset */
	reg_offset =
		(unsigned int)vsp_tbl_rpf_reg_offset[rpf_ch][VSP_REG_CLUT];

	/* lookup table register */
	if (param->format == VSP_IN_RGB_CLUT_DATA ||
	    param->format == VSP_IN_YUV_CLUT_DATA) {
		if (param->clut) {
			dlrewrite32_lut(param->clut, reg_offset);

			/* insert display list */
			head->body_num_minus1++;
			head->body_info[head->body_num_minus1].addr =
				param->clut->hard_addr;
			head->body_info[head->body_num_minus1].size =
				((unsigned int)param->clut->tbl_num) << 3;
		}
	}

	/* set routing register offset */
	reg_offset =
		(unsigned int)vsp_tbl_rpf_reg_offset[rpf_ch][VSP_REG_ROUTE];

	/* routing register */
	VSP_DL_WRITE(0, rpf_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_wpf
 * Description:	Set WPF register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_wpf(
	struct vsp_dl_head_info *head,
	struct vsp_ch_info *ch_info,
	struct vsp_dst_t *param)
{
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	unsigned int reg_offset;
	unsigned int reg_temp;
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* set control register offset */
	reg_offset = VSP_WPF0_OFFSET;

	/* source RPF register */
	VSP_DL_WRITE(VSP_WPF_SRCRPFL, wpf_info->val_srcrpf);

	/* input size clipping register */
	VSP_DL_WRITE(VSP_WPF_HSZCLIP, wpf_info->val_hszclip);
	VSP_DL_WRITE(VSP_WPF_VSZCLIP, wpf_info->val_vszclip);

	/* output format register */
	VSP_DL_WRITE(VSP_WPF_OUTFMT, wpf_info->val_outfmt);

	/* data swapping register */
	VSP_DL_WRITE(VSP_WPF_DSWAP, param->swap);

	/* rounding control register */
	reg_temp = (((unsigned int)param->cbrm) << 28);
	reg_temp |= (((unsigned int)param->abrm) << 24);
	reg_temp |= (((unsigned int)param->athres) << 16);
	reg_temp |= (((unsigned int)param->clmd) << 12);
	VSP_DL_WRITE(VSP_WPF_RNDCTRL, reg_temp);

	/* rotation control register */
	VSP_DL_WRITE(
		VSP_WPF_ROT_CTRL,
		(VSP_WPF_ROTCTRL_LN16 | VSP_WPF_ROTCTRL_LMEM));

	/* memory stride register */
	VSP_DL_WRITE(VSP_WPF_DSTM_STRIDE_Y, param->stride);
	VSP_DL_WRITE(VSP_WPF_DSTM_STRIDE_C, param->stride_c);

	/* address register */
	VSP_DL_WRITE(VSP_WPF_DSTM_ADDR_Y, wpf_info->val_addr_y);
	VSP_DL_WRITE(VSP_WPF_DSTM_ADDR_C0, wpf_info->val_addr_c0);
	VSP_DL_WRITE(VSP_WPF_DSTM_ADDR_C1, wpf_info->val_addr_c1);

	/* set routing register offset */
	reg_offset = VSP_DPR_WPF0_FPORCH;

	/* timing control register */
	VSP_DL_WRITE(0, 0x00000500);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_sru
 * Description:	Set SRU register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_sru(
	struct vsp_dl_head_info *head,
	struct vsp_sru_info *sru_info,
	struct vsp_sru_t *param)
{
	unsigned int *body0, *body;
	unsigned int reg_temp;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* super resolution control register 0 */
	reg_temp = vsp_tbl_sru_param[param->enscl][0];
	reg_temp |= (unsigned int)param->mode;
	reg_temp |= (unsigned int)param->param;
	reg_temp |= VSP_SRU_CTRL_EN;
	dlwrite32(&body, VSP_SRU_CTRL0, reg_temp);

	/* super resolution control register 1 */
	reg_temp = vsp_tbl_sru_param[param->enscl][1];
	dlwrite32(&body, VSP_SRU_CTRL1, reg_temp);

	/* super resolution control register 2 */
	reg_temp = vsp_tbl_sru_param[param->enscl][2];
	dlwrite32(&body, VSP_SRU_CTRL2, reg_temp);

	/* routing register */
	dlwrite32(&body, VSP_DPR_SRU_ROUTE, sru_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_uds
 * Description:	Set UDS register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_uds(
	struct vsp_dl_head_info *head,
	struct vsp_uds_info *uds_info,
	struct vsp_uds_t *param)
{
	unsigned int *body0, *body;
	unsigned int reg_temp;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* scaling control register */
	dlwrite32(&body, VSP_UDS_CTRL, uds_info->val_ctrl);

	/* scaling factor register */
	reg_temp = ((unsigned int)param->x_ratio) << 16;
	reg_temp |= (unsigned int)param->y_ratio;
	dlwrite32(&body, VSP_UDS_SCALE, reg_temp);

	/* alpha data threshold setting register(AON=1) */
	reg_temp = ((unsigned int)param->athres1) << 8;
	reg_temp |= (unsigned int)param->athres0;
	dlwrite32(&body, VSP_UDS_ALPTH, reg_temp);

	/* alpha data replacing value setting register(AON=1) */
	reg_temp = ((unsigned int)param->anum2) << 16;
	reg_temp |= ((unsigned int)param->anum1) << 8;
	reg_temp |= (unsigned int)param->anum0;
	dlwrite32(&body, VSP_UDS_ALPVAL, reg_temp);

	/* passband register */
	dlwrite32(&body, VSP_UDS_PASS_BWIDTH, uds_info->val_pass);

	/* scaling filter horizontal phase register */
	dlwrite32(&body, VSP_UDS_HPHASE, uds_info->val_hphase);

	/* 2D IPC setting register(not used) */
	dlwrite32(&body, VSP_UDS_IPC, 0);

	/* horizontal input clipping register */
	dlwrite32(&body, VSP_UDS_HSZCLIP, uds_info->val_hszclip);

	/* output size clipping register */
	dlwrite32(&body, VSP_UDS_CLIP_SIZE, uds_info->val_clip);

	/* routing register */
	dlwrite32(&body, VSP_DPR_UDS_ROUTE, uds_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_lut
 * Description:	Set LUT register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_lut(
	struct vsp_dl_head_info *head,
	struct vsp_lut_info *lut_info,
	struct vsp_lut_t *param)
{
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* insert LUT table to display list */
	head->body_num_minus1++;
	head->body_info[head->body_num_minus1].addr = param->lut.hard_addr;
	head->body_info[head->body_num_minus1].size =
		((unsigned int)param->lut.tbl_num) << 3;

	/* control register */
	dlwrite32(&body, VSP_LUT_CTRL, VSP_LUT_CTRL_EN);

	/* routing register */
	dlwrite32(&body, VSP_DPR_LUT_ROUTE, lut_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_clu
 * Description:	Set CLU register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_clu(
	struct vsp_dl_head_info *head,
	struct vsp_clu_info *clu_info,
	struct vsp_clu_t *param)
{
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* control register */
	dlwrite32(&body, VSP_CLU_CTRL, clu_info->val_ctrl);

	/* CLU table register */
	if (clu_info->val_ctrl & VSP_CLU_CTRL_AAI) {
		/* automatic address increment mode */
		dlwrite32(&body, VSP_CLU_TBL_ADDR, 0);
	}

	/* insert CLU table to display list */
	head->body_num_minus1++;
	head->body_info[head->body_num_minus1].addr = param->clu.hard_addr;
	head->body_info[head->body_num_minus1].size =
		((unsigned int)param->clu.tbl_num) << 3;

	/* routing register */
	dlwrite32(&body, VSP_DPR_CLU_ROUTE, clu_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_hst
 * Description:	Set HST register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_hst(
	struct vsp_dl_head_info *head, struct vsp_hst_info *hst_info)
{
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* control register */
	dlwrite32(&body, VSP_HST_CTRL, VSP_HST_CTRL_EN);

	/* routing register */
	dlwrite32(&body, VSP_DPR_HST_ROUTE, hst_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_hsi
 * Description:	Set HSI register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_hsi(
	struct vsp_dl_head_info *head, struct vsp_hsi_info *hsi_info)
{
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* control register */
	dlwrite32(&body, VSP_HSI_CTRL, VSP_HSI_CTRL_EN);

	/* routing register */
	dlwrite32(&body, VSP_DPR_HSI_ROUTE, hsi_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_bru
 * Description:	Set BRU register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_bru(
	struct vsp_dl_head_info *head,
	struct vsp_bru_info *bru_info,
	struct vsp_bru_t *param)
{
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* input control register */
	dlwrite32(&body, VSP_BRU_INCTRL, bru_info->val_inctrl);

	/* input virtual address */
	dlwrite32(&body, VSP_BRU_VIRRPF_SIZE, bru_info->val_vir_size);

	/* display location register */
	dlwrite32(&body, VSP_BRU_VIRRPF_LOC, bru_info->val_vir_loc);

	/* color information register */
	dlwrite32(&body, VSP_BRU_VIRRPF_COL, bru_info->val_vir_color);

	/* Blend/ROP UNIT A control register */
	dlwrite32(&body, VSP_BRUA_CTRL, bru_info->val_ctrl[0]);
	dlwrite32(&body, VSP_BRUA_BLD, bru_info->val_bld[0]);

	/* Blend/ROP UNIT B control register */
	dlwrite32(&body, VSP_BRUB_CTRL, bru_info->val_ctrl[1]);
	dlwrite32(&body, VSP_BRUB_BLD, bru_info->val_bld[1]);

	/* Blend/ROP UNIT C control register */
	dlwrite32(&body, VSP_BRUC_CTRL, bru_info->val_ctrl[2]);
	dlwrite32(&body, VSP_BRUC_BLD, bru_info->val_bld[2]);

	/* Blend/ROP UNIT D control register */
	dlwrite32(&body, VSP_BRUD_CTRL, bru_info->val_ctrl[3]);
	dlwrite32(&body, VSP_BRUD_BLD, bru_info->val_bld[3]);

	/* ROP UNIT control register */
	dlwrite32(&body, VSP_BRU_ROP, bru_info->val_rop);

	/* Blend/ROP UNIT E control register */
	dlwrite32(&body, VSP_BRUE_CTRL, bru_info->val_ctrl[4]);
	dlwrite32(&body, VSP_BRUE_BLD, bru_info->val_bld[4]);

	/* routing register */
	dlwrite32(&body, VSP_DPR_BRU_ROUTE, bru_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_brs
 * Description:	Set BRS register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_brs(
	struct vsp_dl_head_info *head,
	struct vsp_brs_info *brs_info,
	struct vsp_brs_t *param)
{
	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* input control register */
	dlwrite32(&body, VSP_BRS_INCTRL, brs_info->val_inctrl);

	/* input virtual address */
	dlwrite32(&body, VSP_BRS_VIRRPF_SIZE, brs_info->val_vir_size);

	/* display location register */
	dlwrite32(&body, VSP_BRS_VIRRPF_LOC, brs_info->val_vir_loc);

	/* color information register */
	dlwrite32(&body, VSP_BRS_VIRRPF_COL, brs_info->val_vir_color);

	/* Blend/ROP UNIT A control register */
	dlwrite32(&body, VSP_BRSA_CTRL, brs_info->val_ctrl[0]);
	dlwrite32(&body, VSP_BRSA_BLD, brs_info->val_bld[0]);

	/* Blend/ROP UNIT B control register */
	dlwrite32(&body, VSP_BRSB_CTRL, brs_info->val_ctrl[1]);
	dlwrite32(&body, VSP_BRSB_BLD, brs_info->val_bld[1]);

	/* routing register */
	dlwrite32(&body, VSP_DPR_BRS_ROUTE, brs_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_hgo
 * Description:	Set HGO register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_hgo(
	struct vsp_dl_head_info *head,
	struct vsp_prv_data *prv,
	struct vsp_hgo_t *param)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];
	struct vsp_hgo_info *hgo_info = &ch_info->hgo_info;

	unsigned int *body0, *body;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* detection window offset register */
	dlwrite32(&body, VSP_HGO_OFFSET, hgo_info->val_offset);

	/* detection window size register */
	dlwrite32(&body, VSP_HGO_SIZE, hgo_info->val_size);

	if (prv->rdata.start_reservation == 2) {
		/* storing address register */
		dlwrite32(&body, VSP_HGO_HISTADD, param->hard_addr);

		/* swapping register */
		dlwrite32(&body, VSP_HGO_HSWAP, 0x00000004);

		/* set automatically histogram write */
		hgo_info->val_mode |= VSP_HGO_MODE_AUTOW;
	}

	/* mode register */
	dlwrite32(&body, VSP_HGO_MODE, hgo_info->val_mode);

	/* smppt register */
	dlwrite32(&body, VSP_DPR_HGO_SMPPT, hgo_info->val_dpr);

	/* set HGO write buffer */
	if (prv->rdata.start_reservation == 1)
		dlwrite32(&body, VSP_HGO_WBUFS, (unsigned int)prv->widx);

	/* reset */
	if (hgo_info->val_dpr == VSP_DPR_SMPPT_NOT_USE) {
		dlwrite32(&body, VSP_HGO_REGRST, VSP_HGO_REGRST_RCPART);
	} else {
		dlwrite32(&body, VSP_HGO_REGRST, hgo_info->val_regrst);

		/* set register reset (2nd or more partition) */
		hgo_info->val_regrst = VSP_HGO_REGRST_RCPART;
	}

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_hgt
 * Description:	Set HGT register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_hgt(
	struct vsp_dl_head_info *head,
	struct vsp_prv_data *prv,
	struct vsp_hgt_t *param)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];
	struct vsp_hgt_info *hgt_info = &ch_info->hgt_info;

	unsigned int *body0, *body;
	unsigned int reg_temp;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* detection window offset register */
	dlwrite32(&body, VSP_HGT_OFFSET, hgt_info->val_offset);

	/* detection window size register */
	dlwrite32(&body, VSP_HGT_SIZE, hgt_info->val_size);

	if (prv->rdata.start_reservation == 2) {
		/* storing address register */
		dlwrite32(&body, VSP_HGT_HISTADD, param->hard_addr);

		/* swapping register */
		dlwrite32(&body, VSP_HGT_HSWAP, 0x00000004);

		/* set automatically histogram write */
		reg_temp = VSP_HGT_MODE_AUTOW;
	} else {
		reg_temp = 0;
	}

	/* mode register */
	reg_temp |= ((unsigned int)param->x_skip) << 2;
	reg_temp |= (unsigned int)param->y_skip;
	dlwrite32(&body, VSP_HGT_MODE, reg_temp);

	/* heu area register */
	reg_temp = ((unsigned int)param->area[0].lower) << 16;
	reg_temp |= (unsigned int)param->area[0].upper;
	dlwrite32(&body, VSP_HGT_HUE_AREA0, reg_temp);

	reg_temp = ((unsigned int)param->area[1].lower) << 16;
	reg_temp |= (unsigned int)param->area[1].upper;
	dlwrite32(&body, VSP_HGT_HUE_AREA1, reg_temp);

	reg_temp = ((unsigned int)param->area[2].lower) << 16;
	reg_temp |= (unsigned int)param->area[2].upper;
	dlwrite32(&body, VSP_HGT_HUE_AREA2, reg_temp);

	reg_temp = ((unsigned int)param->area[3].lower) << 16;
	reg_temp |= (unsigned int)param->area[3].upper;
	dlwrite32(&body, VSP_HGT_HUE_AREA3, reg_temp);

	reg_temp = ((unsigned int)param->area[4].lower) << 16;
	reg_temp |= (unsigned int)param->area[4].upper;
	dlwrite32(&body, VSP_HGT_HUE_AREA4, reg_temp);

	reg_temp = ((unsigned int)param->area[5].lower) << 16;
	reg_temp |= (unsigned int)param->area[5].upper;
	dlwrite32(&body, VSP_HGT_HUE_AREA5, reg_temp);

	/* smppt register */
	dlwrite32(&body, VSP_DPR_HGT_SMPPT, hgt_info->val_dpr);

	/* set HGT write buffer */
	if (prv->rdata.start_reservation == 1)
		dlwrite32(&body, VSP_HGT_WBUFS, (unsigned int)prv->widx);

	/* reset */
	if (hgt_info->val_dpr == VSP_DPR_SMPPT_NOT_USE) {
		dlwrite32(&body, VSP_HGT_REGRST, VSP_HGT_REGRST_RCPART);
	} else {
		dlwrite32(&body, VSP_HGT_REGRST, hgt_info->val_regrst);

		/* set register reset (2nd or more partition) */
		hgt_info->val_regrst = VSP_HGT_REGRST_RCPART;
	}

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_shp
 * Description:	Set SHP register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_shp(
	struct vsp_dl_head_info *head,
	struct vsp_shp_info *shp_info,
	struct vsp_shp_t *param)
{
	unsigned int *body0, *body;
	unsigned int reg_temp;

	/* set pointer */
	body = (unsigned int *)head;
	body += ((head->body_info[0].size + VSP_DL_HEAD_SIZE) >> 2);
	body0 = body;

	/* set control register0 */
	reg_temp = ((unsigned int)param->limit0) << 24;
	reg_temp |= ((unsigned int)param->gain0) << 16;
	reg_temp |= (unsigned int)param->mode;
	reg_temp |= VSP_SHP_SHP_EN;
	dlwrite32(&body, VSP_SHP_CTRL0, reg_temp);

	/* set control register1 */
	reg_temp = ((unsigned int)param->limit11) << 24;
	reg_temp |= ((unsigned int)param->gain11) << 16;
	reg_temp |= ((unsigned int)param->limit10) << 8;
	reg_temp |= (unsigned int)param->gain10;
	dlwrite32(&body, VSP_SHP_CTRL1, reg_temp);

	/* set control register2 */
	reg_temp = ((unsigned int)param->limit21) << 24;
	reg_temp |= ((unsigned int)param->gain21) << 16;
	reg_temp |= ((unsigned int)param->limit20) << 8;
	reg_temp |= (unsigned int)param->gain20;
	dlwrite32(&body, VSP_SHP_CTRL2, reg_temp);

	/* routing register */
	dlwrite32(&body, VSP_DPR_SHP_ROUTE, shp_info->val_dpr);

	/* add size */
	head->body_info[0].size +=
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
}

/******************************************************************************
 * Function:		vsp_ins_set_dl_for_module
 * Description:	Set modules register value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_dl_for_module(
	struct vsp_dl_head_info *head,
	struct vsp_prv_data *prv,
	struct vsp_ctrl_t *ctrl_param)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];
	unsigned long module = ch_info->reserved_module;

	/* set super-resolution parameter */
	if (module & VSP_SRU_USE) {
		vsp_ins_set_dl_for_sru(
			head, &ch_info->sru_info, ctrl_param->sru);
	}

	/* set up down scaler parameter */
	if (module & VSP_UDS_USE) {
		vsp_ins_set_dl_for_uds(
			head, &ch_info->uds_info, ctrl_param->uds);
	}

	/* set look up table parameter */
	if (module & VSP_LUT_USE) {
		vsp_ins_set_dl_for_lut(
			head, &ch_info->lut_info, ctrl_param->lut);
	}

	/* set CLU parameter */
	if (module & VSP_CLU_USE) {
		vsp_ins_set_dl_for_clu(
			head, &ch_info->clu_info, ctrl_param->clu);
	}

	/* set hue saturation value transform parameter */
	if (module & VSP_HST_USE)
		vsp_ins_set_dl_for_hst(head, &ch_info->hst_info);

	/* set hue saturation value inverse transform parameter */
	if (module & VSP_HSI_USE)
		vsp_ins_set_dl_for_hsi(head, &ch_info->hsi_info);

	/* set blend ROP parameter */
	if (module & VSP_BRU_USE) {
		vsp_ins_set_dl_for_bru(
			head, &ch_info->bru_info, ctrl_param->bru);
	}

	/* set blend ROP sub parameter */
	if (module & VSP_BRS_USE) {
		vsp_ins_set_dl_for_brs(
			head, &ch_info->brs_info, ctrl_param->brs);
	}

	/* set histogram generator-one dimension parameter */
	if (module & VSP_HGO_USE)
		vsp_ins_set_dl_for_hgo(head, prv, ctrl_param->hgo);

	/* set histogram generator-two dimension parameter */
	if (module & VSP_HGT_USE)
		vsp_ins_set_dl_for_hgt(head, prv, ctrl_param->hgt);

	/* set sharpness parameter */
	if (module & VSP_SHP_USE) {
		vsp_ins_set_dl_for_shp(
			head, &ch_info->shp_info, ctrl_param->shp);
	}
}

/******************************************************************************
 * Function:		vsp_ins_set_part_full
 * Description:	Set all registers value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_part_full(
	struct vsp_prv_data *prv, struct vsp_start_t *st_par)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];

	struct vsp_dl_head_info *head =
		(struct vsp_dl_head_info *)(st_par->dl_par.virt_addr);

	unsigned char rpf_ch;
	unsigned char rpf_lp;

	unsigned long rpf_order = st_par->rpf_order;

	/* initialize DL header */
	memset(head, 0, VSP_DL_HEAD_SIZE);
	ch_info->next_dl_addr += VSP_DL_HEAD_SIZE;
	head->body_info[0].addr = ch_info->next_dl_addr;

	/* init DRP register */
	vsp_ins_set_dl_for_dpr(head, prv);

	/* input module */
	for (rpf_lp = 0; rpf_lp < st_par->rpf_num; rpf_lp++) {
		/* get RPF channel */
		rpf_ch = (unsigned char)(rpf_order & 0xf);

		vsp_ins_set_dl_for_rpf(
			head, ch_info, rpf_ch, st_par->src_par[rpf_lp]);

		rpf_order >>= 4;
	}

	/* processing module */
	vsp_ins_set_dl_for_module(head, prv, st_par->ctrl_par);

	/* output module */
	vsp_ins_set_dl_for_wpf(head, ch_info, st_par->dst_par);

	/* finalize DL header */
	ch_info->next_dl_addr += VSP_DL_BODY_SIZE;
	head->next_head_addr = ch_info->next_dl_addr;
	head->next_frame_ctrl = 2;
}

/******************************************************************************
 * Function:		vsp_ins_set_part_diff
 * Description:	Set diff registers value to display list.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_part_diff(
	struct vsp_ch_info *ch_info, struct vsp_start_t *st_par)
{
	struct vsp_rpf_info *rpf_info;
	struct vsp_wpf_info *wpf_info;
	unsigned char rpf_ch;
	unsigned int reg_offset;

	struct vsp_dl_head_info *head;
	unsigned int *body0, *body;

	head = (struct vsp_dl_head_info *)
		VSP_DL_HARD_TO_VIRT(ch_info->next_dl_addr);

	body = (unsigned int *)head;
	body += (VSP_DL_HEAD_SIZE >> 2);
	body0 = body;

	/* initialize DL header */
	memset(head, 0, VSP_DL_HEAD_SIZE);
	ch_info->next_dl_addr += VSP_DL_HEAD_SIZE;
	head->body_info[0].addr = ch_info->next_dl_addr;

	/* set RPF parameter */
	rpf_ch = ch_info->src_info[0].rpf_ch;
	rpf_info = &ch_info->rpf_info[rpf_ch];
	reg_offset =
		(unsigned int)vsp_tbl_rpf_reg_offset[rpf_ch][VSP_REG_CTRL];

	/* basic read size register */
	VSP_DL_WRITE(VSP_RPF_SRC_BSIZE, rpf_info->val_bsize);

	/* extended read size register */
	VSP_DL_WRITE(VSP_RPF_SRC_ESIZE, rpf_info->val_bsize);

	/* address register */
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_Y, rpf_info->val_addr_y);
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_C0, rpf_info->val_addr_c0);
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_C1, rpf_info->val_addr_c1);
	VSP_DL_WRITE(VSP_RPF_SRCM_ADDR_AI, rpf_info->val_addr_ai);

	/* set WPF parameter */
	wpf_info = &ch_info->wpf_info;
	reg_offset = VSP_WPF0_OFFSET;

	/* input size clipping register */
	VSP_DL_WRITE(VSP_WPF_HSZCLIP, wpf_info->val_hszclip);

	/* address register */
	VSP_DL_WRITE(VSP_WPF_DSTM_ADDR_Y, wpf_info->val_addr_y);
	VSP_DL_WRITE(VSP_WPF_DSTM_ADDR_C0, wpf_info->val_addr_c0);
	VSP_DL_WRITE(VSP_WPF_DSTM_ADDR_C1, wpf_info->val_addr_c1);

	/* set UDS parameter */
	if (ch_info->reserved_module & VSP_UDS_USE) {
		/* scaling filter horizontal phase register */
		dlwrite32(&body, VSP_UDS_HPHASE, ch_info->uds_info.val_hphase);

		/* horizontal input clipping register */
		dlwrite32(
			&body, VSP_UDS_HSZCLIP, ch_info->uds_info.val_hszclip);

		/* output size clipping register */
		dlwrite32(&body, VSP_UDS_CLIP_SIZE, ch_info->uds_info.val_clip);
	}

	/* set HGO parameter */
	if (ch_info->reserved_module & VSP_HGO_USE) {
		/* detection window offset register */
		dlwrite32(&body, VSP_HGO_OFFSET, ch_info->hgo_info.val_offset);

		/* detection window size register */
		dlwrite32(&body, VSP_HGO_SIZE, ch_info->hgo_info.val_size);

		/* smppt register */
		dlwrite32(&body, VSP_DPR_HGO_SMPPT, ch_info->hgo_info.val_dpr);

		/* reset */
		if (ch_info->hgo_info.val_dpr == VSP_DPR_SMPPT_NOT_USE) {
			dlwrite32(&body, VSP_HGO_REGRST, VSP_HGO_REGRST_RCPART);
		} else {
			dlwrite32(
				&body,
				VSP_HGO_REGRST,
				ch_info->hgo_info.val_regrst);

			/* set register reset (2nd or more partition) */
			ch_info->hgo_info.val_regrst = VSP_HGO_REGRST_RCPART;
		}
	}

	/* set HGT parameter */
	if (ch_info->reserved_module & VSP_HGT_USE) {
		/* detection window offset register */
		dlwrite32(&body, VSP_HGT_OFFSET, ch_info->hgt_info.val_offset);

		/* detection window size register */
		dlwrite32(&body, VSP_HGT_SIZE, ch_info->hgt_info.val_size);

		/* smppt register */
		dlwrite32(&body, VSP_DPR_HGT_SMPPT, ch_info->hgt_info.val_dpr);

		/* reset */
		if (ch_info->hgt_info.val_dpr == VSP_DPR_SMPPT_NOT_USE) {
			dlwrite32(&body, VSP_HGT_REGRST, VSP_HGT_REGRST_RCPART);
		} else {
			dlwrite32(
				&body,
				VSP_HGT_REGRST,
				ch_info->hgt_info.val_regrst);

			/* set register reset (2nd or more partition) */
			ch_info->hgt_info.val_regrst = VSP_HGT_REGRST_RCPART;
		}
	}

	/* finalize DL header */
	head->body_info[0].size =
		(unsigned int)((unsigned long)(body) - (unsigned long)(body0));
	ch_info->next_dl_addr += VSP_DL_PART_SIZE;
	head->next_head_addr = ch_info->next_dl_addr;
	head->next_frame_ctrl = 2;
}

/******************************************************************************
 * Function:		vsp_ins_get_bpp_luma
 * Description:	Get byte per pixel of RGB/Y.
 * Returns:		byte per pixel.
 ******************************************************************************/
unsigned int vsp_ins_get_bpp_luma(
	unsigned short format, unsigned short offset)
{
	unsigned int bpp;

	switch (format & 0xff) {
	case VSP_IN_YUV444_SEMI_PLANAR:
	case VSP_IN_YUV422_SEMI_PLANAR:
	case VSP_IN_YUV420_SEMI_PLANAR:
	case VSP_IN_YUV444_PLANAR:
	case VSP_IN_YUV422_PLANAR:
	case VSP_IN_YUV420_PLANAR:
		/* semi planar and planar */
		bpp = (unsigned int)offset;
		break;
	case VSP_IN_YUV422_INTERLEAVED0:
	case VSP_IN_YUV422_INTERLEAVED1:
		/* interleaved */
		bpp = (unsigned int)(offset * 2);
		break;
	case VSP_IN_YUV444_INTERLEAVED:
	case VSP_IN_YUV420_INTERLEAVED:
		/* interleaved */
		bpp = (unsigned int)(offset * 3);
		break;
	default:
		/* RGB format */
		bpp = (unsigned int)((format & 0x0f00) >> 8);
		bpp *= (unsigned int)offset;
		break;
	}

	return bpp;
}

/******************************************************************************
 * Function:		vsp_ins_get_bpp_chroma
 * Description:	Get byte per pixel of choroma.
 * Returns:		byte per pixel.
 ******************************************************************************/
unsigned int vsp_ins_get_bpp_chroma(
	unsigned short format, unsigned short offset)
{
	unsigned int bpp;

	switch (format & 0xff) {
	case VSP_IN_YUV422_PLANAR:
	case VSP_IN_YUV420_PLANAR:
		/* planar */
		bpp = (unsigned int)(offset >> 1);
		break;
	case VSP_IN_YUV422_SEMI_PLANAR:
	case VSP_IN_YUV420_SEMI_PLANAR:
	case VSP_IN_YUV444_PLANAR:
		/* semi planar and planar(4:4:4) */
		bpp = (unsigned int)offset;
		break;
	case VSP_IN_YUV444_SEMI_PLANAR:
		/* semi planar(4:4:4) */
		bpp = (unsigned int)(offset << 1);
		break;
	default:
		bpp = 0;
		break;
	}

	return bpp;
}

/******************************************************************************
 * Function:		vsp_ins_get_line_luma
 * Description:	Get byte per line of RGB/Y.
 * Returns:		byte per line.
 ******************************************************************************/
unsigned int vsp_ins_get_line_luma(
	unsigned short format, unsigned short offset)
{
	if (format == VSP_IN_YUV420_INTERLEAVED)
		return (unsigned int)(offset >> 1);
	else
		return (unsigned int)offset;
}

/******************************************************************************
 * Function:		vsp_ins_get_line_chroma
 * Description:	Get byte per line of chroma.
 * Returns:		byte per line.
 ******************************************************************************/
unsigned int vsp_ins_get_line_chroma(
	unsigned short format, unsigned short offset)
{
	if (format == VSP_IN_YUV420_SEMI_NV12 ||
	    format == VSP_IN_YUV420_SEMI_NV21 ||
	    format == VSP_IN_YUV420_PLANAR)
		return (unsigned int)(offset >> 1);
	else
		return (unsigned int)offset;
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_src_addr
 * Description:	Replace source address of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_src_addr(
	struct vsp_part_info *part_info,
	struct vsp_rpf_info *rpf_info,
	struct vsp_src_t *src_par,
	unsigned short offset)
{
	unsigned int temp;

	/* replace luma address */
	temp = vsp_ins_get_bpp_luma(src_par->format, offset);
	rpf_info->val_addr_y = part_info->rpf_addr_y + temp;

	/* replace chroma address */
	temp = vsp_ins_get_bpp_chroma(src_par->format, offset);
	if (rpf_info->val_addr_c0)
		rpf_info->val_addr_c0 = part_info->rpf_addr_c0 + temp;
	if (rpf_info->val_addr_c1)
		rpf_info->val_addr_c1 = part_info->rpf_addr_c1 + temp;

	/* replace alpha plane */
	if (rpf_info->val_addr_ai) {
		rpf_info->val_addr_ai =
			part_info->rpf_addr_ai + (unsigned int)offset;
	}
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_dst_addr
 * Description:	Replace destination address of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_dst_addr(
	struct vsp_ch_info *ch_info,
	struct vsp_dst_t *dst_par,
	unsigned short width)
{
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	unsigned int temp_y;
	unsigned int temp_c;

	/* calculate offset */
	if (dst_par->rotation <= VSP_ROT_V_FLIP) {
		/* move horizontal */
		temp_y = vsp_ins_get_bpp_luma(
			dst_par->format, part_info->div_size);
		temp_c = vsp_ins_get_bpp_chroma(
			dst_par->format, part_info->div_size);
	} else if (dst_par->rotation <= VSP_ROT_180) {
		/* move horizontal */
		temp_y = vsp_ins_get_bpp_luma(dst_par->format, width);
		temp_c = vsp_ins_get_bpp_chroma(dst_par->format, width);
	} else {
		/* move vertical */
		temp_y = vsp_ins_get_line_luma(
			dst_par->format, part_info->div_size);
		temp_y *= (unsigned int)dst_par->stride;
		temp_c = vsp_ins_get_line_chroma(
			dst_par->format, part_info->div_size);
		temp_c *= (unsigned int)dst_par->stride_c;
	}

	/* replace address */
	if (dst_par->rotation == VSP_ROT_OFF ||
	    dst_par->rotation == VSP_ROT_V_FLIP ||
	    dst_par->rotation == VSP_ROT_90 ||
	    dst_par->rotation == VSP_ROT_90_H_FLIP) {
		/* increment */
		wpf_info->val_addr_y += temp_y;
		if (wpf_info->val_addr_c0 != 0)
			wpf_info->val_addr_c0 += temp_c;
		if (wpf_info->val_addr_c1 != 0)
			wpf_info->val_addr_c1 += temp_c;
	} else {
		/* decrement */
		wpf_info->val_addr_y -= temp_y;
		if (wpf_info->val_addr_c0 != 0)
			wpf_info->val_addr_c0 -= temp_c;
		if (wpf_info->val_addr_c1 != 0)
			wpf_info->val_addr_c1 -= temp_c;
	}
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_uds_module
 * Description:	Replace UDS module of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_uds_module(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned int *l_pos,
	unsigned int *r_pos)
{
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_uds_info *uds_info = &ch_info->uds_info;

	struct vsp_sru_t *sru_par = st_par->ctrl_par->sru;
	struct vsp_uds_t *uds_par = st_par->ctrl_par->uds;

	unsigned int l_temp = *l_pos;
	unsigned int r_temp = *r_pos;

	/* check SRU module */
	if ((ch_info->reserved_module & VSP_SRU_USE) &&
	    part_info->sru_first_flag == 0) {
		if (sru_par->mode == VSP_SRU_MODE2) {
			l_temp >>= 1;
			r_temp >>= 1;
		}
	}

	/* check UDS module */
	if (ch_info->reserved_module & VSP_UDS_USE &&
	    ch_info->reserved_module & VSP_SRU_USE) {
		unsigned int ratio =
			(unsigned int)uds_par->x_ratio;

		/* replace clipping size */
		uds_info->val_clip &= 0x0000FFFF;
		uds_info->val_clip |= ((r_temp - l_temp) << 16);

		/* calculate scaling */
		if (uds_par->amd == VSP_AMD_NO &&
		    uds_par->x_ratio < VSP_UDS_SCALE_1_1) {
			if (l_temp == 0)
				l_temp = 4096 - ratio;
			else
				l_temp = (l_temp - 1) * ratio + 4096;
			r_temp = (r_temp - 1) * ratio + 4096;
		} else {
			l_temp *= ratio;
			r_temp *= ratio;
		}

		/* add horizontal filter phase of control register */
		uds_info->val_ctrl |= VSP_UDS_CTRL_AMDSLH;

		/* replace scaling filter horizontal phase */
		if (l_temp & 0xfff)
			uds_info->val_hphase = (4096 - (l_temp & 0xfff)) << 16;
		else
			uds_info->val_hphase = 0;
		if (r_temp & 0xfff)
			uds_info->val_hphase |= (4096 - (r_temp & 0xfff));
	} else if (ch_info->reserved_module & VSP_UDS_USE) {
		unsigned int ratio =
			(unsigned int)uds_par->x_ratio;
		unsigned short mh, fh, alpha, mha;
		unsigned short dst_pos0 = *l_pos;
		unsigned short dst_pos1 = *r_pos - 1;
		unsigned short sub_src;
		unsigned short add_src = 0;
		unsigned short phase_edge = 0;
		unsigned short phase_residual = 0;
		unsigned short src_pos0 = 0;
		unsigned short src_pos1 = 0;
		unsigned short dst_pos0_pb = 0;
		struct vsp_dst_t *dst_par = st_par->dst_par;
		struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;
		struct vsp_part_info *part_info = &ch_info->part_info;
		unsigned short width;
		unsigned short hstp, hedp;

		if (dst_par->rotation <= VSP_ROT_180)
			width = dst_par->width;
		else
			width = dst_par->height;

		/* <procedure1> Calculate temporary src_pos0 position */
		mh = (ratio & 0xf000) >> 12;
		fh = (ratio & 0x0fff);
		if (!mh && !fh) {	/* check error */
			APRINT("%s: x_ratio is zero!!\n", __func__);
			return;
		}
		alpha = 4096 * mh + fh;
		if (mh <= 3) {
			mha = 1;
			sub_src = 1;
		} else if (mh <= 7) {
			mha = 2;
			sub_src = 2;
		} else { /* 8 to 15 */
			mha = 4;
			sub_src = 2;
		}
		if (!mh && uds_par->amd == VSP_AMD)
			phase_edge = (4096 - fh) / 2;

		if (*l_pos) {
			/* 2nd partition to last partition */
			src_pos0 = (((dst_pos0 * alpha - phase_edge * mha)
				     / (4096 * mha)) - sub_src) * mha;
		}

		/* <procedure2> Calculate temporary dst_pos0_pb position.
		 *              (temporal dst_pos0_pb)
		 */
		if (*l_pos) {
			/* 2nd partition to last partition */
			dst_pos0_pb = (src_pos0 * 4096 + phase_edge * mha)
				      / alpha;
		}
		/* <procedure3> Decide dst_pos0_pb position */
		if (mha == 2) {
			if ((dst_pos0_pb % 2) && (alpha % 2)) {
				/* dst_pos0_pb and alpha is odd */
				dst_pos0_pb -= 1;
			}
		} else if (mha == 4) {
			if ((dst_pos0_pb % 2) || (alpha % 2)) {
				/* dst_pos0_pb or(and) alpha is odd */
				if (!(alpha % 2)) {
					/* alpha is even (dst_pos0_pb is odd) */
					if (alpha % 4)
						dst_pos0_pb -= 1;
				} else {
					/* alpha is odd */
					dst_pos0_pb = dst_pos0_pb / 4 * 4;
				}
			}
		}

		if (*l_pos) {
			int gap = part_info->margin - 1;

			wpf_info->val_hszclip &= 0xff00ffff;
			wpf_info->val_hszclip |=
				((dst_pos0 - dst_pos0_pb + 1 + gap) << 16);
		}

		/* <procedure4> Calculate src_pos0 position from dst_pos0_pb */
		if ((alpha * dst_pos0_pb) > (phase_edge * mha))
			phase_residual =
				(alpha * dst_pos0_pb - phase_edge * mha)
				 % (4096 * mha);
		if (phase_residual)
			add_src = 1;
		if (*l_pos) {
			/* 2nd partition to last partition */
			src_pos0 = ((dst_pos0_pb * alpha - phase_edge * mha)
				    / (4096 * mha) + add_src) * mha;
		}
		/* <procedure5> Calculate src_pos1 position from dst_pos1 */
		if (*r_pos != width) {
			/* 1st partition to 2nd last partition */
			src_pos1 = (((dst_pos1 * alpha - phase_edge * mha)
				     / (4096 * mha)) + 2) * mha + (mha / 2);
		} else {
			/* last partition */
			src_pos1 = (*r_pos) * ratio / 4096 - 1;
		}

		/* <procedure6> Calculate hstp */
		if (!(*l_pos)) {
			/* 1st partition */
			hstp = phase_edge;
		} else {
			/* 2nd partition to last partition */
			if (phase_residual)
				hstp = 4096 - phase_residual / mha;
			else
				hstp = 0;
		}

		/* <procedure7> Calculate hedp */
		if (*r_pos != width) {
			/* 1st partition to 2nd last partition */
			hedp = 0;
		} else {
			/* last partition */
			hedp = phase_edge;
		}
		/* replace clipping size */
		uds_info->val_clip &= 0x0000FFFF;
		uds_info->val_clip |= ((dst_pos1 - dst_pos0_pb + 1) << 16);

		l_temp = src_pos0 * 4096;
		r_temp = (src_pos1 + 1) * 4096;

		/* add horizontal filter phase of control register */
		uds_info->val_ctrl |= VSP_UDS_CTRL_AMDSLH;
		uds_info->val_hphase = (hstp << 16) | hedp;
	} else {
		l_temp *= 4096;
		r_temp *= 4096;
	}

	*l_pos = l_temp;
	*r_pos = r_temp;
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_rpf_module
 * Description:	Replace RPF module of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_rpf_module(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned int *l_pos,
	unsigned int *r_pos)
{
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_rpf_info *rpf_info =
		&ch_info->rpf_info[ch_info->src_info[0].rpf_ch];
	struct vsp_uds_info *uds_info = &ch_info->uds_info;

	struct vsp_src_t *src_par = st_par->src_par[0]; /* input source 0 */
	struct vsp_sru_t *sru_par = st_par->ctrl_par->sru;

	unsigned int offset;
	unsigned int width;

	unsigned int sru_scale = 1;
	unsigned int clip_offset = 0;

	/* calculate source offset and width */
	offset = VSP_ROUND_UP(*l_pos, 4096);
	width = VSP_ROUND_UP(*r_pos, 4096) - offset;

	if ((ch_info->reserved_module & VSP_SRU_USE) &&
	    part_info->sru_first_flag == 1) {
		if (sru_par->mode == VSP_SRU_MODE2)
			sru_scale = 2;
	}

	/* adjust clipping offset */
	clip_offset = (offset % (2 * sru_scale));

	/* adjust UDS input clipping area */
	if (ch_info->reserved_module & VSP_UDS_USE) {
		/* replace horizontal input clipping register */
		uds_info->val_hszclip = clip_offset << 16;
		uds_info->val_hszclip |= (unsigned int)width;
		uds_info->val_hszclip |= VSP_UDS_HSZCLIP_HCEN;
	}

	/* calculate source horizontal size */
	offset = VSP_ROUND_DOWN(offset - clip_offset, sru_scale);
	width = VSP_ROUND_UP(width + clip_offset, sru_scale);

	if ((width % 2) == 1)
		width++;

	/* replace source address */
	vsp_ins_replace_part_src_addr(
		part_info, rpf_info, src_par, (unsigned short)offset);

	/* replace basic and extended read size */
	rpf_info->val_bsize = width << 16;
	rpf_info->val_bsize |= (unsigned int)src_par->height;
	rpf_info->val_esize = rpf_info->val_bsize;
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_connection_module
 * Description:	Replace connecting module of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_connection_module(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned short dst_offset,
	unsigned short dst_width)
{
	struct vsp_part_info *part_info = &ch_info->part_info;
	struct vsp_wpf_info *wpf_info = &ch_info->wpf_info;

	struct vsp_dst_t *dst_par = st_par->dst_par;
	unsigned short width;

	unsigned int l_pos;
	unsigned int r_pos;

	if (dst_par->rotation <= VSP_ROT_180)
		width = dst_par->width;
	else
		width = dst_par->height;

	/* calculate partition position with margin */
	l_pos = (unsigned int)VSP_CLIP0(dst_offset, part_info->margin);
	if (width > dst_offset + dst_width + part_info->margin) {
		r_pos = (unsigned int)
			(dst_offset + dst_width + part_info->margin);
	} else {
		r_pos = (unsigned int)width;
		dst_width = width - dst_offset;
	}

	/* replace horizontal clipping size */
	wpf_info->val_hszclip = VSP_WPF_HSZCLIP_HCEN;
	wpf_info->val_hszclip |= ((unsigned int)dst_width);

	if (dst_offset != 0) {
		/* add horizontal clipping offset */
		wpf_info->val_hszclip |=
			(((unsigned int)part_info->margin) << 16);

		/* replace destinationaddress */
		vsp_ins_replace_part_dst_addr(ch_info, dst_par, dst_width);
	}

	/* replace UDS module parameter */
	vsp_ins_replace_part_uds_module(ch_info, st_par, &l_pos, &r_pos);

	/* replace RPF module parameter */
	vsp_ins_replace_part_rpf_module(ch_info, st_par, &l_pos, &r_pos);
}

/******************************************************************************
 * Function:		vsp_ins_get_part_offset
 * Description:	Get source offset value of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_get_part_offset(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned int *offset)
{
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_sru_t *sru_par = st_par->ctrl_par->sru;
	struct vsp_uds_t *uds_par = st_par->ctrl_par->uds;

	unsigned int temp = *offset;

	if ((ch_info->reserved_module & VSP_SRU_USE) &&
	    part_info->sru_first_flag == 0) {
		if (sru_par->mode == VSP_SRU_MODE2)
			temp >>= 1;
	}

	if (ch_info->reserved_module & VSP_UDS_USE) {
		unsigned int ratio =
			(unsigned int)uds_par->x_ratio;

		if (uds_par->amd == VSP_AMD_NO &&
		    uds_par->x_ratio < VSP_UDS_SCALE_1_1) {
			if (temp == 0)
				temp = 4096 - ratio;
			else
				temp = (temp - 1) * ratio + 4096;
		} else {
			temp *= ratio;
		}
	} else {
		temp *= 4096;
	}

	if ((ch_info->reserved_module & VSP_SRU_USE) &&
	    part_info->sru_first_flag == 1) {
		if (sru_par->mode == VSP_SRU_MODE2)
			temp >>= 1;
	}

	*offset = temp;
}

/******************************************************************************
 * Function:		vsp_ins_get_part_sampling_offset
 * Description:	Get sampling offset of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_get_part_sampling_offset(
	struct vsp_start_t *st_par,
	unsigned long sampling,
	unsigned int *offset)
{
	struct vsp_src_t *src_par = st_par->src_par[0];	/* input source 0 */
	unsigned int ratio = 4096;
	unsigned int temp = *offset;

	unsigned long connect;

	if (sampling == VSP_SMPPT_SRC1)
		connect = 0;	/* end */
	else
		connect = src_par->connect;

	while (connect != 0) {
		switch (connect) {
		case VSP_SRU_USE:
			if (st_par->ctrl_par->sru->mode == VSP_SRU_MODE2)
				temp <<= 1;

			if (sampling == VSP_SMPPT_SRU)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->sru->connect;
			break;
		case VSP_UDS_USE:
			ratio = st_par->ctrl_par->uds->x_ratio;

			if (st_par->ctrl_par->uds->amd == VSP_AMD_NO &&
			    st_par->ctrl_par->uds->x_ratio < VSP_UDS_SCALE_1_1)
				temp = temp + ratio - 4096;

			if (sampling == VSP_SMPPT_UDS)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->uds->connect;

			break;
		case VSP_LUT_USE:
			if (sampling == VSP_SMPPT_LUT)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->lut->connect;
			break;
		case VSP_CLU_USE:
			if (sampling == VSP_SMPPT_CLU)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->clu->connect;
			break;
		case VSP_HST_USE:
			if (sampling == VSP_SMPPT_HST)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->hst->connect;
			break;
		case VSP_HSI_USE:
			if (sampling == VSP_SMPPT_HSI)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->hsi->connect;
			break;
		case VSP_SHP_USE:
			if (sampling == VSP_SMPPT_SHP)
				connect = 0;	/* end */
			else
				connect = st_par->ctrl_par->shp->connect;
			break;
		default:
			connect = 0;	/* end */
			break;
		}
	}

	*offset = VSP_ROUND_UP(temp, ratio);
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_window_of_hgo
 * Description:	Replace HGO detect window of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_window_of_hgo(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned int left,
	unsigned int right,
	unsigned int margin)
{
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_hgo_info *hgo_info = &ch_info->hgo_info;
	struct vsp_hgo_t *hgo_par = st_par->ctrl_par->hgo;

	unsigned int x_offset;
	unsigned int width;

	unsigned int temp_l = hgo_par->x_offset;
	unsigned int temp_r = hgo_par->x_offset + hgo_par->width;

	if (temp_l < right && temp_r > left) {
		/* calculate offset */
		x_offset = VSP_CLIP0(temp_l, left);
		if (temp_r > right)
			width = (right - left) - x_offset;
		else
			width = temp_r - left;

		/* replace offset value */
		hgo_info->val_offset &= 0x0000FFFF;
		hgo_info->val_offset |= ((x_offset + margin) << 16);

		/* replace size value */
		hgo_info->val_size &= 0x0000FFFF;
		hgo_info->val_size |= (width << 16);

		/* set DPR value */
		hgo_info->val_dpr = part_info->hgo_smppt;
	} else {
		/* set DPR value (disconnect) */
		hgo_info->val_dpr = VSP_DPR_SMPPT_NOT_USE;
	}
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_window_of_hgt
 * Description:	Replace HGT detect window of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_window_of_hgt(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned int left,
	unsigned int right,
	unsigned int margin)
{
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_hgt_info *hgt_info = &ch_info->hgt_info;
	struct vsp_hgt_t *hgt_par = st_par->ctrl_par->hgt;

	unsigned int x_offset;
	unsigned int width;

	unsigned int temp_l = hgt_par->x_offset;
	unsigned int temp_r = hgt_par->x_offset + hgt_par->width;

	if (temp_l < right && temp_r > left) {
		/* calculate offset */
		x_offset = VSP_CLIP0(temp_l, left);
		if (temp_r > right)
			width = (right - left) - x_offset;
		else
			width = temp_r - left;

		/* replace offset value */
		hgt_info->val_offset &= 0x0000FFFF;
		hgt_info->val_offset |= ((x_offset + margin) << 16);

		/* replace size value */
		hgt_info->val_size &= 0x0000FFFF;
		hgt_info->val_size |= (width << 16);

		/* set DPR value */
		hgt_info->val_dpr = part_info->hgt_smppt;
	} else {
		/* set DPR value (disconnect) */
		hgt_info->val_dpr = VSP_DPR_SMPPT_NOT_USE;
	}
}

/******************************************************************************
 * Function:		vsp_ins_replace_part_independent_module
 * Description:	Replace independent module of partition.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_replace_part_independent_module(
	struct vsp_ch_info *ch_info,
	struct vsp_start_t *st_par,
	unsigned short dst_offset,
	unsigned short dst_width)
{
	struct vsp_hgo_t *hgo_par = st_par->ctrl_par->hgo;
	struct vsp_hgt_t *hgt_par = st_par->ctrl_par->hgt;

	unsigned int left;
	unsigned int right;
	unsigned int margin;

	if (ch_info->reserved_module & VSP_HGO_USE) {
		left = (unsigned int)dst_offset;
		right = (unsigned int)(dst_offset + dst_width);
		if (left == 0)
			margin = 0;
		else
			margin = (unsigned int)ch_info->part_info.margin;

		/* calculate partition expanded offset of valid area */
		vsp_ins_get_part_offset(ch_info, st_par, &left);
		vsp_ins_get_part_sampling_offset(
			st_par, hgo_par->sampling, &left);

		vsp_ins_get_part_offset(ch_info, st_par, &right);
		vsp_ins_get_part_sampling_offset(
			st_par, hgo_par->sampling, &right);

		vsp_ins_get_part_offset(ch_info, st_par, &margin);
		vsp_ins_get_part_sampling_offset(
			st_par, hgo_par->sampling, &margin);

		/* replace detect window of HGO */
		vsp_ins_replace_part_window_of_hgo(
			ch_info, st_par, left, right, margin);
	}

	if (ch_info->reserved_module & VSP_HGT_USE) {
		left = (unsigned int)dst_offset;
		right = (unsigned int)(dst_offset + dst_width);
		if (left == 0)
			margin = 0;
		else
			margin = (unsigned int)ch_info->part_info.margin;

		/* calculate partition expanded offset of valid area */
		vsp_ins_get_part_offset(ch_info, st_par, &left);
		vsp_ins_get_part_sampling_offset(
			st_par, hgt_par->sampling, &left);

		vsp_ins_get_part_offset(ch_info, st_par, &right);
		vsp_ins_get_part_sampling_offset(
			st_par, hgt_par->sampling, &right);

		vsp_ins_get_part_offset(ch_info, st_par, &margin);
		vsp_ins_get_part_sampling_offset(
			st_par, hgt_par->sampling, &margin);

		/* replace detect window of HGT */
		vsp_ins_replace_part_window_of_hgt(
			ch_info, st_par, left, right, margin);
	}
}

/******************************************************************************
 * Function:		vsp_ins_set_part_parameter
 * Description:	Set partition parameter.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_set_part_parameter(
	struct vsp_prv_data *prv, struct vsp_start_t *st_par)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];
	struct vsp_part_info *part_info = &ch_info->part_info;

	struct vsp_dst_t *dst_par = st_par->dst_par;
	struct vsp_dl_head_info *pre_head =
		(struct vsp_dl_head_info *)st_par->dl_par.virt_addr;

	unsigned short width;
	unsigned short dst_offset = 0;

	if (dst_par->rotation <= VSP_ROT_180)
		width = dst_par->width;
	else
		width = dst_par->height;

	if (ch_info->wpf_info.val_outfmt & VSP_WPF_OUTFMT_FCNL) {
		if (dst_par->rotation == VSP_ROT_H_FLIP ||
		    dst_par->rotation == VSP_ROT_180) {
			/* enable FCNL compression */
			/* horizontal flipping, or 180 degree rotation */
			dst_offset = width % part_info->div_size;

			if (dst_offset > 0) {
				/* replace partition register except HGO, HGT */
				vsp_ins_replace_part_connection_module(
					ch_info, st_par, 0, dst_offset);

				/* replace partition register for HGO, HGT */
				vsp_ins_replace_part_independent_module(
					ch_info, st_par, 0, dst_offset);

				/* set display list of partition */
				vsp_ins_set_part_full(prv, st_par);
			}
		}
	}

	/* partition loop */
	while (width > dst_offset) {
		/*
		 * If number of remaining pixels of horizontal is small,
		 * it change margin size so as not to be the minimum
		 * horizontal size.
		 */
		if ((width - dst_offset) < 32) {
			if (part_info->margin == 64)
				part_info->margin <<= 1;
			else
				part_info->margin <<= 2;
		}

		/* replace partition register except HGO, HGT */
		vsp_ins_replace_part_connection_module(
			ch_info, st_par, dst_offset, part_info->div_size);

		/* replace partition register for HGO, HGT */
		vsp_ins_replace_part_independent_module(
			ch_info, st_par, dst_offset, part_info->div_size);

		/* set display list of partition */
		if (dst_offset == 0) {
			/* 1st partition */
			vsp_ins_set_part_full(prv, st_par);
		} else {
			/* set next frame auto start of previous header */
			pre_head->next_frame_ctrl = 1;

			/* update DL header */
			pre_head = (struct vsp_dl_head_info *)
				VSP_DL_HARD_TO_VIRT(ch_info->next_dl_addr);

			/* 2nd or more partition */
			vsp_ins_set_part_diff(ch_info, st_par);
		}

		/* update offset */
		dst_offset += part_info->div_size;
	}
}

/******************************************************************************
 * Function:		vsp_ins_get_hgo_register
 * Description:	Get histogram from HGO register.
 * Returns:		0
 ******************************************************************************/
static long vsp_ins_get_hgo_register(struct vsp_prv_data *prv)
{
	struct vsp_res_data *rdata = &prv->rdata;
	struct vsp_hgo_info *hgo_info =
		&prv->ch_info[prv->ridx].hgo_info;

	unsigned int *dst;

	unsigned int offset;
	unsigned int i;

	if (rdata->start_reservation == 1) {
		/* set HGO read buffer */
		vsp_write_reg(
			(unsigned int)prv->ridx, prv->vsp_reg, VSP_HGO_RBUFS);
	}

	if (rdata->start_reservation < 2) {
		if ((hgo_info->val_mode & VSP_HGO_MODE_STEP) == 0) {
			/* set pointer */
			dst = hgo_info->virt_addr;

			/* read value */
			offset = VSP_HGO_R_HISTO_OFFSET;
			for (i = 0; i < 64; i++) {
				*dst++ = vsp_read_reg(prv->vsp_reg, offset);
				offset += 4;
			}

			offset = VSP_HGO_G_HISTO_OFFSET;
			for (i = 0; i < 64; i++) {
				*dst++ = vsp_read_reg(prv->vsp_reg, offset);
				offset += 4;
			}

			offset = VSP_HGO_B_HISTO_OFFSET;
			for (i = 0; i < 64; i++) {
				*dst++ = vsp_read_reg(prv->vsp_reg, offset);
				offset += 4;
			}
		} else {
			/* set pointer */
			dst = hgo_info->virt_addr;

			for (i = 0; i < 256; i++) {
				/* set reading address */
				vsp_write_reg(
					i, prv->vsp_reg, VSP_HGO_EXT_HIST_ADDR);

				/* read value */
				*dst++ = vsp_read_reg(
					prv->vsp_reg, VSP_HGO_EXT_HIST_DATA);
			}
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_get_hgt_register
 * Description:	Get histogram from HGT register.
 * Returns:		0
 ******************************************************************************/
static long vsp_ins_get_hgt_register(struct vsp_prv_data *prv)
{
	struct vsp_res_data *rdata = &prv->rdata;
	struct vsp_hgt_info *hgt_info =
		&prv->ch_info[prv->ridx].hgt_info;

	unsigned int *dst = hgt_info->virt_addr;
	unsigned int offset = VSP_HGT_HIST_OFFSET;
	int i;

	if (rdata->start_reservation == 1) {
		/* set HGT read buffer */
		vsp_write_reg(
			(unsigned int)prv->ridx, prv->vsp_reg, VSP_HGT_RBUFS);
	}

	if (rdata->start_reservation < 2) {
		for (i = 0; i < 192; i++) {
			*dst++ = vsp_read_reg(prv->vsp_reg, offset);
			offset += 4;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_get_module_register
 * Description:	Get module register value.
 * Returns:		0
 *	return of vsp_ins_get_hgo_register()
 *	return of vsp_ins_get_hgt_register()
 ******************************************************************************/
static long vsp_ins_get_module_register(
	struct vsp_prv_data *prv, unsigned long module)
{
	long ercd;

	/* set histogram generator-one dimension parameter */
	if (module & VSP_HGO_USE) {
		ercd = vsp_ins_get_hgo_register(prv);
		if (ercd)
			return ercd;
	}

	/* set histogram generator-two dimension parameter */
	if (module & VSP_HGT_USE) {
		ercd = vsp_ins_get_hgt_register(prv);
		if (ercd)
			return ercd;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_set_start_parameter
 * Description:	Set vsp_start_t parameter.
 * Returns:		0
 ******************************************************************************/
long vsp_ins_set_start_parameter(
	struct vsp_prv_data *prv, struct vsp_start_t *param)
{
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];

	/* set display list write address */
	ch_info->next_dl_addr = param->dl_par.hard_addr;

	if (ch_info->part_info.div_flag == 0) {
		/* unnecessary partition */
		vsp_ins_set_part_full(prv, param);
	} else {
		/* necessary partition */
		vsp_ins_set_part_parameter(prv, param);
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_start_processing
 * Description:	Start VSP processing.
 * Returns:		void
 ******************************************************************************/
void vsp_ins_start_processing(struct vsp_prv_data *prv)
{
	struct vsp_res_data *rdata = &prv->rdata;
	struct vsp_ch_info *ch_info = &prv->ch_info[prv->widx];

	/* set display list header address */
	vsp_write_reg(
		ch_info->wpf_info.val_dl_addr,
		prv->vsp_reg,
		VSP_DL_HDR_ADDR0);

	/* enable interrupt */
	vsp_write_reg(VSP_IRQ_FRMEND, prv->vsp_reg, VSP_WPF0_IRQ_ENB);

	/* start */
	vsp_write_reg(VSP_CMD_STRCMD, prv->vsp_reg, VSP_WPF0_CMD);

	if (rdata->start_reservation != 0) {
		/* update write index */
		prv->widx = 1 - prv->widx;
	}
}

/******************************************************************************
 * Function:		vsp_ins_stop_processing
 * Description:	Forced stop VSP processing.
 * Returns:		0
 ******************************************************************************/
long vsp_ins_stop_processing(struct vsp_prv_data *prv)
{
	unsigned int status;
	unsigned int loop_cnt;

	/* disable interrupt */
	vsp_write_reg(0, prv->vsp_reg, VSP_WPF0_IRQ_ENB);

	/* clear interrupt */
	vsp_write_reg(0, prv->vsp_reg, VSP_WPF0_IRQ_STA);

	/* dummy read */
	vsp_read_reg(prv->vsp_reg, VSP_WPF0_IRQ_STA);
	vsp_read_reg(prv->vsp_reg, VSP_WPF0_IRQ_STA);

	/* init loop counter */
	loop_cnt = VSP_STATUS_LOOP_CNT;

	/* read status register of VSP */
	status = vsp_read_reg(prv->vsp_reg, VSP_STATUS);

	if (status & VSP_STATUS_WPF0) {
		/* software reset */
		vsp_write_reg(VSP_SRESET_WPF0, prv->vsp_reg, VSP_SRESET);

		/* waiting reset process */
		do {
			/* sleep */
			msleep(VSP_STATUS_LOOP_TIME);

			/* read status register of VSP */
			status = vsp_read_reg(prv->vsp_reg, VSP_STATUS);
		} while ((status & VSP_STATUS_WPF0) &&
			(--loop_cnt > 0));
	}

	/* disable callback function */
	prv->ch_info[0].cb_func = NULL;
	prv->ch_info[1].cb_func = NULL;

	/* callback function */
	if (loop_cnt != 0) {
		vsp_ins_cb_function(prv, R_VSPM_CANCEL);
		vsp_ins_cb_function(prv, R_VSPM_CANCEL);
	} else {
		APRINT("%s: happen to timeout after reset of VSP!!\n",
		       __func__);
		vsp_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
		vsp_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
	}

	/* init loop counter */
	loop_cnt = VSP_STATUS_LOOP_CNT;

	/* read status register of FCP */
	status = vsp_read_reg(prv->fcp_reg, VSP_FCP_STA);

	if (status & VSP_FCP_STA_ACT) {
		/* reset */
		vsp_write_reg(VSP_FCP_RST_SOFTRST, prv->fcp_reg, VSP_FCP_RST);

		/* waiting reset process */
		do {
			/* sleep */
			msleep(VSP_STATUS_LOOP_TIME);

			/* read status register of FCP */
			status = vsp_read_reg(prv->fcp_reg, VSP_FCP_STA);
		} while ((status & VSP_FCP_STA_ACT) &&
			(--loop_cnt > 0));
	}

	if (loop_cnt == 0) {
		APRINT("%s: happen to timeout after reset of FCP!!\n",
		       __func__);
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_wait_processing
 * Description:	Waiting VSP processing.
 * Returns:		0
 ******************************************************************************/
long vsp_ins_wait_processing(struct vsp_prv_data *prv)
{
	unsigned int loop_cnt = VSP_STATUS_LOOP_CNT;

	do {
		/* sleep */
		msleep(VSP_STATUS_LOOP_TIME);

		if (prv->ch_info[0].status != VSP_STAT_RUN &&
		    prv->ch_info[1].status != VSP_STAT_RUN)
			break;
	} while (--loop_cnt > 0);

	if (loop_cnt == 0) {
		APRINT("%s: happen to timeout!!\n", __func__);
		vsp_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
		vsp_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_software_reset
 * Description:	Software reset of VSP and FCP.
 * Returns:		void
 ******************************************************************************/
static void vsp_ins_software_reset(struct vsp_prv_data *prv)
{
	/* Forced stop the VSP processing */
	(void)vsp_ins_stop_processing(prv);
}

/******************************************************************************
 * Function:		vsp_ins_get_vsp_resource
 * Description:	Get VSP resource.
 * Returns:		0/E_VSP_PARA_INPAR
 ******************************************************************************/
long vsp_ins_get_vsp_resource(struct vsp_prv_data *prv)
{
	struct device_node *np = prv->pdev->dev.of_node;
	struct vsp_res_data *rdata = &prv->rdata;

	/* read usable RPF bits */
	of_property_read_u32(np, "renesas,#rpf", &rdata->usable_rpf);
	if (rdata->usable_rpf >= (1 << VSP_RPF_MAX))
		return E_VSP_PARA_INPAR;

	/* read usable clut of RPF bits */
	of_property_read_u32(np, "renesas,#rpf_clut", &rdata->usable_rpf_clut);
	if (~rdata->usable_rpf & rdata->usable_rpf_clut)
		return E_VSP_PARA_INPAR;

	/* read usable WPF bits */
	rdata->usable_wpf = 0x00000001;	/* Fixed */

	/* read usable WPF bits */
	of_property_read_u32(np, "renesas,#wpf_rot", &rdata->usable_wpf_rot);
	if (~rdata->usable_wpf & rdata->usable_wpf_rot)
		return E_VSP_PARA_INPAR;

	/* read usable module */
	rdata->usable_module = 0;
	if (of_property_read_bool(np, "renesas,has-sru"))
		rdata->usable_module |= VSP_SRU_USE;

	if (of_property_read_bool(np, "renesas,has-uds"))
		rdata->usable_module |= VSP_UDS_USE;

	if (of_property_read_bool(np, "renesas,has-lut"))
		rdata->usable_module |= VSP_LUT_USE;

	if (of_property_read_bool(np, "renesas,has-clu"))
		rdata->usable_module |= VSP_CLU_USE;

	if (of_property_read_bool(np, "renesas,has-hst"))
		rdata->usable_module |= VSP_HST_USE;

	if (of_property_read_bool(np, "renesas,has-hsi"))
		rdata->usable_module |= VSP_HSI_USE;

	if (of_property_read_bool(np, "renesas,has-bru"))
		rdata->usable_module |= VSP_BRU_USE;

	if (of_property_read_bool(np, "renesas,has-brs"))
		rdata->usable_module |= VSP_BRS_USE;

	if (of_property_read_bool(np, "renesas,has-hgo"))
		rdata->usable_module |= VSP_HGO_USE;

	if (of_property_read_bool(np, "renesas,has-hgt"))
		rdata->usable_module |= VSP_HGT_USE;

	if (of_property_read_bool(np, "renesas,has-shp"))
		rdata->usable_module |= VSP_SHP_USE;

	/* read out standing value */
	of_property_read_u32(
		np,
		"renesas,#read_outstanding",
		&rdata->read_outstanding);
	if (rdata->read_outstanding != FCP_MODE_64 &&
	    rdata->read_outstanding != FCP_MODE_16)
		return E_VSP_PARA_INPAR;

	/* read start resevation value */
	of_property_read_u32(
		np,
		"renesas,#start_reservation",
		&rdata->start_reservation);

	/* bus access control value */
	rdata->burst_enable = !of_property_read_u32(np,
						    "renesas,#burst_access",
						    &rdata->burst_access);

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_enable_clock
 * Description:	Enable VSP/FCP clock supply.
 * Returns:		0/E_VSP_NO_CLK
 ******************************************************************************/
long vsp_ins_enable_clock(struct vsp_prv_data *prv)
{
	struct platform_device *pdev = prv->pdev;
	struct device *dev = &pdev->dev;

	int ercd;

	/* wake up device */
	ercd = pm_runtime_get_sync(dev);
	if (ercd < 0) {
		EPRINT("%s: failed to pm_runtime_get_sync!! ercd=%d\n",
		       __func__, ercd);
		return E_VSP_NO_CLK;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_disable_clock
 * Description:	Disable VSP/FCP clock supply.
 * Returns:		0
 ******************************************************************************/
long vsp_ins_disable_clock(struct vsp_prv_data *prv)
{
	struct platform_device *pdev = prv->pdev;

	/* mark device as idle */
	pm_runtime_put_sync(&pdev->dev);

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_init_vsp_reg
 * Description:	Initialize VSP hardware.
 * Returns:		0
 ******************************************************************************/
static long vsp_ins_init_vsp_reg(struct vsp_prv_data *prv)
{
	struct vsp_res_data *rdata = &prv->rdata;
	unsigned int reg_temp;

	/* initialize dynamic clock stop setting */
	vsp_write_reg(0x00000808, prv->vsp_reg, VSP_CLK_DCSWT);

	/* initialize bus access control register */
	if (rdata->burst_enable) {
		if (rdata->burst_access == 1)
			reg_temp = VSP_RPF_BAC_B512P;
		else
			reg_temp = VSP_RPF_BAC_B256P;

		if (rdata->usable_rpf & VSP_RPF0_USE)
			vsp_write_reg(reg_temp, prv->vsp_reg,
				      VSP_RPF0_OFFSET + VSP_RPF_BAC);

		if (rdata->usable_rpf & VSP_RPF1_USE)
			vsp_write_reg(reg_temp, prv->vsp_reg,
				      VSP_RPF1_OFFSET + VSP_RPF_BAC);

		if (rdata->usable_rpf & VSP_RPF2_USE)
			vsp_write_reg(reg_temp, prv->vsp_reg,
				      VSP_RPF2_OFFSET + VSP_RPF_BAC);

		if (rdata->usable_rpf & VSP_RPF3_USE)
			vsp_write_reg(reg_temp, prv->vsp_reg,
				      VSP_RPF3_OFFSET + VSP_RPF_BAC);

		if (rdata->usable_rpf & VSP_RPF4_USE)
			vsp_write_reg(reg_temp, prv->vsp_reg,
				      VSP_RPF4_OFFSET + VSP_RPF_BAC);
	}

	/* initialize DL control register */
	vsp_write_reg(
		VSP_DL_CTRL_WAIT |
		VSP_DL_CTRL_RLM0 |
		VSP_DL_CTRL_DLE,
		prv->vsp_reg,
		VSP_DL_CTRL);

	/* initialize DL swap register */
	vsp_write_reg(VSP_DL_SWAP_LWS, prv->vsp_reg, VSP_DL_SWAP);

	/* clear interrupt */
	vsp_write_reg(0, prv->vsp_reg, VSP_WPF0_IRQ_STA);

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_init_fcp_reg
 * Description:	Initialize FCP hardware.
 * Returns:		0
 ******************************************************************************/
static long vsp_ins_init_fcp_reg(struct vsp_prv_data *prv)
{
	/* set configuration register */
	vsp_write_reg(
		prv->rdata.read_outstanding, prv->fcp_reg, VSP_FCP_CFG0);

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_init_reg
 * Description:	Initialize hardware register.
 * Returns:		0/E_VSP_DEF_REG
 *	return of vsp_ins_init_vsp_reg()
 *	return of vsp_ins_init_fcp_reg()
 ******************************************************************************/
long vsp_ins_init_reg(struct vsp_prv_data *prv)
{
	struct resource *res;
	long ercd = 0;

	/* get an I/O memory resource of VSP */
	res = platform_get_resource(prv->pdev, IORESOURCE_MEM, 0);
	if (!res) {
		EPRINT("%s: failed to get resource of VSP!!\n", __func__);
		return E_VSP_DEF_REG;
	}

	/* remap an I/O memory of VSP */
	prv->vsp_reg = ioremap(res->start, resource_size(res));
	if (!prv->vsp_reg) {
		EPRINT("%s: failed to ioremap of VSP!!\n", __func__);
		return E_VSP_DEF_REG;
	}

	/* get an I/O memory resource for FCP */
	res = platform_get_resource(prv->pdev, IORESOURCE_MEM, 1);
	if (!res) {
		EPRINT("%s: failed to get resource of FCP!!\n", __func__);
		iounmap(prv->vsp_reg);
		return E_VSP_DEF_REG;
	}

	/* remap and I/O memory of FCP */
	prv->fcp_reg = ioremap(res->start, resource_size(res));
	if (!prv->fcp_reg) {
		EPRINT("%s: failed to ioremap of FCP!!\n", __func__);
		iounmap(prv->vsp_reg);
		return E_VSP_DEF_REG;
	}

	/* software reset */
	vsp_ins_software_reset(prv);

	/* initialize VSP register */
	ercd = vsp_ins_init_vsp_reg(prv);
	if (ercd)
		return ercd;

	/* initialize FCP register */
	ercd = vsp_ins_init_fcp_reg(prv);

	return ercd;
}

/******************************************************************************
 * Function:		vsp_ins_quit_reg
 * Description:	Finalize VSP hardware.
 * Returns:		0
 ******************************************************************************/
long vsp_ins_quit_reg(struct vsp_prv_data *prv)
{
	/* unmap an I/O register of FCP */
	if (prv->fcp_reg) {
		iounmap(prv->fcp_reg);
		prv->fcp_reg = NULL;
	}

	/* unmap an I/O register of VSP */
	if (prv->vsp_reg) {
		iounmap(prv->vsp_reg);
		prv->vsp_reg = NULL;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_cb_function
 * Description:	Callback function processing.
 * Returns:		void
 ******************************************************************************/
void vsp_ins_cb_function(struct vsp_prv_data *prv, long ercd)
{
	struct vsp_ch_info *ch_info;

	void (*cb_func)
		(unsigned long id, long ercd, void *userdata);
	unsigned long id;
	void *userdata;

	/* check parameter */
	if (!prv) {
		APRINT("%s: private data is null!!\n", __func__);
		return;
	}

	ch_info = &prv->ch_info[prv->ridx];
	if (ch_info->status == VSP_STAT_RUN) {
		/* copy histogram */
		(void)vsp_ins_get_module_register(
			prv, ch_info->reserved_module);

		ch_info->reserved_rpf = 0;
		ch_info->reserved_module = 0;

		/* set callback information */
		cb_func = ch_info->cb_func;
		id = (unsigned long)prv->ridx;
		userdata = ch_info->cb_userdata;

		if (prv->rdata.start_reservation != 0) {
			/* update read index */
			prv->ridx = 1 - prv->ridx;
		}

		/* update status */
		ch_info->status = VSP_STAT_READY;

		/* callback function */
		if (cb_func)
			cb_func(id, ercd, userdata);
	}
}

/******************************************************************************
 * Function:		vsp_ins_ih
 * Description:	Interrupt handler.
 * Returns:		IRQ_HANDLED
 ******************************************************************************/
static irqreturn_t vsp_ins_ih(int irq, void *dev)
{
	struct vsp_prv_data *prv = (struct vsp_prv_data *)dev;
	struct vsp_ch_info *ch_info;

	unsigned int tmp;

	/* check finished channel */
	ch_info = &prv->ch_info[prv->ridx];
	if (ch_info->status == VSP_STAT_RUN) {
		/* read control register */
		tmp = vsp_read_reg(prv->vsp_reg, VSP_WPF0_IRQ_STA);

		if ((tmp & VSP_IRQ_FRMEND) == VSP_IRQ_FRMEND) {
			/* clear interrupt */
			vsp_write_reg(0, prv->vsp_reg, VSP_WPF0_IRQ_STA);

			/* dummy read */
			vsp_read_reg(prv->vsp_reg, VSP_WPF0_IRQ_STA);
			vsp_read_reg(prv->vsp_reg, VSP_WPF0_IRQ_STA);

			/* callback function */
			vsp_ins_cb_function(prv, R_VSPM_OK);
		}
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * Function:		vsp_ins_reg_ih
 * Description:	Registory interrupt handler.
 * Returns:		0/E_VSP_DEF_INH
 ******************************************************************************/
long vsp_ins_reg_ih(struct vsp_prv_data *prv)
{
	int ercd;

	/* get irq information from platform */
	prv->irq = platform_get_resource(prv->pdev, IORESOURCE_IRQ, 0);
	if (!prv->irq) {
		EPRINT("%s: failed to get IRQ resource!!\n", __func__);
		return E_VSP_DEF_INH;
	}

	/* registory interrupt handler */
	ercd = request_irq(
		prv->irq->start,
		vsp_ins_ih,
		IRQF_SHARED,
		dev_name(&prv->pdev->dev),
		prv);
	if (ercd) {
		EPRINT("%s: failed to request irq!! ercd=%d, irq=%d\n",
		       __func__, ercd, (int)prv->irq->start);
		prv->irq = NULL;
		return E_VSP_DEF_INH;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_ins_unreg_ih
 * Description:	Unregistory interrupt handler.
 * Returns:		0
 ******************************************************************************/
long vsp_ins_unreg_ih(struct vsp_prv_data *prv)
{
	/* release interrupt handler */
	if (prv->irq) {
		free_irq(prv->irq->start, prv);
		prv->irq = NULL;
	}

	return 0;
}
