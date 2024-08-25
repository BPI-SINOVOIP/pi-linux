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

#ifndef __VSPM_IF_H__
#define __VSPM_IF_H__

#define DEVFILE		"vspm_if"

/* common */
enum {
	VSPM_CMD_INIT = 0,
	VSPM_CMD_QUIT,
	VSPM_CMD_ENTRY,
	VSPM_CMD_CANCEL,
	VSPM_CMD_GET_STATUS,
	VSPM_CMD_WAIT_INTERRUPT,
	VSPM_CMD_WAIT_THREAD,
	VSPM_CMD_STOP_THREAD,
};

#define VSPM_IOC_MAGIC 'v'

/* for 64bit */
struct vspm_if_entry_t {
	struct vspm_if_entry_req_t {
		char priority;
		struct vspm_job_t *job_param;
		void *user_data;
		void *cb_func;
	} req;
	struct vspm_if_entry_rsp_t {
		long ercd;
		unsigned long job_id;
	} rsp;
};

struct vspm_if_cb_rsp_t {
	long ercd;
	void *cb_func;
	unsigned long job_id;
	long result;
	void *user_data;
};

#define VSPM_IOC_CMD_INIT \
	_IOR(VSPM_IOC_MAGIC, VSPM_CMD_INIT, struct vspm_init_t)
#define VSPM_IOC_CMD_QUIT \
	_IO(VSPM_IOC_MAGIC, VSPM_CMD_QUIT)
#define VSPM_IOC_CMD_ENTRY \
	_IOWR(VSPM_IOC_MAGIC, VSPM_CMD_ENTRY, struct vspm_if_entry_t)
#define VSPM_IOC_CMD_CANCEL \
	_IOR(VSPM_IOC_MAGIC, VSPM_CMD_CANCEL, unsigned long)
#define VSPM_IOC_CMD_GET_STATUS \
	_IOWR(VSPM_IOC_MAGIC, VSPM_CMD_GET_STATUS, struct vspm_status_t)
#define VSPM_IOC_CMD_WAIT_INTERRUPT \
	_IOWR(VSPM_IOC_MAGIC, VSPM_CMD_WAIT_INTERRUPT, struct vspm_if_cb_rsp_t)
#define VSPM_IOC_CMD_WAIT_THREAD \
	_IO(VSPM_IOC_MAGIC, VSPM_CMD_WAIT_THREAD)
#define VSPM_IOC_CMD_STOP_THREAD \
	_IO(VSPM_IOC_MAGIC, VSPM_CMD_STOP_THREAD)

/* for 32bit */
struct vspm_compat_init_t {
	unsigned int use_ch;
	unsigned short mode;
	unsigned short type;
	union {
		unsigned int vsp;
		unsigned int fdp;
		unsigned int isu;
	} par;
};

struct vspm_compat_entry_t {
	struct vspm_compat_entry_req_t {
		char priority;
		unsigned int job_param;
		unsigned int user_data;
		unsigned int cb_func;
	} req;
	struct vspm_compat_entry_rsp_t {
		int ercd;
		unsigned int job_id;
	} rsp;
};

struct vspm_compat_job_t {
	unsigned short type;
	union {
		unsigned int vsp;
		unsigned int fdp;
		unsigned int isu;
	} par;
};

struct compat_vsp_dl_t {
	unsigned int hard_addr;
	unsigned int virt_addr;
	unsigned short tbl_num;
	unsigned int mem_par;
};

struct compat_vsp_irop_unit_t {
	unsigned char op_mode;
	unsigned char ref_sel;
	unsigned char bit_sel;
	unsigned int comp_color;
	unsigned int irop_color0;
	unsigned int irop_color1;
};

struct compat_vsp_ckey_unit_t {
	unsigned char mode;
	unsigned int color1;
	unsigned int color2;
};

struct compat_vsp_alpha_unit_t {
	unsigned int addr_a;
	unsigned short stride_a;
	unsigned char swap;
	unsigned char asel;
	unsigned char aext;
	unsigned char anum0;
	unsigned char anum1;
	unsigned char afix;
	unsigned int irop;
	unsigned int ckey;
	unsigned int mult;
};

struct compat_vsp_src_t {
	unsigned int addr;
	unsigned int addr_c0;
	unsigned int addr_c1;
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
	unsigned int vircolor;
	unsigned int clut;
	unsigned int alpha;
	unsigned int connect;
};

struct compat_vsp_dst_t {
	unsigned int addr;
	unsigned int addr_c0;
	unsigned int addr_c1;
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
	unsigned int fcp;
};

struct compat_vsp_sru_t {
	unsigned char mode;
	unsigned char param;
	unsigned short enscl;
	unsigned char fxa;
	unsigned int connect;
};

struct compat_vsp_uds_t {
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
	unsigned int connect;
};

struct compat_vsp_lut_t {
	struct compat_vsp_dl_t lut;
	unsigned char fxa;
	unsigned int connect;
};

struct compat_vsp_clu_t {
	unsigned char mode;
	struct compat_vsp_dl_t clu;
	unsigned char fxa;
	unsigned int connect;
};

struct compat_vsp_hst_t {
	unsigned char fxa;
	unsigned int connect;
};

struct compat_vsp_hsi_t {
	unsigned char fxa;
	unsigned int connect;
};

struct compat_vsp_bld_vir_t {
	unsigned short width;
	unsigned short height;
	unsigned short x_position;
	unsigned short y_position;
	unsigned char pwd;
	unsigned int color;
};

struct compat_vsp_bru_t {
	unsigned int lay_order;
	unsigned char adiv;
	unsigned int dither_unit[5];
	unsigned int blend_virtual;
	unsigned int blend_unit[5];
	unsigned int rop_unit;
	unsigned int connect;
};

struct compat_vsp_hgo_t {
	unsigned int hard_addr;
	unsigned int virt_addr;
	unsigned int mem_par;
	unsigned short width;
	unsigned short height;
	unsigned short x_offset;
	unsigned short y_offset;
	unsigned char binary_mode;
	unsigned char maxrgb_mode;
	unsigned char step_mode;
	unsigned short x_skip;
	unsigned short y_skip;
	unsigned int sampling;
};

struct compat_vsp_hgt_t {
	unsigned int hard_addr;
	unsigned int virt_addr;
	unsigned int mem_par;
	unsigned short width;
	unsigned short height;
	unsigned short x_offset;
	unsigned short y_offset;
	unsigned short x_skip;
	unsigned short y_skip;
	struct vsp_hue_area_t area[6];
	unsigned int sampling;
};

struct compat_vsp_shp_t {
	unsigned char mode;
	unsigned char gain0;
	unsigned char limit0;
	unsigned char gain10;
	unsigned char limit10;
	unsigned char gain11;
	unsigned char limit11;
	unsigned char gain20;
	unsigned char limit20;
	unsigned char gain21;
	unsigned char limit21;
	unsigned char fxa;
	unsigned int connect;
};

struct compat_vsp_ctrl_t {
	unsigned int sru;
	unsigned int uds;
	unsigned int lut;
	unsigned int clu;
	unsigned int hst;
	unsigned int hsi;
	unsigned int bru;
	unsigned int hgo;
	unsigned int hgt;
	unsigned int shp;
};

struct compat_vsp_start_t {
	unsigned char rpf_num;
	unsigned int rpf_order;
	unsigned int use_module;
	unsigned int src_par[5];
	unsigned int dst_par;
	unsigned int ctrl_par;
	struct compat_vsp_dl_t dl_par;
};

/* ISU */
struct compat_isu_alpha_unit_t {
        unsigned char asel;
        unsigned char anum0;
        unsigned char anum1;
        unsigned char anum2;
        unsigned char athres0;
        unsigned char athres1;
};

struct compat_isu_td_unit_t {
	unsigned char	grada_mode;
	unsigned char	grada_step;
	unsigned int 	init_val;
};

struct compat_isu_csc_t {
        unsigned char           csc;
        unsigned short          offset[3][2];
        unsigned short          clip[3][2];
        unsigned short          k_matrix[3][3];
};

struct compat_isu_src_t {
        unsigned int           addr;
        unsigned int           addr_c;
        unsigned short          stride;
        unsigned short          stride_c;
        unsigned short          width;
        unsigned short          height;
        unsigned char           format;
        unsigned char           swap;
        unsigned int 		td;
        unsigned int		alpha;
        unsigned char           uv_conv;
};
struct compat_isu_dst_t {
        unsigned int           addr;
        unsigned int           addr_c;
        unsigned short          stride;
        unsigned short          stride_c;
        unsigned char           format;
        unsigned char           swap;
        unsigned int            csc;
	unsigned int		alpha;
};
struct compat_isu_rs_t {
        unsigned short   start_x;
        unsigned short   start_y;
        unsigned short   tune_x;
        unsigned short   tune_y;
        unsigned short   crop_w;
        unsigned short   crop_h;
        unsigned char    pad_mode;
        unsigned int     pad_val;
        unsigned int     x_ratio;
        unsigned int     y_ratio;
};
struct compat_isu_start_t {
        unsigned int src_par;
        unsigned int dst_par;
        unsigned int rs_par;
        unsigned int dl_hard_addr;
};

/*FDP */
struct compat_fdp_pic_t {
	unsigned int picid;
	unsigned char chroma_format;
	unsigned short width;
	unsigned short height;
	unsigned char progressive_sequence;
	unsigned char progressive_frame;
	unsigned char picture_structure;
	unsigned char repeat_first_field;
	unsigned char top_field_first;
};

struct compat_fdp_refbuf_t {
	unsigned int next_buf;
	unsigned int cur_buf;
	unsigned int prev_buf;
};

struct compat_fdp_fproc_t {
	unsigned int seq_par;
	unsigned int in_pic;
	unsigned char last_seq_indicator;
	unsigned char current_field;
	unsigned char interpolated_line;
	unsigned char out_format;
	unsigned int out_buf;
	unsigned int ref_buf;
	unsigned int fcp_par;
	unsigned int ipc_par;
};

struct compat_fdp_start_t {
	unsigned char fdpgo;
	unsigned int fproc_par;
};

struct vspm_compat_status_t {
	unsigned int fdp;
};

struct compat_fdp_status_t {
	unsigned int picid;
	unsigned int vcycle;
	unsigned int sensor[18];
};

struct vspm_compat_cb_rsp_t {
	int ercd;
	unsigned int cb_func;
	unsigned int job_id;
	int result;
	unsigned int user_data;
};

#define VSPM_IOC_CMD_INIT32 \
	_IOR(VSPM_IOC_MAGIC, \
	VSPM_CMD_INIT, \
	struct vspm_compat_init_t)
#define VSPM_IOC_CMD_ENTRY32 \
	_IOWR(VSPM_IOC_MAGIC, \
	VSPM_CMD_ENTRY, \
	struct vspm_compat_entry_t)
#define VSPM_IOC_CMD_CANCEL32 \
	_IOR(VSPM_IOC_MAGIC, \
	VSPM_CMD_CANCEL, \
	unsigned int)
#define VSPM_IOC_CMD_GET_STATUS32 \
	_IOWR(VSPM_IOC_MAGIC, \
	VSPM_CMD_GET_STATUS, \
	struct vspm_compat_status_t)
#define VSPM_IOC_CMD_WAIT_INTERRUPT32 \
	_IOWR(VSPM_IOC_MAGIC, \
	VSPM_CMD_WAIT_INTERRUPT, \
	struct vspm_compat_cb_rsp_t)

#endif /* __VSPM_IF_H__ */
