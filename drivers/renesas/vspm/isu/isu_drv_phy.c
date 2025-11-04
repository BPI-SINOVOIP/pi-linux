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
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "isu_drv_public.h"
#include "isu_drv_local.h"

/******************************************************************************
 * Function:		isu_write_reg
 * Description:	Write to register
 * Returns:		void
 ******************************************************************************/
inline void isu_write_reg(
	unsigned int data, void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *reg =
		(unsigned int __iomem *)base;
	reg += (offset >> 2);
	iowrite32(data, reg);
}

/******************************************************************************
 * Function:		isu_read_reg
 * Description:	Read from register
 * Returns:		void
 ******************************************************************************/
inline unsigned int isu_read_reg(void __iomem *base, unsigned int offset)
{
	unsigned int __iomem *reg =
		(unsigned int __iomem *)base;
	reg += (offset >> 2);
	return ioread32(reg);
}

/******************************************************************************
 * Function:		isu_ins_set_reg_for_rpf
 * Description:	Set RPF register value.
 * Returns:		void
 ******************************************************************************/
static void isu_ins_set_reg_for_rpf(struct isu_prv_data *prv,
struct isu_ch_info *ch_info)
{
	struct isu_rpf_info *rpf_info = &ch_info->rpf_info;
	unsigned int reg_temp;

	/* RPF Source Image Size Registers */
	reg_temp = (((unsigned int)rpf_info->width) << 16) & 0x0FFF0000;
	reg_temp |= ((unsigned int)rpf_info->height & 0x00000FFF);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_SRC_SIZE);

	/* RPF Source Stride Registers */
	reg_temp = ((unsigned int)rpf_info->stride) << 16;
	reg_temp |= (unsigned int)rpf_info->stride_c;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_SRC_STRD);

	/* Start address for Plane 0 */
	reg_temp = ((rpf_info->addr) >> 32) & 0x00000003;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_SRC_ADDH_PL0);
	reg_temp = (unsigned int)rpf_info->addr;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_SRC_ADDL_PL0);

	/* Start address for Plane 1 */
	reg_temp = ((rpf_info->addr_c) >> 32) & 0x00000003;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_SRC_ADDH_PL1);
	reg_temp = (unsigned int)rpf_info->addr_c;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_SRC_ADDL_PL1);

	/* RPF Source Image Format Registers */
	reg_temp = ((unsigned int)rpf_info->format) & 0x0000003F;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RPF_FMT);

	/* RPF Source Image UV Format Register */
	isu_write_reg(rpf_info->uv_bin, prv->isu_reg, ISU_RPF_UVBIN);

	/* RPF Source Image Data Swap Registers */
	isu_write_reg(rpf_info->swap, prv->isu_reg, ISU_RPF_SRC_DSWAP);

	/* RPF Source ALPHA Data Selection Registers */
	if (rpf_info->format == ISU_ARGB1555)
		isu_write_reg(rpf_info->rpf_alpha_val, prv->isu_reg, ISU_RPF_ALPH_SEL);

	/* RPF Source TEST Data Register1 */
	isu_write_reg(rpf_info->src_td1, prv->isu_reg, ISU_RPF_SRC_TD1);
	/* RPF Source TEST Data Register2 */
	isu_write_reg(rpf_info->src_td2, prv->isu_reg, ISU_RPF_SRC_TD2);
}

/******************************************************************************
 * Function:		isu_ins_set_reg_for_rs
 * Description:	Set RS register value.
 * Returns:		void
 ******************************************************************************/
static void isu_ins_set_reg_for_rs(struct isu_prv_data *prv,
	struct isu_ch_info *ch_info)
{
	struct isu_rs_info *rs_info = &ch_info->rs_info;
	unsigned int reg_temp;

	/* RS Scaling Factor Registers 0 */
	isu_write_reg(rs_info->x_scale, prv->isu_reg, ISU_RS_HSCALE);

	/* RS Scaling Factor Registers 1 */
	isu_write_reg(rs_info->y_scale, prv->isu_reg, ISU_RS_VSCALE);

	/* RS Output Image Start Position Registers */
	reg_temp = (rs_info->start_x << 16);
	reg_temp |= (rs_info->start_y);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RS_STPOS);

	/* RS Output Image Start Position Tuning Registers */
	reg_temp = (rs_info->tune_x << 16);
	reg_temp |= (rs_info->tune_y);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RS_POS_TUNE);

	/* RS Output Size Crop Registers */
	reg_temp = (rs_info->crop_w << 16);
	reg_temp |= (rs_info->crop_h);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_RS_OS_CROP);

	/* RS CROP Padding Mode Registers */
	isu_write_reg(rs_info->pad_mode, prv->isu_reg, ISU_RS_PADDMODE);
	isu_write_reg(rs_info->pad_val, prv->isu_reg, ISU_RS_PADDVAL);
}

/******************************************************************************
 * Function:		isu_ins_set_reg_for_wpf
 * Description:	Set WPF register value.
 * Returns:		void
 ******************************************************************************/
static void isu_ins_set_reg_for_wpf(struct isu_prv_data *prv,
struct isu_ch_info *ch_info)
{
	struct isu_wpf_info *wpf_info = &ch_info->wpf_info;
	unsigned int reg_temp;
	unsigned long reg_temp0;

	/* WPF Destination Plane0 Address Registers 0 */
	reg_temp0 = (((unsigned long)wpf_info->addr) >> 32) & 0x00000003;
	reg_temp = (unsigned int)reg_temp0;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_DST_ADDH_PL0);
	reg_temp = (unsigned int)wpf_info->addr;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_DST_ADDL_PL0);

	/* WPF Destination Plane0 Address Registers 1 */
	reg_temp0 = (((unsigned long)wpf_info->addr_c) >> 32) & 0x00000003;
	reg_temp = (unsigned int)reg_temp0;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_DST_ADDH_PL1);
	reg_temp = (unsigned int)wpf_info->addr_c;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_DST_ADDL_PL1);

	/* WPF Destination Stride Registers */
	reg_temp = ((unsigned int)wpf_info->stride) << 16;
	reg_temp |= (unsigned int)wpf_info->stride_c;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_DST_STRD);

	/* WPF Destination Image Format Registers */
	reg_temp = ((unsigned int)wpf_info->format) & 0x0000003F;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_FMT);

	/* WPF Color Collection Control Registers */
	isu_write_reg(wpf_info->ccol, prv->isu_reg, ISU_WPF_CCOL);

	/* WPF Color Collection MUL Coefficient Registers1 */
	reg_temp = wpf_info->k_matrix[0][0];
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_MUL1);

	/* WPF Color Collection MUL Coefficient Registers2 */
	reg_temp = (wpf_info->k_matrix[0][2] << 16);
	reg_temp |= (wpf_info->k_matrix[0][1]);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_MUL2);

	/* WPF Color Collection MUL Coefficient Registers3 */
	reg_temp = wpf_info->k_matrix[1][0];
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_MUL3);

	/* WPF Color Collection MUL Coefficient Registers4 */
	reg_temp = (wpf_info->k_matrix[1][2] << 16);
	reg_temp |= (wpf_info->k_matrix[1][1]);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_MUL4);

	/* WPF Color Collection MUL Coefficient Registers5 */
	reg_temp = wpf_info->k_matrix[2][0];
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_MUL5);

	/* WPF Color Collection MUL Coefficient Registers6 */
	reg_temp = (wpf_info->k_matrix[2][2] << 16);
	reg_temp |= (wpf_info->k_matrix[2][1]);
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_MUL6);

	/* WPF Color Collection Offset Coefficient Registers1 */
	reg_temp = (wpf_info->offset[0][0] << 24);
	reg_temp |= ((wpf_info->offset[1][0] << 16));
	reg_temp |= ((wpf_info->offset[2][0] << 8));
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_OFST1);

	/* WPF Color Collection Offset Coefficient Registers2 */
	reg_temp = (wpf_info->offset[0][1] << 24);
	reg_temp |= ((wpf_info->offset[1][1] << 16));
	reg_temp |= ((wpf_info->offset[2][1] << 8));
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_OFST2);

	/* WPF Color Collection Clip Registers1 */
	reg_temp = wpf_info->clip[0][CLIP_MIN_INX];
	reg_temp |= ((wpf_info->clip[0][CLIP_MAX_INX] << 8));
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_CLP1);

	/* WPF Color Collection Clip Registers2 */
	reg_temp = (wpf_info->clip[1][CLIP_MAX_INX] << 24);
	reg_temp |= ((wpf_info->clip[1][CLIP_MIN_INX] << 16));
	reg_temp |= ((wpf_info->clip[2][CLIP_MAX_INX] << 8));
	reg_temp |= ((wpf_info->clip[2][CLIP_MIN_INX]));
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_CLP2);

	/* WPF Destination Image Data Swap Registers */
	reg_temp = ((unsigned int)wpf_info->swap) & 0x00000007;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_WPF_DST_DSWAP);

	/* WPF Destination ALPHA Selection Registers1 */
	isu_write_reg(wpf_info->alpha_asel1, prv->isu_reg, ISU_WPF_ALPH_SEL1);
	/* WPF Destination ALPHA Selection Registers2 */
	isu_write_reg(wpf_info->alpha_asel2, prv->isu_reg, ISU_WPF_ALPH_SEL2);
	isu_write_reg(wpf_info->alpha_val, prv->isu_reg, ISU_WPF_ALPH_VAL);
}

/******************************************************************************
 * Function:		isu_ins_set_start_parameter
 * Description:	Set isu_start_t parameter.
 * Returns:		0
 ******************************************************************************/
long isu_ins_set_start_parameter(
struct isu_prv_data *prv)
{
	struct isu_ch_info *ch_info = &prv->ch_info;

	if(!(ch_info->dl_info)){
		/* input module */
		isu_ins_set_reg_for_rpf(prv, ch_info);

		/* Scale module */
		isu_ins_set_reg_for_rs(prv, ch_info);

		/* output module */
		isu_ins_set_reg_for_wpf(prv, ch_info);
	} else
		isu_write_reg(ch_info->dl_info, prv->isu_reg, ISU_FM_DL_STADDL);

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_init_isu_reg
 * Description:	Initialize ISU hardware.
 * Returns:		0
 ******************************************************************************/
static long isu_ins_init_isu_reg(struct isu_prv_data *prv)
{
	/* Sets the upper limit to the burst length of the AXI-Master. */
	isu_write_reg(0x000f000f, prv->isu_reg, ISU_AXI_BLEN);

	/* clear interrupt */
	isu_write_reg(0, prv->isu_reg, ISU_FM_INT_STA);

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_init_reg
 * Description:	Initialize hardware register.
 * Returns:		0/E_ISU_DEF_REG
 *	return of isu_ins_init_isu_reg()
 ******************************************************************************/
long isu_ins_init_reg(struct isu_prv_data *prv)
{
struct resource *res;
long ercd = 0;

/* get an I/O memory resource of ISU */
	res = platform_get_resource(prv->pdev, IORESOURCE_MEM, 0);
	if (!res) {
		EPRINT("%s: failed to get resource of ISU!!\n", __func__);
		return E_ISU_DEF_REG;
	}

/* remap an I/O memory of ISU */
	prv->isu_reg = ioremap(res->start, resource_size(res));
	if (!prv->isu_reg) {
		EPRINT("%s: failed to ioremap of ISU!!\n", __func__);
		return E_ISU_DEF_REG;
	}

/* initialize ISU register */
	ercd = isu_ins_init_isu_reg(prv);

	return ercd;
}

/******************************************************************************
 * Function:		isu_ins_quit_reg
 * Description:	Finalize ISU hardware.
 * Returns:		0
 ******************************************************************************/
long isu_ins_quit_reg(struct isu_prv_data *prv)
{
		/* unmap an I/O register of ISU */
	if (prv->isu_reg) {
		iounmap(prv->isu_reg);
		prv->isu_reg = NULL;
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_enable_clock
 * Description:	Enable ISU clock supply.
 * Returns:		0/E_ISU_NO_CLK
 ******************************************************************************/
long isu_ins_enable_clock(struct isu_prv_data *prv)
{
	struct platform_device *pdev = prv->pdev;
	struct device *dev = &pdev->dev;
	int ercd;

	/* wake up device */
	ercd = pm_runtime_get_sync(dev);
	if (ercd < 0) {
		EPRINT("%s: failed to pm_runtime_get_sync!! ercd=%d\n",
			__func__, ercd);
		return E_ISU_NO_CLK;
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_disable_clock
 * Description:	Disable ISU clock supply.
 * Returns:		0
 ******************************************************************************/
long isu_ins_disable_clock(struct isu_prv_data *prv)
{
	struct platform_device *pdev = prv->pdev;

	/* mark device as idle */
	pm_runtime_put_sync(&pdev->dev);

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_get_isu_resource
 * Description:	Get ISU resource.
 * Returns:		0/E_ISU_PARA_INPAR
 ******************************************************************************/
long isu_ins_get_isu_resource(struct isu_prv_data *prv)
{
	struct device_node *np = prv->pdev->dev.of_node;
	struct isu_res_data *rdata = &prv->rdata;

/* read usable RPF bits */
	of_property_read_u32(np, "renesas,#rpf", &rdata->usable_rpf);
	if (rdata->usable_rpf >= (1 << ISU_RPF_MAX))
		return E_ISU_PARA_INPAR;

/* read usable WPF bits */
	of_property_read_u32(np, "renesas,#wpf", &rdata->usable_wpf);
	if (rdata->usable_wpf >= (1 << ISU_RPF_MAX))
		return E_ISU_PARA_INPAR;

	rdata->usable_module = 0;

	if (of_property_read_bool(np, "renesas,has-rs"))
		rdata->usable_module |= ISU_RS_USE;

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_start_processing
 * Description:	Start ISU processing.
 * Returns:		void
 ******************************************************************************/
void isu_ins_start_processing(struct isu_prv_data *prv)
{
	struct isu_ch_info *ch_info = &prv->ch_info;

	if(!(ch_info->dl_info)){
		/* enable interrupt */
		isu_write_reg(ISU_INT_FRENDE, prv->isu_reg, ISU_FM_INT_EN);
		/* start */
		isu_write_reg(ISU_START, prv->isu_reg, ISU_FM_FRCON);
	} else {
		isu_write_reg(ISU_INT_FRENDE|ISU_INT_LISTERRE, prv->isu_reg, ISU_FM_INT_EN);
		isu_write_reg(ISU_START|ISU_DESON, prv->isu_reg, ISU_FM_FRCON);
	}

}

/******************************************************************************
 * Function:		isu_ins_stop_processing
 * Description:	Forced stop ISU processing.
 * Returns:		0
 ******************************************************************************/
long isu_ins_stop_processing(struct isu_prv_data *prv)
{
	unsigned int status;
	unsigned int loop_cnt, reg_temp= 0;

	/* disable interrupt */
	isu_write_reg(0, prv->isu_reg, ISU_FM_INT_EN);

	/* clear interrupt */
	isu_write_reg(0, prv->isu_reg, ISU_FM_INT_STA);

	/* dummy read */
	isu_read_reg(prv->isu_reg,ISU_FM_INT_STA);
	isu_read_reg(prv->isu_reg,ISU_FM_INT_STA);

	/* Enable Controls interrupt by the completion of emergency stop */
	isu_write_reg(ISU_INT_SRSTENDE, prv->isu_reg, ISU_FM_INT_EN);

	/* Enable emergency stop */
	reg_temp |= ISU_STOP;
	isu_write_reg(reg_temp, prv->isu_reg, ISU_FM_STOP);

	/* init loop counter */
	loop_cnt = ISU_STATUS_LOOP_CNT;

	/* Waiting emergency stop process */
	do {
		/* sleep */
		msleep(ISU_STATUS_LOOP_TIME);

		/* read status register of ISU */
		status = (isu_read_reg(prv->isu_reg,ISU_FM_INT_STA))&ISU_INT_SRSTENDE;
	} while ((status!=ISU_INT_SRSTENDE)&&(--loop_cnt>0));

	/* disable callback function */
	prv->ch_info.cb_func = NULL;

	/* callback function */
	if (loop_cnt != 0) {
		isu_ins_cb_function(prv, R_VSPM_CANCEL);
		isu_ins_cb_function(prv, R_VSPM_CANCEL);
	} else {
		APRINT("%s: happen to timeout after reset of ISU!!\n",__func__);
		isu_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
		isu_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_wait_processing
 * Description:	Waiting ISU processing.
 * Returns:		0
 ******************************************************************************/
long isu_ins_wait_processing(struct isu_prv_data *prv)
{
	unsigned int loop_cnt = ISU_STATUS_LOOP_CNT;

	do {
		/* sleep */
		msleep(ISU_STATUS_LOOP_TIME);

		if (prv->ch_info.status != ISU_STAT_RUN)
			break;
	} while (--loop_cnt > 0);

	if (loop_cnt == 0) {
		APRINT("%s: happen to timeout!!\n", __func__);
		isu_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
		isu_ins_cb_function(prv, R_VSPM_DRIVER_ERR);
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_cb_function
 * Description:	Callback function processing.
 * Returns:		void
 ******************************************************************************/
void isu_ins_cb_function(struct isu_prv_data *prv, long ercd)
{
	struct isu_ch_info *ch_info;

	void (*cb_func)
		(unsigned long id, long ercd, void *userdata);
	unsigned long id;
	void *userdata;

	/* check parameter */
	if (!prv) {
		APRINT("%s: private data is null!!\n", __func__);
		return;
	}

	ch_info = &prv->ch_info;
	if (ch_info->status == ISU_STAT_RUN) {
		/* set callback information */
		cb_func = ch_info->cb_func;
		id = 0;
		userdata = ch_info->cb_userdata;

		/* update status */
		ch_info->status = ISU_STAT_READY;

		/* callback function */
		if (cb_func)
			cb_func(id, ercd, userdata);
	}
}

/******************************************************************************
 * Function:		isu_ins_ih
 * Description:	Interrupt handler.
 * Returns:		IRQ_HANDLED
 ******************************************************************************/
static irqreturn_t isu_ins_ih(int irq, void *dev)
{
	struct isu_prv_data *prv = (struct isu_prv_data *)dev;
	struct isu_ch_info *ch_info;
	unsigned int status;

	/* check finished channel */
	ch_info = &prv->ch_info;
	if (ch_info->status == ISU_STAT_RUN) {
		/* read control register */
		status = isu_read_reg(prv->isu_reg, ISU_FM_INT_STA);
		if (!(ch_info->dl_info)){
			/* Frame end interruption by rising edge detection */
			/* dummy read */
			isu_read_reg(prv->isu_reg, ISU_FM_INT_STA);
			isu_read_reg(prv->isu_reg, ISU_FM_INT_STA);

			/* callback function */
			isu_ins_cb_function(prv, R_VSPM_OK);
		} else {
			if ((status & ISU_INT_LISTERR)==ISU_INT_LISTERR){
				/* clear interrupt */
				isu_write_reg(0, prv->isu_reg, ISU_FM_INT_STA);

				/* callback function */
				isu_ins_cb_function(prv, E_ISU_DL_FORMAT);
			} else {
			        /* Frame end interruption by rising edge detection */
				/* dummy read */
				isu_read_reg(prv->isu_reg, ISU_FM_INT_STA);
				isu_read_reg(prv->isu_reg, ISU_FM_INT_STA);

				/* callback function */
				isu_ins_cb_function(prv, R_VSPM_OK);
			}
		}
	}
	return IRQ_HANDLED;
}

/******************************************************************************
 * Function:		isu_ins_reg_ih
 * Description:	Registory interrupt handler.
 * Returns:		0/E_ISU_DEF_INH
 ******************************************************************************/
long isu_ins_reg_ih(struct isu_prv_data *prv)
{
	int ercd;

	/* get irq information from platform */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	prv->irq = platform_get_resource(prv->pdev, IORESOURCE_IRQ, 0);
	if (!prv->irq) {
		EPRINT("%s: failed to get IRQ resource!!\n", __func__);
		return E_ISU_DEF_INH;
	}
#else
	prv->irq = platform_get_irq(prv->pdev, 0);
	if (prv->irq < 0) {
		EPRINT("%s: failed to get IRQ resource!!\n", __func__);
		return E_ISU_DEF_INH;
	}
#endif

	/* registory interrupt handler */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	ercd = request_irq(
		prv->irq->start,
		isu_ins_ih,
		IRQF_SHARED,
		dev_name(&prv->pdev->dev),
		prv);
	if (ercd) {
		EPRINT("%s: failed to request irq!! ercd=%d, irq=%d\n",
			__func__, ercd, (int)prv->irq->start);
		prv->irq = NULL;
		return E_VSP_DEF_INH;
	}
#else
	ercd = request_irq(
		prv->irq,
		isu_ins_ih,
		IRQF_SHARED,
		dev_name(&prv->pdev->dev),
		prv);
	if (ercd) {
		EPRINT("%s: failed to request irq!! ercd=%d, irq=%d\n",
			__func__, ercd, prv->irq);
		return E_VSP_DEF_INH;
	}
#endif

	return 0;
}

/******************************************************************************
 * Function:		isu_ins_unreg_ih
 * Description:	Unregistory interrupt handler.
 * Returns:		0
 ******************************************************************************/
long isu_ins_unreg_ih(struct isu_prv_data *prv)
{
	/* release interrupt handler */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	if (prv->irq) {
		free_irq(prv->irq->start, prv);
		prv->irq = NULL;
	}
#else
	if (prv->irq >= 0) {
		free_irq(prv->irq, prv);
	}
#endif

	return 0;
}
