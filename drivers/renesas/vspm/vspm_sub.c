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

#include <linux/kthread.h>

#include "frame.h"

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "vspm_lib_public.h"

/******************************************************************************
 * Function:		vspm_thread
 * Description:	VSPM thread main routine.
 * Returns:		0
 ******************************************************************************/
static int vspm_thread(void *name)
{
	vspm_task();
	return 0;
}

/******************************************************************************
 * Function:		vspm_init
 * Description:	Create the VSPM thread.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_init(struct vspm_drvdata *pdrv)
{
	long drv_ercd;
	int ercd;

	/* Initialize the framework */
	fw_initialize();

	/* Register VSPM task to framework */
	ercd = fw_task_register(TASK_VSPM);
	if (ercd) {
		APRINT("failed to fw_task_register\n");
		return R_VSPM_NG;
	}

	/* create vspm thread */
	pdrv->task = kthread_run(vspm_thread, pdrv, THREADNAME);
	if (IS_ERR(pdrv->task)) {
		APRINT("failed to kthread_run\n");
		/* forced unregister VSPM task */
		(void)fw_task_unregister(TASK_VSPM);
		return R_VSPM_NG;
	}

	/* Send FUNC_TASK_INIT message */
	drv_ercd = fw_send_function(
		TASK_VSPM,
		FUNC_TASK_INIT,
		sizeof(struct vspm_drvdata),
		pdrv);
	if (drv_ercd != FW_OK) {
		APRINT("failed to fw_send_function(FUNC_TASK_INIT)\n");
		/* forced unregister VSPM task */
		(void)fw_task_unregister(TASK_VSPM);
		return R_VSPM_NG;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_quit
 * Description:	Delete the VSPM thread.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_quit(struct vspm_drvdata *pdrv)
{
	long drv_ercd;

	/* Send FUNC_TASK_QUIT message */
	drv_ercd = fw_send_function(
		TASK_VSPM,
		FUNC_TASK_QUIT,
		sizeof(struct vspm_drvdata),
		pdrv);
	if (drv_ercd != FW_OK) {
		APRINT("failed to fw_send_function(FUNC_TASK_QUIT)\n");
		return R_VSPM_NG;
	}

	/* Unregister VSPM task */
	(void)fw_task_unregister(TASK_VSPM);

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_suspend
 * Description:	Suspend the VSPM thread.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_suspend(struct vspm_drvdata *pdrv)
{
	long ercd;

	/* Send FUNC_TASK_SUSPEND message */
	ercd = fw_send_function(
		TASK_VSPM,
		FUNC_TASK_SUSPEND,
		sizeof(struct vspm_drvdata),
		pdrv);
	if (ercd != FW_OK) {
		APRINT("failed to fw_send_function(FUNC_TASK_SUSPEND)\n");
		return R_VSPM_NG;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_resume
 * Description:	Resume the VSPM thread.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_resume(struct vspm_drvdata *pdrv)
{
	long ercd;

	/* Send FUNC_TASK_RESUME message */
	ercd = fw_send_function(
		TASK_VSPM,
		FUNC_TASK_RESUME,
		sizeof(struct vspm_drvdata),
		pdrv);
	if (ercd != FW_OK) {
		APRINT("failed to fw_send_function(FUNC_TASK_RESUME)\n");
		return R_VSPM_NG;
	}

	return R_VSPM_OK;
}

/******************************************************************************
 * Function:		vspm_cancel
 * Description:	Cancel the VSPM thread.
 * Returns:		R_VSPM_OK/R_VSPM_NG
 ******************************************************************************/
long vspm_cancel(struct vspm_privdata *priv)
{
	long ercd;

	ercd = vspm_lib_forced_cancel(priv);
	if (ercd != R_VSPM_OK) {
		APRINT("failed to vspm_lib_forced_cancel %d\n", (int)ercd);
		return R_VSPM_NG;
	}

	return R_VSPM_OK;
}
