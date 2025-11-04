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

#include <linux/string.h>

#include "frame.h"

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "vspm_lib_public.h"
#include "vspm_common.h"

#include "isu_drv_public.h"

/******************************************************************************
 * Function:		vspm_ins_isu_ch
 * Description:	Get channel number from module_id.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_isu_ch(unsigned short module_id, unsigned char *ch)
{
	/* check range */
	if (!IS_ISU_CH(module_id)) {
		EPRINT("%s: Invalid module ID module_id=%d\n",
		       __func__, module_id);
		return R_VSPM_NG;
	}

	/* set channel */
	*ch = (unsigned char)(module_id - VSPM_ISU_CH_OFFSET);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_cb_isu
 * Description:	Callback function.
 * Returns:		void
 ******************************************************************************/
static void vspm_cb_isu(unsigned long id, long ercd, void *userdata)
{
	unsigned long module_id = (unsigned long)userdata;

	/* callback function */
	vspm_inc_ctrl_on_driver_complete((unsigned short)module_id, ercd);
}

/******************************************************************************
 * Function:		vspm_ins_isu_initialize
 * Description:	Initialize ISU driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_isu_initialize(
	struct vspm_usable_res_info *usable, struct vspm_drvdata *pdrv)
{
	struct isu_init_t init_param;
	struct isu_open_t open_param;

	long ercd;

	unsigned int usable_bits = 0;
	unsigned char ch;
	int i, j;

	/* set parameter */
	init_param.ip_num = VSPM_ISU_IP_MAX;

	/* initialize driver */
	ercd = isu_lib_init(&init_param);
	if (ercd) {
		EPRINT("%s: failed to init!! ercd=%ld\n", __func__, ercd);
		return R_VSPM_NG;
	}

	ch = VSPM_ISU_IP_MAX - 1;
	for (i = 0; i < VSPM_ISU_IP_MAX; i++) {
		if (pdrv->isu_pdev[ch]) {
			/* set parameter */
			open_param.pdev = pdrv->isu_pdev[ch];

			/* open channel */
			ercd = isu_lib_open(ch, &open_param);
			if (ercd) {
				EPRINT("%s: failed to open!! (%d, %ld)\n",
				       __func__, ch, ercd);
				/* forced quit */
				(void)isu_lib_quit();
				return R_VSPM_NG;
			}

			/* set usable channel bits */
			for (j = 0; j < VSPM_ISU_CH_MAX; j++) {
				usable_bits <<= 1;
				usable_bits |= 0x1;
			}
		} else {
			/* skip usable channel bits */
			for (j = 0; j < VSPM_ISU_CH_MAX; j++)
				usable_bits <<= 1;
		}
		ch--;
	}

	/* set usable channel bits */
	usable_bits <<= VSPM_ISU_CH_OFFSET;
	usable->ch_bits |= usable_bits;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_execute
 * Description:	Execute VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of isu_lib_start()
 ******************************************************************************/
long vspm_ins_isu_execute(unsigned short module_id, struct isu_start_t *isu_par)
{
	struct isu_start_t *start_param;
	unsigned char ch = 0;
	struct isu_status_t status;
	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_isu_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;

	start_param = (struct isu_start_t *)isu_par;

	ercd = isu_lib_get_status(ch, &status);
	if (ercd) {
		EPRINT("%s: failed to get status!! ercd=%ld\n", __func__, ercd);
		return R_VSPM_NG;
	}

	/* execute ISU process */
	ercd = isu_lib_start(
		ch,
		(void *)vspm_cb_isu,
		start_param,
		(void *)(unsigned long)module_id);
	if (ercd)
		return ercd;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_exec_complete
 * Description:	Complete VSP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_isu_exec_complete(unsigned short module_id)
{
	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_cancel
 * Description:	Cancel VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_isu_cancel(unsigned short module_id)
{
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_isu_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;

	/* abort process */
	ercd = isu_lib_abort(ch);
	if (ercd) {
		EPRINT("%s: failed to cancel!! (%d, %ld)\n",
		       __func__, ch, ercd);
		return R_VSPM_NG;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_quit
 * Description:	Finalize VSP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_isu_quit(struct vspm_usable_res_info *usable)
{
	long ercd;

	unsigned int usable_bits = 0;
	int i;

	/* quit driver with close all channels */
	ercd = isu_lib_quit();
	if (ercd) {
		EPRINT("%s: failed to quit!!() ercd=%ld\n",
		       __func__, ercd);
		return R_VSPM_NG;
	}

	/* clear usable channel bits */
	for (i = 0; i < VSPM_ISU_CH_NUM; i++) {
		usable_bits <<= 1;
		usable_bits |= 0x1;
	}

	usable_bits <<= VSPM_ISU_CH_OFFSET;
	usable->ch_bits &= ~(usable_bits);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_execute_low_delay
 * Description:	Execute VSP driver VSPM task through.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of isu_lib_start()
 ******************************************************************************/
long vspm_ins_isu_execute_low_delay(
	unsigned short module_id,
	struct vspm_api_param_entry *entry)
{
	struct isu_start_t *start_param;
	unsigned char ch = 0;
	struct isu_status_t status;
	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_isu_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;

	start_param = entry->p_ip_par->par.isu;

	ercd = isu_lib_get_status(ch, &status);
	if (ercd) {
		EPRINT("%s: failed to get status!! ercd=%ld\n", __func__, ercd);
		return R_VSPM_NG;
	}

	/* execute ISU process */
	ercd = isu_lib_start(
		ch,
		(void *)entry->pfn_complete_cb,
		start_param,
		entry->user_data);
	if (ercd)
		return ercd;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_suspend
 * Description:	Suspend VSP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_isu_suspend(void)
{
	unsigned char ch;
	long ercd;

	for (ch = 0; ch < VSPM_ISU_IP_MAX; ch++) {
		/* suspend */
		ercd = isu_lib_suspend(ch);
		if (ercd != 0) {
			APRINT("%s: failed to suspend ch=%d\n",
			       __func__, ch);
		}
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_isu_resume
 * Description:	Resume VSP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_isu_resume(void)
{
	unsigned char ch;
	long ercd;

	for (ch = 0; ch < VSPM_ISU_IP_MAX; ch++) {
		/* resume */
		ercd = isu_lib_resume(ch);
		if (ercd != 0) {
			APRINT("%s: failed to resume ch=%d\n",
			       __func__, ch);
		}
	}

	return R_VSPM_OK;
}
