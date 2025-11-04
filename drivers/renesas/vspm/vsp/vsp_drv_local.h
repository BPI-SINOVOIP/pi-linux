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

#ifndef __VSP_DRV_LOCAL_H__
#define __VSP_DRV_LOCAL_H__

/* define macro */
#define VSP_DL_HARD_TO_VIRT(addr) \
	((void *)((unsigned long)(st_par->dl_par.virt_addr) + \
	((unsigned long)(addr) - (unsigned long)(st_par->dl_par.hard_addr))))
#define VSP_DL_WRITE(offset, data) \
	dlwrite32(&body, reg_offset + (offset), (data))

#define VSP_UDS_SCALE_AMD1(width, ratio) \
	(((unsigned int)(width) << 12) / (ratio))
#define VSP_UDS_SCALE_AMD0(width, ratio) \
	(VSP_UDS_SCALE_AMD1((width) - 1, ratio) + 1)

#define VSP_ROUND_UP(base, div) \
	(((base) + ((div) - 1)) / (div))
#define VSP_ROUND_DOWN(base, div) \
	((base) / (div))

#define VSP_CLIP0(base, sub) \
	(((base) > (sub)) ? ((base) - (sub)) : 0)

/* define IP number */
#define VSP_IP_MAX				VSPM_VSP_IP_MAX

/* define display list */
#define VSP_DL_HEAD_SIZE		128
#define VSP_DL_BODY_SIZE		1408
#define VSP_DL_PART_SIZE		384

/* define partition process */
#define VSP_PART_SIZE			256
#define VSP_PART_MARGIN			2

/* define status read counter */
#define VSP_STATUS_LOOP_TIME	(2)
#define VSP_STATUS_LOOP_CNT		(500)

/* define module maximum */
#define VSP_RPF_MAX				(5)
#define VSP_WPF_MAX				(1)
#define VSP_BRU_IN_MAX			(5)
#define VSP_BRS_IN_MAX			(2)

/* define status */
#define VSP_STAT_NOT_INIT		0
#define VSP_STAT_INIT			1
#define VSP_STAT_READY			2
#define VSP_STAT_RUN			3

/* define */
#define VSP_FALSE				0
#define VSP_TRUE				1

/* define color space */
#define VSP_COLOR_NO			(0)
#define VSP_COLOR_RGB			(1)
#define VSP_COLOR_YUV			(2)
#define VSP_COLOR_HSV			(3)

/* define usable pass route */
#define VSP_RPF_USABLE_DPR \
	(VSP_SRU_USE |	\
	 VSP_UDS_USE |	\
	 VSP_LUT_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HST_USE |	\
	 VSP_BRU_USE |	\
	 VSP_BRS_USE |	\
	 VSP_SHP_USE)

#define VSP_SRU_USABLE_DPR \
	(VSP_UDS_USE |	\
	 VSP_LUT_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HST_USE |	\
	 VSP_SHP_USE)

#define VSP_UDS_USABLE_DPR \
	(VSP_SRU_USE |	\
	 VSP_LUT_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HST_USE |	\
	 VSP_SHP_USE)

#define VSP_LUT_USABLE_DPR \
	(VSP_SRU_USE |	\
	 VSP_UDS_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HST_USE |	\
	 VSP_HSI_USE |	\
	 VSP_BRU_USE |	\
	 VSP_SHP_USE)

#define VSP_CLU_USABLE_DPR \
	(VSP_SRU_USE |	\
	 VSP_UDS_USE |	\
	 VSP_LUT_USE |	\
	 VSP_HST_USE |	\
	 VSP_HSI_USE |	\
	 VSP_BRU_USE |	\
	 VSP_SHP_USE)

#define VSP_HST_USABLE_DPR \
	(VSP_LUT_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HSI_USE)

#define VSP_HSI_USABLE_DPR \
	(VSP_SRU_USE |	\
	 VSP_UDS_USE |	\
	 VSP_LUT_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HST_USE |	\
	 VSP_SHP_USE)

#define VSP_SHP_USABLE_DPR \
	(VSP_SRU_USE |	\
	 VSP_UDS_USE |	\
	 VSP_LUT_USE |	\
	 VSP_CLU_USE |	\
	 VSP_HST_USE)

#define VSP_BRU_USABLE_DPR \
	(VSP_LUT_USE |	\
	 VSP_CLU_USE)

#define VSP_BRS_USABLE_DPR	0

/* define register offset */
#define VSP_WPF0_CMD			(0x0000)

#define VSP_CLK_DCSWT			(0x0018)

#define VSP_SRESET				(0x0028)
#define VSP_STATUS				(0x0038)

#define VSP_WPF0_IRQ_ENB		(0x0048)
#define VSP_WPF0_IRQ_STA		(0x004C)

/* Display list control register offset */
#define VSP_DL_CTRL				(0x0100)
#define VSP_DL_HDR_ADDR0		(0x0104)
#define VSP_DL_HDR_ADDR1		(0x0108)
#define VSP_DL_HDR_ADDR2		(0x010C)
#define VSP_DL_HDR_ADDR3		(0x0110)
#define VSP_DL_SWAP				(0x0114)
#define VSP_DL_EXT_CTRL			(0x011C)
#define VSP_DL_BODY_SIZE0		(0x0120)

/* RPFn control registers offset */
#define VSP_RPF0_OFFSET			(0x0300)
#define VSP_RPF1_OFFSET			(0x0400)
#define VSP_RPF2_OFFSET			(0x0500)
#define VSP_RPF3_OFFSET			(0x0600)
#define VSP_RPF4_OFFSET			(0x0700)

#define VSP_RPF_SRC_BSIZE		(0x0000)
#define VSP_RPF_SRC_ESIZE		(0x0004)
#define VSP_RPF_INFMT			(0x0008)
#define VSP_RPF_DSWAP			(0x000C)
#define VSP_RPF_LOC				(0x0010)
#define VSP_RPF_ALPH_SEL		(0x0014)
#define VSP_RPF_VRTCOL_SET		(0x0018)
#define VSP_RPF_MSKCTRL			(0x001C)
#define VSP_RPF_MSKSET0			(0x0020)
#define VSP_RPF_MSKSET1			(0x0024)
#define VSP_RPF_CKEY_CTRL		(0x0028)
#define VSP_RPF_CKEY_SET0		(0x002C)
#define VSP_RPF_CKEY_SET1		(0x0030)
#define VSP_RPF_SRCM_PSTRIDE	(0x0034)
#define VSP_RPF_SRCM_ASTRIDE	(0x0038)
#define VSP_RPF_SRCM_ADDR_Y		(0x003C)
#define VSP_RPF_SRCM_ADDR_C0	(0x0040)
#define VSP_RPF_SRCM_ADDR_C1	(0x0044)
#define VSP_RPF_SRCM_ADDR_AI	(0x0048)
#define VSP_RPF_BAC				(0x0050)
#define VSP_RPF_MULT_ALPHA		(0x006C)

#define VSP_RPF0_CLUT_OFFSET	(0x4000)
#define VSP_RPF1_CLUT_OFFSET	(0x4400)
#define VSP_RPF2_CLUT_OFFSET	(0x4800)
#define VSP_RPF3_CLUT_OFFSET	(0x4C00)
#define VSP_RPF4_CLUT_OFFSET	(0x5000)

/* WPF control registers offset */
#define VSP_WPF0_OFFSET			(0x1000)

#define VSP_WPF_SRCRPFL			(0x0000)
#define VSP_WPF_HSZCLIP			(0x0004)
#define VSP_WPF_VSZCLIP			(0x0008)
#define VSP_WPF_OUTFMT			(0x000C)
#define VSP_WPF_DSWAP			(0x0010)
#define VSP_WPF_RNDCTRL			(0x0014)
#define VSP_WPF_ROT_CTRL		(0x0018)
#define VSP_WPF_DSTM_STRIDE_Y	(0x001C)
#define VSP_WPF_DSTM_STRIDE_C	(0x0020)
#define VSP_WPF_DSTM_ADDR_Y		(0x0024)
#define VSP_WPF_DSTM_ADDR_C0	(0x0028)
#define VSP_WPF_DSTM_ADDR_C1	(0x002C)

/* DPR control registers offset */
#define VSP_DPR_RPF0_ROUTE		(0x2000)
#define VSP_DPR_RPF1_ROUTE		(0x2004)
#define VSP_DPR_RPF2_ROUTE		(0x2008)
#define VSP_DPR_RPF3_ROUTE		(0x200C)
#define VSP_DPR_RPF4_ROUTE		(0x2010)
#define VSP_DPR_WPF0_FPORCH		(0x2014)
#define VSP_DPR_SRU_ROUTE		(0x2024)
#define VSP_DPR_UDS_ROUTE		(0x2028)
#define VSP_DPR_LUT_ROUTE		(0x203C)
#define VSP_DPR_CLU_ROUTE		(0x2040)
#define VSP_DPR_HST_ROUTE		(0x2044)
#define VSP_DPR_HSI_ROUTE		(0x2048)
#define VSP_DPR_BRU_ROUTE		(0x204C)
#define VSP_DPR_BRS_ROUTE		(0x2050)
#define VSP_DPR_HGO_SMPPT		(0x2054)
#define VSP_DPR_HGT_SMPPT		(0x2058)
#define VSP_DPR_SHP_ROUTE		(0x2060)
#define VSP_DPR_UIF0_ROUTE		(0x2064)

/* SRU control registers offset */
#define VSP_SRU_CTRL0			(0x2200)
#define VSP_SRU_CTRL1			(0x2204)
#define VSP_SRU_CTRL2			(0x2208)

/* UDS control registers offset */
#define VSP_UDS_CTRL			(0x2300)
#define VSP_UDS_SCALE			(0x2304)
#define VSP_UDS_ALPTH			(0x2308)
#define VSP_UDS_ALPVAL			(0x230C)
#define VSP_UDS_PASS_BWIDTH		(0x2310)
#define VSP_UDS_HPHASE			(0x2314)
#define VSP_UDS_IPC				(0x2318)
#define VSP_UDS_HSZCLIP			(0x231C)
#define VSP_UDS_CLIP_SIZE		(0x2324)
#define VSP_UDS_FILL_COLOR		(0x2328)

/* LUT control registers offset */
#define VSP_LUT_CTRL			(0x2800)
#define VSP_LUT_TBL_OFFSET		(0x7000)

/* CLU control registers offset */
#define VSP_CLU_CTRL			(0x2900)
#define VSP_CLU_TBL_ADDR		(0x7400)
#define VSP_CLU_TBL_DATA		(0x7404)

/* HST control registers offset */
#define VSP_HST_CTRL			(0x2A00)

/* HSI control registers offset */
#define VSP_HSI_CTRL			(0x2B00)

/* BRU control registers offset */
#define VSP_BRU_INCTRL			(0x2C00)
#define VSP_BRU_VIRRPF_SIZE		(0x2C04)
#define VSP_BRU_VIRRPF_LOC		(0x2C08)
#define VSP_BRU_VIRRPF_COL		(0x2C0C)
#define VSP_BRUA_CTRL			(0x2C10)
#define VSP_BRUA_BLD			(0x2C14)
#define VSP_BRUB_CTRL			(0x2C18)
#define VSP_BRUB_BLD			(0x2C1C)
#define VSP_BRUC_CTRL			(0x2C20)
#define VSP_BRUC_BLD			(0x2C24)
#define VSP_BRUD_CTRL			(0x2C28)
#define VSP_BRUD_BLD			(0x2C2C)
#define VSP_BRU_ROP				(0x2C30)
#define VSP_BRUE_CTRL			(0x2C34)
#define VSP_BRUE_BLD			(0x2C38)

/* HGO control registers offset */
#define VSP_HGO_OFFSET			(0x3000)
#define VSP_HGO_SIZE			(0x3004)
#define VSP_HGO_MODE			(0x3008)
#define VSP_HGO_R_HISTO_OFFSET	(0x3030)
#define VSP_HGO_G_HISTO_OFFSET	(0x3140)
#define VSP_HGO_B_HISTO_OFFSET	(0x3250)
#define VSP_HGO_EXT_HIST_ADDR	(0x335C)
#define VSP_HGO_EXT_HIST_DATA	(0x3360)
#define VSP_HGO_WBUFS			(0x3364)
#define VSP_HGO_RBUFS			(0x3368)
#define VSP_HGO_HISTADD			(0x336C)
#define VSP_HGO_HSWAP			(0x3370)
#define VSP_HGO_REGRST			(0x33FC)

/* HGT control registers offset */
#define VSP_HGT_OFFSET			(0x3400)
#define VSP_HGT_SIZE			(0x3404)
#define VSP_HGT_MODE			(0x3408)
#define VSP_HGT_HUE_AREA0		(0x340C)
#define VSP_HGT_HUE_AREA1		(0x3410)
#define VSP_HGT_HUE_AREA2		(0x3414)
#define VSP_HGT_HUE_AREA3		(0x3418)
#define VSP_HGT_HUE_AREA4		(0x341C)
#define VSP_HGT_HUE_AREA5		(0x3420)
#define VSP_HGT_HIST_OFFSET		(0x3450)
#define VSP_HGT_WBUFS			(0x3764)
#define VSP_HGT_RBUFS			(0x3768)
#define VSP_HGT_HISTADD			(0x376C)
#define VSP_HGT_HSWAP			(0x3770)
#define VSP_HGT_REGRST			(0x37FC)

/* BRS control registers offset */
#define VSP_BRS_INCTRL			(0x3900)
#define VSP_BRS_VIRRPF_SIZE		(0x3904)
#define VSP_BRS_VIRRPF_LOC		(0x3908)
#define VSP_BRS_VIRRPF_COL		(0x390C)
#define VSP_BRSA_CTRL			(0x3910)
#define VSP_BRSA_BLD			(0x3914)
#define VSP_BRSB_CTRL			(0x3918)
#define VSP_BRSB_BLD			(0x391C)

/* SHP control registers offset */
#define VSP_SHP_CTRL0			(0x3E00)
#define VSP_SHP_CTRL1			(0x3E04)
#define VSP_SHP_CTRL2			(0x3E08)

/* FCP register */
#define VSP_FCP_CFG0			(0x0004)
#define VSP_FCP_RST				(0x0010)
#define VSP_FCP_STA				(0x0018)

/* define register set value */
#define VSP_CMD_STRCMD			(0x00000001)
#define VSP_IRQ_FRMEND			(0x00000002)
#define VSP_SRESET_WPF0			(0x00000001)
#define VSP_STATUS_WPF0			(0x00000100)

#define VSP_DL_CTRL_WAIT		(0x01000000)
#define VSP_DL_CTRL_RLM0		(0x00000008)
#define VSP_DL_CTRL_DLE			(0x00000001)

#define VSP_DL_SWAP_LWS			(0x00000004)
#define VSP_DL_SWAP_WDS			(0x00000002)
#define VSP_DL_SWAP_BTS			(0x00000001)

#define VSP_RPF_INFMT_VIR		(0x10000000)
#define VSP_RPF_INFMT_SPYCS		(0x00008000)
#define VSP_RPF_INFMT_SPUVS		(0x00004000)
#define VSP_RPF_INFMT_RDTM1		(0x00000400)
#define VSP_RPF_INFMT_RDTM0		(0x00000200)
#define VSP_RPF_INFMT_CSC		(0x00000100)
#define VSP_RPF_INFMT_RDFMT		(0x0000007F)

#define VSP_RPF_INFMT_MSK \
	(VSP_RPF_INFMT_SPYCS |\
	 VSP_RPF_INFMT_SPUVS |\
	 VSP_RPF_INFMT_RDFMT)

#define VSP_RPF_MSKCTRL_EN		(0x01000000)

#define VSP_RPF_CKEY_CTRL_LTH	(0x00000100)
#define VSP_RPF_CKEY_CTRL_CV	(0x00000010)
#define VSP_RPF_CKEY_CTRL_SAPE1	(0x00000002)
#define VSP_RPF_CKEY_CTRL_SAPE0	(0x00000001)

#define VSP_RPF_BAC_B256P		(0x00000000)
#define VSP_RPF_BAC_B512P		(0x00010000)

#define VSP_WPF_HSZCLIP_HCEN	(0x10000000)
#define VSP_WPF_VSZCLIP_VCEN	(0x10000000)

#define VSP_WPF_OUTFMT_PXA		(0x00800000)
#define VSP_WPF_OUTFMT_ODE		(0x00400000)
#define VSP_WPF_OUTFMT_FCNL		(0x00100000)
#define VSP_WPF_OUTFMT_ROT		(0x00040000)
#define VSP_WPF_OUTFMT_SPYCS	(0x00008000)
#define VSP_WPF_OUTFMT_SPUVS	(0x00004000)
#define VSP_WPF_OUTFMT_DITH		(0x00003000)
#define VSP_WPF_OUTFMT_WRTM1	(0x00000400)
#define VSP_WPF_OUTFMT_WRTM0	(0x00000200)
#define VSP_WPF_OUTFMT_CSC		(0x00000100)
#define VSP_WPF_OUTFMT_WRFMT	(0x0000007F)

#define VSP_WPF_OUTFMT_MSK \
	(VSP_WPF_OUTFMT_SPYCS | \
	 VSP_WPF_OUTFMT_SPUVS | \
	 VSP_WPF_OUTFMT_WRFMT)

#define VSP_WPF_ROTCTRL_LN16	(0x00020000)
#define VSP_WPF_ROTCTRL_LMEM	(0x00000100)

#define VSP_DPR_ROUTE_UIF0		(0x01000008)
#define VSP_DPR_ROUTE_SRU		(0x00000010)
#define VSP_DPR_ROUTE_UDS		(0x00000011)
#define VSP_DPR_ROUTE_LUT		(0x00000016)
#define VSP_DPR_ROUTE_CLU		(0x0000001D)
#define VSP_DPR_ROUTE_HST		(0x0000001E)
#define VSP_DPR_ROUTE_HSI		(0x0000001F)
#define VSP_DPR_ROUTE_SHP		(0x0000002E)
#define VSP_DPR_ROUTE_BRU0		(0x00000017)
#define VSP_DPR_ROUTE_BRU1		(0x00000018)
#define VSP_DPR_ROUTE_BRU2		(0x00000019)
#define VSP_DPR_ROUTE_BRU3		(0x0000001A)
#define VSP_DPR_ROUTE_BRU4		(0x00000031)
#define VSP_DPR_ROUTE_BRS0		(0x00000026)
#define VSP_DPR_ROUTE_BRS1		(0x00000027)
#define VSP_DPR_ROUTE_WPF0		(0x00000038)
#define VSP_DPR_ROUTE_NOT_USE	(0x0000003F)
#define VSP_DPR_ROUTE_BRSSEL		(0x10000000)

#define VSP_DPR_SMPPT_RPF0		(0x00000000)
#define VSP_DPR_SMPPT_RPF1		(0x00000001)
#define VSP_DPR_SMPPT_RPF2		(0x00000002)
#define VSP_DPR_SMPPT_RPF3		(0x00000003)
#define VSP_DPR_SMPPT_RPF4		(0x00000004)
#define VSP_DPR_SMPPT_UIF0		(0x00000008)
#define VSP_DPR_SMPPT_SRU		(0x00000010)
#define VSP_DPR_SMPPT_UDS		(0x00000011)
#define VSP_DPR_SMPPT_LUT		(0x00000016)
#define VSP_DPR_SMPPT_BRU		(0x0000001B)
#define VSP_DPR_SMPPT_CLU		(0x0000001D)
#define VSP_DPR_SMPPT_HST		(0x0000001E)
#define VSP_DPR_SMPPT_HSI		(0x0000001F)
#define VSP_DPR_SMPPT_SHP		(0x0000002E)
#define VSP_DPR_SMPPT_NOT_USE	(0x0000073F)

#define VSP_SRU_CTRL_EN			(0x00000001)

#define VSP_UDS_CTRL_AMD		(0x40000000)
#define VSP_UDS_CTRL_FMD		(0x20000000)
#define VSP_UDS_CTRL_BLADV		(0x10000000)
#define VSP_UDS_CTRL_AON		(0x02000000)
#define VSP_UDS_CTRL_ATHON		(0x01000000)
#define VSP_UDS_CTRL_BC			(0x00100000)	/* multi tap */
#define VSP_UDS_CTRL_NN			(0x000F0000)	/* nearest neighbor */
#define VSP_UDS_CTRL_AMDSLH		(0x00000004)

#define VSP_UDS_SCALE_1_8		(0x8000)
#define VSP_UDS_SCALE_1_4		(0x4000)	/* quarter */
#define VSP_UDS_SCALE_1_2		(0x2000)	/* half */
#define VSP_UDS_SCALE_1_1		(0x1000)
#define VSP_UDS_SCALE_2_1		(0x0800)	/* double */
#define VSP_UDS_SCALE_4_1		(0x0400)	/* quadruple */
#define VSP_UDS_SCALE_8_1		(0x0200)
#define VSP_UDS_SCALE_16_1		(0x0100)	/* maximum scale */
#define VSP_UDS_SCALE_MANT		(0xF000)
#define VSP_UDS_SCALE_FRAC		(0x0FFF)

#define VSP_UDS_HSZCLIP_HCEN	(0x10000000)

#define VSP_LUT_CTRL_EN			(0x00000001)

#define VSP_CLU_CTRL_3D			(0x00000000)
#define VSP_CLU_CTRL_2D			(0x0000D372)
#define VSP_CLU_CTRL_AAI		(0x10000000)
#define VSP_CLU_CTRL_MVS		(0x01000000)
#define VSP_CLU_CTRL_EN			(0x00000001)

#define VSP_HST_CTRL_EN			(0x00000001)

#define VSP_HSI_CTRL_EN			(0x00000001)

#define VSP_HGO_MODE_AUTOW		(0x00001000)
#define VSP_HGO_MODE_STEP		(0x00000400)
#define VSP_HGO_REGRST_RCPART	(0x00000010)
#define VSP_HGO_REGRST_RCLEA	(0x00000001)

#define VSP_HGT_MODE_AUTOW		(0x00001000)
#define VSP_HGT_REGRST_RCPART	(0x00000010)
#define VSP_HGT_REGRST_RCLEA	(0x00000001)

#define VSP_SHP_SHP_EN			(0x00000001)

#define VSP_FCP_RST_SOFTRST		(0x00000001)
#define VSP_FCP_STA_ACT			(0x00000001)

enum {
	VSP_REG_CTRL = 0,
	VSP_REG_CLUT,
	VSP_REG_ROUTE,
	VSP_REG_MAX
};

enum {
	VSP_BROP_DST_A = 0,
	VSP_BROP_SRC_A,
	VSP_BROP_DST_R,
	VSP_BROP_SRC_C,
	VSP_BROP_SRC_D,
	VSP_BROP_SRC_E,
	VSP_BROP_MAX
};

enum {
	VSP_BRS_DST_A = 0,
	VSP_BRS_SRC_A,
	VSP_BRS_SRC_B,
	VSP_BRS_MAX
};

/* Display list information structure */
struct vsp_dl_body_info {
	unsigned int size;
	unsigned int addr;
};

struct vsp_dl_head_info {
	unsigned int body_num_minus1;
	struct vsp_dl_body_info body_info[8];
	unsigned int next_head_addr;
	unsigned int next_frame_ctrl;
};

/* partition information */
struct vsp_part_info {
	unsigned short div_flag;	/* partition flag */
	unsigned short div_size;	/* division size */
	unsigned short margin;		/* margin size */
	unsigned short sru_first_flag;

	unsigned int rpf_addr_y;
	unsigned int rpf_addr_c0;
	unsigned int rpf_addr_c1;
	unsigned int rpf_addr_ai;

	unsigned int hgo_smppt;
	unsigned int hgt_smppt;
};

/* RPF information structure */
struct vsp_rpf_info {
	unsigned int val_bsize;
	unsigned int val_esize;
	unsigned int val_infmt;
	unsigned int val_dswap;
	unsigned int val_loc;
	unsigned int val_alpha_sel;
	unsigned int val_vrtcol;
	unsigned int val_mskctrl;
	unsigned int val_mskset[2];
	unsigned int val_ckey_ctrl;
	unsigned int val_ckey_set[2];
	unsigned int val_astride;
	unsigned int val_addr_y;
	unsigned int val_addr_c0;
	unsigned int val_addr_c1;
	unsigned int val_addr_ai;
	unsigned int val_mult_alph;

	unsigned int val_dpr;
};

/* SRU information structure */
struct vsp_sru_info {
	unsigned int val_dpr;
};

/* UDS information structure */
struct vsp_uds_info {
	unsigned int val_ctrl;
	unsigned int val_pass;
	unsigned int val_hphase;
	unsigned int val_hszclip;
	unsigned int val_clip;

	unsigned int val_dpr;
};

/* LUT information structure */
struct vsp_lut_info {
	unsigned int val_dpr;
};

/* CLU information structure */
struct vsp_clu_info {
	unsigned int val_ctrl;

	unsigned int val_dpr;
};

/* HST information structure */
struct vsp_hst_info {
	unsigned int val_dpr;
};

/* HSI information structure */
struct vsp_hsi_info {
	unsigned int val_dpr;
};

/* BRU information structure */
struct vsp_bru_info {
	unsigned int val_inctrl;

	unsigned int val_vir_loc;
	unsigned int val_vir_color;
	unsigned int val_vir_size;

	unsigned int val_ctrl[5];
	unsigned int val_bld[5];
	unsigned int val_rop;

	unsigned int val_dpr;
};

/* BRS information structure */
struct vsp_brs_info {
	unsigned int val_inctrl;

	unsigned int val_vir_loc;
	unsigned int val_vir_color;
	unsigned int val_vir_size;

	unsigned int val_ctrl[2];
	unsigned int val_bld[2];

	unsigned int val_dpr;
};

/* HGO information structure */
struct vsp_hgo_info {
	unsigned int *virt_addr;
	unsigned int val_hard_addr;
	unsigned int val_offset;
	unsigned int val_size;
	unsigned int val_mode;
	unsigned int val_regrst;
	unsigned int val_dpr;
};

/* HGT information structure */
struct vsp_hgt_info {
	unsigned int *virt_addr;
	unsigned int val_hard_addr;
	unsigned int val_offset;
	unsigned int val_size;
	unsigned int val_regrst;
	unsigned int val_dpr;
};

/* SHP information structure */
struct vsp_shp_info {
	unsigned int val_dpr;
};

/* WPF information structure */
struct vsp_wpf_info {
	unsigned int val_srcrpf;
	unsigned int val_hszclip;
	unsigned int val_vszclip;
	unsigned int val_outfmt;
	unsigned int val_addr_y;
	unsigned int val_addr_c0;
	unsigned int val_addr_c1;
	unsigned int val_dl_addr;
};

struct vsp_src_info {
	unsigned char rpf_ch;
	unsigned char color;
	unsigned int width;
	unsigned int height;
	unsigned int x_position;
	unsigned int y_position;
	unsigned int master;
};

/* channel information structure */
struct vsp_ch_info {
	unsigned char status;

	void (*cb_func)
		(unsigned long id, long ercd, void *userdata);
	void *cb_userdata;

	unsigned long reserved_rpf;
	unsigned long reserved_module;

	unsigned long next_module;

	struct vsp_part_info part_info;

	struct vsp_src_info src_info[VSP_RPF_MAX + 1];
	unsigned char src_idx;
	unsigned char src_cnt;

	unsigned char wpf_cnt;
	unsigned char bru_cnt;
	unsigned char brs_cnt;

	unsigned int next_dl_addr;

	struct vsp_rpf_info rpf_info[VSP_RPF_MAX];
	struct vsp_sru_info sru_info;
	struct vsp_uds_info uds_info;
	struct vsp_lut_info lut_info;
	struct vsp_clu_info clu_info;
	struct vsp_hst_info hst_info;
	struct vsp_hsi_info hsi_info;
	struct vsp_bru_info bru_info;
	struct vsp_hgo_info hgo_info;
	struct vsp_hgt_info hgt_info;
	struct vsp_brs_info brs_info;
	struct vsp_shp_info shp_info;
	struct vsp_wpf_info wpf_info;
};

/* private data structure */
struct vsp_prv_data {
	struct platform_device *pdev;
	void __iomem *vsp_reg;
	void __iomem *fcp_reg;
	struct resource *irq;
	struct vsp_res_data {
		unsigned int usable_rpf;
		unsigned int usable_rpf_clut;
		unsigned int usable_wpf;
		unsigned int usable_wpf_rot;
		unsigned int usable_module;
		unsigned int read_outstanding;
		unsigned int start_reservation;
		unsigned int burst_access;
		bool burst_enable;
	} rdata;

	struct vsp_ch_info ch_info[2];
	unsigned char widx;
	unsigned char ridx;
};

/* define local functions */
long vsp_ins_check_init_parameter(struct vsp_init_t *param);
long vsp_ins_check_open_parameter(struct vsp_open_t *param);
long vsp_ins_check_start_parameter(
	struct vsp_prv_data *prv, struct vsp_start_t *param);

long vsp_ins_set_start_parameter(
	struct vsp_prv_data *prv, struct vsp_start_t *param);
void vsp_ins_start_processing(struct vsp_prv_data *prv);
long vsp_ins_stop_processing(struct vsp_prv_data *prv);
long vsp_ins_wait_processing(struct vsp_prv_data *prv);

long vsp_ins_get_vsp_resource(struct vsp_prv_data *prv);

long vsp_ins_enable_clock(struct vsp_prv_data *prv);
long vsp_ins_disable_clock(struct vsp_prv_data *prv);

long vsp_ins_init_reg(struct vsp_prv_data *prv);
long vsp_ins_quit_reg(struct vsp_prv_data *prv);

void vsp_ins_cb_function(struct vsp_prv_data *prv, long ercd);

long vsp_ins_reg_ih(struct vsp_prv_data *prv);
long vsp_ins_unreg_ih(struct vsp_prv_data *prv);

long vsp_ins_get_vsp_ip_num(
	unsigned char *vsp, unsigned char *wpf, unsigned char ch);

unsigned int vsp_ins_get_bpp_luma(
	unsigned short format, unsigned short offset);
unsigned int vsp_ins_get_bpp_chroma(
	unsigned short format, unsigned short offset);
unsigned int vsp_ins_get_line_luma(
	unsigned short format, unsigned short offset);
unsigned int vsp_ins_get_line_chroma(
	unsigned short format, unsigned short offset);

#endif
