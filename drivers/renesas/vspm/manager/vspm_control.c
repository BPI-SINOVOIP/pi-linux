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

static struct vspm_ctrl_info g_vspm_ctrl_info;

/******************************************************************************
 * Function:		vspm_ins_mask_low_bits
 * Description:	Mask low bits.
 * Returns:		void
 ******************************************************************************/
static void vspm_ins_mask_low_bits(unsigned int *bits, unsigned int num)
{
	unsigned int mask_bits = 0;
	unsigned int i;

	for (i = 0; i < num; i++) {
		mask_bits <<= 1;
		mask_bits |= 0x1;
	}

	*bits &= mask_bits;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_get_ch_lsb
 * Description:	Get channel from LSB.
 * Returns:		channel number/On error is VSPM_CH_MAX
 ******************************************************************************/
static unsigned short vspm_ins_ctrl_get_ch_lsb(unsigned int bits)
{
	unsigned short cnt = 0;

	/* check bits */
	if (!bits)
		return VSPM_CH_MAX;

	/* select low bit */
	bits &= -bits;

	/* count bit offset */
	bits--;
	while (bits) {
		bits &= (bits - 1);
		cnt++;
	}

	return cnt;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_get_ch_msb
 * Description:	Get channel from MSB.
 * Returns:		channel number/On error is VSPM_CH_MAX
 ******************************************************************************/
static unsigned short vspm_ins_ctrl_get_ch_msb(unsigned int bits)
{
	unsigned short cnt = 0;
	unsigned int msb_bits;

	/* check bits */
	if (!bits)
		return VSPM_CH_MAX;

	/* search msb bit */
	do {
		msb_bits = bits;
		bits &= (bits - 1);
	} while (bits);

	/* count bit offset */
	msb_bits--;
	while (msb_bits) {
		msb_bits &= (msb_bits - 1);
		cnt++;
	}

	return cnt;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_initialize
 * Description:	Initialize VSP Manager.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vspm_inc_sort_queue_initialize()
 *	return of vspm_ins_vsp_initialize()
 *	return of vspm_ins_fdp_initialize()
 *	return of vspm_ins_isu_initialize()
 ******************************************************************************/
long vspm_ins_ctrl_initialize(struct vspm_drvdata *pdrv)
{
	struct vspm_usable_res_info *usable = &g_vspm_ctrl_info.usable_info;
	long ercd;
	/* check parameter */
	if (!pdrv)
		return R_VSPM_NG;

	/* clear the VSPM driver control information table */
	memset(&g_vspm_ctrl_info, 0, sizeof(g_vspm_ctrl_info));

	/* initialize the queue information table */
	ercd = vspm_inc_sort_queue_initialize(&g_vspm_ctrl_info.queue_info);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_inc_sort_queue_initialize %ld\n", ercd);
		return ercd;
	}

	/* initialize VSP driver */
	ercd = vspm_ins_vsp_initialize(usable, pdrv);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_ins_vsp_initialize %ld\n", ercd);
		return ercd;
	}

	/* initialize FDP driver */
	ercd = vspm_ins_fdp_initialize(usable, pdrv);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_ins_fdp_initialize %ld\n", ercd);
		/* forced quit */
		(void)vspm_ins_vsp_quit(usable);
		return ercd;
	}

	/* initialize ISU driver */
	ercd = vspm_ins_isu_initialize(usable, pdrv);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_ins_isu_initialize %ld\n", ercd);
		/* forced quit */
		(void)vspm_ins_isu_quit(usable);
		return ercd;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_quit
 * Description:	Finalize VSP Manager.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vspm_ins_vsp_quit()
 *	return of vspm_ins_fdp_quit()
 ******************************************************************************/
long vspm_ins_ctrl_quit(struct vspm_drvdata *pdrv)
{
	struct vspm_usable_res_info *usable = &g_vspm_ctrl_info.usable_info;
	long ercd;

	/* Finalize VSP driver */
	ercd = vspm_ins_vsp_quit(usable);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_ins_vsp_quit %ld\n", ercd);
		return ercd;
	}

	/* Finalize FDP driver */
	ercd = vspm_ins_fdp_quit(usable);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_ins_fdp_quit %ld\n", ercd);
		return ercd;
	}

	/* Finalize ISU driver */
	ercd = vspm_ins_isu_quit(usable);
	if (ercd != R_VSPM_OK) {
		EPRINT("failed to vspm_ins_isu_quit %ld\n", ercd);
		return ercd;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_suspend
 * Description:	Pre-suspend processing.
 *	This function is executing when just before suspend.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_ctrl_suspend(struct vspm_drvdata *pdrv)
{
	/* suspend VSP driver */
	(void)vspm_ins_vsp_suspend();

	/* suspend FDP driver */
	(void)vspm_ins_fdp_suspend();

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_suspend
 * Description:	Post-resume processing.
 *	This function is executing when after resume.
 * Returns:		R_VSPM_OK
 ******************************************************************************/
long vspm_ins_ctrl_resume(struct vspm_drvdata *pdrv)
{
	/* resume VSP driver */
	(void)vspm_ins_vsp_resume();

	/* resume FDP driver */
	(void)vspm_ins_fdp_resume();

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_set_mode
 * Description:	Set operation mode.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vspm_ins_ctrl_mode_param_check()
 ******************************************************************************/
long vspm_ins_ctrl_set_mode(struct vspm_api_param_mode *mode)
{
	struct vspm_usable_res_info *usable = &g_vspm_ctrl_info.usable_info;

	struct vspm_request_res_info *request;
	struct vspm_init_t *param;

	unsigned int use_ch_bits = 0;
	long ercd;

	/* check pointer */
	if (!mode)
		return R_VSPM_NG;

	request = &mode->priv->request_info;
	param = mode->param;

	if (param) {
		/* check parameter */
		ercd = vspm_ins_ctrl_mode_param_check(&use_ch_bits, mode);
		if (ercd)
			return ercd;

		/* set reserved channel */
		if (param->mode == VSPM_MODE_OCCUPY)
			usable->occupy_bits |= use_ch_bits;

		/* set request parameter */
		request->ch_bits = use_ch_bits;
		request->type = param->type;
		request->mode = param->mode;

		/* set process information */
		switch (param->type) {
		case VSPM_TYPE_FDP_AUTO:
			memset(
				&request->fdp_info,
				0,
				sizeof(struct vspm_fdp_proc_info));
			if (param->par.fdp) {
				request->fdp_info.stlmsk_addr[0] =
					param->par.fdp->hard_addr[0];
				request->fdp_info.stlmsk_addr[1] =
					param->par.fdp->hard_addr[1];
			}
			break;
		default:
			break;
		}
	} else {
		/* clear reserved channel */
		if (request->mode == VSPM_MODE_OCCUPY)
			usable->occupy_bits &= ~(request->ch_bits);

		/* clear request parameter */
		request->ch_bits = 0;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_regist_entry
 * Description:	Registory entry parameter.
 * Returns:		R_VSPM_OK/R_VSPM_NG/R_VSPM_QUE_FULL
 ******************************************************************************/
long vspm_ins_ctrl_regist_entry(struct vspm_api_param_entry *entry)
{
	struct vspm_job_info *job_info;

	long ercd;

	/* Register in the job management table */
	job_info = vspm_ins_job_entry(&g_vspm_ctrl_info.job_manager, entry);
	if (!job_info) {
		EPRINT("failed to vspm_ins_job_entry\n");
		return R_VSPM_QUE_FULL;
	}

	/* Add a job information to the queue */
	ercd = vspm_inc_sort_queue_entry(
		&g_vspm_ctrl_info.queue_info, job_info);
	if (ercd != R_VSPM_OK) {
		/* Remove a job */
		vspm_ins_job_remove(job_info);
		EPRINT("failed to vspm_inc_sort_queue_entry %ld\n", ercd);
		return R_VSPM_NG;
	}

	/* Save the job id */
	*entry->p_job_id = vspm_ins_job_get_job_id(job_info);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_exec_entry
 * Description:	Execute entry parameter.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vspm_ins_vsp_execute_low_delay()
 *	return of vspm_ins_fdp_execute_low_delay()
 ******************************************************************************/
long vspm_ins_ctrl_exec_entry(struct vspm_api_param_entry *entry)
{
	struct vspm_request_res_info *request = &entry->priv->request_info;

	unsigned short module_id;
	long ercd;

	/* get channel from LSB */
	module_id = vspm_ins_ctrl_get_ch_lsb(request->ch_bits);
	if (module_id == VSPM_CH_MAX)
		return R_VSPM_NG;

	if (request->type == VSPM_TYPE_VSP_AUTO) {
		/* execute VSP processing */
		ercd = vspm_ins_vsp_execute_low_delay(module_id, entry);
	} else if (request->type == VSPM_TYPE_FDP_AUTO) {
		/* execute FDP processing */
		ercd = vspm_ins_fdp_execute_low_delay(module_id, entry);
	} else if (request->type == VSPM_TYPE_ISU_AUTO) {
		/* execute ISU processing */
		ercd = vspm_ins_isu_execute_low_delay(module_id, entry);
	} else {
		ercd = R_VSPM_NG;
	}

	if (ercd == R_VSPM_OK)
		*entry->p_job_id = 0;	/* not used */

	return ercd;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_on_complete
 * Description:	Complete a job.
 * Returns:		R_VSPM_OK/R_VSPM_PARAERR
 *	return of vspm_ins_exec_complete()
 *	return of vspm_ins_job_execute_complete()
v******************************************************************************/
long vspm_ins_ctrl_on_complete(unsigned short module_id, long result)
{
	struct vspm_job_info *job_info;

	long ercd;

	/* Check the parameter */
	if (!(g_vspm_ctrl_info.usable_info.ch_bits &
			VSPM_CH_TO_BIT(module_id))) {
		EPRINT("%s Invalid module_id\n", __func__);
		return R_VSPM_PARAERR;
	}

	/* Get job information of the job that is running */
	job_info = vspm_ins_exec_get_current_job_info(
		&g_vspm_ctrl_info.exec_info, module_id);
	if (!job_info) {
		ercd = R_VSPM_OK;
		goto dispatch;
	}

	if (vspm_ins_job_get_status(job_info) != VSPM_JOB_STATUS_EXECUTING) {
		ercd = R_VSPM_OK;
		goto dispatch;
	}

	/* Notification that the IP processing is complete */
	ercd = vspm_ins_exec_complete(&g_vspm_ctrl_info.exec_info, module_id);
	if (ercd) {
		EPRINT("failed to vspm_ins_exec_complete %ld\n", ercd);
		goto dispatch;
	}

	/* Inform the completion of the job to the job management */
	ercd = vspm_ins_job_execute_complete(job_info, result, module_id);
	if (ercd) {
		EPRINT("failed to vspm_ins_job_execute_complete %ld\n", ercd);
		goto dispatch;
	}

dispatch:
	/* One job is completed, execute the next job */
	if (vspm_inc_sort_queue_get_count(&g_vspm_ctrl_info.queue_info) > 0)
		vspm_ins_ctrl_dispatch();

	return ercd;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_get_status
 * Description:	Get status of entry jobs.
 * Returns:		VSPM_STATUS_NO_ENTRY/VSPM_STATUS_WAIT/VSPM_STATUS_ACTIVE
 ******************************************************************************/
long vspm_ins_ctrl_get_status(unsigned long job_id)
{
	struct vspm_job_info *job_info;
	unsigned long status;

	long rtncd = VSPM_STATUS_NO_ENTRY;

	/* Search a job information */
	job_info = vspm_ins_job_find_job_info(
		&g_vspm_ctrl_info.job_manager, job_id);
	if (job_info) {
		/* Get a job status */
		status = vspm_ins_job_get_status(job_info);
		if (status == VSPM_JOB_STATUS_ENTRY)
			rtncd = VSPM_STATUS_WAIT;
		else if (status == VSPM_JOB_STATUS_EXECUTING)
			rtncd = VSPM_STATUS_ACTIVE;
	}

	return rtncd;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_queue_cancel
 * Description:	Cancel a job.
 * Returns:		R_VSPM_OK/VSPM_STATUS_NO_ENTRY/VSPM_STATUS_ACTIVE
 ******************************************************************************/
long vspm_ins_ctrl_queue_cancel(unsigned long job_id)
{
	struct vspm_job_info *job_info;
	unsigned long status;

	long rtncd = VSPM_STATUS_NO_ENTRY;

	/* Search a job information */
	job_info = vspm_ins_job_find_job_info(
		&g_vspm_ctrl_info.job_manager, job_id);
	if (job_info) {
		status = vspm_ins_job_get_status(job_info);
		if (status == VSPM_JOB_STATUS_EXECUTING) {
			rtncd = VSPM_STATUS_ACTIVE;
		} else if (status == VSPM_JOB_STATUS_ENTRY) {
			unsigned short index;

			/* Search a job information from queue */
			rtncd = vspm_inc_sort_queue_find_item(
				&g_vspm_ctrl_info.queue_info, job_info, &index);
			if (rtncd == R_VSPM_OK) {
				/* Remove a job information from queue */
				(void)vspm_inc_sort_queue_remove(
					&g_vspm_ctrl_info.queue_info, index);
			}

			/* Cancel the job */
			(void)vspm_ins_job_cancel(job_info);

			rtncd = R_VSPM_OK;
		}
	}

	return rtncd;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_forced_cancel
 * Description:	Forced cancel a job.
 * Returns:		R_VSPM_OK
 *	return of vspm_ins_exec_cancel()
 ******************************************************************************/
long vspm_ins_ctrl_forced_cancel(struct vspm_api_param_forced_cancel *cancel)
{
	struct vspm_job_info *job_info;

	long ercd;
	int i;

	job_info = &g_vspm_ctrl_info.job_manager.job_info[0];
	for (i = 0; i < VSPM_MAX_ELEMENTS; i++) {
		if (job_info->entry.priv == cancel->priv &&
		    job_info->status != VSPM_JOB_STATUS_EMPTY) {
			if (job_info->status == VSPM_JOB_STATUS_ENTRY) {
				unsigned short index;

				/* Search a job information from queue */
				ercd = vspm_inc_sort_queue_find_item(
					&g_vspm_ctrl_info.queue_info,
					job_info,
					&index);
				if (ercd == R_VSPM_OK) {
					/* Remove a job info from queue */
					(void)vspm_inc_sort_queue_remove(
						&g_vspm_ctrl_info.queue_info,
						index);
				}

				/* Cancel the job */
				(void)vspm_ins_job_cancel(job_info);
			} else if (job_info->status ==
					VSPM_JOB_STATUS_EXECUTING) {
				/* Cancel of executing IP */
				ercd = vspm_ins_exec_cancel(
					&g_vspm_ctrl_info.exec_info,
					job_info->ch_num);
				if (ercd) {
					EPRINT(
						"failed to vspm_ins_exec_cancel %ld\n",
						ercd);
					return ercd;
				}

				/* Calcel the executing job */
				(void)vspm_ins_job_execute_complete(
					job_info,
					R_VSPM_CANCEL,
					job_info->ch_num);
			}
		}

		job_info++;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_cancel_entry
 * Description:	Cancel entry parameter.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 *	return of vspm_ins_vsp_cancel()
 *	return of vspm_ins_fdp_cancel()
 ******************************************************************************/
long vspm_ins_ctrl_cancel_entry(struct vspm_privdata *priv)
{
	struct vspm_request_res_info *request = &priv->request_info;
	unsigned short module_id;
	long ercd;

	/* get channel from LSB */
	module_id = vspm_ins_ctrl_get_ch_lsb(request->ch_bits);
	if (module_id == VSPM_CH_MAX)
		return R_VSPM_NG;

	if (request->type == VSPM_TYPE_VSP_AUTO) {
		/* cancel VSP processing */
		ercd = vspm_ins_vsp_cancel(module_id);
	} else if (request->type == VSPM_TYPE_FDP_AUTO) {
		/* cancel FDP processing */
		ercd = vspm_ins_fdp_cancel(module_id);
	} else if (request->type == VSPM_TYPE_ISU_AUTO) {
		/* cancel ISU processing */
		ercd = vspm_ins_isu_cancel(module_id);
	} else {
		ercd = R_VSPM_NG;
	}

	return ercd;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_mode_param_check
 * Description:	Check operaton mode.
 * Returns:		R_VSPM_OK/R_VSPM_PARAERR/R_VSPM_ALREADY_USED
 ******************************************************************************/
long vspm_ins_ctrl_mode_param_check(
	unsigned int *use_bits, struct vspm_api_param_mode *mode)
{
	struct vspm_usable_res_info *usable = &g_vspm_ctrl_info.usable_info;
	struct vspm_init_t *param = mode->param;

	unsigned int bits = param->use_ch;

	/* check type */
	if (param->type == VSPM_TYPE_VSP_AUTO) {
		/* bit mask */
		vspm_ins_mask_low_bits(&bits, VSPM_VSP_CH_NUM);
		/* shift channel bits */
		bits <<= VSPM_VSP_CH_OFFSET;
	} else if (param->type == VSPM_TYPE_FDP_AUTO) {
		/* bit mask */
		vspm_ins_mask_low_bits(&bits, VSPM_FDP_CH_NUM);
		/* shift channel bits */
		bits <<= VSPM_FDP_CH_OFFSET;
	} else if (param->type == VSPM_TYPE_ISU_AUTO) {
		vspm_ins_mask_low_bits(&bits, VSPM_ISU_CH_NUM);
		/* shift channel bits */
		bits <<= VSPM_ISU_CH_OFFSET;
	} else {
		EPRINT("%s: Invalid type!! type=%d\n",
		       __func__, param->type);
		return R_VSPM_PARAERR;
	}

	/* check usable channel */
	bits &= usable->ch_bits;
	if (!bits) {
		EPRINT("%s: Invalid channel!! use_ch=0x%08x\n",
		       __func__, param->use_ch);
		return R_VSPM_PARAERR;
	}

	/* check mode */
	if (param->mode == VSPM_MODE_MUTUAL) {
		/* no processing */
	} else if (param->mode == VSPM_MODE_OCCUPY) {
		/* check occupy channel */
		bits &= ~(usable->occupy_bits);
		if (!bits) {
			EPRINT("%s: Already used!! use_ch=0x%08x\n",
			       __func__, param->use_ch);
			return R_VSPM_ALREADY_USED;
		}

		/* select low bit */
		bits &= -bits;
	} else {
		EPRINT("%s: Invalid mode!! mode=%d\n", __func__, param->mode);
		return R_VSPM_PARAERR;
	}

	/* set using channel bits */
	*use_bits = bits;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_entry_param_check
 * Description:	Check entry parameter.
 * Returns:		R_VSPM_OK/R_VSPM_PARAERR
 *	return of vspm_ins_ctrl_assign_channel()
 ******************************************************************************/
long vspm_ins_ctrl_entry_param_check(struct vspm_api_param_entry *entry)
{
	struct vspm_job_t *ip_par;
	long ercd;

	/* check job ID pointer */
	if (!entry->p_job_id) {
		EPRINT("%s p_job_id is NULL\n", __func__);
		return R_VSPM_PARAERR;
	}

	/* check job priority */
	if (entry->job_priority < VSPM_PRI_MIN ||
	    entry->job_priority > VSPM_PRI_MAX) {
		EPRINT("%s Invalid job_priority %d\n",
		       __func__, entry->job_priority);
		return R_VSPM_PARAERR;
	}

	/* check callback pointer */
	if (!entry->pfn_complete_cb) {
		EPRINT("%s pfn_complete_cb is NULL\n", __func__);
		return R_VSPM_PARAERR;
	}

	/* check IP information pointer */
	if (!entry->p_ip_par) {
		EPRINT("%s p_ip_par is NULL\n", __func__);
		return R_VSPM_PARAERR;
	}
	ip_par = entry->p_ip_par;

	switch (ip_par->type) {
	case VSPM_TYPE_VSP_AUTO:
		if (!ip_par->par.vsp) {
			EPRINT("%s par.vsp is NULL\n", __func__);
			return R_VSPM_PARAERR;
		}
		break;
	case VSPM_TYPE_FDP_AUTO:
		if (!ip_par->par.fdp) {
			EPRINT("%s par.fdp is NULL\n", __func__);
			return R_VSPM_PARAERR;
		}
		break;
	case VSPM_TYPE_ISU_AUTO:
		if (!ip_par->par.isu) {
			EPRINT("%s par.isu is NULL\n", __func__);
			return R_VSPM_PARAERR;
		}
		break;
	default:
		EPRINT("%s Illegal type 0x%04x\n",
		       __func__, ip_par->type);
		return R_VSPM_PARAERR;
	}

	/* pre assign channel */
	ercd = vspm_ins_ctrl_assign_channel(
		ip_par,
		&entry->priv->request_info,
		&g_vspm_ctrl_info.usable_info,
		NULL);
	if (ercd) {
		EPRINT("%s can't assign IP!! 0x%08x\n",
		       __func__, entry->priv->request_info.ch_bits);
		return ercd;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_dispatch
 * Description:	Execute the scheduling and processing.
 * Returns:		void
 ******************************************************************************/
void vspm_ins_ctrl_dispatch(void)
{
	struct vspm_usable_res_info usable;

	unsigned short queue_count;
	unsigned short refer_index = 0;

	long ercd;

	/* Get the number of entry */
	queue_count = vspm_inc_sort_queue_get_count(
		&g_vspm_ctrl_info.queue_info);

	while (queue_count > refer_index) {
		struct vspm_job_info *job_info;
		struct vspm_job_t *p_ip_par;
		struct vspm_request_res_info *request;
		unsigned short module_id;

		/* Set the channel available */
		usable = g_vspm_ctrl_info.usable_info;

		/* Update the execution status of the current module */
		vspm_ins_exec_update_current_status(
			&g_vspm_ctrl_info.exec_info, &usable);

		/* Get a 1st job information from queue */
		ercd = vspm_inc_sort_queue_refer(
			&g_vspm_ctrl_info.queue_info, refer_index, &job_info);
		if (ercd) {
			/* not found Job */
			return;
		}

		/* Get IP parameter */
		p_ip_par = vspm_ins_job_get_ip_param(job_info);

		/* Get request parameter */
		request = vspm_ins_job_get_request_param(job_info);

		/* assign channel */
		ercd = vspm_ins_ctrl_assign_channel(
			p_ip_par, request, &usable, &module_id);
		if (ercd) {
			/* not assigned */
			refer_index++;
			continue;
		}

		/* Remove a job information from queue */
		(void)vspm_inc_sort_queue_remove(
			&g_vspm_ctrl_info.queue_info, refer_index);

		/* Get the number of entry */
		queue_count = vspm_inc_sort_queue_get_count(
			&g_vspm_ctrl_info.queue_info);

		/* Inform the start of the job to the job management */
		(void)vspm_ins_job_execute_start(job_info, module_id);

		/* Start the process */
		ercd = vspm_ins_exec_start(
			&g_vspm_ctrl_info.exec_info,
			module_id,
			p_ip_par,
			job_info);
		if (ercd) {
			EPRINT("failed to vspm_ins_exec_start");
			EPRINT("ercd=%ld, module_id=%d\n", ercd, module_id);

			/* Info the comp of the job to the job management */
			(void)vspm_ins_job_execute_complete(
				job_info, ercd, module_id);
		}
	}
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_assign_channel
 * Description:	Assignment channel.
 * Returns:		R_VSPM_OK/R_VSPM_PARAERR/R_VSPM_OCCUPY_CH
 ******************************************************************************/
long vspm_ins_ctrl_assign_channel(
	struct vspm_job_t *ip_par,
	struct vspm_request_res_info *request,
	struct vspm_usable_res_info *usable,
	unsigned short *ch)
{
	unsigned int assign_bits = request->ch_bits;
	unsigned short ch_num = VSPM_CH_MAX;

	/* check occupy for mutual mode */
	if (request->mode == VSPM_MODE_MUTUAL) {
		/* except occupy channel */
		assign_bits &= ~(usable->occupy_bits);
		if (assign_bits == 0)
			return R_VSPM_OCCUPY_CH;

		/* except using channel */
		assign_bits &= usable->ch_bits;
		if (assign_bits == 0)
			return R_VSPM_PARAERR;
	}

	if (ip_par->type == VSPM_TYPE_VSP_AUTO) {
		/* shift channel bits */
		assign_bits >>= VSPM_VSP_CH_OFFSET;
		/* bit mask */
		vspm_ins_mask_low_bits(&assign_bits, VSPM_VSP_CH_NUM);
		/* update assigneble vsp channel bits */
		assign_bits &= vspm_ins_ctrl_get_usable_vsp_ch_bits(
			ip_par->par.vsp, usable);
		/* get channel from MSB */
		ch_num = vspm_ins_ctrl_get_ch_msb(assign_bits);
		ch_num += VSPM_VSP_CH_OFFSET;
	} else if (ip_par->type == VSPM_TYPE_FDP_AUTO) {
		/* shift channel bits */
		assign_bits >>= VSPM_FDP_CH_OFFSET;
		/* bit mask */
		vspm_ins_mask_low_bits(&assign_bits, VSPM_FDP_CH_NUM);
		/* get channel from LSB */
		ch_num = vspm_ins_ctrl_get_ch_lsb(assign_bits);
		ch_num += VSPM_FDP_CH_OFFSET;
	} else if (ip_par->type == VSPM_TYPE_ISU_AUTO) {
		/* shift channel bits */
		assign_bits >>= VSPM_ISU_CH_OFFSET;
		/* bit mask */
                vspm_ins_mask_low_bits(&assign_bits, VSPM_ISU_CH_NUM);
		ch_num = vspm_ins_ctrl_get_ch_lsb(assign_bits);
                ch_num += VSPM_ISU_CH_OFFSET;
	}

	/* check assignment channel number */
	if (ch_num >= VSPM_CH_MAX)
		return R_VSPM_PARAERR;

	/* set channel number */
	if (ch)
		*ch = ch_num;

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_count_bits
 * Description:	Get bit count of 1.
 * Returns:		bit count
 ******************************************************************************/
static unsigned char vspm_ins_ctrl_count_bits(unsigned int bits)
{
	unsigned char cnt = 0;

	bits &= 0xffff;
	while (bits) {
		bits &= (bits - 1);
		cnt++;
	}

	return cnt;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_get_req_clut_count
 * Description:	Get request clut count of RPF.
 * Returns:		clut count
 ******************************************************************************/
static unsigned char vspm_ins_ctrl_get_req_clut_count(
	struct vsp_start_t *vsp_par)
{
	struct vsp_src_t **src_par;
	unsigned char cnt = 0;

	int i;

	src_par = &vsp_par->src_par[0];
	for (i = 0; i < 5; i++) {
		if (*src_par) {
			if (((*src_par)->format == VSP_IN_RGB_CLUT_DATA) ||
			    ((*src_par)->format == VSP_IN_YUV_CLUT_DATA)) {
				cnt++;
			}
		}
		src_par++;
	}

	return cnt;
}

/******************************************************************************
 * Function:		vspm_ins_ctrl_get_usable_vsp_ch_bits
 * Description:	Get usable VSP channel bits.
 * Returns:		usable RPF channel count.
 ******************************************************************************/
unsigned int vspm_ins_ctrl_get_usable_vsp_ch_bits(
	struct vsp_start_t *vsp_par, struct vspm_usable_res_info *usable)
{
	struct vspm_usable_vsp_res_info *vsp_res;

	unsigned char usable_cnt;
	unsigned char use_cnt;
	unsigned char assign_flag;

	unsigned int ch_bits = 0;
	int i;

	vsp_res = &usable->vsp_res[VSPM_VSP_IP_MAX - 1];
	for (i = 0; i < VSPM_VSP_IP_MAX; i++) {
		assign_flag = 1;

		/* check RPF module */
		usable_cnt =
			vspm_ins_ctrl_count_bits(vsp_res->rpf_bits);
		if (usable_cnt < vsp_par->rpf_num)
			assign_flag = 0;

		/* check RPF(CLUT) module */
		usable_cnt = vspm_ins_ctrl_count_bits(
			vsp_res->rpf_bits & vsp_res->rpf_clut_bits);
		use_cnt = vspm_ins_ctrl_get_req_clut_count(vsp_par);
		if (usable_cnt < use_cnt)
			assign_flag = 0;

		/* check WPF(rotation) module */
		if (vsp_par->dst_par) {
			if (vsp_par->dst_par->rotation >= VSP_ROT_H_FLIP &&
			    vsp_res->wpf_rot_bits == 0)
				assign_flag = 0;
		}

		/* check other module */
		if (((unsigned long)vsp_res->module_bits &
				vsp_par->use_module) != vsp_par->use_module)
			assign_flag = 0;

		ch_bits <<= 1;
		if (assign_flag) {
			/* set assignable bits */
			ch_bits |= 0x1;
		}

		vsp_res--;
	}

	return ch_bits;
}

/******************************************************************************
 * Function:		vspm_inc_ctrl_on_driver_complete
 * Description:	Complete processing.
 * Returns:		void
 ******************************************************************************/
void vspm_inc_ctrl_on_driver_complete(unsigned short module_id, long result)
{
	struct vspm_api_param_on_complete api_param;

	api_param.module_id = module_id;
	api_param.result = result;

	(void)fw_send_event(
		TASK_VSPM,
		FUNCTIONID_VSPM_BASE + EVENT_VSPM_DRIVER_ON_COMPLETE,
		sizeof(api_param),
		&api_param);
}

