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

#ifndef _FDP_DRV_HARDW_H_
#define _FDP_DRV_HARDW_H_

/* base register address */
#define P_FDP	(obj->fdp_reg)
#define P_FCP	(obj->fcp_reg)

/* define status read counter */
#define FDP_STATUS_LOOP_TIME	(2)
#define FDP_STATUS_LOOP_CNT		(500)

/* define LUT table maximum size */
#define FDP_LUT_TBL_MAX			(3)

/* General control registers */
#define FD1_CTL_CMD			(0x0000)
#define FD1_CTL_SGCMD		(0x0004)
#define FD1_CTL_REGEND		(0x0008)
#define FD1_CTL_CHACT		(0x000C)
#define FD1_CTL_OPMODE		(0x0010)
#define FD1_CTL_VPERIOD		(0x0014)
#define FD1_CTL_CLKCTRL		(0x0018)
#define FD1_CTL_SRESET		(0x001C)
#define FD1_CTL_STATUS		(0x0024)
#define FD1_CTL_VCYCLE_STAT	(0x0028)
#define FD1_CTL_IRQENB		(0x0038)
#define FD1_CTL_IRQSTA		(0x003C)
#define FD1_CTL_IRQFSET		(0x0040)

/* RPF control registers */
#define FD1_RPF_SIZE		(0x0060)
#define FD1_RPF_FORMAT		(0x0064)
#define FD1_RPF_PSTRIDE		(0x0068)
#define FD1_RPF0_ADDR_Y		(0x006C)
#define FD1_RPF0_ADDR_C0	(0x0070)
#define FD1_RPF0_ADDR_C1	(0x0074)
#define FD1_RPF1_ADDR_Y		(0x0078)
#define FD1_RPF1_ADDR_C0	(0x007C)
#define FD1_RPF1_ADDR_C1	(0x0080)
#define FD1_RPF2_ADDR_Y		(0x0084)
#define FD1_RPF2_ADDR_C0	(0x0088)
#define FD1_RPF2_ADDR_C1	(0x008C)
#define FD1_RPF_SMSK_ADDR	(0x0090)
#define FD1_RPF_SWAP		(0x0094)

/* WPF control registers */
#define FD1_WPF_FORMAT		(0x00C0)
#define FD1_WPF_RNDCTL		(0x00C4)
#define FD1_WPF_PSTRIDE		(0x00C8)
#define FD1_WPF_ADDR_Y		(0x00CC)
#define FD1_WPF_ADDR_C0		(0x00D0)
#define FD1_WPF_ADDR_C1		(0x00D4)
#define FD1_WPF_SWAP		(0x00D8)

/* IPC control registers */
#define FD1_IPC_MODE		(0x0100)
#define FD1_IPC_SMSK_THRESH	(0x0104)
#define FD1_IPC_COMB_DET	(0x0108)
#define FD1_IPC_MOTDEC		(0x010C)

#define FD1_IPC_DLI_BLEND	(0x0120)
#define FD1_IPC_DLI_HGAIN	(0x0124)
#define FD1_IPC_DLI_SPRS	(0x0128)
#define FD1_IPC_DLI_ANGLE	(0x012C)
#define FD1_IPC_DLI_ISOPIX0	(0x0130)
#define FD1_IPC_DLI_ISOPIX1	(0x0134)

#define FD1_IPC_SENSOR_TH0	(0x0140)
#define FD1_IPC_SENSOR_TH1	(0x0144)

#define FD1_SENSOR_CTL0		(0x0170)
#define FD1_SENSOR_CTL1		(0x0174)
#define FD1_SENSOR_CTL2		(0x0178)
#define FD1_SENSOR_CTL3		(0x017C)
#define FD1_SENSOR_OFFSET	(0x0180)
#define FD1_SENSOR_0			(0x0180)
#define FD1_SENSOR_1			(0x0184)
#define FD1_SENSOR_2			(0x0188)
#define FD1_SENSOR_3			(0x018C)
#define FD1_SENSOR_4			(0x0190)
#define FD1_SENSOR_5			(0x0194)
#define FD1_SENSOR_6			(0x0198)
#define FD1_SENSOR_7			(0x019C)
#define FD1_SENSOR_8			(0x01A0)
#define FD1_SENSOR_9			(0x01A4)
#define FD1_SENSOR_10		(0x01A8)
#define FD1_SENSOR_11		(0x01AC)
#define FD1_SENSOR_12		(0x01B0)
#define FD1_SENSOR_13		(0x01B4)
#define FD1_SENSOR_14		(0x01B8)
#define FD1_SENSOR_15		(0x01BC)
#define FD1_SENSOR_16		(0x01C0)
#define FD1_SENSOR_17		(0x01C4)

#define FD1_IPC_LMEM		(0x01E0)

#define FD1_IP_INTDATA		(0x0800)

/* LUT */
#define FD1_DIF_ADJ_OFFSET	(0x1000)
#define FD1_SAD_ADJ_OFFSET	(0x1400)
#define FD1_BLD_GAIN_OFFSET	(0x1800)
#define FD1_DIF_GAIN_OFFSET	(0x1C00)
#define FD1_MDET_OFFSET		(0x2000)

/* FCP */
#define FD1_FCP_VCR			(0x0000)
#define FD1_FCP_RST			(0x0010)
#define FD1_FCP_STA			(0x0018)
#define FD1_FCP_TL_CTRL		(0x0070)
#define FD1_FCP_PICINFO1	(0x00C4)
#define FD1_BA_ANC_Y0		(0x0100)
#define FD1_BA_ANC_Y1		(0x0104)
#define FD1_BA_ANC_Y2		(0x0108)
#define FD1_BA_ANC_C		(0x010C)
#define FD1_BA_REF_Y0		(0x0110)
#define FD1_BA_REF_Y1		(0x0114)
#define FD1_BA_REF_Y2		(0x0118)
#define FD1_BA_REF_C		(0x011C)

/* FDP1 start valud */
#define FD1_CTL_CMD_STRCMD_MSK			(0x00000001)

/* Channel activation value */
#define FD1_CTL_CHACT_SMSK_WRITE		(0x00000200)
#define FD1_CTL_CHACT_WRITE				(0x00000100)
#define FD1_CTL_CHACT_SMSK_READ			(0x00000008)
#define FD1_CTL_CHACT_NEX_READ			(0x00000004)
#define FD1_CTL_CHACT_CUR_READ			(0x00000002)
#define FD1_CTL_CHACT_PRE_READ			(0x00000001)

/* Opration mode value */
#define FD1_CTL_OPMODE_INTERLACE	(0x00000000) /* interlace */
#define FD1_CTL_OPMODE_PROGRESSIVE	(0x00000010) /* progressive */
#define FD1_CTL_OPMODE_PRG_MSK		(0x00000010)

#define FD1_CTL_OPMODE_INTERRUPT	(0x00000000) /* interrupt */
#define FD1_CTL_OPMODE_BEST_EFFORT	(0x00000001) /* best effort */
#define FD1_CTL_OPMODE_NO_INTERRUPT	(0x00000002) /* no interrupt */
#define FD1_CTL_OPMODE_VIMD_MSK		(0x00000003)

/* Clock control value */
#define FD1_CTL_CLK_CTRL_CSTP_N			(0x00000001)

/* Software reset value */
#define FD1_CTL_SRESET_SRST				(0x00000001)

/* Status value */
#define FD1_CTL_STATUS_REG_END			(0x00000400)
#define FD1_CTL_STATUS_FRAME_END		(0x00000100)
#define FD1_CTL_STATUS_BSY				(0x00000001)

/* Interrupt value */
#define FD1_CTL_IRQ_VSYNC_ERR			(0x00010000)
#define FD1_CTL_IRQ_VSYNC_END			(0x00000010)
#define FD1_CTL_IRQ_FRAME_END			(0x00000001)

/* RPF/WPF data swap value */
#define FD1_SWAP_LL					(0x8)
#define FD1_SWAP_L					(0x4)
#define FD1_SWAP_W					(0x2)
#define FD1_SWAP_B					(0x1)
#define FD1_RPF_SWAP_ISWAP \
	(FD1_SWAP_LL | FD1_SWAP_L | FD1_SWAP_W | FD1_SWAP_B)
#define FD1_WPF_SWAP_SSWAP \
	(FD1_RPF_SWAP_ISWAP << 4)
#define FD1_WPF_SWAP_OSWAP \
	(FD1_SWAP_LL | FD1_SWAP_L | FD1_SWAP_W | FD1_SWAP_B)
#define FD1_WPF_SWAP_OSWAP_FCP \
	(FD1_SWAP_LL)

/* Source picture format value */
#define FD1_RPF_FORMAT_CIPM_INT			(0x00010000)
#define FD1_RPF_FORMAT_CIPM_PRG			(0x00000000)
#define FD1_RPF_FORMAT_CF_TOP			(0x00000000)
#define FD1_RPF_FORMAT_CF_BOTTOM		(0x00000100)

#define FD1_RPF_FORMAT_YUV444_SEMI		(0x00000040)
#define FD1_RPF_FORMAT_YUV422_NV16		(0x00000041)
#define FD1_RPF_FORMAT_YUV422_NV61		(0x00001041)
#define FD1_RPF_FORMAT_YUV420_NV12		(0x00000042) /* TLEN */
#define FD1_RPF_FORMAT_YUV420_NV21		(0x00001042) /* TLEN */

#define FD1_RPF_FORMAT_YUV444_INTER		(0x00000046)
#define FD1_RPF_FORMAT_YUV422_UYVY		(0x00000047)
#define FD1_RPF_FORMAT_YUV422_YUY2		(0x00002047) /* FCNL */
#define FD1_RPF_FORMAT_YUV422_YVYU		(0x00003047)
#define FD1_RPF_FORMAT_YUV422_TYPE1		(0x00000048)
#define FD1_RPF_FORMAT_YUV444_PLANAR	(0x0000004A) /* FCNL */

#define FD1_RPF_FORMAT_YUV422_YV16		(0x0000004B) /* FCNL */
#define FD1_RPF_FORMAT_YUV420_YV12		(0x0000004C) /* FCNL */
#define FD1_RPF_FORMAT_YUV420_YU12		FD1_RPF_FORMAT_YUV420_YV12

/* Destination picture format value */
#define FD1_WPF_FORMAT_FCNL_ENABLE		(0x00100000)

#define FD1_WPF_FORMAT_YUV444_SEMI		(0x00000040)
#define FD1_WPF_FORMAT_YUV422_NV16		(0x00000041)
#define FD1_WPF_FORMAT_YUV422_NV61		(0x00004041)
#define FD1_WPF_FORMAT_YUV420_NV12		(0x00000042)
#define FD1_WPF_FORMAT_YUV420_NV21		(0x00004042)

#define FD1_WPF_FORMAT_YUV444_INTER		(0x00000046)
#define FD1_WPF_FORMAT_YUV422_UYVY		(0x00000047)
#define FD1_WPF_FORMAT_YUV422_YUY2		(0x00008047)
#define FD1_WPF_FORMAT_YUV422_YVYU		(0x0000C047)
#define FD1_WPF_FORMAT_YUV422_TYPE1		(0x00000048)
#define FD1_WPF_FORMAT_YUV444_PLANAR	(0x0000004A)

#define FD1_WPF_FORMAT_YUV422_YV16		(0x0000004B)
#define FD1_WPF_FORMAT_YUV420_YV12		(0x0000004C)
#define FD1_WPF_FORMAT_YUV420_YU12		FD1_WPF_FORMAT_YUV420_YV12

/* IPC mode value */
#define FD1_IPC_MODE_DLI				(0x00000100)
#define FD1_IPC_MODE_DIM_ADAPTIVE_2D_3D	(0x00000000)
#define FD1_IPC_MODE_DIM_FIXED_2D		(0x00000001)
#define FD1_IPC_MODE_DIM_FIXED_3D		(0x00000002)
#define FD1_IPC_MODE_DIM_PREVIOUS_FIELD	(0x00000003)
#define FD1_IPC_MODE_DIM_NEXT_FIELD		(0x00000004)
#define FD1_IPC_MODE_DIM_MSK			(0x00000007)

/* Still mask threshold value */
#define FD1_IPC_SMSK_THRESH_FSM0		(0x00010000)
#define FD1_IPC_SMSK_THRESH_SMSK_TH		(0x02)

/* Comb detection parameter value */
#define FD1_IPC_COMB_DET_CMB_OFST		((0x20) << 16)
#define FD1_IPC_COMB_DET_CMB_MAX		((0x00) << 8)
#define FD1_IPC_COMB_DET_CMB_GRAD		(0x40)

/* Motion decision parameter value */
#define FD1_IPC_MOTDEC_MOV_COEF			((0x80) << 8)
#define FD1_IPC_MOTDEC_STL_COEF			(0x20)

/* DLI blend parameter value */
#define FD1_IPC_DLI_BLEND_BLD_GRAD		((0x80) << 16)
#define FD1_IPC_DLI_BLEND_BLD_MAX		((0xFF) << 8)
#define FD1_IPC_DLI_BLEND_BLD_OFST		(0x02)

/* DLI horizontal frequency gain value */
#define FD1_IPC_DLI_HGAIN_HG_GRAD		((0x10) << 16)
#define FD1_IPC_DLI_HGAIN_HG_OFST		((0x00) << 8)
#define FD1_IPC_DLI_HGAIN_HG_MAX		(0xFF)

/* DLI suppression parameter value */
#define FD1_IPC_DLI_SPRS_SPRS_GRAD		((0x90) << 16)
#define FD1_IPC_DLI_SPRS_SPRS_OFST		((0x04) << 8)
#define FD1_IPC_DLI_SPRS_SPRS_MAX		(0xFF)

/* DLI angle parameter register value */
#define FD1_IPC_DLI_ANGLE_ASEL45		((0x04) << 16)
#define FD1_IPC_DLI_ANGLE_ASEL22		((0x08) << 8)
#define FD1_IPC_DLI_ANGLE_ASEL15		(0x0C)

/* DLI isolated pixel parameter value */
#define FD1_IPC_DLI_ISOPIX_MAX45		((0xFF) << 24)
#define FD1_IPC_DLI_ISOPIX_GRAD45		((0x10) << 16)
#define FD1_IPC_DLI_ISOPIX_MAX22		((0xFF) << 8)
#define FD1_IPC_DLI_ISOPIX_GRAD22		(0x10)
#define FD1_IPC_DLI_ISOPIX_MAX15		((0xFF) << 8)
#define FD1_IPC_DLI_ISOPIX_GRAD15		(0x10)

/* IPC sensor threshold value */
#define FD1_IPC_SENSOR_VFQ_TH			((0x20) << 24)
#define FD1_IPC_SENSOR_HFQ_TH			((0x20) << 16)
#define FD1_IPC_SENSOR_DIF_TH			((0x80) << 8)
#define FD1_IPC_SENSOR_SAD_TH			(0x80)
#define FD1_IPC_SENSOR_DETECTOR_SEL		((0x00) << 16)
#define FD1_IPC_SENSOR_COMB_TH			((0x00) << 8)
#define FD1_IPC_SENSOR_FREQ_TH			(0x00)

/* Sensor control value */
#define FD1_SENSOR_CTL0_FRM_LVTH		((2) << 12)
#define FD1_SENSOR_CTL0_FLD_LVTH		((2) << 8)
#define FD1_SENSOR_CTL0_FD_EN			(1)

#define FD1_SENSOR_CTL1_XS				(0)
#define FD1_SENSOR_CTL1_YS				(0)

/* Line memory pixel number value */
#define FD1_IPC_LMEM_PNUM				(1024)
#define FD1_IPC_LMEM_PNUM_FCP			(960)

/* FCP register value */
#define FD1_FCP_RST_RIIF				(0x00400000)
#define FD1_FCP_RST_RSIF				(0x00200000)
#define FD1_FCP_RST_DCMP				(0x00100000)
#define FD1_FCP_RST_SOFTRST				(0x00000001)
#define FD1_FCP_STA_ACT					(0x00000001)
#define FD1_FCP_TL_CTRL_TLEN			(0x80000000)

void fdp_write_reg_lut_tbl(
	const unsigned char *data,
	void __iomem *base,
	unsigned int offset);

void fdp_rewrite_reg(
	unsigned int data,
	unsigned int mask,
	void __iomem *base,
	unsigned int offset);

void fdp_write_reg(
	unsigned int data,
	void __iomem *base,
	unsigned int offset);

unsigned int fdp_read_reg(
	void __iomem *base,
	unsigned int offset);

#endif
