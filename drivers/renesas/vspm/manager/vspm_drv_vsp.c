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

#include <linux/string.h>

#include "frame.h"

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "vspm_lib_public.h"
#include "vspm_common.h"

#include "vsp_drv_public.h"

/******************************************************************************
 * Function:		vspm_ins_vsp_ch
 * Description:	Get channel number from module_id.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_vsp_ch(unsigned short module_id, unsigned char *ch)
{
	/* check range */
	if (!IS_VSP_CH(module_id)) {
		EPRINT("%s: Invalid module ID module_id=%d\n",
		       __func__, module_id);
		return R_VSPM_NG;
	}

	/* set channel */
	*ch = (unsigned char)(module_id - VSPM_VSP_CH_OFFSET);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_get_bit_count
 * Description:	Get bit count.
 * Returns:		bit count value.
 ******************************************************************************/
static long vspm_ins_get_bit_count(unsigned int bits)
{
	unsigned long cnt = 0;

	/* count bit */
	bits--;
	while (bits) {
		bits &= (bits - 1);
		cnt++;
	}

	return cnt;
}

/******************************************************************************
 * Function:		vspm_ins_assign_rpf
 * Description:	Assignment RPF channel.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
static long vspm_ins_assign_rpf(
	unsigned char ch, struct vsp_start_t *start_param)
{
	struct vsp_src_t **src_par;
	struct vsp_status_t status;

	unsigned int not_clut_bits;
	unsigned int clut_bits;

	unsigned int bit;
	unsigned long rpf_order = 0;

	long ercd;

	unsigned char num;

	/* get status */
	ercd = vsp_lib_get_status(ch, &status);
	if (ercd) {
		EPRINT("%s: failed to get status!! ercd=%ld\n", __func__, ercd);
		return R_VSPM_NG;
	}

	/* get RPF bits */
	not_clut_bits = status.rpf_bits & ~(status.rpf_clut_bits);
	clut_bits = status.rpf_bits & status.rpf_clut_bits;

	src_par = &start_param->src_par[0];
	for (num = 0; num < start_param->rpf_num; num++) {
		unsigned char use_rpf_flag = 1;
		unsigned char use_rpf_clut_flag = 1;

		if (num >= 5) {
			/* rpf_num = 5 or over is not supported. Stop assign. */
			break;
		}

		if (*src_par) {
			if (((*src_par)->format == VSP_IN_RGB_CLUT_DATA) ||
			    ((*src_par)->format == VSP_IN_YUV_CLUT_DATA)) {
				/* If using CLUT of RPF, */
				/* RPF unsupported CLUT can not be used. */
				use_rpf_flag = 0;
			}
		}

		/* searching bit of RPF unsupported CLUT. */
		if (use_rpf_flag) {
			bit = not_clut_bits & -not_clut_bits;
			if (bit != 0) {
				rpf_order |= vspm_ins_get_bit_count(bit)
					<< (num * 4);

				not_clut_bits &= ~(bit);
				use_rpf_clut_flag = 0;
			}
		}

		/* searching bit of RPF supported CLUT */
		if (use_rpf_clut_flag) {
			bit = clut_bits & -clut_bits;
			if (bit != 0) {
				rpf_order |= vspm_ins_get_bit_count(bit)
					<< (num * 4);

				clut_bits &= ~(bit);
			} else {
				EPRINT("%s: nothing to usable channel.\n",
				       __func__);
				return R_VSPM_NG;
			}
		}
		src_par++;
	}

	/* set assigned RPF channel */
	start_param->rpf_order = rpf_order;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_cb_vsp
 * Description:	Callback function.
 * Returns:		void
 ******************************************************************************/
static void vspm_cb_vsp(unsigned long id, long ercd, void *userdata)
{
	unsigned long module_id = (unsigned long)userdata;

	/* callback function */
	vspm_inc_ctrl_on_driver_complete((unsigned short)module_id, ercd);
}

/******************************************************************************
 * Function:		vspm_ins_vsp_initialize
 * Description:	Initialize VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_vsp_initialize(
	struct vspm_usable_res_info *usable, struct vspm_drvdata *pdrv)
{
	struct vsp_init_t init_param;
	struct vsp_open_t open_param;
	struct vsp_status_t status;

	long ercd;

	unsigned int usable_bits = 0;
	unsigned char ch;
	int i, j;

	/* set parameter */
	init_param.ip_num = VSPM_VSP_IP_MAX;

	/* initialize driver */
	ercd = vsp_lib_init(&init_param);
	if (ercd) {
		EPRINT("%s: failed to init!! ercd=%ld\n", __func__, ercd);
		return R_VSPM_NG;
	}

	ch = VSPM_VSP_IP_MAX - 1;
	for (i = 0; i < VSPM_VSP_IP_MAX; i++) {
		if (pdrv->vsp_pdev[ch]) {
			/* set parameter */
			open_param.pdev = pdrv->vsp_pdev[ch];

			/* open channel */
			ercd = vsp_lib_open(ch, &open_param);
			if (ercd) {
				EPRINT("%s: failed to open!! (%d, %ld)\n",
				       __func__, ch, ercd);
				/* forced quit */
				(void)vsp_lib_quit();
				return R_VSPM_NG;
			}

			/* get status */
			ercd = vsp_lib_get_status(ch, &status);
			if (ercd) {
				EPRINT("%s: failed to get!! (%d, %ld)\n",
				       __func__, ch, ercd);
				/* forced quit */
				(void)vsp_lib_quit();
				return R_VSPM_NG;
			}

			/* set usable vsp resource */
			usable->vsp_res[ch].module_bits = status.module_bits;
			usable->vsp_res[ch].rpf_bits = status.rpf_bits;
			usable->vsp_res[ch].rpf_clut_bits =
				status.rpf_clut_bits;
			usable->vsp_res[ch].wpf_rot_bits = status.wpf_rot_bits;

			/* set usable channel bits */
			for (j = 0; j < VSPM_VSP_CH_MAX; j++) {
				usable_bits <<= 1;
				usable_bits |= 0x1;
			}
		} else {
			/* skip usable channel bits */
			for (j = 0; j < VSPM_VSP_CH_MAX; j++)
				usable_bits <<= 1;
		}
		ch--;
	}

	/* set usable channel bits */
	usable_bits <<= VSPM_VSP_CH_OFFSET;
	usable->ch_bits |= usable_bits;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_execute
 * Description:	Execute VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vsp_lib_start()
 ******************************************************************************/
long vspm_ins_vsp_execute(unsigned short module_id, struct vsp_start_t *vsp_par)
{
	struct vsp_start_t *start_param;
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_vsp_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;

	start_param = (struct vsp_start_t *)vsp_par;

	/* assign RPF channel */
	ercd = vspm_ins_assign_rpf(ch, start_param);
	if (ercd)
		return R_VSPM_NG;

	/* execute VSP process */
	ercd = vsp_lib_start(
		ch,
		(void *)vspm_cb_vsp,
		start_param,
		(void *)(unsigned long)module_id);
	if (ercd)
		return ercd;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_exec_complete
 * Description:	Complete VSP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_vsp_exec_complete(unsigned short module_id)
{
	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_cancel
 * Description:	Cancel VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_vsp_cancel(unsigned short module_id)
{
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_vsp_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;

	/* abort process */
	ercd = vsp_lib_abort(ch);
	if (ercd) {
		EPRINT("%s: failed to cancel!! (%d, %ld)\n",
		       __func__, ch, ercd);
		return R_VSPM_NG;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_quit
 * Description:	Finalize VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_vsp_quit(struct vspm_usable_res_info *usable)
{
	long ercd;

	unsigned int usable_bits = 0;
	int i;

	/* quit driver with close all channels */
	ercd = vsp_lib_quit();
	if (ercd) {
		EPRINT("%s: failed to quit!!() ercd=%ld\n",
		       __func__, ercd);
		return R_VSPM_NG;
	}

	/* clear usable channel bits */
	for (i = 0; i < VSPM_VSP_CH_NUM; i++) {
		usable_bits <<= 1;
		usable_bits |= 0x1;
	}

	usable_bits <<= VSPM_VSP_CH_OFFSET;
	usable->ch_bits &= ~(usable_bits);

	/* clear usable vsp resource */
	for (i = 0; i < VSPM_VSP_IP_MAX; i++) {
		usable->vsp_res[i].module_bits = 0;
		usable->vsp_res[i].rpf_bits = 0;
		usable->vsp_res[i].rpf_clut_bits = 0;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_execute_low_delay
 * Description:	Execute VSP driver VSPM task through.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vsp_lib_start()
 ******************************************************************************/
long vspm_ins_vsp_execute_low_delay(
	unsigned short module_id,
	struct vspm_api_param_entry *entry)
{
	struct vsp_start_t *start_param;
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_vsp_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;

	start_param = entry->p_ip_par->par.vsp;

	/* assign RPF channel */
	ercd = vspm_ins_assign_rpf(ch, start_param);
	if (ercd)
		return R_VSPM_NG;

	/* execute VSP process */
	ercd = vsp_lib_start(
		ch,
		(void *)entry->pfn_complete_cb,
		start_param,
		entry->user_data);
	if (ercd)
		return ercd;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_suspend
 * Description:	Suspend VSP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_vsp_suspend(void)
{
	unsigned char ch;
	long ercd;

	for (ch = 0; ch < VSPM_VSP_IP_MAX; ch++) {
		/* suspend */
		ercd = vsp_lib_suspend(ch);
		if (ercd != 0) {
			APRINT("%s: failed to suspend ch=%d\n",
			       __func__, ch);
		}
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_vsp_resume
 * Description:	Resume VSP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_vsp_resume(void)
{
	unsigned char ch;
	long ercd;

	for (ch = 0; ch < VSPM_VSP_IP_MAX; ch++) {
		/* resume */
		ercd = vsp_lib_resume(ch);
		if (ercd != 0) {
			APRINT("%s: failed to resume ch=%d\n",
			       __func__, ch);
		}
	}

	return R_VSPM_OK;
}
