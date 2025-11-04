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

#ifndef __VSPM_MAIN_H__
#define __VSPM_MAIN_H__

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/clk.h>

#define DEVNAME				"vspm"
#define DEVNUM				1
#define THREADNAME			DEVNAME
#define DRVNAME				DEVNAME

#define VSP_CLK_NAME		"vsp"
#define FDP_CLK_NAME		"fdp"
#define FCP_CLK_NAME		"fcp"
#define ISU_ACLK_NAME		"isu_aclk"
#define ISU_PCLK_NAME		"isu_pclk"
#define ISU_ARST_NAME		"aresetn"
#define ISU_PRST_NAME		"presetn"
#define CLKNUM			2

/* vspm driver data structure */
struct vspm_drvdata {
	struct platform_device *vsp_pdev[VSPM_VSP_IP_MAX];
	struct clk *vsp_clks[VSPM_VSP_IP_MAX][CLKNUM];
	struct platform_device *fdp_pdev[VSPM_FDP_IP_MAX];
	struct clk *fdp_clks[VSPM_FDP_IP_MAX][CLKNUM];
	struct platform_device *isu_pdev[VSPM_ISU_IP_MAX];
	struct clk *isu_clks[VSPM_ISU_IP_MAX][CLKNUM];
	struct task_struct *task;
	atomic_t counter;
	atomic_t suspend;
	struct semaphore init_sem;
};

/* FDP process information structure */
struct vspm_fdp_proc_info {
	struct fdp_seq_t seq_par;	/* sequence parameter */
	unsigned int stlmsk_addr[2];/* Still mask address */
	unsigned char stlmsk_idx;	/* Still mask index */
	unsigned char set_prev_flag;/* flag of previous setting */
	unsigned char seq_cnt;		/* sequence counter */
	struct fdp_status_t status;
};

/* request information structure */
struct vspm_request_res_info {
	unsigned int ch_bits;		/* request channel bit */
	unsigned short type;		/* using IP */
	unsigned short mode;		/* operation mode */
	struct vspm_fdp_proc_info fdp_info; /* FDP process information */
};

/* vspm device file private data structure */
struct vspm_privdata {
	struct vspm_drvdata *pdrv;
	struct vspm_request_res_info request_info;
};

/* subroutines */
long vspm_init(struct vspm_drvdata *pdrv);
long vspm_quit(struct vspm_drvdata *pdrv);
long vspm_suspend(struct vspm_drvdata *pdrv);
long vspm_resume(struct vspm_drvdata *pdrv);
long vspm_cancel(struct vspm_privdata *priv);

#endif /* __VSPM_MAIN_H__ */
