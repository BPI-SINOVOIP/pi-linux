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

#ifndef __VSPM_LIB_PUBLIC_H__
#define __VSPM_LIB_PUBLIC_H__

/* entry parameter */
struct vspm_api_param_entry {
	struct vspm_privdata *priv;
	unsigned long *p_job_id;
	char job_priority;
	struct vspm_job_t *p_ip_par;
	PFN_VSPM_COMPLETE_CALLBACK pfn_complete_cb;
	void *user_data;
};

/* set mode parameter */
struct vspm_api_param_mode {
	struct vspm_privdata *priv;
	struct vspm_init_t *param;
};

/* forced cancel parameter */
struct vspm_api_param_forced_cancel {
	struct vspm_privdata *priv;
};

/* complete processing parameter */
struct vspm_api_param_on_complete {
	unsigned short module_id;
	long result;
};

/* library functions */
void vspm_task(void);
long vspm_lib_entry(
	struct vspm_api_param_entry *entry);
long vspm_lib_queue_cancel(
	struct vspm_privdata *priv, unsigned long job_id);
long vspm_lib_forced_cancel(
	struct vspm_privdata *priv);
long vspm_lib_set_mode(
	struct vspm_privdata *priv, struct vspm_init_t *param);
long vspm_lib_get_status(
	struct vspm_privdata *priv, struct vspm_status_t *param);

#endif
