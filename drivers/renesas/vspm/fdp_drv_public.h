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

#ifndef __FDP_DRV_PUBLIC_H__
#define __FDP_DRV_PUBLIC_H__

#define E_FDP_DEF_INH				(-100)
#define E_FDP_DEF_REG				(-101)
#define E_FDP_NO_MEM				(-102)
#define E_FDP_NO_INIT				(-103)
#define E_FDP_INVALID_PARAM			(-104)
#define E_FDP_INVALID_STATE			(-105)
#define E_FDP_NO_CLK				(-107)

struct fdp_obj_t {
	/* status information */
	unsigned char status;

	/* process information */
	struct vspm_fdp_proc_info *proc_info;

	/* callback information */
	struct fdp_cb_info_t {
		void *userdata2;
		void (*fdp_cb2)(unsigned long id, long ercd, void *userdata);
	} cb_info;

	/* register value */
	unsigned int ctrl_mode;
	unsigned int ctrl_chact;
	unsigned int rpf_format;
	unsigned int rpf0_addr_y;
	unsigned int rpf2_addr_y;
	unsigned int wpf_format;
	unsigned int wpf_swap;
	unsigned int ipc_mode;
	unsigned int ipc_lmem;
	unsigned int fcp_ref_addr_y0;
	unsigned int fcp_ref_addr_y2;
	unsigned int fcp_anc_addr_y0;
	unsigned int fcp_anc_addr_y2;

	/* platform information */
	struct platform_device *pdev;
	struct resource *irq;
	void __iomem *fdp_reg;
	void __iomem *fcp_reg;
	unsigned int lut_tbl_idx;
};

long fdp_lib_init(struct fdp_obj_t **obj);
long fdp_lib_quit(struct fdp_obj_t *obj);
long fdp_lib_open(struct fdp_obj_t *obj);
long fdp_lib_close(struct fdp_obj_t *obj);
long fdp_lib_start(struct fdp_obj_t *obj, struct fdp_start_t *start_par);
long fdp_lib_abort(struct fdp_obj_t *obj);
long fdp_lib_suspend(struct fdp_obj_t *obj);
long fdp_lib_resume(struct fdp_obj_t *obj);

#endif
