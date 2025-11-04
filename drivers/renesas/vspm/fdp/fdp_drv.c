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

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

#include "fdp_drv_public.h"
#include "fdp_drv_local.h"
#include "fdp_drv_hw.h"

/******************************************************************************
 * Function:		fdp_lib_init
 * Description:	Initialize FDP driver
 * Returns:		0/E_FDP_INVALID_PARAM
 *	return of fdp_ins_allocate_memory()
 ******************************************************************************/
long fdp_lib_init(struct fdp_obj_t **obj)
{
	long ercd;

	/* check parameter */
	if (!obj)
		return E_FDP_INVALID_PARAM;

	/* allocate memory */
	ercd = fdp_ins_allocate_memory(obj);
	if (ercd)
		return ercd;

	/* update status */
	(*obj)->status = FDP_STAT_INIT;

	return 0;
}

/******************************************************************************
 * Function:		fdp_lib_quit
 * Description:	Finalize FDP driver
 * Returns:		0/E_FDP_INVALID_PARAM
 *	return of fdp_ins_release_memory()
 ******************************************************************************/
long fdp_lib_quit(struct fdp_obj_t *obj)
{
	long ercd;

	/* check parameter */
	if (!obj)
		return E_FDP_INVALID_PARAM;

	if (obj->status == FDP_STAT_RUN) {
		ercd = fdp_lib_abort(obj);
		if (ercd)
			return ercd;
	}

	if (obj->status == FDP_STAT_READY) {
		ercd = fdp_lib_close(obj);
		if (ercd)
			return ercd;
	}

	if (obj->status == FDP_STAT_INIT) {
		/* update status */
		obj->status = FDP_STAT_NO_INIT;

		/* release memory */
		ercd = fdp_ins_release_memory(obj);
		if (ercd)
			return ercd;
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_lib_open
 * Description:	Initialize FDP hardware
 * Returns:		0/E_FDP_INVALID_PARAM/E_FDP_INVALID_STATE
 *	return of fdp_ins_get_resource()
 *	return of fdp_ins_enable_clock()
 *	return of fdp_ins_init_reg()
 *	return of fdp_reg_inth()
 ******************************************************************************/
long fdp_lib_open(struct fdp_obj_t *obj)
{
	long ercd;

	/* check parameter */
	if (!obj)
		return E_FDP_INVALID_PARAM;

	/* check status */
	if (obj->status != FDP_STAT_INIT)
		return E_FDP_INVALID_STATE;

	/* get FDP resource */
	ercd = fdp_ins_get_resource(obj);
	if (ercd)
		goto err_exit1;

	/* enable clock */
	ercd = fdp_ins_enable_clock(obj);
	if (ercd)
		goto err_exit1;

	/* initialize register */
	ercd = fdp_ins_init_reg(obj);
	if (ercd)
		goto err_exit2;

	/* registory interrupt handler */
	ercd = fdp_reg_inth(obj);
	if (ercd)
		goto err_exit3;

	/* update status */
	obj->status = FDP_STAT_READY;

	return 0;

err_exit3:
	(void)fdp_ins_quit_reg(obj);

err_exit2:
	(void)fdp_ins_disable_clock(obj);

err_exit1:
	return ercd;
}

/******************************************************************************
 * Function:		fdp_lib_close
 * Description:	Finalize FDP hardware
 * Returns:		0/E_FDP_INVALID_PARAM/E_FDP_INVALID_STATE
 *	return of fdp_free_inth()
 *	return of fdp_ins_quit_reg()
 *	return of fdp_ins_disable_clock()
 ******************************************************************************/
long fdp_lib_close(struct fdp_obj_t *obj)
{
	long ercd;

	/* check parameter */
	if (!obj)
		return E_FDP_INVALID_PARAM;

	/* check status */
	if (obj->status != FDP_STAT_READY)
		return E_FDP_INVALID_STATE;

	/* unregistory interrupt handler */
	ercd = fdp_free_inth(obj);
	if (ercd)
		return ercd;

	/* finalize register */
	ercd = fdp_ins_quit_reg(obj);
	if (ercd)
		return ercd;

	/* disable clock */
	ercd = fdp_ins_disable_clock(obj);
	if (ercd)
		return ercd;

	/* update status */
	obj->status = FDP_STAT_INIT;

	return 0;
}

/******************************************************************************
 * Function:		fdp_lib_start
 * Description:	Start FDP processing
 * Returns:		0/E_FDP_INVALID_PARAM/E_FDP_INVALID_STATE
 *	return of fdp_ins_check_start_parameter()
 ******************************************************************************/
long fdp_lib_start(struct fdp_obj_t *obj, struct fdp_start_t *start_par)
{
	long ercd;

	/* check parameter */
	if (!obj)
		return E_FDP_INVALID_PARAM;

	/* check status */
	if (obj->status != FDP_STAT_READY)
		return E_FDP_INVALID_STATE;

	if (!obj->fdp_reg)
		return E_FDP_INVALID_STATE;

	/* check start parameter */
	ercd = fdp_ins_check_start_param(obj, start_par);
	if (ercd)
		return ercd;

	/* update status */
	obj->status = FDP_STAT_RUN;

	/* FDP process start parameter setting */
	fdp_ins_start_processing(obj, start_par);

	return 0;
}

/******************************************************************************
 * Function:		fdp_lib_abort
 * Description:	Abort FDP processing
 * Returns:		0/E_FDP_INVALID_PARAM
 ******************************************************************************/
long fdp_lib_abort(struct fdp_obj_t *obj)
{
	/* check parameter */
	if (!obj)
		return E_FDP_INVALID_PARAM;

	/* check status */
	if (obj->status == FDP_STAT_RUN)
		fdp_ins_stop_processing(obj);

	return 0;
}

/******************************************************************************
 * Function:		fdp_lib_suspend
 * Description:	Suspend of FDP processing
 * Returns:		0
 *	return of fdp_free_inth().
 *	return of fdp_ins_quit_reg().
 ******************************************************************************/
long fdp_lib_suspend(struct fdp_obj_t *obj)
{
	long ercd;

	if (obj &&
	    obj->fdp_reg) {
		if (obj->status == FDP_STAT_RUN) {
			/* waiting processing finish */
			fdp_ins_wait_processing(obj);
		}

		if (obj->status == FDP_STAT_READY) {
			/* unregistory interrupt handler */
			ercd = fdp_free_inth(obj);
			if (ercd)
				return ercd;

			/* finalize register */
			ercd = fdp_ins_quit_reg(obj);
			if (ercd)
				return ercd;
		}
	}

	return 0;
}

/******************************************************************************
 * Function:		fdp_lib_resume
 * Description:	Resume of FDP processing
 * Returns:		0
 *	return of fdp_ins_init_reg().
 *	return of fdp_reg_inth().
 ******************************************************************************/
long fdp_lib_resume(struct fdp_obj_t *obj)
{
	long ercd;

	if (obj &&
	    !obj->fdp_reg) {
		if (obj->status == FDP_STAT_READY) {
			/* reinitialize register */
			ercd = fdp_ins_init_reg(obj);
			if (ercd)
				return ercd;

			/* reregistory interrupt handler */
			ercd = fdp_reg_inth(obj);
			if (ercd) {
				fdp_ins_quit_reg(obj);
				return ercd;
			}
		}
	}

	return 0;
}
