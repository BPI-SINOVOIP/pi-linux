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

/******************************************************************************
 * Function:		vspm_lib_entry
 * Description:	Entry of various IP operations.
 * Returns:		R_VSPM_OK
 *	return of vspm_ins_ctrl_entry_param_check()
 *	return of fw_send_function()
 *	return of vspm_ins_ctrl_exec_entry()
 ******************************************************************************/
long vspm_lib_entry(struct vspm_api_param_entry *entry)
{
	struct vspm_request_res_info *request;
	long ercd;

	/* check entry parameter */
	ercd = vspm_ins_ctrl_entry_param_check(entry);
	if (ercd)
		return ercd;

	request = &entry->priv->request_info;
	if (request->mode == VSPM_MODE_MUTUAL) {
		/* mutual mode */
		/* entry */
		ercd = fw_send_function(
			TASK_VSPM,
			FUNCTIONID_VSPM_BASE + FUNC_VSPM_ENTRY,
			sizeof(struct vspm_api_param_entry),
			entry);
		if (ercd)
			return ercd;

		/* dispatch */
		(void)fw_send_event(
			TASK_VSPM,
			FUNCTIONID_VSPM_BASE + EVENT_VSPM_DISPATCH,
			0,
			NULL);
	} else {
		/* occupy mode */
		ercd = vspm_ins_ctrl_exec_entry(entry);
		if (ercd)
			return ercd;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_lib_queue_cancel
 * Description:	Cancel the job.
 * Returns:		return of fw_send_function()
 *	return of vspm_ins_ctrl_cancel_entry()
 ******************************************************************************/
long vspm_lib_queue_cancel(
	struct vspm_privdata *priv, unsigned long job_id)
{
	struct vspm_request_res_info *request = &priv->request_info;
	long ercd;

	if (request->mode == VSPM_MODE_MUTUAL) {
		/* mutual mode */
		ercd = fw_send_function(
			TASK_VSPM,
			FUNCTIONID_VSPM_BASE + FUNC_VSPM_CANCEL,
			sizeof(job_id),
			&job_id);
	} else {
		/* occupy mode */
		ercd = vspm_ins_ctrl_cancel_entry(priv);
	}

	return ercd;
}

/******************************************************************************
 * Function:		vspm_lib_forced_cancel
 * Description:	Forced cancel the job.
 * Returns:		return of fw_send_function()
 ******************************************************************************/
long vspm_lib_forced_cancel(struct vspm_privdata *priv)
{
	struct vspm_api_param_forced_cancel cancel;

	cancel.priv = priv;

	return fw_send_function(
		TASK_VSPM,
		FUNCTIONID_VSPM_BASE + FUNC_VSPM_FORCED_CANCEL,
		sizeof(cancel),
		&cancel);
}

/******************************************************************************
 * Function:		vspm_lib_set_mode
 * Description:	Set VSP manager operation mode.
 * Returns:		return of fw_send_function()
 ******************************************************************************/
long vspm_lib_set_mode(struct vspm_privdata *priv, struct vspm_init_t *param)
{
	struct vspm_api_param_mode mode;

	mode.priv = priv;
	mode.param = param;

	return fw_send_function(
		TASK_VSPM,
		FUNCTIONID_VSPM_BASE + FUNC_VSPM_SET_MODE,
		sizeof(mode),
		&mode);
}

/******************************************************************************
 * Function:		vspm_lib_get_status
 * Description:	Get status of processing information.
 * Returns:		R_VSPM_OK/R_VSPM_PARAERR
 ******************************************************************************/
long vspm_lib_get_status(
	struct vspm_privdata *priv, struct vspm_status_t *param)
{
	struct vspm_request_res_info *request = &priv->request_info;

	/* check parameter */
	if (!param->fdp)
		return R_VSPM_PARAERR;

	if (request->type != VSPM_TYPE_FDP_AUTO)
		return R_VSPM_PARAERR;

	/* copy parameter */
	memcpy(
		param->fdp,
		&request->fdp_info.status,
		sizeof(struct fdp_status_t));

	return R_VSPM_OK;
}
