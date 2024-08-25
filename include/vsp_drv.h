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

#ifndef _VSP_DRV_H_
#define _VSP_DRV_H_

/* error code */
#define E_VSP_PARA_CB				(-201)
#define E_VSP_PARA_INPAR			(-202)

#define E_VSP_PARA_RPFNUM		(-210)
#define E_VSP_PARA_RPFORDER		(-211)
#define E_VSP_PARA_USEMODULE	(-212)
#define E_VSP_PARA_OUTPAR		(-213)
#define E_VSP_PARA_CTRLPAR		(-214)
#define E_VSP_PARA_NOIN			(-215)
#define E_VSP_PARA_CONNECT		(-216)	/* illegal connection */
#define E_VSP_PARA_NOPARENT		(-217)	/* not found parent layer */
#define E_VSP_PARA_NOINPUT		(-218)	/* no input */
#define E_VSP_PARA_NOMODULE		(-219)	/* no module */

#define E_VSP_PARA_IN_ADR			(-220)
#define E_VSP_PARA_IN_ADRC0			(-221)
#define E_VSP_PARA_IN_ADRC1			(-222)
#define E_VSP_PARA_IN_WIDTH			(-223)
#define E_VSP_PARA_IN_HEIGHT		(-224)
#define E_VSP_PARA_IN_WIDTHEX		(-225)
#define E_VSP_PARA_IN_HEIGHTEX		(-226)
#define E_VSP_PARA_IN_XOFFSET		(-227)
#define E_VSP_PARA_IN_YOFFSET		(-228)
#define E_VSP_PARA_IN_FORMAT		(-229)
#define E_VSP_PARA_IN_SWAP			(-230)	/* not used */
#define E_VSP_PARA_IN_XPOSI			(-231)
#define E_VSP_PARA_IN_YPOSI			(-232)
#define E_VSP_PARA_IN_CIPM			(-233)
#define E_VSP_PARA_IN_CEXT			(-234)
#define E_VSP_PARA_IN_CSC			(-235)
#define E_VSP_PARA_IN_ITURBT		(-236)
#define E_VSP_PARA_IN_CLRCNG		(-237)
#define E_VSP_PARA_IN_VIR			(-238)
#define E_VSP_PARA_IN_ALPHA			(-239)
#define E_VSP_PARA_IN_CONNECT		(-240)
#define E_VSP_PARA_IN_PWD			(-241)

#define E_VSP_PARA_OSD_CLUT			(-250)
#define E_VSP_PARA_OSD_SIZE			(-251)

#define E_VSP_PARA_ALPHA_ADR		(-260)	/* illegal address pointer */
#define E_VSP_PARA_ALPHA_CKEY		(-261)
#define E_VSP_PARA_ALPHA_ASWAP		(-262)	/* not used */
#define E_VSP_PARA_ALPHA_ASEL		(-263)
#define E_VSP_PARA_ALPHA_AEXT		(-264)
#define E_VSP_PARA_ALPHA_IROP		(-265)
#define E_VSP_PARA_ALPHA_MSKEN		(-266)
#define E_VSP_PARA_ALPHA_BSEL		(-267)
#define E_VSP_PARA_ALPHA_MULT		(-268)

#define E_VSP_PARA_OUT_ADR			(-270)
#define E_VSP_PARA_OUT_ADRC0		(-271)
#define E_VSP_PARA_OUT_ADRC1		(-272)
#define E_VSP_PARA_OUT_WIDTH		(-273)
#define E_VSP_PARA_OUT_HEIGHT		(-274)
#define E_VSP_PARA_OUT_XOFFSET		(-275)
#define E_VSP_PARA_OUT_YOFFSET		(-276)
#define E_VSP_PARA_OUT_XCLIP		(-277)
#define E_VSP_PARA_OUT_YCLIP		(-278)
#define E_VSP_PARA_OUT_FORMAT		(-279)
#define E_VSP_PARA_OUT_SWAP			(-280)
#define E_VSP_PARA_OUT_PXA			(-281)
#define E_VSP_PARA_OUT_XCOFFSET		(-282)
#define E_VSP_PARA_OUT_YCOFFSET		(-283)
#define E_VSP_PARA_OUT_CSC			(-284)
#define E_VSP_PARA_OUT_ITURBT		(-285)
#define E_VSP_PARA_OUT_CLRCNG		(-286)
#define E_VSP_PARA_OUT_CBRM			(-287)
#define E_VSP_PARA_OUT_ABRM			(-288)
#define E_VSP_PARA_OUT_CLMD			(-289)
#define E_VSP_PARA_OUT_ROTATION		(-290)
#define E_VSP_PARA_OUT_DITH			(-291)
#define E_VSP_PARA_OUT_INHSV		(-292)	/* illegal input color space */
#define E_VSP_PARA_OUT_INWIDTH		(-293)	/* illegal input image width */
#define E_VSP_PARA_OUT_INHEIGHT		(-294)	/* illegal input image height */
#define E_VSP_PARA_OUT_NOTCOLOR		(-295)	/* not equal color space */
#define E_VSP_PARA_OUT_STRIDE_Y		(-296)
#define E_VSP_PARA_OUT_STRIDE_C		(-297)

#define E_VSP_PARA_BRU_LAYORDER		(-300)
#define E_VSP_PARA_BRU_ADIV			(-301)
#define E_VSP_PARA_BRU_DITH_MODE	(-302)	/* illegal dithering mode */
#define E_VSP_PARA_BRU_DITH_BPP		(-303)	/* illegal dithering format */
#define E_VSP_PARA_BRU_CONNECT		(-304)
#define E_VSP_PARA_BRU_INHSV		(-305)	/* illegal input color space */
#define E_VSP_PARA_BRU_INCOLOR		(-306)	/* different color space */

#define E_VSP_PARA_VIR_ADR		(-310)	/* illegal address pointer */
#define E_VSP_PARA_VIR_WIDTH	(-311)
#define E_VSP_PARA_VIR_HEIGHT	(-312)
#define E_VSP_PARA_VIR_XPOSI	(-313)
#define E_VSP_PARA_VIR_YPOSI	(-314)
#define E_VSP_PARA_VIR_PWD		(-315)	/* illegal pwd */

#define E_VSP_PARA_BLEND_RBC		(-320)
#define E_VSP_PARA_BLEND_CROP		(-321)
#define E_VSP_PARA_BLEND_AROP		(-322)
#define E_VSP_PARA_BLEND_FORM		(-323)
#define E_VSP_PARA_BLEND_COEFX		(-324)
#define E_VSP_PARA_BLEND_COEFY		(-325)
#define E_VSP_PARA_BLEND_AFORM		(-326)
#define E_VSP_PARA_BLEND_ACOEFX		(-327)
#define E_VSP_PARA_BLEND_ACOEFY		(-328)

#define E_VSP_PARA_ROP_CROP			(-330)
#define E_VSP_PARA_ROP_AROP			(-331)

#define E_VSP_PARA_SRU_MODE			(-340)
#define E_VSP_PARA_SRU_PARAM		(-341)
#define E_VSP_PARA_SRU_ENSCL		(-342)
#define E_VSP_PARA_SRU_CONNECT		(-343)
#define E_VSP_PARA_SRU_WIDTH		(-344)	/* illegal image width */
#define E_VSP_PARA_SRU_HEIGHT		(-345)	/* illegal image height */
#define E_VSP_PARA_SRU_INHSV		(-346)	/* illegal input color space */

#define E_VSP_PARA_UDS_AMD			(-350)
#define E_VSP_PARA_UDS_FMD			(-351)
#define E_VSP_PARA_UDS_CLIP			(-352)
#define E_VSP_PARA_UDS_ALPHA		(-353)
#define E_VSP_PARA_UDS_COMP			(-354)
#define E_VSP_PARA_UDS_CONNECT		(-355)
#define E_VSP_PARA_UDS_XRATIO		(-356)	/* illegal x_ratio */
#define E_VSP_PARA_UDS_YRATIO		(-357)	/* illegal y_ratio */
#define E_VSP_PARA_UDS_OUTCWIDTH	(-358)	/* illegal clipping width */
#define E_VSP_PARA_UDS_OUTCHEIGHT	(-359)	/* illegal clipping height */
#define E_VSP_PARA_UDS_INWIDTH		(-360)	/* illegal input width */
#define E_VSP_PARA_UDS_INHEIGHT		(-361)	/* illegal input height */

#define E_VSP_PARA_BRS_LAYORDER		(-370)
#define E_VSP_PARA_BRS_ADIV		(-371)
#define E_VSP_PARA_BRS_DITH_MODE	(-372)	/* illegal dithering mode */
#define E_VSP_PARA_BRS_DITH_BPP		(-373)	/* illegal dithering format */
#define E_VSP_PARA_BRS_CONNECT		(-374)
#define E_VSP_PARA_BRS_INHSV		(-375)	/* illegal input color space */
#define E_VSP_PARA_BRS_INCOLOR		(-376)	/* different color space */

#define E_VSP_PARA_LUT_ADR			(-600)	/* illegal DL address */
#define E_VSP_PARA_LUT_SIZE			(-601)	/* illegal DL size */
#define E_VSP_PARA_LUT_CONNECT		(-602)	/* illegal connecting module */

#define E_VSP_PARA_CLU_MODE			(-610)	/* illegal mode */
#define E_VSP_PARA_CLU_ADR			(-611)	/* illegal DL address */
#define E_VSP_PARA_CLU_SIZE			(-613)	/* illegal DL size */
#define E_VSP_PARA_CLU_CONNECT		(-614)	/* illegal connecting module */

#define E_VSP_PARA_HST_NOTRGB		(-630)	/* illegal input color space */
#define E_VSP_PARA_HST_CONNECT		(-631)	/* illegal connecting module */

#define E_VSP_PARA_HSI_NOTHSV		(-640)	/* illegal input color space */
#define E_VSP_PARA_HSI_CONNECT		(-641)	/* illegal connecting module */

#define E_VSP_PARA_HGO_ADR		(-660)	/* illegal address pointer */
#define E_VSP_PARA_HGO_WIDTH	(-661)	/* illegal widht */
#define E_VSP_PARA_HGO_HEIGHT	(-662)	/* illegal height */
#define E_VSP_PARA_HGO_XOFFSET	(-663)	/* illegal x_offset */
#define E_VSP_PARA_HGO_YOFFSET	(-664)	/* illegal y_offset */
#define E_VSP_PARA_HGO_BINMODE	(-665)	/* illegal binary mode */
#define E_VSP_PARA_HGO_MAXRGB	(-669)	/* illegal source component */
#define E_VSP_PARA_HGO_STEP		(-730)	/* illegal step mode */
#define E_VSP_PARA_HGO_XSKIP	(-666)	/* illegal skip mode */
#define E_VSP_PARA_HGO_YSKIP	(-667)	/* illegal skip mode */
#define E_VSP_PARA_HGO_SMMPT	(-668)	/* illegal sampling module */

#define E_VSP_PARA_HGT_ADR		(-670)	/* illegal address pointer */
#define E_VSP_PARA_HGT_WIDTH	(-671)	/* illegal width */
#define E_VSP_PARA_HGT_HEIGHT	(-672)	/* illegal height */
#define E_VSP_PARA_HGT_XOFFSET	(-673)	/* illegal x_offset */
#define E_VSP_PARA_HGT_YOFFSET	(-674)	/* illegal y_offset */
#define E_VSP_PARA_HGT_AREA		(-675)	/* illegal hue area pointer */
#define E_VSP_PARA_HGT_XSKIP	(-676)	/* illegal skip mode */
#define E_VSP_PARA_HGT_YSKIP	(-677)	/* illegal skip mode */
#define E_VSP_PARA_HGT_SMMPT	(-678)	/* illegal sampling module */

#define E_VSP_PARA_SHP_INYUV	(-690)	/* illegal input color space */
#define E_VSP_PARA_SHP_WIDTH	(-691)	/* illegal input width */
#define E_VSP_PARA_SHP_HEIGHT	(-692)	/* illegal input height */
#define E_VSP_PARA_SHP_MODE		(-693)	/* illegal mode */
#define E_VSP_PARA_SHP_CONNECT	(-694)	/* illegal connecting module */

#define E_VSP_PARA_NOSRU			(-650)
#define E_VSP_PARA_NOUDS			(-651)
#define E_VSP_PARA_NOLUT			(-652)
#define E_VSP_PARA_NOCLU			(-653)
#define E_VSP_PARA_NOHST			(-654)
#define E_VSP_PARA_NOHSI			(-655)
#define E_VSP_PARA_NOBRU			(-656)
#define E_VSP_PARA_NOHGO			(-657)
#define E_VSP_PARA_NOHGT			(-658)
#define E_VSP_PARA_NOSHP			(-659)

#define E_VSP_PARA_DL_ADR			(-680)
#define E_VSP_PARA_DL_SIZE			(-681)

#define E_VSP_BUSY_RPF_OVER			(-700)
#define E_VSP_BUSY_MODULE_OVER		(-701)

#define E_VSP_PARA_NOBRS			(-710)

#define E_VSP_PARA_FCNL				(-740)

/* struct vsp_start_t.use_module */
#define VSP_SRU_USE		(0x0001) /* super-resolution */
#define VSP_UDS_USE		(0x0002) /* up down scaler */
#define VSP_LUT_USE		(0x0010) /* look up table */
#define VSP_CLU_USE		(0x0020) /* cubic look up table */
#define VSP_HST_USE		(0x0040) /* hue saturation value transform */
#define VSP_HSI_USE		(0x0080) /* hue saturation value inverse */
#define VSP_BRU_USE		(0x0100) /* blend rop */
#define VSP_HGO_USE		(0x0200) /* histogram generator-one */
#define VSP_HGT_USE		(0x0400) /* histogram generator-two */
#define VSP_SHP_USE		(0x0800) /* sharpness */
#define VSP_BRS_USE		(0x1000) /* blend rop sub */

/* RPF module parameter */
/* input format */
#define VSP_IN_RGB332				(0x0100) /* RGB332 */
#define VSP_IN_XRGB4444				(0x0201) /* XRGB4444 */
#define VSP_IN_RGBX4444				(0x0202) /* RGBX4444 */
#define VSP_IN_XRGB1555				(0x0204) /* XRGB1555 */
#define VSP_IN_RGBX5551				(0x0205) /* RGBX5551 */
#define VSP_IN_RGB565				(0x0206) /* RGB565 */
#define VSP_IN_AXRGB86666			(0x0407) /* AXRGB86666 */
#define VSP_IN_RGBXA66668			(0x0408) /* RGBXA66668 */
#define VSP_IN_XRGBA66668			(0x0409) /* XRGBA66668 */
#define VSP_IN_ARGBX86666			(0x040A) /* ARGBX86666 */
#define VSP_IN_AXXXRGB82666			(0x040B) /* AXXXRGB82666 */
#define VSP_IN_XXXRGBA26668			(0x040C) /* XXXRGBA26668 */
#define VSP_IN_ARGBXXX86662			(0x040D) /* ARGBXXX86662 */
#define VSP_IN_RGBXXXA66628			(0x040E) /* RGBXXXA66628 */
#define VSP_IN_XRGB6666				(0x030F) /* XRGB6666 */
#define VSP_IN_RGBX6666				(0x0310) /* RGBX6666 */
#define VSP_IN_XXXRGB2666			(0x0311) /* XXXRGB2666 */
#define VSP_IN_RGBXXX6662			(0x0312) /* RGBXXX6662 */
#define VSP_IN_ARGB8888				(0x0413) /* ARGB8888 */
#define VSP_IN_RGBA8888				(0x0414) /* RGBA8888 */
#define VSP_IN_RGB888				(0x0315) /* RGB888 */
#define VSP_IN_XXRGB7666			(0x0416) /* XXRGB7666 */
#define VSP_IN_XRGB14666			(0x0417) /* XRGB14666 */
#define VSP_IN_BGR888				(0x0318) /* BGR888 */
#define VSP_IN_ARGB4444				(0x0219) /* ARGB4444 */
#define VSP_IN_RGBA4444				(0x021A) /* RGBA4444 */
#define VSP_IN_ARGB1555				(0x021B) /* ARGB1555 */
#define VSP_IN_RGBA5551				(0x021C) /* RGBA5551 */
#define VSP_IN_ABGR4444				(0x021D) /* ABGR4444 */
#define VSP_IN_BGRA4444				(0x021E) /* BGRA4444 */
#define VSP_IN_ABGR1555				(0x021F) /* ABGR1555 */
#define VSP_IN_BGRA5551				(0x0220) /* BGRA5551 */
#define VSP_IN_XXXBGR2666			(0x0321) /* XXXBGR2666 */
#define VSP_IN_ABGR8888				(0x0422) /* ABGR8888 */
#define VSP_IN_XRGB16565			(0x0423) /* XRGB16565 */
#define VSP_IN_RGB_CLUT_DATA		(0x013F) /* RGB_CLUT_DATA */
#define VSP_IN_YUV444_SEMI_PLANAR	(0x0040) /* YCbCr4:4:4 Semi-Planar */
#define VSP_IN_YUV422_SEMI_PLANAR	(0x0041) /* YCbCr4:2:2 Semi-Planar */
#define VSP_IN_YUV420_SEMI_PLANAR	(0x0042) /* YCbCr4:2:0 Semi-Planar */
#define VSP_IN_YUV444_INTERLEAVED	(0x0046) /* YCbCr4:4:4 Interleaved */
#define VSP_IN_YUV422_INTERLEAVED0	(0x0047) /* YCbCr4:2:2 Inter type0 */
#define VSP_IN_YUV422_INTERLEAVED1	(0x0048) /* YCbCr4:2:2 Inter type1 */
#define VSP_IN_YUV420_INTERLEAVED	(0x0049) /* YCbCr4:2:0 Interleaved */
#define VSP_IN_YUV444_PLANAR		(0x004A) /* YCbCr4:4:4 Planar */
#define VSP_IN_YUV422_PLANAR		(0x004B) /* YCbCr4:2:2 Planar */
#define VSP_IN_YUV420_PLANAR		(0x004C) /* YCbCr4:2:0 Planar */
#define VSP_IN_YUV_CLUT_DATA		(0x017F) /* YCbCr_CLUT_DATA */

#define VSP_IN_YUV422_SEMI_NV16		VSP_IN_YUV422_SEMI_PLANAR
#define VSP_IN_YUV422_SEMI_NV61		(0x4041) /* YCbCr4:2:2 Semi(NV61) */
#define VSP_IN_YUV420_SEMI_NV12		VSP_IN_YUV420_SEMI_PLANAR
#define VSP_IN_YUV420_SEMI_NV21		(0x4042) /* YCbCr4:2:0 semi(NV21) */
#define VSP_IN_YUV422_INT0_UYVY		VSP_IN_YUV422_INTERLEAVED0
#define VSP_IN_YUV422_INT0_YUY2		(0x8047) /* YCbCr4:2:2 type0(YUY2) */
#define VSP_IN_YUV422_INT0_YVYU		(0xC047) /* YCbCr4:2:2 type0(YVYU) */

/* swap setting */
#define VSP_SWAP_NO			(0x00)	/* disable */
#define VSP_SWAP_B			(0x01)	/* byte units */
#define VSP_SWAP_W			(0x02)	/* word units */
#define VSP_SWAP_L			(0x04)	/* longword units */
#define VSP_SWAP_LL			(0x08)	/* LONG LWORD units */

/* parent setting */
#define VSP_LAYER_CHILD		(0x01)	/* child */
#define VSP_LAYER_PARENT	(0x02)	/* parent */

/* horizontal chrominance interpolation method setting */
#define VSP_CIPM_0_HOLD		(0x00)	/* nearest-neighbor method */
#define VSP_CIPM_BI_LINEAR	(0x01)	/* bilinear method */

/* lower-bit color data extension method setting */
#define VSP_CEXT_EXPAN		(0x00)	/* extended with 0 */
#define VSP_CEXT_COPY		(0x01)	/* copied to the lower-order bits */
#define VSP_CEXT_EXPAN_MAX	(0x02)	/* extended with 0 */
				/* maximum value is limitede to 0xFF */

/* color space conversion enable */
#define VSP_CSC_OFF			(0x00)	/* disable */
#define VSP_CSC_ON			(0x01)	/* enable */

/* select of color space conversion ITU-R BT */
#define VSP_ITURBT_601		(0x00)	/* ITU-R BT601 */
#define VSP_ITURBT_709		(0x01)	/* ITU-R BT709 */

/* select of color space conversion scale */
#define VSP_ITU_COLOR		(0x00)	/* YUV[16,235/140] <-> RGB[0,255] */
#define VSP_FULL_COLOR		(0x01)	/* Full scale */

/* virtual input enable */
#define VSP_NO_VIR			(0x00)	/* disable */
#define VSP_VIR				(0x01)	/* enable */

/* comparison color data setting */
#define VSP_ALPHA_NO		(0x00)	/* disable */
#define VSP_ALPHA_AL1		(0x01)	/* comparision alpha1 enable */
#define VSP_ALPHA_AL2		(0x02)	/* comparision alpha2 enable */

/* alpha format and processing method select */
#define VSP_ALPHA_NUM1	(0x00)	/* 1,4 or 8bit packed alpha + plane alpha */
#define VSP_ALPHA_NUM2	(0x01)	/* 8-bit plane alpha */
#define VSP_ALPHA_NUM3	(0x02)	/* 1-bit packed alpha + plane alpha */
#define VSP_ALPHA_NUM4	(0x03)	/* 1-bit plane alpha */
#define VSP_ALPHA_NUM5	(0x04)	/* Fixed alpha */

/* lower-bit alpha value extension method set */
#define VSP_AEXT_EXPAN		(0x00)	/* extended with 0 */
#define VSP_AEXT_COPY		(0x01)	/* copied to the lower-order bits */
#define VSP_AEXT_EXPAN_MAX	(0x02)	/* extended with 0. max limited 0xFF */

/* ROP oprator */
enum {
	VSP_IROP_NOP = 0,				/* NOP(D) */
	VSP_IROP_AND,					/* AND(S & D) */
	VSP_IROP_AND_REVERSE,			/* AND_REVERSE(S & ~D) */
	VSP_IROP_COPY,					/* COPY(S) */
	VSP_IROP_AND_INVERTED,			/* AND_INVERTED(~S & D) */
	VSP_IROP_CLEAR,					/* CLEAR(0) */
	VSP_IROP_XOR,					/* XOR(S ^ D) */
	VSP_IROP_OR,					/* OR(S | D) */
	VSP_IROP_NOR,					/* NOR(~(S | D)) */
	VSP_IROP_EQUIV,					/* EQUIV(~(S ^ D)) */
	VSP_IROP_INVERT,				/* INVERT(~D) */
	VSP_IROP_OR_REVERSE,			/* OR_REVERSE(S | ~D) */
	VSP_IROP_COPY_INVERTED,			/* COPY_INVERTED(~S) */
	VSP_IROP_OR_INVERTED,			/* OR_INVERTED(~S | D) */
	VSP_IROP_NAND,					/* NAND(~(S & D)) */
	VSP_IROP_SET,					/* SET(all 1) */
	VSP_IROP_MAX
};

/* mask generation specification */
#define VSP_MSKEN_ALPHA		(0x00)	/* use alpha plane */
#define VSP_MSKEN_COLOR		(0x01)	/* use comparison value */

/* alpha bit counte conversion selection */
#define VSP_ALPHA_8BIT		(0x00) /* 8bit alpha is converted to 1bit */
#define VSP_ALPHA_1BIT		(0x01) /* alpha value goes through */

/* color key unit mode select */
#define VSP_CKEY_THROUGH	(0x00) /* alpha and image data go through */
#define VSP_CKEY_TRANS_COLOR1	(0x01) /* transparent color mode */
#define VSP_CKEY_TRANS_COLOR2	(0x02) /* (1 color or 2 colors) */
#define VSP_CKEY_MATCHED_COLOR	(0x03) /* color replace mode */
#define VSP_CKEY_LUMA_THRESHOLD (0x04) /* color-luma threshold mode */

/* multiple unit mode select */
#define VSP_MULT_THROUGH		(0x00) /* alpha data go through */
#define VSP_MULT_RATIO			(0x01) /* multiple by ratio */
#define VSP_MULT_ALPHA			(0x02) /* multiple by alpha data */
#define VSP_MULT_RATIO_ALPHA	(0x03) /* multiple by ratio and alpha */

/* WPF module parameter */
/* output format */
#define VSP_OUT_RGB332				VSP_IN_RGB332
#define VSP_OUT_XRGB4444			VSP_IN_XRGB4444
#define VSP_OUT_RGBX4444			VSP_IN_RGBX4444
#define VSP_OUT_XRGB1555			VSP_IN_XRGB1555
#define VSP_OUT_RGBX5551			VSP_IN_RGBX5551
#define VSP_OUT_RGB565				VSP_IN_RGB565
#define VSP_OUT_PXRGB86666			VSP_IN_AXRGB86666
#define VSP_OUT_RGBXP66668			VSP_IN_RGBXA66668
#define VSP_OUT_XRGBP66668			VSP_IN_XRGBA66668
#define VSP_OUT_PRGBX86666			VSP_IN_ARGBX86666
#define VSP_OUT_PXXXRGB82666		VSP_IN_AXXXRGB82666
#define VSP_OUT_XXXRGBP26668		VSP_IN_XXXRGBA26668
#define VSP_OUT_PRGBXXX86662		VSP_IN_ARGBXXX86662
#define VSP_OUT_RGBXXXP66628		VSP_IN_RGBXXXA66628
#define VSP_OUT_XRGB6666			VSP_IN_XRGB6666
#define VSP_OUT_RGBX6666			VSP_IN_RGBX6666
#define VSP_OUT_XXXRGB2666			VSP_IN_XXXRGB2666
#define VSP_OUT_RGBXXX6662			VSP_IN_RGBXXX6662
#define VSP_OUT_PRGB8888			VSP_IN_ARGB8888
#define VSP_OUT_RGBP8888			VSP_IN_RGBA8888
#define VSP_OUT_RGB888				VSP_IN_RGB888
#define VSP_OUT_XXRGB7666			VSP_IN_XXRGB7666
#define VSP_OUT_XRGB14666			VSP_IN_XRGB14666
#define VSP_OUT_BGR888				VSP_IN_BGR888
#define VSP_OUT_PRGB4444			VSP_IN_ARGB4444
#define VSP_OUT_RGBP4444			VSP_IN_RGBA4444
#define VSP_OUT_PRGB1555			VSP_IN_ARGB1555
#define VSP_OUT_RGBP5551			VSP_IN_RGBA5551
#define VSP_OUT_PBGR4444			VSP_IN_ABGR4444
#define VSP_OUT_BGRP4444			VSP_IN_BGRA4444
#define VSP_OUT_PBGR1555			VSP_IN_ABGR1555
#define VSP_OUT_BGRP5551			VSP_IN_BGRA5551
#define VSP_OUT_XXXBGR2666			VSP_IN_XXXBGR2666
#define VSP_OUT_PBGR8888			VSP_IN_ABGR8888
#define VSP_OUT_XRGB16565			VSP_IN_XRGB16565
#define VSP_OUT_YUV444_SEMI_PLANAR	VSP_IN_YUV444_SEMI_PLANAR
#define VSP_OUT_YUV422_SEMI_PLANAR	VSP_IN_YUV422_SEMI_PLANAR
#define VSP_OUT_YUV420_SEMI_PLANAR	VSP_IN_YUV420_SEMI_PLANAR
#define VSP_OUT_YUV444_INTERLEAVED	VSP_IN_YUV444_INTERLEAVED
#define VSP_OUT_YUV422_INTERLEAVED0	VSP_IN_YUV422_INTERLEAVED0
#define VSP_OUT_YUV422_INTERLEAVED1	VSP_IN_YUV422_INTERLEAVED1
#define VSP_OUT_YUV420_INTERLEAVED	VSP_IN_YUV420_INTERLEAVED
#define VSP_OUT_YUV444_PLANAR		VSP_IN_YUV444_PLANAR
#define VSP_OUT_YUV422_PLANAR		VSP_IN_YUV422_PLANAR
#define VSP_OUT_YUV420_PLANAR		VSP_IN_YUV420_PLANAR

#define VSP_OUT_YUV422_SEMI_NV16	VSP_IN_YUV422_SEMI_NV16
#define VSP_OUT_YUV422_SEMI_NV61	VSP_IN_YUV422_SEMI_NV61
#define VSP_OUT_YUV420_SEMI_NV12	VSP_IN_YUV420_SEMI_NV12
#define VSP_OUT_YUV420_SEMI_NV21	VSP_IN_YUV420_SEMI_NV21
#define VSP_OUT_YUV422_INT0_UYVY	VSP_IN_YUV422_INT0_UYVY
#define VSP_OUT_YUV422_INT0_YUY2	VSP_IN_YUV422_INT0_YUY2
#define VSP_OUT_YUV422_INT0_YVYU	VSP_IN_YUV422_INT0_YVYU

/* PAD data select */
#define VSP_PAD_P				(0x00)	/* PAD value */
#define VSP_PAD_IN				(0x01)	/* alpha value */

/* bit count reduction method selection for data storage in packed RGB */
#define VSP_CSC_ROUND_DOWN			(0x00)	/* truncated */
#define VSP_CSC_ROUND_OFF			(0x01)	/* rounding */

/* bit count reduction method selection for data storage in PAD */
#define VSP_CONVERSION_ROUNDDOWN	(0x00)	/* truncated */
#define VSP_CONVERSION_ROUNDING		(0x01)	/* rounding */
#define VSP_CONVERSION_THRESHOLD	(0x02)	/* comparison */

/* color data clipping method */
#define VSP_CLMD_NO				(0x00)	/* not clipped */
#define VSP_CLMD_MODE1			(0x01)	/* YCbCr mode1 */
#define VSP_CLMD_MODE2			(0x02)	/* YCbCr mode2 */

/* FCNL compression activation */
#define VSP_FCNL_OFF	(0x00)	/* not compress data with FCNL */
#define VSP_FCNL_ON		(0x01)	/* compress data with FCNL */

/* rotation processing select */
enum {
	VSP_ROT_OFF = 0,	/* no rotation or flipping */
	VSP_ROT_V_FLIP,		/* vertical flipping */
	VSP_ROT_H_FLIP,		/* horizontal flipping */
	VSP_ROT_180,		/* 180 rotation */
	VSP_ROT_90,			/* 90 rotation */
	VSP_ROT_90_V_FLIP,	/* 90 rotation and V flipping */
	VSP_ROT_90_H_FLIP,	/* 90 rotation and H flipping */
	VSP_ROT_270			/* 270 rotation */
};

/* SRU module parameter */
/* super-resolution mode */
#define VSP_SRU_MODE1				(0x00)	/* without scaling */
#define VSP_SRU_MODE2				(0x40)	/* double scale-up */

/* super-resolution color */
#define VSP_SRU_RCR					(0x08)	/* R or Cr */
#define VSP_SRU_GY					(0x04)	/* G or Y */
#define VSP_SRU_BCB					(0x02)	/* B or Cb */

/* super-resolution level */
enum {
	VSP_SCL_LEVEL1 = 0,		/* weak */
	VSP_SCL_LEVEL2,
	VSP_SCL_LEVEL3,
	VSP_SCL_LEVEL4,
	VSP_SCL_LEVEL5,
	VSP_SCL_LEVEL6,			/* strong */
	VSP_SCL_LEVEL_MAX
};

/* UDS module parameter */
/* pixel count at scale-up */
#define VSP_AMD_NO	(0x00)	/* 1 + int((n-1)*scale-up factor) */
#define VSP_AMD		(0x01)	/* int(n*scale-up factor) */

/* alpha data threshold comparision enable/disable */
#define VSP_CLIP_OFF				(0x00)	/* disable */
#define VSP_CLIP_ON					(0x01)	/* enable */

/* scale-up/down of alpha plane */
#define VSP_ALPHA_OFF				(0x00)	/* disable */
#define VSP_ALPHA_ON				(0x01)	/* enable */

/* pixel component interpolation method */
#define VSP_COMPLEMENT_BIL			(0x00)	/* bilinear */
#define VSP_COMPLEMENT_NN			(0x01)	/* nearest neighbor */
#define VSP_COMPLEMENT_BC			(0x02)	/* multi-tap mode */

/* CLU module parameter */
/* LUT dimension number */
#define VSP_CLU_MODE_3D			(0x00)	/* 3D mode */
#define VSP_CLU_MODE_2D			(0x01)	/* 2D mode */
#define VSP_CLU_MODE_3D_AUTO	(0x80)	/* 3D mode with auto addr inc */
#define VSP_CLU_MODE_2D_AUTO	(0x81)	/* 2D mode with auto addr inc */

/* BRU module parameter */
/* select input layer */
#define VSP_LAY_NO				(0x00)	/* */
#define VSP_LAY_1				(0x01)	/* input source 1 */
#define VSP_LAY_2				(0x02)	/* input source 2 */
#define VSP_LAY_3				(0x03)	/* input source 3 */
#define VSP_LAY_4				(0x04)	/* input source 4 */
#define VSP_LAY_VIRTUAL			(0x05)	/* input virtual */
#define VSP_LAY_5				(0x06)	/* input source 5 */

/* color data normalization */
#define VSP_DIVISION_OFF		(0x00)	/* disable */
#define VSP_DIVISION_ON			(0x01)	/* enable */

/* dithering mode */
#define VSP_DITH_COLOR_REDUCTION (0x01) /* color reduction dithering */
#define VSP_DITH_ORDERED_DITHER	 (0x02) /* ordered dither */

/* number of color after dithering */
#define VSP_DITH_OFF		(0x00)	/* disable */
#define VSP_DITH_18BPP		(0x01)	/* 18bpp(RGB666:260000 colors) */
#define VSP_DITH_16BPP		(0x02)	/* 16bpp(RGB565:65535 colors) */
#define VSP_DITH_15BPP		(0x03)	/* 15bpp(RGB555:32768 colors) */
#define VSP_DITH_12BPP		(0x04)	/* 12bpp(RGB666:4096 colors) */
#define VSP_DITH_8BPP		(0x05)	/* 8bpp(RGB666:256 colors) */

/* ROP Unit method */
#define VSP_RBC_ROP				(0x00)	/* raster opration */
#define VSP_RBC_BLEND			(0x01)	/* blend opration */

/* blending expression selection */
#define VSP_FORM_BLEND0				(0x00)	/* + */
#define VSP_FORM_BLEND1				(0x01)	/* - */

/* blending coefficient X selection */
enum {
	VSP_COEFFICIENT_BLENDX1 = 0,	/* (DST) */
	VSP_COEFFICIENT_BLENDX2,		/* 255 - (DST) */
	VSP_COEFFICIENT_BLENDX3,		/* (SRC) */
	VSP_COEFFICIENT_BLENDX4,		/* 255 - (SRC) */
	VSP_COEFFICIENT_BLENDX5,		/* (acoefx_fix) */
	VSP_COEFFICIENT_BLENDX_MAX
};

/* blending coefficient Y selection */
enum {
	VSP_COEFFICIENT_BLENDY1 = 0,	/* (DST) */
	VSP_COEFFICIENT_BLENDY2,		/* 255 - (DST) */
	VSP_COEFFICIENT_BLENDY3,		/* (SRC) */
	VSP_COEFFICIENT_BLENDY4,		/* 255 - (SRC) */
	VSP_COEFFICIENT_BLENDY5,		/* (acoefx_fix) */
	VSP_COEFFICIENT_BLENDY_MAX
};

/* blending alpha creation expression */
#define VSP_FORM_ALPHA0				(0x00)	/* + */
#define VSP_FORM_ALPHA1				(0x01)	/* - */

/* alpha creation coefficient X */
enum {
	VSP_COEFFICIENT_ALPHAX1 = 0,	/* (DST) */
	VSP_COEFFICIENT_ALPHAX2,		/* 255 - (DST) */
	VSP_COEFFICIENT_ALPHAX3,		/* (SRC) */
	VSP_COEFFICIENT_ALPHAX4,		/* 255 - (SRC) */
	VSP_COEFFICIENT_ALPHAX5,		/* (acoefx_fix) */
	VSP_COEFFICIENT_ALPHAX_MAX
};

/* alpha creation coefficient Y */
enum {
	VSP_COEFFICIENT_ALPHAY1 = 0,	/* (DST) */
	VSP_COEFFICIENT_ALPHAY2,		/* 255 - (DST) */
	VSP_COEFFICIENT_ALPHAY3,		/* (SRC) */
	VSP_COEFFICIENT_ALPHAY4,		/* 255 - (SRC) */
	VSP_COEFFICIENT_ALPHAY5,		/* (acoefx_fix) */
	VSP_COEFFICIENT_ALPHAY_MAX
};

/* HGO,HGT module parameter */
#define VSP_STRAIGHT_BINARY		(0x00)	/* straight binary mode */
#define VSP_OFFSET_BINARY		(0x50)	/* offset binary mode */

#define VSP_MAXRGB_OFF			(0x00)	/* 3 color components */
#define VSP_MAXRGB_ON			(0x80)	/* max value of RGB */

#define VSP_STEP_64				(0x00)	/* 64 step mode */
#define VSP_STEP_256			(0x04)	/* 256 step mode */

#define VSP_SKIP_OFF			(0x00)	/* disable */
#define VSP_SKIP_1_2			(0x01)	/* every two pixels */
#define VSP_SKIP_1_4			(0x02)	/* every four pixels */

#define VSP_SMPPT_SRC1				0
#define VSP_SMPPT_SRC2				1
#define VSP_SMPPT_SRC3				2
#define VSP_SMPPT_SRC4				3
#define VSP_SMPPT_SRC5				4
#define VSP_SMPPT_SRU				16
#define VSP_SMPPT_UDS				17
#define VSP_SMPPT_LUT				22
#define VSP_SMPPT_BRU				27
#define VSP_SMPPT_CLU				29
#define VSP_SMPPT_HST				30
#define VSP_SMPPT_HSI				31
#define VSP_SMPPT_SHP				46

/* SHP module parameter */
#define VSP_SHP_SHARP			(0x00)	/* shapness */
#define VSP_SHP_UNSHARP			(0x02)	/* unsharpness */

struct vsp_dl_t {
	unsigned int hard_addr;	/* DL buffer address for H/W IP */
	void *virt_addr;		/* DL buffer address for CPU */
	unsigned short tbl_num;	/* table number(max 16383) */
	void *mem_par;			/* reserved */
};

struct vsp_irop_unit_t {
	unsigned char op_mode;
	unsigned char ref_sel;
	unsigned char bit_sel;
	unsigned long comp_color;
	unsigned long irop_color0;
	unsigned long irop_color1;
};

struct vsp_ckey_unit_t {
	unsigned char mode;
	unsigned long color1;
	unsigned long color2;
};

struct vsp_mult_unit_t {
	unsigned char a_mmd;
	unsigned char p_mmd;
	unsigned char ratio;
};

struct vsp_alpha_unit_t {
	unsigned int addr_a;
	unsigned short stride_a;
	unsigned char swap;
	unsigned char asel;
	unsigned char aext;
	unsigned char anum0;
	unsigned char anum1;
	unsigned char afix;
	struct vsp_irop_unit_t *irop;
	struct vsp_ckey_unit_t *ckey;
	struct vsp_mult_unit_t *mult;
};

struct vsp_src_t {
	unsigned int addr;		/* Y or RGB buffer address */
	unsigned int addr_c0;	/* CbCr or CB buffer address */
	unsigned int addr_c1;	/* Cr buffer address */
	unsigned short stride;
	unsigned short stride_c;
	unsigned short width;
	unsigned short height;
	unsigned short width_ex;
	unsigned short height_ex;
	unsigned short x_offset;
	unsigned short y_offset;
	unsigned short format;
	unsigned char swap;
	unsigned short x_position;
	unsigned short y_position;
	unsigned char pwd;
	unsigned char cipm;
	unsigned char cext;
	unsigned char csc;
	unsigned char iturbt;
	unsigned char clrcng;
	unsigned char vir;
	unsigned long vircolor;
	struct vsp_dl_t *clut;
	struct vsp_alpha_unit_t *alpha;
	unsigned long connect;
};

struct vsp_dst_t {
	unsigned int addr;		/* Y or RGB buffer address */
	unsigned int addr_c0;	/* CbCr or CB buffer address */
	unsigned int addr_c1;	/* Cr buffer address */
	unsigned short stride;
	unsigned short stride_c;
	unsigned short width;
	unsigned short height;
	unsigned short x_offset;
	unsigned short y_offset;
	unsigned short format;
	unsigned char swap;
	unsigned char pxa;
	unsigned char pad;
	unsigned short x_coffset;
	unsigned short y_coffset;
	unsigned char csc;
	unsigned char iturbt;
	unsigned char clrcng;
	unsigned char cbrm;
	unsigned char abrm;
	unsigned char athres;
	unsigned char clmd;
	unsigned char dith;
	unsigned char rotation;
	struct fcp_info_t *fcp;
};

/* SRU parameter */
struct vsp_sru_t {
	unsigned char mode;
	unsigned char param;
	unsigned short enscl;
	unsigned char fxa;
	unsigned long connect;
};

/* UDS parameter */
struct vsp_uds_t {
	unsigned char amd;
	unsigned char clip;
	unsigned char alpha;
	unsigned char complement;
	unsigned char athres0;
	unsigned char athres1;
	unsigned char anum0;
	unsigned char anum1;
	unsigned char anum2;
	unsigned short x_ratio;
	unsigned short y_ratio;
	unsigned long connect;
};

/* LUT parameter */
struct vsp_lut_t {
	struct vsp_dl_t lut;
	unsigned char fxa;
	unsigned long connect;
};

/* CLU parameter */
struct vsp_clu_t {
	unsigned char mode;
	struct vsp_dl_t clu;
	unsigned char fxa;
	unsigned long connect;
};

/* HST parameter */
struct vsp_hst_t {
	unsigned char fxa;
	unsigned long connect;
};

/* HSI parameter */
struct vsp_hsi_t {
	unsigned char fxa;
	unsigned long connect;
};

/* BRU parameter */
struct vsp_bld_dither_t {
	unsigned char mode;		/* dither mode */
	unsigned char bpp;		/* number of colort after dither */
};

struct vsp_bld_vir_t {
	unsigned short width;
	unsigned short height;
	unsigned short x_position;
	unsigned short y_position;
	unsigned char pwd;
	unsigned long color;
};

struct vsp_bld_ctrl_t {
	unsigned char rbc;
	unsigned char crop;
	unsigned char arop;
	unsigned char blend_formula;
	unsigned char blend_coefx;
	unsigned char blend_coefy;
	unsigned char aformula;
	unsigned char acoefx;
	unsigned char acoefy;
	unsigned char acoefx_fix;
	unsigned char acoefy_fix;
};

struct vsp_bld_rop_t {
	unsigned char crop;
	unsigned char arop;
};

struct vsp_bru_t {
	unsigned long lay_order;
	unsigned char adiv;
	struct vsp_bld_dither_t *dither_unit[5];
	struct vsp_bld_vir_t *blend_virtual;
	struct vsp_bld_ctrl_t *blend_unit_a;
	struct vsp_bld_ctrl_t *blend_unit_b;
	struct vsp_bld_ctrl_t *blend_unit_c;
	struct vsp_bld_ctrl_t *blend_unit_d;
	struct vsp_bld_ctrl_t *blend_unit_e;
	struct vsp_bld_rop_t *rop_unit;
	unsigned long connect;
};

/* BRS parameter */
struct vsp_brs_t {
	unsigned long lay_order;
	unsigned char adiv;
	struct vsp_bld_dither_t *dither_unit[2];
	struct vsp_bld_vir_t *blend_virtual;
	struct vsp_bld_ctrl_t *blend_unit_a;
	struct vsp_bld_ctrl_t *blend_unit_b;
	unsigned long connect;
};

/* HGO parameter */
struct vsp_hgo_t {
	/* histogram detection window */
	unsigned int hard_addr;		/* use 1088 bytes */
	void *virt_addr;
	void *mem_par;				/* reserved */
	unsigned short width;		/* horizontal size */
	unsigned short height;		/* vertical size */
	unsigned short x_offset;	/* horizontal offset */
	unsigned short y_offset;	/* vertical offset*/
	unsigned char binary_mode;	/* offset binary mode */
	unsigned char maxrgb_mode;	/* source component setting */
	unsigned char step_mode;	/* step of Y/MaxRGB setting */
	unsigned short x_skip;		/* horizontal pixel skipping mode */
	unsigned short y_skip;		/* vertical pixel skipping mode */

	unsigned long sampling;		/* sampling module */
};

/* HGT parameter */
struct vsp_hue_area_t {
	unsigned char lower;		/* lower boundary value */
	unsigned char upper;		/* upper boundary value */
};

struct vsp_hgt_t {
	/* histogram detection window */
	unsigned int hard_addr;		/* use 800 bytes */
	void *virt_addr;
	void *mem_par;				/* reserved */
	unsigned short width;		/* horizontal size */
	unsigned short height;		/* vertical size */
	unsigned short x_offset;	/* horizontal offset */
	unsigned short y_offset;	/* vertical offset*/
	unsigned short x_skip;		/* horizontal pixel skipping mode */
	unsigned short y_skip;		/* vertical pixel skipping mode */
	struct vsp_hue_area_t area[6];	/* hue area */

	unsigned long sampling;		/* sampling module */
};

/* SHP parameter */
struct vsp_shp_t {
	unsigned char mode;
	unsigned char gain0;		/* param 0 */
	unsigned char limit0;		/* param 1 */
	unsigned char gain10;		/* param 2 */
	unsigned char limit10;		/* param 3 */
	unsigned char gain11;		/* param 4 */
	unsigned char limit11;		/* param 5 */
	unsigned char gain20;		/* param 6 */
	unsigned char limit20;		/* param 7 */
	unsigned char gain21;		/* param 6 */
	unsigned char limit21;		/* param 7 */
	unsigned char fxa;			/* fixed alpha value */
	unsigned long connect;		/* connection module */
};

struct vsp_ctrl_t {
	struct vsp_sru_t *sru;		/* super-resolution */
	struct vsp_uds_t *uds;		/* up down scaler */
	struct vsp_lut_t *lut;		/* look up table */
	struct vsp_clu_t *clu;		/* cubic look up table */
	struct vsp_hst_t *hst;		/* hue saturation value transform */
	struct vsp_hsi_t *hsi;		/* hue saturation value inverse */
	struct vsp_bru_t *bru;		/* blend rop */
	struct vsp_brs_t *brs;		/* blend rop sub */
	struct vsp_hgo_t *hgo;		/* histogram generator-one */
	struct vsp_hgt_t *hgt;		/* histogram generator-two */
	struct vsp_shp_t *shp;		/* sharpness */
};

struct vsp_start_t {
	unsigned char rpf_num;			/* RPF number */
	unsigned long rpf_order;		/* RPF order */
	unsigned long use_module;		/* using module */
	struct vsp_src_t *src_par[5];	/* source parameter */
	struct vsp_dst_t *dst_par;		/* destination parameter */
	struct vsp_ctrl_t *ctrl_par;	/* module parameter */
	struct vsp_dl_t dl_par;			/* work memory for DL */
};
#endif
