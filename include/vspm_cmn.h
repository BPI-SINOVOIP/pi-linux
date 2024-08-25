/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2015-2021 Renesas Electronics Corporation
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

#ifndef _VSPM_API_H_
#define _VSPM_API_H_

/* VSP Manager APIs return codes */
#define R_VSPM_OK			(0)
#define	R_VSPM_NG			(-1)	/* abnormal termination */
#define	R_VSPM_PARAERR		(-2)	/* illegal parameter */
#define	R_VSPM_SEQERR		(-3)	/* sequence error */
#define R_VSPM_QUE_FULL		(-4)	/* request queue full */
#define R_VSPM_CANCEL		(-5)	/* processing was canceled */
#define R_VSPM_ALREADY_USED	(-6)	/* already used all channel */
#define R_VSPM_OCCUPY_CH	(-7)	/* occupy channel */
#define R_VSPM_DRIVER_ERR	(-10)	/* IP error(in driver) */

/* using channel */
#define VSPM_EMPTY_CH		(0xFFFFFFFF)
#define VSPM_USE_CH0		(0x00000001)
#define VSPM_USE_CH1		(0x00000002)
#define VSPM_USE_CH2		(0x00000004)
#define VSPM_USE_CH3		(0x00000008)
#define VSPM_USE_CH4		(0x00000010)
#define VSPM_USE_CH5		(0x00000020)
#define VSPM_USE_CH6		(0x00000040)
#define VSPM_USE_CH7		(0x00000080)

/* operation mode */
enum {
	VSPM_MODE_MUTUAL = 0,
	VSPM_MODE_OCCUPY
};

/* select IP */
enum {
	VSPM_TYPE_VSP_AUTO = 0,
	VSPM_TYPE_FDP_AUTO ,
	VSPM_TYPE_ISU_AUTO
};

/* Job priority */
#define VSPM_PRI_MAX		((char)126)
#define VSPM_PRI_MIN		((char)1)

#define VSPM_PRI_LOW		((char)32)
#define VSPM_PRI_STD		((char)64)
#define VSPM_PRI_HIGH		((char)96)

/* State of the entry */
#define VSPM_STATUS_WAIT		1
#define VSPM_STATUS_ACTIVE		2
#define VSPM_STATUS_NO_ENTRY	3

/* Renesas near-lossless compression setting */
#define FCP_FCNL_DISABLE		(0)
#define FCP_FCNL_ENABLE			(1)

/* Tile/Linear conversion setting */
#define FCP_TL_DISABLE			(0)
#define FCP_TL_ENABLE			(1)

struct vspm_init_vsp_t {
	/* reserved */
};

struct vspm_init_fdp_t {
	unsigned int hard_addr[2];
};

/* initialize parameter structure */
struct vspm_init_t {
	unsigned int use_ch;
	unsigned short mode;
	unsigned short type;
	union {
		struct vspm_init_vsp_t *vsp;
		struct vspm_init_vsp_t *isu;
		struct vspm_init_fdp_t *fdp;
	} par;
};

/* entry parameter structure */
struct vspm_job_t {
	unsigned short type;
	union {
		struct vsp_start_t *vsp;
		struct fdp_start_t *fdp;
		struct isu_start_t *isu;
	} par;
};

/* status parameter structure */
struct vspm_status_t {
	struct fdp_status_t *fdp;
};

/* FCP information structure */
struct fcp_info_t {
	unsigned char fcnl;
	unsigned char tlen;
	unsigned short pos_y;
	unsigned short pos_c;
	unsigned short stride_div16;
	unsigned int ba_anc_prev_y;
	unsigned int ba_anc_cur_y;
	unsigned int ba_anc_next_y;
	unsigned int ba_anc_cur_c;
	unsigned int ba_ref_prev_y;
	unsigned int ba_ref_cur_y;
	unsigned int ba_ref_next_y;
	unsigned int ba_ref_cur_c;
};

#endif
