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

#include "fdp_drv_public.h"

static struct fdp_obj_t *g_fdp_obj[VSPM_FDP_IP_MAX] = {NULL};

/******************************************************************************
 * Function:		vspm_ins_fdp_ch
 * Description:	Get channel number from module_id.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_fdp_ch(unsigned short module_id, unsigned char *ch)
{
	/* chack range */
	if (!IS_FDP_CH(module_id)) {
		EPRINT("%s: Invalid module ID module_id=%d\n",
		       __func__, module_id);
		return R_VSPM_NG;
	}

	/* set channel */
	*ch = (unsigned char)(module_id - VSPM_FDP_CH_OFFSET);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_cb_fdp
 * Description:	Callback function.
 * Returns:		void
 ******************************************************************************/
static void vspm_cb_fdp(unsigned long id, long ercd, void *userdata)
{
	unsigned long module_id = (unsigned long)userdata;

	vspm_inc_ctrl_on_driver_complete((unsigned short)module_id, ercd);
}

/******************************************************************************
 * Function:		vspm_ins_fdp_initialize
 * Description:	Initialize FDP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_fdp_initialize(
	struct vspm_usable_res_info *usable, struct vspm_drvdata *pdrv)
{
	long ercd;

	unsigned int usable_bits = 0;
	unsigned char ch;
	int i, j;

	ch = VSPM_FDP_IP_MAX - 1;
	for (i = 0; i < VSPM_FDP_IP_MAX; i++) {
		if (pdrv->fdp_pdev[ch]) {
			/* init the FDP driver */
			ercd = fdp_lib_init(&g_fdp_obj[ch]);
			if (ercd)
				goto err_exit;

			/* set platform device information */
			g_fdp_obj[ch]->pdev = pdrv->fdp_pdev[ch];

			/* open the FDP driver */
			ercd = fdp_lib_open(g_fdp_obj[ch]);
			if (ercd)
				goto err_exit;

			/* set usable channel bits */
			for (j = 0; j < VSPM_FDP_CH_MAX; j++) {
				usable_bits <<= 1;
				usable_bits |= 0x1;
			}
		} else {
			/* skip usable channel bits */
			for (j = 0; j < VSPM_FDP_CH_MAX; j++)
				usable_bits <<= 1;
		}
		ch--;
	}

	usable_bits <<= VSPM_FDP_CH_OFFSET;
	usable->ch_bits |= usable_bits;

	return R_VSPM_OK;

err_exit:
	for (i = 0; i < VSPM_FDP_IP_MAX; i++) {
		if (g_fdp_obj[i]) {
			(void)fdp_lib_quit(g_fdp_obj[i]);
			g_fdp_obj[i] = NULL;
		}
	}

	return R_VSPM_NG;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_execute
 * Description:	Execute FDP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of fdp_lib_start()
 ******************************************************************************/
long vspm_ins_fdp_execute(
	unsigned short module_id,
	struct fdp_start_t *fdp_par,
	struct vspm_request_res_info *request)
{
	struct vspm_fdp_proc_info *proc_info = &request->fdp_info;

	struct fdp_obj_t *obj;
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_fdp_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;
	obj = g_fdp_obj[ch];

	/* check parameter */
	if (!obj)
		return R_VSPM_NG;

	/* check status */
	if (obj->status != FDP_STAT_READY)
		return E_FDP_INVALID_STATE;

	if (!obj->fdp_reg)
		return E_FDP_INVALID_STATE;

	/* set processing information */
	obj->proc_info = proc_info;

	/* set callback function */
	obj->cb_info.userdata2 = (void *)(unsigned long)module_id;
	obj->cb_info.fdp_cb2 = (void *)vspm_cb_fdp;

	/* execute FDP process */
	ercd = fdp_lib_start(obj, fdp_par);
	if (ercd)
		return ercd;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_exec_complete
 * Description:	Complete FDP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_fdp_exec_complete(unsigned short module_id)
{
	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_cancel
 * Description:	Cancel FDP driver.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_fdp_cancel(unsigned short module_id)
{
	struct fdp_obj_t *obj;
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_fdp_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;
	obj = g_fdp_obj[ch];

	/* abort process */
	ercd = fdp_lib_abort(obj);
	if (ercd)
		return R_VSPM_NG;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_quit
 * Description:	Finalize FDP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_fdp_quit(struct vspm_usable_res_info *usable)
{
	unsigned int usable_bits = 0;
	int i, j;

	for (i = 0; i < VSPM_FDP_IP_MAX; i++) {
		if (g_fdp_obj[i]) {
			/* finalize FDP driver */
			(void)fdp_lib_quit(g_fdp_obj[i]);

			g_fdp_obj[i] = NULL;

			/* clear usable channel bits */
			for (j = 0; j < VSPM_FDP_CH_MAX; j++) {
				usable_bits <<= 1;
				usable_bits |= 0x1;
			}
		} else {
			/* skip usable channel bits */
			for (j = 0; j < VSPM_FDP_CH_MAX; j++)
				usable_bits <<= 1;
		}
	}

	usable_bits <<= VSPM_FDP_CH_OFFSET;
	usable->ch_bits &= ~(usable_bits);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_execute_low_delay
 * Description:	Execute FDP driver VSPM task through.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of fdp_lib_start()
 ******************************************************************************/
long vspm_ins_fdp_execute_low_delay(
	unsigned short module_id,
	struct vspm_api_param_entry *entry)
{
	struct vspm_fdp_proc_info *proc_info =
		&entry->priv->request_info.fdp_info;
	struct fdp_start_t *start_par =
		entry->p_ip_par->par.fdp;

	struct fdp_obj_t *obj;
	unsigned char ch = 0;

	long ercd;

	/* convert module ID to channel */
	ercd = vspm_ins_fdp_ch(module_id, &ch);
	if (ercd)
		return R_VSPM_NG;
	obj = g_fdp_obj[ch];

	/* check parameter */
	if (!obj)
		return R_VSPM_NG;

	/* check status */
	if (obj->status != FDP_STAT_READY)
		return E_FDP_INVALID_STATE;

	if (!obj->fdp_reg)
		return E_FDP_INVALID_STATE;

	/* set processing information */
	obj->proc_info = proc_info;

	/* set callback function */
	obj->cb_info.userdata2 = entry->user_data;
	obj->cb_info.fdp_cb2 = (void *)entry->pfn_complete_cb;

	/* execute FDP process */
	ercd = fdp_lib_start(obj, start_par);
	if (ercd)
		return ercd;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_suspend
 * Description:	Suspend FDP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_fdp_suspend(void)
{
	unsigned char ch;
	long ercd;

	for (ch = 0; ch < VSPM_FDP_IP_MAX; ch++) {
		/* suspend */
		ercd = fdp_lib_suspend(g_fdp_obj[ch]);
		if (ercd != 0) {
			APRINT("%s: failed to suspend ch=%d\n",
			       __func__, ch);
		}
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_fdp_resume
 * Description:	Resume FDP driver.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_fdp_resume(void)
{
	unsigned char ch;
	long ercd;

	for (ch = 0; ch < VSPM_FDP_IP_MAX; ch++) {
		/* resume */
		ercd = fdp_lib_resume(g_fdp_obj[ch]);
		if (ercd != 0) {
			APRINT("%s: failed to resume ch=%d\n",
			       __func__, ch);
		}
	}

	return R_VSPM_OK;
}
