/*************************************************************************/ /*
 * ISUM
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

#include <linux/slab.h>

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"

#include "isu_drv_public.h"
#include "isu_drv_local.h"

static struct isu_prv_data *g_isu_obj[ISU_IP_MAX] = {NULL};

/******************************************************************************
 * Function:		isu_lib_init
 * Description:	Initialize ISU driver
 * Returns:		0/E_ISU_NO_MEM
 *	return of isu_ins_check_init_parameter()
 ******************************************************************************/
long isu_lib_init(struct isu_init_t *param)
{
	struct isu_prv_data *prv;

	unsigned int i;

	long ercd;

	/* check initialize parameter */
	ercd = isu_ins_check_init_parameter(param);
	if (ercd)
		return ercd;

	for (i = 0; i < param->ip_num; i++) {
		/* allocate memory */
		prv = kzalloc(sizeof(struct isu_prv_data), GFP_KERNEL);
		if (!prv)
			goto err_exit;

		/* update status */
		prv->ch_info.status = ISU_STAT_INIT;

		g_isu_obj[i] = prv;
	}

	return 0;

err_exit:
	for (i = 0; i < param->ip_num; i++) {
		kfree(g_isu_obj[i]);
		g_isu_obj[i] = NULL;
	}

	return E_ISU_NO_MEM;
}

/******************************************************************************
 * Function:		isu_lib_quit
 * Description:	Finalize ISU driver
 * Returns:		0
 *	return of isu_lib_abort()
 *	return of isu_lib_close()
 ******************************************************************************/
long isu_lib_quit(void)
{
	struct isu_prv_data **prv = &g_isu_obj[0];

	long ercd;

	unsigned int i;

	for (i = 0; i < ISU_IP_MAX; i++) {
		if (*prv) {
			/* check condition */
			if ((*prv)->ch_info.status == ISU_STAT_RUN) {
				/* stop ISU processing */
				ercd = isu_lib_abort((unsigned char)i);
				if (ercd)
					return ercd;
			}

			if ((*prv)->ch_info.status == ISU_STAT_READY) {
				ercd = isu_lib_close((unsigned char)i);
				if (ercd)
					return ercd;
			}

			/* release memory */
			kfree(*prv);
			*prv = NULL;
		}
		prv++;
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_lib_open
 * Description:	Initialize FDP channel.
 * Returns:		0/E_ISU_PARA_CH/E_ISU_NO_INIT/E_ISU_INVALID_STATE
 *	return of isu_ins_get_pdata()
 *	return of isu_ins_enable_clock()
 *	return of isu_ins_init_reg()
 *	return of isu_ins_reg_ih()
 ******************************************************************************/
long isu_lib_open(unsigned char ch, struct isu_open_t *param)
{
	struct isu_prv_data *prv;

	long ercd;

	/* check open parameter */
	ercd = isu_ins_check_open_parameter(param);
	if (ercd)
		goto err_exit1;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX) {
		ercd = E_ISU_PARA_CH;
		goto err_exit1;
	}

	if (!g_isu_obj[ch]) {
		ercd = E_ISU_NO_INIT;
		goto err_exit1;
	}
	prv = g_isu_obj[ch];

	/* check status */
	if (prv->ch_info.status != ISU_STAT_INIT){
		ercd = E_ISU_INVALID_STATE;
		goto err_exit1;
	}

	prv->pdev = param->pdev;

	/* set open parameter */
	ercd = isu_ins_get_isu_resource(prv);
	if (ercd)
		goto err_exit1;

	/*dh enable clock */
	ercd = isu_ins_enable_clock(prv);
	if (ercd)
		goto err_exit1;

	/* initialize register */
	ercd = isu_ins_init_reg(prv);
	if (ercd)
		goto err_exit2;

	/* registory interrupt handler */
	ercd = isu_ins_reg_ih(prv);
	if (ercd)
		goto err_exit3;

	/* update status */
	prv->ch_info.status = ISU_STAT_READY;

	return 0;

err_exit3:
	(void)isu_ins_quit_reg(prv);

err_exit2:
	(void)isu_ins_disable_clock(prv);

err_exit1:
	return ercd;
}

/******************************************************************************
 * Function:		isu_lib_close
 * Description:	Finalize FDP channel.
 * Returns:		0/E_ISU_PARA_CH/E_ISU_NO_INIT/E_ISU_INVALID_STATE
 *	return of isu_ins_unreg_ih()
 *	return of isu_ins_quit_reg()
 *	return of isu_ins_disable_clock()
 ******************************************************************************/
long isu_lib_close(unsigned char ch)
{
	struct isu_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX)
		return E_ISU_PARA_CH;

	/* check condition */
	if (!g_isu_obj[ch])
		return E_ISU_NO_INIT;
	prv = g_isu_obj[ch];

	/* check status */
	if (prv->ch_info.status != ISU_STAT_READY)
		return E_ISU_INVALID_STATE;

	/* unregistory interrupt handler */
	ercd = isu_ins_unreg_ih(prv);
	if (ercd)
		return ercd;

	/* Finalize register */
	ercd = isu_ins_quit_reg(prv);
	if (ercd)
		return ercd;

	/* disable clock */
	ercd = isu_ins_disable_clock(prv);
	if (ercd)
		return ercd;

	/* update status */
	prv->ch_info.status = ISU_STAT_INIT;

	return 0;
}

/******************************************************************************
 * Function:		isu_lib_start
 * Description:	Start ISU processing
 * Returns: 0/E_ISU_PARA_CB/E_ISU_PARA_INPAR/E_ISU_PARA_CH/E_ISU_NO_INIT/E_ISU_INVALID_STATE
	returns isu_ins_check_start_parameter
	returns isu_ins_set_start_parameter
 ******************************************************************************/
long isu_lib_start(unsigned char ch,
			void *callback,
			struct isu_start_t *param,
			void *userdata)
{
	struct isu_prv_data *prv;
	struct isu_ch_info *ch_info;

	long ercd;
	/* check start parameter */
	if (!callback)
		return E_ISU_PARA_CB;

	if (!param)
		return E_ISU_PARA_INPAR;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX)
		return E_ISU_PARA_CH;

	if (!g_isu_obj[ch])
		return E_ISU_NO_INIT;

	prv = g_isu_obj[ch];

	/* check write index */
	ch_info = &prv->ch_info;

	/* check status */
	if (ch_info->status != ISU_STAT_READY)
		return E_ISU_INVALID_STATE;

	if (!prv->isu_reg)
		return E_ISU_INVALID_STATE;

	/* update status */
	ch_info->status = ISU_STAT_RUN;

	/* check start parameter */
	ercd = isu_ins_check_start_parameter(prv, param);
	if (ercd) {
		/* update status */
		ch_info->status = ISU_STAT_READY;

		return ercd;
	}

	/* set start parameter */
	ercd = isu_ins_set_start_parameter(prv);
	if (ercd) {
		/* update status */
		ch_info->status = ISU_STAT_READY;

		return ercd;
	}

	/* set callback information */
	ch_info->cb_func = callback;
	ch_info->cb_userdata = userdata;

	/* start */
	isu_ins_start_processing(prv);

	return 0;
}

/******************************************************************************
 * Function:		isu_lib_abort
 * Description:	Forced stop of ISU processing
 * Returns:		0/E_ISU_PARA_CH/E_ISU_NO_INIT
 *	return of isu_ins_stop_processing().
 ******************************************************************************/
long isu_lib_abort(unsigned char ch)
{
	struct isu_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX)
		return E_ISU_PARA_CH;

	if (!g_isu_obj[ch])
		return E_ISU_NO_INIT;

	prv = g_isu_obj[ch];

	/* check status */
	if (prv->ch_info.status == ISU_STAT_RUN) {
		/* stop ISU processing */
		ercd = isu_ins_stop_processing(prv);
		if (ercd)
			return ercd;
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_lib_get_status
 * Description:	Get status of ISU processing
 * Returns:		0/E_ISU_PARA_INPAR/E_ISU_PARA_CH/E_ISU_NO_INIT/
 *	E_ISU_INVALID_STATE
 ******************************************************************************/
long isu_lib_get_status(unsigned char ch, struct isu_status_t *status)
{
	struct isu_prv_data *prv;

	/* check parameter */
	if (!status)
		return E_ISU_PARA_INPAR;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX)
		return E_ISU_PARA_CH;

	if (!g_isu_obj[ch])
		return E_ISU_NO_INIT;
	prv = g_isu_obj[ch];

	/* check status */
	if (prv->ch_info.status == ISU_STAT_INIT)
		return E_ISU_INVALID_STATE;

	/* set status */
	status->wpf_bits = prv->rdata.usable_wpf;
	status->rpf_bits = prv->rdata.usable_rpf;

	return 0;
}

/******************************************************************************
 * Function:		isu_lib_suspend
 * Description:	Suspend of ISU processing
 * Returns:		0
 *	return of isu_ins_unreg_ih().
 *	return of isu_ins_quit_reg().
 ******************************************************************************/
long isu_lib_suspend(unsigned char ch)
{
	struct isu_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX)
		return E_ISU_PARA_CH;

	prv = g_isu_obj[ch];

	if (prv && prv->isu_reg) {
		if (prv->ch_info.status == ISU_STAT_RUN) {
			/* waiting processing finish */
			(void)isu_ins_wait_processing(prv);
		}

		if (prv->ch_info.status == ISU_STAT_READY) {
			/* unregistory interrupt handler */
			ercd = isu_ins_unreg_ih(prv);
			if (ercd)
				return ercd;

			/* finalize register */
			ercd = isu_ins_quit_reg(prv);
			if (ercd)
				return ercd;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		isu_lib_resume
 * Description:	Resume of ISU processing
 * Returns:		0
 *	return of isu_ins_init_reg().
 *	return of isu_ins_reg_ih().
 ******************************************************************************/
long isu_lib_resume(unsigned char ch)
{
	struct isu_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= ISU_IP_MAX)
		return E_ISU_PARA_CH;

	prv = g_isu_obj[ch];

	if (prv && !prv->isu_reg) {
		if (prv->ch_info.status == ISU_STAT_READY) {
			/* reinitialize register */
			ercd = isu_ins_init_reg(prv);
			if (ercd)
				return ercd;

			/* reregister interrupt handler */
			ercd = isu_ins_reg_ih(prv);
			if (ercd) {
				(void)isu_ins_quit_reg(prv);
				return ercd;
			}
		}
	}

	return 0;
}
