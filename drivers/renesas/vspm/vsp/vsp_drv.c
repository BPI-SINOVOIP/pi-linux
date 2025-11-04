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

#include <linux/slab.h>

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"

#include "vsp_drv_public.h"
#include "vsp_drv_local.h"

static struct vsp_prv_data *g_vsp_obj[VSP_IP_MAX] = {NULL};

/******************************************************************************
 * Function:		vsp_lib_init
 * Description:	Initialize VSP driver
 * Returns:		0/E_VSP_NO_MEM
 *	return of vsp_ins_check_init_parameter()
 ******************************************************************************/
long vsp_lib_init(struct vsp_init_t *param)
{
	struct vsp_prv_data *prv;

	unsigned int i;

	long ercd;

	/* check initialize parameter */
	ercd = vsp_ins_check_init_parameter(param);
	if (ercd)
		return ercd;

	for (i = 0; i < param->ip_num; i++) {
		/* allocate memory */
		prv = kzalloc(sizeof(struct vsp_prv_data), GFP_KERNEL);
		if (!prv)
			goto err_exit;

		/* update status */
		prv->ch_info[0].status = VSP_STAT_INIT;
		prv->ch_info[1].status = VSP_STAT_INIT;

		g_vsp_obj[i] = prv;
	}

	return 0;

err_exit:
	for (i = 0; i < param->ip_num; i++) {
		kfree(g_vsp_obj[i]);
		g_vsp_obj[i] = NULL;
	}

	return E_VSP_NO_MEM;
}

/******************************************************************************
 * Function:		vsp_lib_quit
 * Description:	Finalize VSP driver
 * Returns:		0
 *	return of vsp_lib_abort()
 *	return of vsp_lib_close()
 ******************************************************************************/
long vsp_lib_quit(void)
{
	struct vsp_prv_data **prv = &g_vsp_obj[0];

	long ercd;

	unsigned int i;

	for (i = 0; i < VSP_IP_MAX; i++) {
		if (*prv) {
			/* check condition */
			if (((*prv)->ch_info[0].status == VSP_STAT_RUN) ||
			    ((*prv)->ch_info[1].status == VSP_STAT_RUN)) {
				/* stop VSP processing */
				ercd = vsp_lib_abort((unsigned char)i);
				if (ercd)
					return ercd;
			}

			if (((*prv)->ch_info[0].status == VSP_STAT_READY) &&
			    ((*prv)->ch_info[1].status == VSP_STAT_READY)) {
				ercd = vsp_lib_close((unsigned char)i);
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
 * Function:		vsp_lib_open
 * Description:	Initialize FDP channel.
 * Returns:		0/E_VSP_PARA_CH/E_VSP_NO_INIT/E_VSP_INVALID_STATE
 *	return of vsp_ins_get_pdata()
 *	return of vsp_ins_enable_clock()
 *	return of vsp_ins_init_reg()
 *	return of vsp_ins_reg_ih()
 ******************************************************************************/
long vsp_lib_open(unsigned char ch, struct vsp_open_t *param)
{
	struct vsp_prv_data *prv;

	long ercd;

	/* check open parameter */
	ercd = vsp_ins_check_open_parameter(param);
	if (ercd)
		goto err_exit1;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX) {
		ercd = E_VSP_PARA_CH;
		goto err_exit1;
	}

	if (!g_vsp_obj[ch]) {
		ercd = E_VSP_NO_INIT;
		goto err_exit1;
	}
	prv = g_vsp_obj[ch];

	/* check status */
	if (prv->ch_info[0].status != VSP_STAT_INIT ||
	    prv->ch_info[1].status != VSP_STAT_INIT) {
		ercd = E_VSP_INVALID_STATE;
		goto err_exit1;
	}

	prv->pdev = param->pdev;

	/* set open parameter */
	ercd = vsp_ins_get_vsp_resource(prv);
	if (ercd)
		goto err_exit1;

	/* enable clock */
	ercd = vsp_ins_enable_clock(prv);
	if (ercd)
		goto err_exit1;

	/* initialize register */
	ercd = vsp_ins_init_reg(prv);
	if (ercd)
		goto err_exit2;

	/* registory interrupt handler */
	ercd = vsp_ins_reg_ih(prv);
	if (ercd)
		goto err_exit3;

	/* update status */
	prv->ch_info[0].status = VSP_STAT_READY;
	prv->ch_info[1].status = VSP_STAT_READY;

	return 0;

err_exit3:
	(void)vsp_ins_quit_reg(prv);

err_exit2:
	(void)vsp_ins_disable_clock(prv);

err_exit1:
	return ercd;
}

/******************************************************************************
 * Function:		vsp_lib_close
 * Description:	Finalize FDP channel.
 * Returns:		0/E_VSP_PARA_CH/E_VSP_NO_INIT/E_VSP_INVALID_STATE
 *	return of vsp_ins_unreg_ih()
 *	return of vsp_ins_quit_reg()
 *	return of vsp_ins_disable_clock()
 ******************************************************************************/
long vsp_lib_close(unsigned char ch)
{
	struct vsp_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX)
		return E_VSP_PARA_CH;

	/* check condition */
	if (!g_vsp_obj[ch])
		return E_VSP_NO_INIT;
	prv = g_vsp_obj[ch];

	/* check status */
	if (prv->ch_info[0].status != VSP_STAT_READY ||
	    prv->ch_info[1].status != VSP_STAT_READY)
		return E_VSP_INVALID_STATE;

	/* unregistory interrupt handler */
	ercd = vsp_ins_unreg_ih(prv);
	if (ercd)
		return ercd;

	/* Finalize register */
	ercd = vsp_ins_quit_reg(prv);
	if (ercd)
		return ercd;

	/* disable clock */
	ercd = vsp_ins_disable_clock(prv);
	if (ercd)
		return ercd;

	/* update status */
	prv->ch_info[0].status = VSP_STAT_INIT;
	prv->ch_info[1].status = VSP_STAT_INIT;

	return 0;
}

/******************************************************************************
 * Function:		vsp_lib_start
 * Description:	Start VSP processing
 * Returns:		0/E_VSP_PARA_CB/E_VSP_PARA_INPAR/E_VSP_PARA_CH
 *	E_VSP_NO_INIT/E_VSP_INVALID_STATE
 *	return of vsp_ins_check_start_parameter()
 *	return of vsp_ins_set_start_parameter()
 ******************************************************************************/
long vsp_lib_start(
	unsigned char ch,
	void *callback,
	struct vsp_start_t *param,
	void *userdata)
{
	struct vsp_prv_data *prv;
	struct vsp_ch_info *ch_info;

	long ercd;

	/* check start parameter */
	if (!callback)
		return E_VSP_PARA_CB;

	if (!param)
		return E_VSP_PARA_INPAR;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX)
		return E_VSP_PARA_CH;

	if (!g_vsp_obj[ch])
		return E_VSP_NO_INIT;

	prv = g_vsp_obj[ch];

	/* check write index */
	if (prv->widx > 1)
		return E_VSP_NO_INIT;
	ch_info = &prv->ch_info[prv->widx];

	/* check status */
	if (ch_info->status != VSP_STAT_READY)
		return E_VSP_INVALID_STATE;

	if (!prv->vsp_reg)
		return E_VSP_INVALID_STATE;

	/* update status */
	ch_info->status = VSP_STAT_RUN;

	/* check start parameter */
	ercd = vsp_ins_check_start_parameter(prv, param);
	if (ercd) {
		/* update status */
		ch_info->status = VSP_STAT_READY;

		return ercd;
	}

	/* set start parameter */
	ercd = vsp_ins_set_start_parameter(prv, param);
	if (ercd) {
		/* update status */
		ch_info->status = VSP_STAT_READY;

		return ercd;
	}

	/* set callback information */
	ch_info->cb_func = callback;
	ch_info->cb_userdata = userdata;

	/* start */
	vsp_ins_start_processing(prv);

	return 0;
}

/******************************************************************************
 * Function:		vsp_lib_abort
 * Description:	Forced stop of VSP processing
 * Returns:		0/E_VSP_PARA_CH/E_VSP_NO_INIT
 *	return of vsp_ins_stop_processing().
 ******************************************************************************/
long vsp_lib_abort(unsigned char ch)
{
	struct vsp_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX)
		return E_VSP_PARA_CH;

	if (!g_vsp_obj[ch])
		return E_VSP_NO_INIT;

	prv = g_vsp_obj[ch];

	/* check status */
	if (prv->ch_info[0].status == VSP_STAT_RUN ||
	    prv->ch_info[1].status == VSP_STAT_RUN) {
		/* stop VSP processing */
		ercd = vsp_ins_stop_processing(prv);
		if (ercd)
			return ercd;
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_lib_get_status
 * Description:	Get status of VSP processing
 * Returns:		0/E_VSP_PARA_INPAR/E_VSP_PARA_CH/E_VSP_NO_INIT/
 *	E_VSP_INVALID_STATE
 ******************************************************************************/
long vsp_lib_get_status(unsigned char ch, struct vsp_status_t *status)
{
	struct vsp_prv_data *prv;

	/* check parameter */
	if (!status)
		return E_VSP_PARA_INPAR;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX)
		return E_VSP_PARA_CH;

	if (!g_vsp_obj[ch])
		return E_VSP_NO_INIT;
	prv = g_vsp_obj[ch];

	/* check status */
	if (prv->ch_info[0].status == VSP_STAT_INIT ||
	    prv->ch_info[1].status == VSP_STAT_INIT)
		return E_VSP_INVALID_STATE;

	/* set status */
	status->module_bits = prv->rdata.usable_module;
	status->rpf_bits = prv->rdata.usable_rpf;
	status->rpf_clut_bits = prv->rdata.usable_rpf_clut;
	status->wpf_rot_bits = prv->rdata.usable_wpf_rot;

	return 0;
}

/******************************************************************************
 * Function:		vsp_lib_suspend
 * Description:	Suspend of VSP processing
 * Returns:		0
 *	return of vsp_ins_unreg_ih().
 *	return of vsp_ins_quit_reg().
 ******************************************************************************/
long vsp_lib_suspend(unsigned char ch)
{
	struct vsp_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX)
		return E_VSP_PARA_CH;

	prv = g_vsp_obj[ch];

	if (prv &&
	    prv->vsp_reg) {
		if (prv->ch_info[0].status == VSP_STAT_RUN ||
		    prv->ch_info[1].status == VSP_STAT_RUN) {
			/* waiting processing finish */
			(void)vsp_ins_wait_processing(prv);
		}

		if (prv->ch_info[0].status == VSP_STAT_READY &&
		    prv->ch_info[1].status == VSP_STAT_READY) {
			/* unregistory interrupt handler */
			ercd = vsp_ins_unreg_ih(prv);
			if (ercd)
				return ercd;

			/* finalize register */
			ercd = vsp_ins_quit_reg(prv);
			if (ercd)
				return ercd;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		vsp_lib_resume
 * Description:	Resume of VSP processing
 * Returns:		0
 *	return of vsp_ins_init_reg().
 *	return of vsp_ins_reg_ih().
 ******************************************************************************/
long vsp_lib_resume(unsigned char ch)
{
	struct vsp_prv_data *prv;

	long ercd;

	/* check channel parameter */
	if (ch >= VSP_IP_MAX)
		return E_VSP_PARA_CH;

	prv = g_vsp_obj[ch];

	if (prv &&
	    !prv->vsp_reg) {
		if (prv->ch_info[0].status == VSP_STAT_READY &&
		    prv->ch_info[1].status == VSP_STAT_READY) {
			/* reinitialize register */
			ercd = vsp_ins_init_reg(prv);
			if (ercd)
				return ercd;

			/* reregister interrupt handler */
			ercd = vsp_ins_reg_ih(prv);
			if (ercd) {
				(void)vsp_ins_quit_reg(prv);
				return ercd;
			}
		}
	}

	return 0;
}
