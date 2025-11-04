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
#include "vspm_task_private.h"

/* function table */
static struct fw_func_tbl g_vspm_func_tbl[] = {
	[FUNC_TASK_INIT - 1] = {
		.func_id = FUNC_TASK_INIT,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_init
	},
	[FUNC_TASK_QUIT - 1] = {
		.func_id = FUNC_TASK_QUIT,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_quit
	},
	[FUNC_TASK_SUSPEND - 1] = {
		.func_id = FUNC_TASK_SUSPEND,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_suspend
	},
	[FUNC_TASK_RESUME - 1] = {
		.func_id = FUNC_TASK_RESUME,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_resume
	},
	[FUNC_VSPM_ENTRY - 1] = {
		.func_id = FUNCTIONID_VSPM_BASE + FUNC_VSPM_ENTRY,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_entry
	},
	[EVENT_VSPM_DRIVER_ON_COMPLETE - 1] = {
		.func_id = FUNCTIONID_VSPM_BASE + EVENT_VSPM_DRIVER_ON_COMPLETE,
		.msg_id	 = MSG_EVENT,
		.func	 = vspm_inm_driver_on_complete
	},
	[FUNC_VSPM_CANCEL - 1] = {
		.func_id = FUNCTIONID_VSPM_BASE + FUNC_VSPM_CANCEL,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_cancel
	},
	[FUNC_VSPM_FORCED_CANCEL - 1] = {
		.func_id = FUNCTIONID_VSPM_BASE + FUNC_VSPM_FORCED_CANCEL,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_forced_cancel
	},
	[FUNC_VSPM_SET_MODE - 1] = {
		.func_id = FUNCTIONID_VSPM_BASE + FUNC_VSPM_SET_MODE,
		.msg_id	 = MSG_FUNCTION,
		.func	 = vspm_inm_set_mode
	},
	[EVENT_VSPM_DISPATCH - 1] = {
		.func_id = FUNCTIONID_VSPM_BASE + EVENT_VSPM_DISPATCH,
		.msg_id	 = MSG_EVENT,
		.func	 = vspm_inm_dispatch
	},
	[EVENT_VSPM_MAX - 1] = {
		.func_id = 0,
		.msg_id	 = 0,
		.func	 = NULL
	}
};

/******************************************************************************
 * Function:		vspm_task
 * Description:	VSPM task entry.
 * Returns:		void
 ******************************************************************************/
void vspm_task(void)
{
	/* start the processing framework */
	fw_execute(TASK_VSPM, g_vspm_func_tbl);
}

/******************************************************************************
 * Function:		vspm_inm_init
 * Description:	Initialize the VSPM task.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
long vspm_inm_init(void *mesp, void *para)
{
	long ercd;

	RESERVED(mesp);

	/* Initialize the VSPM driver control information */
	ercd = vspm_ins_ctrl_initialize((struct vspm_drvdata *)para);
	if (ercd) {
		EPRINT("%s failed to vspm_ins_ctrl_initialize %ld\n",
		       __func__, ercd);
		return FW_NG;
	}

	return FW_OK;
}

/******************************************************************************
 * Function:		vspm_inm_quit
 * Description:	Exit the VSPM task.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
long vspm_inm_quit(void *mesp, void *para)
{
	long ercd;

	RESERVED(mesp);

	/* Exit the VSPM driver */
	ercd = vspm_ins_ctrl_quit((struct vspm_drvdata *)para);
	if (ercd) {
		EPRINT("%s failed to vspm_ins_ctrl_quit %ld\n", __func__, ercd);
		return FW_NG;
	}

	return FW_OK;
}

/******************************************************************************
 * Function:		vspm_inm_suspend
 * Description:	Suspend the VSPM task.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
long vspm_inm_suspend(void *mesp, void *para)
{
	long ercd;

	RESERVED(mesp);

	/* Suspend the VSPM driver control information */
	ercd = vspm_ins_ctrl_suspend((struct vspm_drvdata *)para);
	if (ercd) {
		EPRINT("%s failed to vspm_ins_ctrl_suspend %ld\n",
		       __func__, ercd);
		return FW_NG;
	}

	return FW_OK;
}

/******************************************************************************
 * Function:		vspm_inm_resume
 * Description:	Resume the VSPM task.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
long vspm_inm_resume(void *mesp, void *para)
{
	long ercd;

	RESERVED(mesp);

	/* Resume the VSPM driver */
	ercd = vspm_ins_ctrl_resume((struct vspm_drvdata *)para);
	if (ercd) {
		EPRINT("%s failed to vspm_ins_ctrl_resume %ld\n",
		       __func__, ercd);
		return FW_NG;
	}

	return FW_OK;
}

/******************************************************************************
 * Function:		vspm_inm_entry
 * Description:	Entry of various IP operations.
 * Returns:		return of vspm_ins_ctrl_regist_entry()
 ******************************************************************************/
long vspm_inm_entry(void *mesp, void *para)
{
	long ercd;

	RESERVED(mesp);

	ercd = vspm_ins_ctrl_regist_entry((struct vspm_api_param_entry *)para);
	if (ercd) {
		EPRINT("%s failed to vspm_ins_ctrl_regist_entry %ld\n",
		       __func__, ercd);
	}

	return ercd;
}

/******************************************************************************
 * Function:		vspm_inm_driver_on_complete
 * Description:	IP operations completion.
 * Returns:		FW_OK
 ******************************************************************************/
long vspm_inm_driver_on_complete(void *mesp, void *para)
{
	long ercd;
	struct vspm_api_param_on_complete *api_param =
		(struct vspm_api_param_on_complete *)para;

	RESERVED(mesp);

	ercd = vspm_ins_ctrl_on_complete(
		api_param->module_id, api_param->result);
	if (ercd) {
		EPRINT("%s failed to vspm_ins_ctrl_on_complete %ld\n",
		       __func__, ercd);
	}

	return FW_OK;
}

/******************************************************************************
 * Function:		vspm_inm_cancel
 * Description:	Cancel the job.
 * Returns:		return of vspm_ins_ctrl_queue_cancel()
 ******************************************************************************/
long vspm_inm_cancel(void *mesp, void *para)
{
	RESERVED(mesp);

	return vspm_ins_ctrl_queue_cancel(*(unsigned long *)para);
}

/******************************************************************************
 * Function:		vspm_inm_forced_cancel
 * Description:	Forced cancel the job.
 * Returns:		return of vspm_ins_ctrl_forced_cancel()
 ******************************************************************************/
long vspm_inm_forced_cancel(void *mesp, void *para)
{
	RESERVED(mesp);

	return vspm_ins_ctrl_forced_cancel(
		(struct vspm_api_param_forced_cancel *)para);
}

/******************************************************************************
 * Function:		vspm_inm_set_mode
 * Description:	Set VSP manager operation mode.
 * Returns:		return of vspm_ins_ctrl_set_mode()
 ******************************************************************************/
long vspm_inm_set_mode(void *mesp, void *para)
{
	RESERVED(mesp);

	return vspm_ins_ctrl_set_mode((struct vspm_api_param_mode *)para);
}

/******************************************************************************
 * Function:		vspm_inm_dispatch
 * Description:	Execute the scheduling and processing.
 * Returns:		FW_OK
 ******************************************************************************/
long vspm_inm_dispatch(void *mesp, void *para)
{
	RESERVED(mesp);
	RESERVED(para);

	vspm_ins_ctrl_dispatch();
	return FW_OK;
}

