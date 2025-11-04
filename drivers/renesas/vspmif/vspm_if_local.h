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

#ifndef __VSPM_IF_LOCAL_H__
#define __VSPM_IF_LOCAL_H__

#include <linux/sched.h>

extern struct platform_device *g_vspmif_pdev;

/* define assigned memory size */
#define VSPM_IF_MEM_SIZE			(24576)
#define VSPM_IF_RPF_CLUT_SIZE		(2048)
#define VSPM_IF_HGO_SIZE			(1280)
#define VSPM_IF_HGT_SIZE			(1024)

/* define macro */
#define IPRINT(fmt, args...) \
	pr_info("vspm_if:%d: " fmt, current->pid, ##args)
#define APRINT(fmt, args...) \
	pr_alert("vspm_if:%d: " fmt, current->pid, ##args)
#define EPRINT(fmt, args...) \
	pr_err("vspm_if:%d: " fmt, current->pid, ##args)

#define VSPM_IF_INT_TO_VP(addr) \
	((void *)((unsigned long)(addr)))
#define VSPM_IF_INT_TO_UP(addr) \
	((void __user *)((unsigned long)(addr)))
#define VSPM_IF_UP_TO_INT(addr) \
	((unsigned int)((unsigned long)(addr)))

/* work buffer structure */
struct vspm_if_work_buff_t {
	dma_addr_t hard_addr;
	void *virt_addr;
	unsigned int use_flag;
	unsigned int offset;
	void *next_buff;
};

/* entry data structure */
struct vspm_if_entry_data_t {
	struct list_head list;
	struct vspm_if_private_t *priv;
	struct vspm_if_entry_t entry;
	struct vspm_job_t job;
	union {
		struct vspm_entry_vsp {
			/* parameter to VSP processing */
			struct vsp_start_t par;
			/* input image settings */
			struct vspm_entry_vsp_in {
				struct vsp_src_t in;
				struct vsp_dl_t clut;
				struct vspm_entry_vsp_in_alpha {
					struct vsp_alpha_unit_t alpha;
					struct vsp_irop_unit_t irop;
					struct vsp_ckey_unit_t ckey;
					struct vsp_mult_unit_t mult;
				} alpha;
			} in[5];
			/* output image settings */
			struct vspm_entry_vsp_out {
				struct vsp_dst_t out;
				struct fcp_info_t fcp;
			} out;
			/* conversion processing settings */
			struct vspm_entry_vsp_ctrl {
				struct vsp_ctrl_t ctrl;
				struct vspm_entry_vsp_bru {
					struct vsp_bru_t bru;
					struct vsp_bld_dither_t dither_unit[5];
					struct vsp_bld_vir_t blend_virtual;
					struct vsp_bld_ctrl_t blend_unit[5];
					struct vsp_bld_rop_t rop_unit;
				} bru;
				struct vsp_sru_t sru;
				struct vsp_uds_t uds;
				struct vsp_lut_t lut;
				struct vsp_clu_t clu;
				struct vsp_hst_t hst;
				struct vsp_hsi_t hsi;
				struct vspm_entry_vsp_hgo {
					struct vsp_hgo_t hgo;
					void *user_addr;
				} hgo;
				struct vspm_entry_vsp_hgt {
					struct vsp_hgt_t hgt;
					void *user_addr;
				} hgt;
				struct vsp_shp_t shp;
			} ctrl;
			/* memory settings */
			struct vspm_if_work_buff_t *work_buff;
		} vsp;
		struct vspm_entry_fdp {
			/* parameter to FDP processing */
			struct fdp_start_t par;
			struct vspm_entry_fdp_fproc {
				struct fdp_fproc_t fproc;
				struct fdp_seq_t seq;
				struct fdp_pic_t in_pic;
				struct fdp_imgbuf_t out_buf;
				struct vspm_entry_fdp_ref {
					struct fdp_refbuf_t ref_buf;
					struct fdp_imgbuf_t ref[3];
				} ref;
				struct fcp_info_t fcp;
				struct fdp_ipc_t ipc;
			} fproc;
		} fdp;
		struct vspm_entry_isu {
			/* parameter to VSP processing */
			struct isu_start_t par;
			/* input image settings */
			struct vspm_entry_isu_in {
				struct isu_src_t in;
				struct isu_alpha_unit_t alpha;
				struct isu_td_unit_t td;
			} in;
			/* output image settings */
			struct vspm_entry_isu_out {
				struct isu_dst_t out;
				struct isu_alpha_unit_t alpha;
				struct isu_csc_t	csc;
			} out;
			/* resize processing settings */
			struct isu_rs_t rs;
			/* memory settings */
			struct vspm_if_work_buff_t *work_buff;
		} isu;
	} ip_par;
};

/* callback data structure */
struct vspm_if_cb_data_t {
	struct list_head list;
	struct vspm_if_cb_rsp_t rsp;
	struct vspm_cb_vsp_hgo {
		void *virt_addr;
		void *user_addr;
	} vsp_hgo;
	struct vspm_cb_vsp_hgt {
		void *virt_addr;
		void *user_addr;
	} vsp_hgt;
	struct vspm_if_work_buff_t *vsp_work_buff;
};

/* private data structure */
struct vspm_if_private_t {
	spinlock_t lock;	/* protects the entry list and callback list */
	struct task_struct *thread;
	struct vspm_if_entry_data_t entry_data;
	struct vspm_if_cb_data_t cb_data;
	struct completion wait_interrupt;
	struct completion wait_thread;
	struct semaphore sem;
	struct vspm_if_work_buff_t *work_buff;
	void *handle;
};

/* sub function */
void release_all_entry_data(struct vspm_if_private_t *priv);
void release_all_cb_data(struct vspm_if_private_t *priv);

struct vspm_if_work_buff_t *get_work_buffer(struct vspm_if_private_t *priv);
void release_work_buffers(struct vspm_if_private_t *priv);

int free_vsp_par(struct vspm_entry_vsp *vsp);
int set_vsp_par(
	struct vspm_if_entry_data_t *entry,
	struct vsp_start_t *vsp_par);
int free_cb_vsp_par(struct vspm_if_cb_data_t *cb_data);

void set_cb_rsp_vsp(
	struct vspm_if_cb_data_t *cb_data,
	struct vspm_if_entry_data_t *entry_data);
int set_fdp_par(
	struct vspm_if_entry_data_t *entry,
	struct fdp_start_t *fdp_par);

int set_isu_par(
	struct vspm_if_entry_data_t *entry,
	struct isu_start_t *isu_par);

int set_compat_vsp_par(struct vspm_if_entry_data_t *entry, unsigned int src);
int set_compat_fdp_par(struct vspm_if_entry_data_t *entry, unsigned int src);
int set_compat_isu_par(struct vspm_if_entry_data_t *entry, unsigned int src);

#endif /* __VSPM_IF_LOCAL_H__ */

