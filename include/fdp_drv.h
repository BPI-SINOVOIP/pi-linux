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

#ifndef __FDP_DRV_H__
#define __FDP_DRV_H__

#define E_FDP_PARA_REFBUF		(-253)
#define E_FDP_PARA_STARTPAR		(-300)
#define E_FDP_PARA_FDPGO		(-301)
#define E_FDP_PARA_FPROCPAR		(-302)
#define E_FDP_PARA_SEQPAR		(-303)
#define E_FDP_PARA_INPIC		(-305)
#define E_FDP_PARA_OUTBUF		(-306)
#define E_FDP_PARA_SEQMODE		(-307)
#define E_FDP_PARA_TELECINEMODE	(-308)
#define E_FDP_PARA_INWIDTH		(-309)
#define E_FDP_PARA_INHEIGHT		(-310)
#define E_FDP_PARA_PICWIDTH		(-314)
#define E_FDP_PARA_PICHEIGHT	(-315)
#define E_FDP_PARA_CHROMA		(-316)
#define E_FDP_PARA_PROGSEQ		(-317)
#define E_FDP_PARA_PICSTRUCT	(-318)
#define E_FDP_PARA_REPEATTOP	(-319)
#define E_FDP_PARA_BUFREFRD0	(-321)
#define E_FDP_PARA_BUFREFRD1	(-322)
#define E_FDP_PARA_BUFREFRD2	(-323)
#define E_FDP_PARA_LASTSTART	(-329)
#define E_FDP_PARA_CF			(-330)
#define E_FDP_PARA_INTERPOLATED	(-331)
#define E_FDP_PARA_OUTFORMAT	(-332)
#define E_FDP_PARA_SRC_ADDR		(-350)
#define E_FDP_PARA_SRC_ADDR_C0	(-351)
#define E_FDP_PARA_SRC_ADDR_C1	(-352)
#define E_FDP_PARA_SRC_STRIDE	(-353)
#define E_FDP_PARA_SRC_STRIDE_C	(-354)
#define E_FDP_PARA_DST_ADDR		(-355)
#define E_FDP_PARA_DST_ADDR_C0	(-356)
#define E_FDP_PARA_DST_ADDR_C1	(-357)
#define E_FDP_PARA_DST_STRIDE	(-358)
#define E_FDP_PARA_DST_STRIDE_C	(-359)
#define E_FDP_PARA_STLMSK_ADDR	(-360)
#define E_FDP_PARA_FCNL			(-400)
#define E_FDP_PARA_TLEN			(-401)
#define E_FDP_PARA_FCP_POS		(-402)
#define E_FDP_PARA_FCP_STRIDE	(-403)
#define E_FDP_PARA_BA_ANC		(-404)
#define E_FDP_PARA_BA_REF		(-405)

/* FDP processing status */
enum {
	FDP_STAT_NO_INIT = 0,
	FDP_STAT_INIT,
	FDP_STAT_READY,
	FDP_STAT_RUN,
};

/* FDP processing opration parameter */
enum {
	FDP_NOGO = 0,
	FDP_GO,
};

/* Current field parameter */
enum {
	FDP_CF_TOP = 0,
	FDP_CF_BOTTOM,
};

/* De-interlacing mode parameter */
enum {
	FDP_DIM_PREV = 3,	/* Select previous field */
	FDP_DIM_NEXT,		/* Select next field */
};

/* Input/Output format */
enum {
	FDP_YUV420 = 0,
	FDP_YUV420_PLANAR,
	FDP_YUV420_NV21,
	FDP_YUV422_NV16,
	FDP_YUV422_YUY2,
	FDP_YUV422_UYVY,
	FDP_YUV422_PLANAR,
	FDP_YUV444_PLANAR,
};

#define FDP_YUV420_YV12			FDP_YUV420_PLANAR
#define FDP_YUV420_YU12			FDP_YUV420_PLANAR
#define FDP_YUV422_YV16			FDP_YUV422_PLANAR

/* Sequence mode parameter */
enum {
	FDP_SEQ_PROG = 0,		/* Progressive */
	FDP_SEQ_INTER,			/* Interlace Adaptive 3D/2D */
	FDP_SEQ_INTERH,			/* not used */
	FDP_SEQ_INTER_2D,		/* Interlace Fixed 2D */
	FDP_SEQ_INTERH_2D,		/* not used */
};

/* Telecine mode parameter */
enum {
	FDP_TC_OFF = 0,				/* Disable */
	FDP_TC_ON,					/* not used */
	FDP_TC_FORCED_PULL_DOWN,	/* Force 2-3 pull down mode */
	FDP_TC_INTERPOLATED_LINE,	/* Interpolated line mode */
};

/* Sequence information parameter */
struct fdp_seq_t {
	unsigned char seq_mode;
	unsigned char telecine_mode;
	unsigned short in_width;
	unsigned short in_height;
};

/* Picture information structure */
struct fdp_pic_t {
	unsigned long picid;
	unsigned char chroma_format;	/* input format */
	unsigned short width;			/* picture horizontal size */
	unsigned short height;			/* picture vertical size */
	unsigned char progressive_sequence;
	unsigned char progressive_frame;
	unsigned char picture_structure;
	unsigned char repeat_first_field;
	unsigned char top_field_first;
};

/* Picture image buffer information structure */
struct fdp_imgbuf_t {
	unsigned int addr;
	unsigned int addr_c0;
	unsigned int addr_c1;
	unsigned short stride;
	unsigned short stride_c;
};

/* Reference buffer information structure */
struct fdp_refbuf_t {
	struct fdp_imgbuf_t *next_buf;
	struct fdp_imgbuf_t *cur_buf;
	struct fdp_imgbuf_t *prev_buf;
};

/* De-interlace information structure */
struct fdp_ipc_t {
	unsigned char cmb_ofst;
	unsigned char cmb_max;
	unsigned char cmb_gard;
};

/* Processing information structure */
struct fdp_fproc_t {
	struct fdp_seq_t *seq_par;
	struct fdp_pic_t *in_pic;
	unsigned char last_seq_indicator;
	unsigned char current_field;
	unsigned char interpolated_line;
	unsigned char out_format;
	struct fdp_imgbuf_t *out_buf;
	struct fdp_refbuf_t *ref_buf;
	struct fcp_info_t *fcp_par;
	struct fdp_ipc_t *ipc_par;
};

/* Start information parameter */
struct fdp_start_t {
	unsigned char fdpgo;
	struct fdp_fproc_t *fproc_par;
};

/* Status information parameter */
struct fdp_status_t {
	unsigned long picid;
	unsigned int vcycle;
	unsigned int sensor[18];
};
#endif
