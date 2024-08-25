/*************************************************************************/ /*
 * ISUM
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

#ifndef _ISU_DRV_H_
#define _ISU_DRV_H_

/* error code */
#define E_ISU_DEF_INH			(-100)
#define E_ISU_DEF_REG			(-101)
#define E_ISU_NO_MEM			(-102)
#define E_ISU_NO_INIT			(-103)
#define E_ISU_INVALID_STATE		(-104)
#define E_ISU_NO_CLK			(-105)
#define E_ISU_PARA_CH			(-106)

#define E_ISU_PARA_CB			(-201)
#define E_ISU_PARA_INPAR		(-202)

#define E_ISU_PARA_NOOUT		(-210)
#define E_ISU_PARA_NOIN			(-211)
#define E_ISU_PARA_IN_ADDR		(-412)
#define E_ISU_PARA_IN_STRD		(-413)
#define E_ISU_PARA_IN_WIDTH		(-414)
#define E_ISU_PARA_IN_HEIGHT		(-415)
#define E_ISU_PARA_IN_FORMAT		(-416)
#define E_ISU_PARA_ALPHA_ASEL		(-419)

#define E_ISU_PARA_OUT_ADDR		(-470)
#define E_ISU_PARA_OUT_STRD		(-471)
#define E_ISU_PARA_OUT_CSC		(-472)
#define E_ISU_PARA_OUT_FORMAT		(-475)

#define E_ISU_PARA_RS_CROP_WIDTH	(-550)
#define E_ISU_PARA_RS_CROP_HEIGHT	(-551)
#define E_ISU_PARA_RS_RATIO		(-552)	/* illegal ratio */
#define E_ISU_PARA_RS_START		(-553)	/* illegal resize start pos */
#define E_ISU_PARA_RS_PAD		(-554)  /* illegal pad selection */
#define E_ISU_PARA_RS_TUNE		(-555)  /* illegal tune */

#define E_ISU_PARA_DL_ADDR		(-680)
#define E_ISU_DL_FORMAT			(-682)

#define ISU_RS_USE			(0x01) /* Resizer */

/* lower-bit alpha value extension/compress method set */
#define ISU_AEXT_COPY			(0x00)	/* copied  */
#define ISU_AEXT_EXPAN			(0x01)	/* extended */
#define ISU_AEXT_COMP			(0x02)	/* compressed */
#define ISU_AEXT_CONV			(0x03)  /* converted */

/* RPF module parameter */
/* ISU image format */
#define ISU_ARGB1555			(0x00)
#define ISU_RGB565			(0x01)
#define ISU_BGR666			(0x02)
#define ISU_RGB888			(0x03)
#define ISU_BGR888			(0x04)
#define ISU_ARGB8888			(0x05)
#define ISU_RGBA8888			(0x06)
#define ISU_ABGR8888			(0x07)
#define ISU_YUV422_UYVY			(0x20)
#define ISU_YUV422_YUY2			(0x21)
#define ISU_YUV422_NV16			(0x22)
#define ISU_YUV420_NV12			(0x23)
#define ISU_RAW8			(0x30)
#define ISU_RAW10			(0x31)
#define ISU_RAW12			(0x32)

/* swap setting */
#define ISU_SWAP_NO			(0x00)	/* disable */
#define ISU_SWAP_B			(0x01)	/* byte units */
#define ISU_SWAP_W			(0x02)	/* word units */
#define ISU_SWAP_L			(0x04)	/* longword units */
#define ISU_SWAP_LL			(0x08)	/* LONG LWORD units */

/* maximum number steps tunning */
#define ISU_TUNE_MAX			(4096)

/* clip index */
#define CLIP_MIN_INX			0
#define CLIP_MAX_INX			1

/* color space conversion */
#define ISU_CSC_CUSTOM			(0x00)	/* enable with user custom */
#define ISU_CSC_RAW			(0x01) 	/* enable with 709 standard */
#define ISU_LAYER_NUM			(3)
#define ISU_OFFSET_NUM			(2)
#define ISU_CLIP_NUM	                (2)

/* UV conv */
#define ISU_UV_CONV_OFF                 (0x00)
#define ISU_UV_CONV_ON                  (0x01)
/* select of color space conversion scale */
#define ISU_ITU_COLOR			(0x00)	/* YUV[16,235/140] <-> RGB[0,255] */
#define ISU_FULL_COLOR			(0x01)	/* Full scale */

/* alpha bit counte conversion selection */
#define ISU_ALPHA_8BIT			(0x00) /* 8bit alpha is converted to 1bit */
#define ISU_ALPHA_1BIT			(0x01) /* alpha value goes through */

/* PAD data select */
#define ISU_PAD_P			(0x00)
#define ISU_PAD_IN			(0x01)

/* color data clipping method */
#define ISU_CLMD_NO			(0x00)	/* not clipped */
#define ISU_CLMD_MODE1			(0x01)	/* YCbCr mode1 */
#define ISU_CLMD_MODE2			(0x02)	/* YCbCr mode2 */

struct isu_alpha_unit_t {
    unsigned char	asel;
    unsigned char	anum0;
    unsigned char	anum1;
    unsigned char	anum2;
    unsigned char	athres0;
    unsigned char	athres1;
};

struct isu_td_unit_t {
    unsigned char	grada_mode;
    unsigned char	grada_step;
    unsigned int 	init_val;
};

struct isu_csc_t {
    unsigned char	csc;
    unsigned short	k_matrix[3][3];
    unsigned short	offset[3][2];
    unsigned short	clip[3][2];
};

struct isu_src_t {
    unsigned long	addr;
    unsigned long	addr_c;
    unsigned short	stride;
    unsigned short	stride_c;
    unsigned short	width;
    unsigned short	height;
    unsigned char	format ;
    unsigned char	swap;
    struct isu_td_unit_t	*td ;
    struct isu_alpha_unit_t *alpha ;
    unsigned char	uv_conv;
};

struct isu_dst_t {
    unsigned long	addr;
    unsigned long	addr_c;
    unsigned short	stride;
    unsigned short	stride_c;
    unsigned char	format ;
    unsigned char	swap;
    struct isu_csc_t	*csc;
    struct isu_alpha_unit_t *alpha ;
};


/* RS parameter */
struct isu_rs_t {
    unsigned short	start_x;
    unsigned short	start_y;
    unsigned short	tune_x;
    unsigned short	tune_y;
    unsigned short	crop_w;
    unsigned short	crop_h;
    unsigned char       pad_mode;
    unsigned int	pad_val;
    unsigned int	x_ratio ;
    unsigned int	y_ratio ;
};


struct isu_start_t {
    struct isu_src_t *src_par;	    /* source parameter */
    struct isu_dst_t *dst_par;		/* destination parameter */
    struct isu_rs_t  *rs_par ;       /* Resizer parameter */
    unsigned long     dl_hard_addr;			/* work memory for DL */
};
#endif
