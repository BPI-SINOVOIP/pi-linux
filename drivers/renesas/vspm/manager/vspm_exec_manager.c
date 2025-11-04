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

/******************************************************************************
 * Function:		vspm_ins_exec_start
 * Description:	Execute job.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_exec_start(
	struct vspm_exec_info *exec_info,
	unsigned short module_id,
	struct vspm_job_t *ip_par,
	struct vspm_job_info *job_info)
{
	struct vspm_request_res_info *request;
	unsigned int channel_bit = VSPM_CH_TO_BIT(module_id);
	long ercd;

	if (exec_info->exec_ch_bits & channel_bit) {
		EPRINT("%s Already executing module_id=0x%04x\n",
		       __func__, module_id);
		return R_VSPM_NG;
	}

	/* Update the execution information */
	exec_info->exec_ch_bits |= channel_bit;
	exec_info->p_exec_job_info[module_id] = job_info;

	if (IS_VSP_CH(module_id)) {
		/* Start the VSP process */
		ercd = vspm_ins_vsp_execute(
			module_id, ip_par->par.vsp);
	} else if (IS_FDP_CH(module_id)) {
		/* Get request information */
		request = vspm_ins_job_get_request_param(job_info);

		/* Start the FDP process */
		ercd = vspm_ins_fdp_execute(
			module_id, ip_par->par.fdp, request);
	} else if (IS_ISU_CH(module_id)) {
		/* Start the VSP process */
		ercd = vspm_ins_isu_execute(
			module_id, ip_par->par.isu);
	} else {
		EPRINT("%s Invalid module_id 0x%04x\n", __func__, module_id);
		ercd = R_VSPM_NG;
	}

	if (ercd) {
		/* Update the execution information */
		exec_info->p_exec_job_info[module_id] = NULL;
		exec_info->exec_ch_bits &= ~channel_bit;
	}

	return ercd;
}

/******************************************************************************
 * Function:		vspm_ins_exec_complete
 * Description:	Job completion processing.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_ins_exec_complete(
	struct vspm_exec_info *exec_info, unsigned short module_id)
{
	long ercd;

	if (IS_VSP_CH(module_id)) {
		/* VSP process complete */
		ercd = vspm_ins_vsp_exec_complete(module_id);
		if (ercd)
			return R_VSPM_NG;
	} else if (IS_FDP_CH(module_id)) {
		/* FDP process complete */
		ercd = vspm_ins_fdp_exec_complete(module_id);
		if (ercd)
			return R_VSPM_NG;
	} else if (IS_ISU_CH(module_id)) {
		/* ISU process complete */
		ercd = vspm_ins_isu_exec_complete(module_id);
		if (ercd)
			return R_VSPM_NG;
	} else {
		EPRINT("%s Invalid module_id 0x%04x\n", __func__, module_id);
		return R_VSPM_NG;
	}

	/* clear common information */
	exec_info->p_exec_job_info[module_id] = NULL;
	exec_info->exec_ch_bits &= VSPM_CH_TO_BIT_INVERT(module_id);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_exec_get_current_job_info
 * Description:	Get job information of the currently.
 * Returns:		pointer of job information/On error is NULL.
 ******************************************************************************/
struct vspm_job_info *vspm_ins_exec_get_current_job_info(
	struct vspm_exec_info *exec_info, unsigned short module_id)
{
	if (!(exec_info->exec_ch_bits & VSPM_CH_TO_BIT(module_id)))
		return NULL;

	return exec_info->p_exec_job_info[module_id];
}

/******************************************************************************
 * Function:		vspm_ins_exec_update_current_status
 * Description:	Update status of the running job.
 * Returns:		void
 ******************************************************************************/
void vspm_ins_exec_update_current_status(
	struct vspm_exec_info *exec_info, struct vspm_usable_res_info *usable)
{
	usable->ch_bits &= ~exec_info->exec_ch_bits;
}

/******************************************************************************
 * Function:		vspm_ins_exec_cancel
 * Description:	Cancel of job.
 * Returns:		R_VSPM_OK/R_VSPM_SEQ_PAR/R_VSPM_PARAERR
 *	return of vspm_ins_vsp_cancel()
 ******************************************************************************/
long vspm_ins_exec_cancel(
	struct vspm_exec_info *exec_info, unsigned short module_id)
{
	long ercd;

	if (!(exec_info->exec_ch_bits & VSPM_CH_TO_BIT(module_id))) {
		EPRINT("%s not execute module_id=0x%04x\n",
		       __func__, module_id);
		return R_VSPM_SEQERR;
	}

	if (IS_VSP_CH(module_id)) {
		/* Cancel the VSP process */
		ercd = vspm_ins_vsp_cancel(module_id);
		if (ercd)
			return ercd;
	} else if (IS_FDP_CH(module_id)) {
		/* Cancel the FDP process */
		ercd = vspm_ins_fdp_cancel(module_id);
		if (ercd)
			return ercd;
	} else {
		EPRINT("%s Invalid module_id 0x%04x\n", __func__, module_id);
		return R_VSPM_PARAERR;
	}

	/* clear common information */
	exec_info->p_exec_job_info[module_id] = NULL;
	exec_info->exec_ch_bits &= VSPM_CH_TO_BIT_INVERT(module_id);

	return R_VSPM_OK;
}

