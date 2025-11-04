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

#ifndef __ISU_DRV_PHY_H__
#define __ISU_DRV_PHY_H__

#include <linux/version.h>

/* define register offset */
/* System Management Register offset address*/
#define ISU_FM_DL_STADDH	0x0000  /* FM Descriptor List Address Registers 0 */
#define ISU_FM_DL_STADDL	0x0004  /* FM Descriptor List Address Registers 1 */
#define ISU_FM_FRCON		0x0008  /* FM Frame Control Registers */
#define ISU_FM_STOP		0x000C  /* FM Module Stop Registers */
#define ISU_FM_INT_EN		0x0010  /* FM Interrupt Enable Registers */
#define ISU_FM_INT_STA		0x0014  /* FM Interrupt Status Registers */
#define ISU_FM_INT_FREC        0x0018  /* FM Frame End Interrupt Control Registers */
#define ISU_AXI_ERAC		0x0020  /* AXI Error Action Registers */
#define ISU_AXI_FIFO_CAP	0x002C  /* AXI FIFO Capability Registers */

/* Common Register
Sets input/ouput, reduction ratio and color conversion coefficient
*/
#define ISU_RPF_SRC_SIZE	0x0100  /* RPF Source Image Size Registers */
#define ISU_RPF_SRC_STRD	0x0104  /* RPF Source Stride Registers */
#define ISU_RPF_SRC_ADDH_PL0	0x0108  /* RPF Source Plane0 Address Registers 0 */
#define ISU_RPF_SRC_ADDL_PL0	0x010C  /* RPF Source Plane0 Address Registers 1 */
#define ISU_RPF_SRC_ADDH_PL1	0x0110  /* RPF Source Plane1 Address Registers 0 */
#define ISU_RPF_SRC_ADDL_PL1	0x0114  /* RPF Source Plane1 Address Registers 1 */
#define ISU_RPF_FMT		0x0118  /* RPF Source Image Format Registers */
#define ISU_RPF_UVBIN		0x011C  /* RPF Source Image UV Format Register */
#define ISU_RPF_SRC_DSWAP	0x0120  /* RPF Source Image Data Swap Registers */
#define ISU_RPF_ALPH_SEL	0x0124  /* RPF Source ALPHA Data Selection Registers */
#define ISU_RPF_SRC_TD1		0x0128  /* RPF Source TEST Data Register1 */
#define ISU_RPF_SRC_TD2		0x012C  /* RPF Source TEST Data Register2 */
#define ISU_RS_HSCALE		0x0140  /* RS Scaling Factor Registers 0 */
#define ISU_RS_VSCALE		0x0144  /* RS Scaling Factor Registers 1 */
#define ISU_RS_STPOS		0x0148  /* RS Output Image Start Position Registers */
#define ISU_RS_POS_TUNE		0x014C  /* RS Output Image Start Position Tuning Registers */
#define ISU_RS_OS_CROP		0x0150  /* RS Output Size Crop Registers */
#define ISU_RS_PADDMODE		0x0154  /* RS CROP Padding Mode Registers */
#define ISU_RS_PADDVAL		0x0158  /* RS CROP Padding Value Registers */
#define ISU_WPF_DST_ADDH_PL0	0x0180  /* WPF Destination Plane0 Address Registers 0 */
#define ISU_WPF_DST_ADDL_PL0	0x0184  /* WPF Destination Plane0 Address Registers 1 */
#define ISU_WPF_DST_ADDH_PL1	0x0188  /* WPF Destination Plane1 Address Registers 0 */
#define ISU_WPF_DST_ADDL_PL1	0x018C  /* WPF Destination Plane1 Address Registers 1 */
#define ISU_WPF_DST_STRD	0x0190  /* WPF Destination Stride Registers */
#define ISU_WPF_FMT		0x0194  /* WPF Destination Image Format Registers */
#define ISU_WPF_CCOL		0x0198  /* WPF Color Collection Control Registers */
#define ISU_WPF_MUL1		0x019C  /* WPF Color Collection MUL Coefficient Registers1 */
#define ISU_WPF_MUL2		0x01A0  /* WPF Color Collection MUL Coefficient Registers2 */
#define ISU_WPF_MUL3		0x01A4  /* WPF Color Collection MUL Coefficient Registers3 */
#define ISU_WPF_MUL4		0x01A8  /* WPF Color Collection MUL Coefficient Registers4 */
#define ISU_WPF_MUL5		0x01AC  /* WPF Color Collection MUL Coefficient Registers5 */
#define ISU_WPF_MUL6		0x01B0  /* WPF Color Collection MUL Coefficient Registers6 */
#define ISU_WPF_OFST1		0x01B4  /* WPF Color Collection Offset Coefficient Registers1 */
#define ISU_WPF_OFST2		0x01B8  /* WPF Color Collection Offset Coefficient Registers2 */
#define ISU_WPF_CLP1		0x01BC  /* WPF Color Collection Clip Registers1 */
#define ISU_WPF_CLP2		0x01C0  /* WPF Color Collection Clip Registers2 */
#define ISU_WPF_DST_DSWAP	0x01C4  /* WPF Destination Image Data Swap Registers */
#define ISU_WPF_ALPH_SEL1	0x01C8  /* WPF Destination ALPHA Selection Registers1 */
#define ISU_WPF_ALPH_SEL2	0x01CC  /* WPF Destination ALPHA Selection Registers2 */
#define ISU_WPF_ALPH_VAL	0x01D0  /* WPF Destination ALPHA Value Registers */
#define ISU_AXI_BLEN		0x01F0  /* AXI Max Burst Length Registers */

/* Controler bit */
#define ISU_DESON		0x00010000  /* Processing method selection bit */
#define ISU_START		0x00000001  /* Frame processing start bit */
#define ISU_STOP		0x00000001  /* Frame processing stop bit */

/* Interrupt control bits */
#define ISU_INT_AXIRXERRE	0x10000000	/* Control of AXI bus read error interrupt */
#define ISU_INT_AXITXERRE	0x01000000	/* Control of AXI bus write error interrupt */
#define ISU_INT_LISTERRE	0x00010000  /* Control of "Descriptor List" format violation interrupt */
#define ISU_INT_SRSTENDE	0x00000100  /* Control of emergency stop completion interrupt bit */
#define ISU_INT_DESENDE		0x00000002	/* Control of descriptor footer read completion interrupt */
#define ISU_INT_FRENDE		0x00000001  /* Control of the frame processing end */

/* Interrupt bits */
#define ISU_INT_RRESPERR	0x38000000  /* Displays the status of the AXI read response */
#define ISU_INT_BRESPERR	0x03800000	/* Displays the status of the AXI write response */
#define ISU_INT_LISTERR		0x00010000	/* Displays the status of "Descriptor List" format violations */
#define ISU_INT_SRSTEND		0x00000100	/* Displays the status of the emergency stop completion */
#define ISU_INT_DESEND		0x00000002	/* Displays the status of the descriptor footer read completion */

#define ISU_RPF_TD_USE		(0x80000000)
#define ISU_RPF_AEXT		(0x00010000)
#define ISU_RS_PADSEL		(0x00000001)
#define ISU_RS_NO_RESIZE	(0x00010000)
#define ISU_RPF_UV_CONV		(0x00000001)
#define ISU_WPF_ASEL		(0x00010000)
#define ISU_WPF_CCOL_SEL	(0x00000002)
#define ISU_WPF_CCOL_ASE	(0x00000001)


/* define status read counter */
#define ISU_STATUS_LOOP_TIME	(2)
#define ISU_STATUS_LOOP_CNT	(500)

/* define module maximum */
#define ISU_RPF_MAX		(1)
#define ISU_WPF_MAX		(1)
#define ISU_IP_MAX		(ISU_RPF_MAX)

/* define status */
#define ISU_STAT_NOT_INIT	0
#define ISU_STAT_INIT		1
#define ISU_STAT_READY		2
#define ISU_STAT_RUN		3

/* define */
#define ISU_FALSE		0
#define ISU_TRUE		1

/* define color space */
#define ISU_COLOR_NO		(0)
#define ISU_COLOR_RGB		(1)
#define ISU_COLOR_YUV		(2)
#define ISU_COLOR_RAW		(3)

/* RPF information structure */
struct isu_rpf_info {
	unsigned long addr;
	unsigned long addr_c;
	unsigned short stride;
	unsigned short stride_c;
	unsigned short height;
	unsigned short width;
	unsigned char format;
	unsigned char  swap;	/* Data swap for input images */
	unsigned int rpf_alpha_val;
	unsigned int src_td1;
	unsigned int src_td2;
	unsigned int uv_bin;
};

/* Scaling factor information */
struct isu_rs_info {
	unsigned short start_x;
	unsigned short start_y;
	unsigned short tune_x;
	unsigned short tune_y;
	unsigned short crop_w;
	unsigned short crop_h;
	unsigned char pad_mode;
	unsigned int pad_val;
	unsigned int x_scale;
	unsigned int y_scale;
};

/* WPF information structure */
struct isu_wpf_info {
	unsigned long addr;
	unsigned long addr_c;
	unsigned short stride;
	unsigned short stride_c;
	unsigned char format;
	unsigned char swap;
	unsigned int alpha_asel1;
	unsigned int alpha_asel2;
	unsigned int alpha_val;
	unsigned char ccol;
	unsigned short k_matrix[3][3];
	unsigned short offset[3][2];
	unsigned short clip[3][2];
};

/* channel information structure */
struct isu_ch_info {
	unsigned char status;

	void (*cb_func)
		(unsigned long id, long ercd, void *userdata);
	void *cb_userdata;

	struct isu_rpf_info rpf_info;;
	struct isu_rs_info rs_info;
	struct isu_wpf_info wpf_info;
	unsigned long dl_info;
};

/* private data structure */
struct isu_prv_data {
	struct platform_device *pdev;
	void __iomem *isu_reg;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	struct resource *irq;
#else
	int irq;
#endif
	struct isu_res_data {
		unsigned int usable_rpf;
		unsigned int usable_wpf;
		unsigned int usable_module;
		bool burst_enable;
	} rdata;

	struct isu_ch_info ch_info;
};

/* define local functions */
long isu_ins_check_init_parameter(struct isu_init_t *param);
long isu_ins_check_open_parameter(struct isu_open_t *param);
long isu_ins_check_start_parameter(
	struct isu_prv_data *prv, struct isu_start_t *param);

long isu_ins_set_start_parameter(struct isu_prv_data *prv);
void isu_ins_start_processing(struct isu_prv_data *prv);
long isu_ins_stop_processing(struct isu_prv_data *prv);
long isu_ins_wait_processing(struct isu_prv_data *prv);

long isu_ins_get_isu_resource(struct isu_prv_data *prv);

long isu_ins_enable_clock(struct isu_prv_data *prv);
long isu_ins_disable_clock(struct isu_prv_data *prv);

long isu_ins_init_reg(struct isu_prv_data *prv);
long isu_ins_quit_reg(struct isu_prv_data *prv);

void isu_ins_cb_function(struct isu_prv_data *prv, long ercd);

long isu_ins_reg_ih(struct isu_prv_data *prv);
long isu_ins_unreg_ih(struct isu_prv_data *prv);

#endif
