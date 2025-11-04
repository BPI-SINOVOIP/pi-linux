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

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include "vspm_public.h"
#include "vspm_if.h"
#include "vspm_if_local.h"

void release_all_entry_data(struct vspm_if_private_t *priv)
{
	struct vspm_if_entry_data_t *entry_data;
	struct vspm_if_entry_data_t *next;

	unsigned long lock_flag;

	spin_lock_irqsave(&priv->lock, lock_flag);
	list_for_each_entry_safe(
		entry_data, next, &priv->entry_data.list, list) {
		list_del(&entry_data->list);
		if (entry_data->job.type == VSPM_TYPE_VSP_AUTO)
			free_vsp_par(&entry_data->ip_par.vsp);

		kfree(entry_data);
	}
	spin_unlock_irqrestore(&priv->lock, lock_flag);
}

void release_all_cb_data(struct vspm_if_private_t *priv)
{
	struct vspm_if_cb_data_t *cb_data;
	struct vspm_if_cb_data_t *next;

	unsigned long lock_flag;

	spin_lock_irqsave(&priv->lock, lock_flag);
	list_for_each_entry_safe(cb_data, next, &priv->cb_data.list, list) {
		list_del(&cb_data->list);
		free_cb_vsp_par(cb_data);
		kfree(cb_data);
	}
	spin_unlock_irqrestore(&priv->lock, lock_flag);
}

struct vspm_if_work_buff_t *get_work_buffer(struct vspm_if_private_t *priv)
{
	struct vspm_if_work_buff_t *cur_buff = NULL;
	struct vspm_if_work_buff_t *prev_buff = NULL;

	down(&priv->sem);

	/* search unused work buffer */
	cur_buff = priv->work_buff;
	while (cur_buff) {
		if (cur_buff->use_flag == 0) {
			/* set work buffer */
			cur_buff->use_flag = 1;
			cur_buff->offset = 0;
			up(&priv->sem);
			return cur_buff;
		}

		prev_buff = cur_buff;
		cur_buff = cur_buff->next_buff;
	}

	/* allocate work buffer */
	cur_buff = kzalloc(sizeof(struct vspm_if_work_buff_t), GFP_KERNEL);
	if (!cur_buff) {
		EPRINT("failed to allocate memory\n");
		up(&priv->sem);
		return NULL;
	}

	cur_buff->virt_addr = dma_alloc_coherent(
		&g_vspmif_pdev->dev,
		VSPM_IF_MEM_SIZE,
		&cur_buff->hard_addr,
		GFP_KERNEL);
	if (!cur_buff->virt_addr) {
		EPRINT("failed to allocate work buffer\n");
		kfree(cur_buff);
		up(&priv->sem);
		return NULL;
	}

	/* connect work buffer */
	if (!prev_buff)
		priv->work_buff = cur_buff;
	else
		prev_buff->next_buff = cur_buff;

	/* set work buffer */
	cur_buff->use_flag = 1;
	cur_buff->offset = 0;
	up(&priv->sem);

	return cur_buff;
}

void release_work_buffers(struct vspm_if_private_t *priv)
{
	struct vspm_if_work_buff_t *cur_buff;
	struct vspm_if_work_buff_t *next_buff;

	down(&priv->sem);

	cur_buff = priv->work_buff;
	while (cur_buff) {
		next_buff = cur_buff->next_buff;

		/* release work buffer */
		dma_free_coherent(
			&g_vspmif_pdev->dev,
			VSPM_IF_MEM_SIZE,
			cur_buff->virt_addr,
			cur_buff->hard_addr);

		kfree(cur_buff);

		cur_buff = next_buff;
	}
	priv->work_buff = NULL;
	up(&priv->sem);
}

static int set_vsp_src_clut_par(
	struct vsp_dl_t *clut,
	struct vsp_dl_t *src,
	struct vspm_if_work_buff_t *work_buff)
{
	unsigned long tmp_addr;

	/* copy vsp_dl_t parameter */
	if (copy_from_user(
			clut,
			(void __user *)src,
			sizeof(struct vsp_dl_t))) {
		EPRINT("failed to copy of vsp_dl_t\n");
		return -EFAULT;
	}

	if (clut->virt_addr &&
	    clut->tbl_num > 0 &&
	    clut->tbl_num <= 256) {
		tmp_addr =
			(unsigned long)work_buff->virt_addr +
			(unsigned long)work_buff->offset;

		/* copy color table */
		if (copy_from_user(
				(void *)tmp_addr,
				(void __user *)clut->virt_addr,
				clut->tbl_num * 8)) {
			EPRINT("failed to copy of color table\n");
			return -EFAULT;
		}

		/* set parameter */
		clut->virt_addr = (void *)tmp_addr;
		tmp_addr =
			(unsigned long)work_buff->hard_addr +
			(unsigned long)work_buff->offset;
		clut->hard_addr = (unsigned int)tmp_addr;

		/* increment memory offset */
		work_buff->offset += VSPM_IF_RPF_CLUT_SIZE;
	}

	return 0;
}

static int set_vsp_src_alpha_par(
	struct vspm_entry_vsp_in_alpha *alpha, struct vsp_alpha_unit_t *src)
{
	/* copy vsp_alpha_unit_t parameter */
	if (copy_from_user(
			&alpha->alpha,
			(void __user *)src,
			sizeof(struct vsp_alpha_unit_t))) {
		EPRINT("failed to copy of vsp_alpha_unit_t\n");
		return -EFAULT;
	}

	/* copy vsp_irop_unit_t paramerter */
	if (alpha->alpha.irop) {
		if (copy_from_user(
				&alpha->irop,
				(void __user *)alpha->alpha.irop,
				sizeof(struct vsp_irop_unit_t))) {
			EPRINT("failed to copy of vsp_irop_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.irop = &alpha->irop;
	}

	/* copy vsp_ckey_unit_t paramerter */
	if (alpha->alpha.ckey) {
		if (copy_from_user(
				&alpha->ckey,
				(void __user *)alpha->alpha.ckey,
				sizeof(struct vsp_ckey_unit_t))) {
			EPRINT("failed to copy of vsp_ckey_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.ckey = &alpha->ckey;
	}

	/* copy vsp_mult_unit_t paramerter */
	if (alpha->alpha.mult) {
		if (copy_from_user(
				&alpha->mult,
				(void __user *)alpha->alpha.mult,
				sizeof(struct vsp_mult_unit_t))) {
			EPRINT("failed to copy of vsp_mult_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.mult = &alpha->mult;
	}

	return 0;
}

static int set_vsp_src_par(
	struct vspm_entry_vsp_in *in,
	struct vsp_src_t *src,
	struct vspm_if_work_buff_t *work_buff)
{
	int ercd;

	/* copy vsp_src_t parameter */
	if (copy_from_user(
			&in->in,
			(void __user *)src,
			sizeof(struct vsp_src_t))) {
		EPRINT("failed to copy of vsp_src_t\n");
		return -EFAULT;
	}

	/* copy vsp_dl_t parameter */
	if (in->in.clut) {
		ercd = set_vsp_src_clut_par(&in->clut, in->in.clut, work_buff);
		if (ercd)
			return ercd;
		in->in.clut = &in->clut;
	}

	/* copy vsp_alpha_unit_t parameter */
	if (in->in.alpha) {
		ercd = set_vsp_src_alpha_par(&in->alpha, in->in.alpha);
		if (ercd)
			return ercd;
		in->in.alpha = &in->alpha.alpha;
	}

	return 0;
}

static int set_vsp_dst_par(
	struct vspm_entry_vsp_out *out, struct vsp_dst_t *src)
{
	/* copy vsp_dst_t parameter */
	if (copy_from_user(
			&out->out,
			(void __user *)src,
			sizeof(struct vsp_dst_t))) {
		EPRINT("failed to copy of vsp_dst_t\n");
		return -EFAULT;
	}

	/* copy fcp_info_t parameter */
	if (out->out.fcp) {
		if (copy_from_user(
				&out->fcp,
				(void __user *)out->out.fcp,
				sizeof(struct fcp_info_t))) {
			EPRINT("failed to copy of fcp_info_t\n");
			return -EFAULT;
		}
		out->out.fcp = &out->fcp;
	}

	return 0;
}

static int set_vsp_bru_par(
	struct vspm_entry_vsp_bru *bru, struct vsp_bru_t *src)
{
	struct vsp_bld_ctrl_t **src_blend[5];
	int i;

	/* copy vsp_bru_t parameter */
	if (copy_from_user(
			&bru->bru,
			(void __user *)src,
			sizeof(struct vsp_bru_t))) {
		EPRINT("failed to copy of vsp_bru_t\n");
		return -EFAULT;
	}

	/* copy vsp_bld_dither_t parameter */
	for (i = 0; i < 5; i++) {
		if (bru->bru.dither_unit[i]) {
			if (copy_from_user(
					&bru->dither_unit[i],
					(void __user *)bru->bru.dither_unit[i],
					sizeof(struct vsp_bld_dither_t))) {
				EPRINT("failed to copy of vsp_bld_dither_t\n");
				return -EFAULT;
			}
			bru->bru.dither_unit[i] = &bru->dither_unit[i];
		}
	}

	/* copy vsp_bld_vir_t parameter */
	if (bru->bru.blend_virtual) {
		if (copy_from_user(
				&bru->blend_virtual,
				(void __user *)bru->bru.blend_virtual,
				sizeof(struct vsp_bld_vir_t))) {
			EPRINT("failed to copy of vsp_bld_vir_t\n");
			return -EFAULT;
		}
		bru->bru.blend_virtual = &bru->blend_virtual;
	}

	src_blend[0] = &bru->bru.blend_unit_a;
	src_blend[1] = &bru->bru.blend_unit_b;
	src_blend[2] = &bru->bru.blend_unit_c;
	src_blend[3] = &bru->bru.blend_unit_d;
	src_blend[4] = &bru->bru.blend_unit_e;

	/* copy vsp_bld_ctrl_t parameter */
	for (i = 0; i < 5; i++) {
		if (*src_blend[i]) {
			if (copy_from_user(
					&bru->blend_unit[i],
					(void __user *)*src_blend[i],
					sizeof(struct vsp_bld_ctrl_t))) {
				EPRINT(
					"failed to copy of vsp_bld_ctrl_t\n");
				return -EFAULT;
			}
			*src_blend[i] = &bru->blend_unit[i];
		}
	}

	/* copy vsp_bld_rop_t parameter */
	if (bru->bru.rop_unit) {
		if (copy_from_user(
				&bru->rop_unit,
				(void __user *)bru->bru.rop_unit,
				sizeof(struct vsp_bld_rop_t))) {
			EPRINT("failed to copy of vsp_bld_rop_t\n");
			return -EFAULT;
		}
		bru->bru.rop_unit = &bru->rop_unit;
	}

	return 0;
}

static int set_vsp_hgo_par(
	struct vspm_entry_vsp_hgo *hgo,
	struct vsp_hgo_t *src,
	struct vspm_if_work_buff_t *work_buff)
{
	unsigned long tmp_addr;

	/* copy vsp_hgo_t parameter */
	if (copy_from_user(
			&hgo->hgo,
			(void __user *)src,
			sizeof(struct vsp_hgo_t))) {
		EPRINT("failed to copy of vsp_hgo_t\n");
		return -EFAULT;
	}
	hgo->user_addr = hgo->hgo.virt_addr;

	/* set parameter */
	tmp_addr =
		(unsigned long)work_buff->hard_addr +
		(unsigned long)work_buff->offset;
	hgo->hgo.hard_addr = (unsigned int)tmp_addr;
	tmp_addr =
		(unsigned long)work_buff->virt_addr +
		(unsigned long)work_buff->offset;
	hgo->hgo.virt_addr = (void *)tmp_addr;

	/* increment memory offset */
	work_buff->offset += VSPM_IF_HGO_SIZE;

	return 0;
}

static int set_vsp_hgt_par(
	struct vspm_entry_vsp_hgt *hgt,
	struct vsp_hgt_t *src,
	struct vspm_if_work_buff_t *work_buff)
{
	unsigned long tmp_addr;

	/* copy vsp_hgt_t parameter */
	if (copy_from_user(
			&hgt->hgt,
			(void __user *)src,
			sizeof(struct vsp_hgt_t))) {
		EPRINT("failed to copy of vsp_hgt_t\n");
		return -EFAULT;
	}
	hgt->user_addr = hgt->hgt.virt_addr;

	/* set parameter */
	tmp_addr =
		(unsigned long)work_buff->hard_addr +
		(unsigned long)work_buff->offset;
	hgt->hgt.hard_addr = (unsigned int)tmp_addr;
	tmp_addr =
		(unsigned long)work_buff->virt_addr +
		(unsigned long)work_buff->offset;
	hgt->hgt.virt_addr = (void *)tmp_addr;

	/* increment memory offset */
	work_buff->offset += VSPM_IF_HGT_SIZE;

	return 0;
}

static int set_vsp_ctrl_par(
	struct vspm_entry_vsp_ctrl *ctrl,
	struct vsp_ctrl_t *src,
	struct vspm_if_work_buff_t *work_buff)
{
	int ercd;

	/* copy vsp_ctrl_t parameter */
	if (copy_from_user(
			&ctrl->ctrl,
			(void __user *)src,
			sizeof(struct vsp_ctrl_t))) {
		EPRINT("failed to copy of vsp_ctrl_t\n");
		return -EFAULT;
	}

	/* copy vsp_sru_t parameter */
	if (ctrl->ctrl.sru) {
		if (copy_from_user(
				&ctrl->sru,
				(void __user *)ctrl->ctrl.sru,
				sizeof(struct vsp_sru_t))) {
			EPRINT("failed to copy of vsp_sru_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.sru = &ctrl->sru;
	}

	/* copy vsp_uds_t parameter */
	if (ctrl->ctrl.uds) {
		if (copy_from_user(
				&ctrl->uds,
				(void __user *)ctrl->ctrl.uds,
				sizeof(struct vsp_uds_t))) {
			EPRINT("failed to copy of vsp_uds_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.uds = &ctrl->uds;
	}

	/* copy vsp_lut_t parameter */
	if (ctrl->ctrl.lut) {
		if (copy_from_user(
				&ctrl->lut,
				(void __user *)ctrl->ctrl.lut,
				sizeof(struct vsp_lut_t))) {
			EPRINT("failed to copy of vsp_lut_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.lut = &ctrl->lut;
	}

	/* copy vsp_clu_t parameter */
	if (ctrl->ctrl.clu) {
		if (copy_from_user(
				&ctrl->clu,
				(void __user *)ctrl->ctrl.clu,
				sizeof(struct vsp_clu_t))) {
			EPRINT("failed to copy of vsp_clu_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.clu = &ctrl->clu;
	}

	/* copy vsp_hst_t parameter */
	if (ctrl->ctrl.hst) {
		if (copy_from_user(
				&ctrl->hst,
				(void __user *)ctrl->ctrl.hst,
				sizeof(struct vsp_hst_t))) {
			EPRINT("failed to copy of vsp_hst_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.hst = &ctrl->hst;
	}

	/* copy vsp_hsi_t parameter */
	if (ctrl->ctrl.hsi) {
		if (copy_from_user(
				&ctrl->hsi,
				(void __user *)ctrl->ctrl.hsi,
				sizeof(struct vsp_hsi_t))) {
			EPRINT("failed to copy of vsp_hsi_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.hsi = &ctrl->hsi;
	}

	/* copy vsp_bru_t parameter */
	if (ctrl->ctrl.bru) {
		ercd = set_vsp_bru_par(&ctrl->bru, ctrl->ctrl.bru);
		if (ercd)
			return ercd;
		ctrl->ctrl.bru = &ctrl->bru.bru;
	}

	/* copy vsp_hgo_t parameter */
	if (ctrl->ctrl.hgo) {
		ercd = set_vsp_hgo_par(&ctrl->hgo, ctrl->ctrl.hgo, work_buff);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgo = &ctrl->hgo.hgo;
	}

	/* copy vsp_hgt_t parameter */
	if (ctrl->ctrl.hgt) {
		ercd = set_vsp_hgt_par(&ctrl->hgt, ctrl->ctrl.hgt, work_buff);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgt = &ctrl->hgt.hgt;
	}

	/* copy vsp_shp_t parameter */
	if (ctrl->ctrl.shp) {
		if (copy_from_user(
				&ctrl->shp,
				(void __user *)ctrl->ctrl.shp,
				sizeof(struct vsp_shp_t))) {
			EPRINT("failed to copy of vsp_shp_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.shp = &ctrl->shp;
	}

	return 0;
}

int free_vsp_par(struct vspm_entry_vsp *vsp)
{
	if (vsp->work_buff)
		vsp->work_buff->use_flag = 0;

	return 0;
}

int set_vsp_par(
	struct vspm_if_entry_data_t *entry, struct vsp_start_t *vsp_par)
{
	struct vspm_entry_vsp *vsp = &entry->ip_par.vsp;

	struct vsp_dl_t *dl_par = &vsp->par.dl_par;
	unsigned long tmp_addr;

	int ercd = 0;

	int i;

	/* copy vsp_start_t parameter */
	if (copy_from_user(
			&vsp->par,
			(void __user *)vsp_par,
			sizeof(struct vsp_start_t))) {
		EPRINT("failed to copy of vsp_start_t\n");
		return -EFAULT;
	}

	/* get work buffer */
	vsp->work_buff = get_work_buffer(entry->priv);
	if (!vsp->work_buff)
		return -EFAULT;

	/* copy vsp_src_t parameter */
	for (i = 0; i < 5; i++) {
		if (vsp->par.src_par[i]) {
			ercd = set_vsp_src_par(
				&vsp->in[i],
				vsp->par.src_par[i],
				vsp->work_buff);
			if (ercd)
				goto err_exit;
			vsp->par.src_par[i] = &vsp->in[i].in;
		}
	}

	/* copy vsp_dst_t parameter */
	if (vsp->par.dst_par) {
		ercd = set_vsp_dst_par(&vsp->out, vsp->par.dst_par);
		if (ercd)
			goto err_exit;
		vsp->par.dst_par = &vsp->out.out;
	}

	/* copy vsp_ctrl_t parameter */
	if (vsp->par.ctrl_par) {
		ercd = set_vsp_ctrl_par(
			&vsp->ctrl, vsp->par.ctrl_par, vsp->work_buff);
		if (ercd)
			goto err_exit;
		vsp->par.ctrl_par = &vsp->ctrl.ctrl;
	}

	/* assign memory for display list */
	tmp_addr =
		(unsigned long)vsp->work_buff->hard_addr +
		(unsigned long)vsp->work_buff->offset;
	dl_par->hard_addr = (unsigned int)tmp_addr;
	tmp_addr =
		(unsigned long)vsp->work_buff->virt_addr +
		(unsigned long)vsp->work_buff->offset;
	dl_par->virt_addr = (void *)tmp_addr;
	dl_par->tbl_num = (VSPM_IF_MEM_SIZE - vsp->work_buff->offset) >> 3;

	return 0;

err_exit:
	free_vsp_par(vsp);
	return ercd;
}

int free_cb_vsp_par(struct vspm_if_cb_data_t *cb_data)
{
	if (cb_data->vsp_work_buff)
		cb_data->vsp_work_buff->use_flag = 0;

	return 0;
}

void set_cb_rsp_vsp(
	struct vspm_if_cb_data_t *cb_data,
	struct vspm_if_entry_data_t *entry_data)
{
	struct vspm_entry_vsp_hgo *hgo =
		&entry_data->ip_par.vsp.ctrl.hgo;
	struct vspm_entry_vsp_hgt *hgt =
		&entry_data->ip_par.vsp.ctrl.hgt;

	/* inherits histogram(HGO) buffer address */
	cb_data->vsp_hgo.virt_addr = hgo->hgo.virt_addr;
	cb_data->vsp_hgo.user_addr = hgo->user_addr;

	/* inherits histogram(HGT) buffer address */
	cb_data->vsp_hgt.virt_addr = hgt->hgt.virt_addr;
	cb_data->vsp_hgt.user_addr = hgt->user_addr;

	/* inherits work buffer */
	cb_data->vsp_work_buff = entry_data->ip_par.vsp.work_buff;
}

static int set_fdp_ref_par(
	struct vspm_entry_fdp_ref *ref, struct fdp_refbuf_t *src)
{
	struct fdp_imgbuf_t **src_refbuf[3];
	int i;

	/* copy fdp_refbuf_t parameter */
	if (copy_from_user(
			&ref->ref_buf,
			(void __user *)src,
			sizeof(struct fdp_refbuf_t))) {
		EPRINT("failed to copy of fdp_refbuf_t\n");
		return -EFAULT;
	}

	src_refbuf[0] = &ref->ref_buf.next_buf;
	src_refbuf[1] = &ref->ref_buf.cur_buf;
	src_refbuf[2] = &ref->ref_buf.prev_buf;

	/* copy fdp_imgbuf_t parameter */
	for (i = 0; i < 3; i++) {
		if (*src_refbuf[i]) {
			if (copy_from_user(
					&ref->ref[i],
					(void __user *)*src_refbuf[i],
					sizeof(struct fdp_imgbuf_t))) {
				EPRINT("failed to copy of fdp_imgbuf_t\n");
				return -EFAULT;
			}
			*src_refbuf[i] = &ref->ref[i];
		}
	}

	return 0;
}

static int set_fdp_fproc_par(
	struct vspm_entry_fdp_fproc *fproc, struct fdp_fproc_t *src)
{
	int ercd;

	/* copy fdp_fproc_t parameter */
	if (copy_from_user(
			&fproc->fproc,
			(void __user *)src,
			sizeof(struct fdp_fproc_t))) {
		EPRINT("failed to copy of fdp_fproc_t\n");
		return -EFAULT;
	}

	/* copy fdp_seq_t parameter */
	if (fproc->fproc.seq_par) {
		if (copy_from_user(
				&fproc->seq,
				(void __user *)fproc->fproc.seq_par,
				sizeof(struct fdp_seq_t))) {
			EPRINT("failed to copy of fdp_seq_t\n");
			return -EFAULT;
		}
		fproc->fproc.seq_par = &fproc->seq;
	}

	/* copy fdp_pic_t parameter */
	if (fproc->fproc.in_pic) {
		if (copy_from_user(
				&fproc->in_pic,
				(void __user *)fproc->fproc.in_pic,
				sizeof(struct fdp_pic_t))) {
			EPRINT("failed to copy of fdp_pic_t\n");
			return -EFAULT;
		}
		fproc->fproc.in_pic = &fproc->in_pic;
	}

	/* copy fdp_imgbuf_t parameter */
	if (fproc->fproc.out_buf) {
		if (copy_from_user(
				&fproc->out_buf,
				(void __user *)fproc->fproc.out_buf,
				sizeof(struct fdp_imgbuf_t))) {
			EPRINT("failed to copy of fdp_imgbuf_t\n");
			return -EFAULT;
		}
		fproc->fproc.out_buf = &fproc->out_buf;
	}

	/* copy fdp_refbuf_t parameter */
	if (fproc->fproc.ref_buf) {
		ercd = set_fdp_ref_par(&fproc->ref, fproc->fproc.ref_buf);
		if (ercd)
			return ercd;
		fproc->fproc.ref_buf = &fproc->ref.ref_buf;
	}

	/* copy fcp_info_t parameter */
	if (fproc->fproc.fcp_par) {
		if (copy_from_user(
				&fproc->fcp,
				(void __user *)fproc->fproc.fcp_par,
				sizeof(struct fcp_info_t))) {
			EPRINT("failed to copy of fcp_info_t\n");
			return -EFAULT;
		}
		fproc->fproc.fcp_par = &fproc->fcp;
	}

	/* copy fdp_ipc_t parameter */
	if (fproc->fproc.ipc_par) {
		if (copy_from_user(
				&fproc->ipc,
				(void __user *)fproc->fproc.ipc_par,
				sizeof(struct fdp_ipc_t))) {
			EPRINT("failed to copy of fdp_ipc_t\n");
			return -EFAULT;
		}
		fproc->fproc.ipc_par = &fproc->ipc;
	}

	return 0;
}

int set_fdp_par(
	struct vspm_if_entry_data_t *entry, struct fdp_start_t *fdp_par)
{
	struct vspm_entry_fdp *fdp = &entry->ip_par.fdp;
	int ercd;

	/* copy fdp_start_t parameter */
	if (copy_from_user(
			&fdp->par,
			(void __user *)fdp_par,
			sizeof(struct fdp_start_t))) {
		EPRINT("failed to copy of fdp_start_t\n");
		return -EFAULT;
	};

	/* copy fdp_fproc_t parameter */
	if (fdp->par.fproc_par) {
		ercd = set_fdp_fproc_par(&fdp->fproc, fdp->par.fproc_par);
		if (ercd)
			return ercd;
		fdp->par.fproc_par = &fdp->fproc.fproc;
	}

	return 0;
}
/* Write ISU parameters from user to kernel */
static int set_isu_src_par(
	struct vspm_entry_isu_in *in,
	struct isu_src_t *src,
	struct vspm_if_work_buff_t *work_buff)
{
	/* copy isu_src_t parameter */
	if (copy_from_user(
			&in->in,
			(void __user *)src,
			sizeof(struct isu_src_t))) {
		EPRINT("failed to copy of isu_src_t\n");
		return -EFAULT;
	}

	/* copy isu_alpha_unit_t parameter */
	if (in->in.alpha){
		if (copy_from_user(
				&in->alpha,
				(void __user *)in->in.alpha,
				sizeof(struct isu_alpha_unit_t))) {
			EPRINT("failed to copy of isu_alpha_unit_t\n");
			return -EFAULT;
		}
		in->in.alpha = &in->alpha;
	}

	/* copy isu_td_unit_t parameter */
	if (in->in.td){
		if (copy_from_user(
				&in->td,
				(void __user *)in->in.td,
				sizeof(struct isu_td_unit_t))) {
			EPRINT("failed to copy of isu_td_unit_t\n");
			return -EFAULT;
		}
		in->in.td = &in->td;
	}

	return 0;
}

static int set_isu_dst_par(
	struct vspm_entry_isu_out *out, struct isu_dst_t *src)
{
	/* copy isu_dst_t parameter */
	if (copy_from_user(
			&out->out,
			(void __user *)src,
			sizeof(struct isu_dst_t))) {
		EPRINT("failed to copy of isu_dst_t\n");
		return -EFAULT;
	}
	/* copy isu_alpha_unit_t parameter */
        if (out->out.alpha){
		if (copy_from_user(
				&out->alpha,
				(void __user *)out->out.alpha,
				sizeof(struct isu_alpha_unit_t))) {
			EPRINT("failed to copy of isu_alpha_unit_t\n");
			return -EFAULT;
		}
		out->out.alpha = &out->alpha;
	}

	if (out->out.csc){
		if (copy_from_user(
				&out->csc,
				(void __user *)out->out.csc,
				sizeof(struct isu_csc_t))){
			EPRINT("failed to copy of isu_csc_t\n");
			return -EFAULT;
		}
		out->out.csc = &out->csc;
	}
	return 0;
}

int set_isu_par(
	struct vspm_if_entry_data_t *entry, struct isu_start_t *isu_par)
{
	struct vspm_entry_isu *isu = &entry->ip_par.isu;
	int ercd = 0;

	/* copy isu_start_t parameter */
	if (copy_from_user(
			&isu->par,
			(void __user *)isu_par,
			sizeof(struct isu_start_t))) {
		EPRINT("failed to copy of isu_start_t\n");
		return -EFAULT;
	}

	/* copy isu_src_t parameter */
	if(isu->par.src_par){
		ercd = set_isu_src_par(
			&isu->in,
			isu->par.src_par,
			isu->work_buff);
		if (ercd)
			goto err_exit;
		isu->par.src_par = &isu->in.in;
	}

	/* copy isu_dst_t parameter */
	if(isu->par.dst_par){
		ercd = set_isu_dst_par(
			&isu->out,
			isu->par.dst_par);
		if (ercd)
			goto err_exit;
		isu->par.dst_par = &isu->out.out;
	}

	/* copy isu_rs_t parameter */
	if (isu->par.rs_par) {
		if (copy_from_user(
				&isu->rs,
				(void __user *)isu->par.rs_par,
				sizeof(struct isu_rs_t))) {
			EPRINT("failed to copy of isu_rs_t\n");
			return -EFAULT;
		}
		if (ercd)
			goto err_exit;
		isu->par.rs_par =  &isu->rs;
	}

	return 0;
err_exit:
	return ercd;
}

/* VSP 32 bits */
static int set_compat_vsp_src_clut_par(
	struct vsp_dl_t *clut,
	unsigned int src,
	struct vspm_if_work_buff_t *work_buff)
{
	struct compat_vsp_dl_t compat_dl_par;
	unsigned long tmp_addr;

	/* copy */
	if (copy_from_user(
			&compat_dl_par,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_dl_t))) {
		EPRINT("failed to copy of vsp_dl_t\n");
		return -EFAULT;
	}

	if (compat_dl_par.virt_addr != 0 &&
	    compat_dl_par.tbl_num > 0 &&
	    compat_dl_par.tbl_num <= 256) {
		tmp_addr =
			(unsigned long)work_buff->virt_addr +
			(unsigned long)work_buff->offset;

		/* copy color table */
		if (copy_from_user(
				(void *)tmp_addr,
				VSPM_IF_INT_TO_UP(compat_dl_par.virt_addr),
				compat_dl_par.tbl_num * 8)) {
			EPRINT("failed to copy color table\n");
			return -EFAULT;
		}

		/* set parameter */
		clut->virt_addr = (void *)tmp_addr;
		tmp_addr =
			(unsigned long)work_buff->hard_addr +
			(unsigned long)work_buff->offset;
		clut->hard_addr = (unsigned int)tmp_addr;
		clut->tbl_num = compat_dl_par.tbl_num;

		/* increment memory offset */
		work_buff->offset += VSPM_IF_RPF_CLUT_SIZE;
	}

	return 0;
}

static int set_compat_vsp_irop_par(
	struct vsp_irop_unit_t *irop, unsigned int src)
{
	struct compat_vsp_irop_unit_t compat_irop;

	/* copy */
	if (copy_from_user(
			&compat_irop,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_irop_unit_t))) {
		EPRINT("failed to copy of vsp_irop_unit_t\n");
		return -EFAULT;
	}

	/* set */
	irop->op_mode = compat_irop.op_mode;
	irop->ref_sel = compat_irop.ref_sel;
	irop->bit_sel = compat_irop.bit_sel;
	irop->comp_color = (unsigned long)compat_irop.comp_color;
	irop->irop_color0 = (unsigned long)compat_irop.irop_color0;
	irop->irop_color1 = (unsigned long)compat_irop.irop_color1;

	return 0;
}

static int set_compat_vsp_ckey_par(
	struct vsp_ckey_unit_t *ckey, unsigned int src)
{
	struct compat_vsp_ckey_unit_t compat_ckey;

	/* copy */
	if (copy_from_user(
			&compat_ckey,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_ckey_unit_t))) {
		EPRINT("failed to copy of vsp_ckey_unit_t\n");
		return -EFAULT;
	}

	/* set */
	ckey->mode = compat_ckey.mode;
	ckey->color1 = (unsigned long)compat_ckey.color1;
	ckey->color2 = (unsigned long)compat_ckey.color2;

	return 0;
}

static int set_compat_vsp_src_alpha_par(
	struct vspm_entry_vsp_in_alpha *alpha, unsigned int src)
{
	struct compat_vsp_alpha_unit_t compat_alpha;
	int ercd;

	/* copy vsp_alpha_unit_t parameter */
	if (copy_from_user(
			&compat_alpha,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_alpha_unit_t))) {
		EPRINT("failed to copy of vsp_alpha_unit_t\n");
		return -EFAULT;
	}

	alpha->alpha.addr_a = compat_alpha.addr_a;
	alpha->alpha.stride_a = compat_alpha.stride_a;
	alpha->alpha.swap = compat_alpha.swap;
	alpha->alpha.asel = compat_alpha.asel;
	alpha->alpha.aext = compat_alpha.aext;
	alpha->alpha.anum0 = compat_alpha.anum0;
	alpha->alpha.anum1 = compat_alpha.anum1;
	alpha->alpha.afix = compat_alpha.afix;

	/* copy vsp_irop_unit_t paramerter */
	if (compat_alpha.irop) {
		ercd = set_compat_vsp_irop_par(&alpha->irop, compat_alpha.irop);
		if (ercd)
			return ercd;
		alpha->alpha.irop = &alpha->irop;
	}

	/* copy vsp_ckey_unit_t paramerter */
	if (compat_alpha.ckey) {
		ercd = set_compat_vsp_ckey_par(&alpha->ckey, compat_alpha.ckey);
		if (ercd)
			return ercd;
		alpha->alpha.ckey = &alpha->ckey;
	}

	/* copy vsp_mult_unit_t paramerter */
	if (compat_alpha.mult) {
		if (copy_from_user(
				&alpha->mult,
				VSPM_IF_INT_TO_UP(compat_alpha.mult),
				sizeof(struct vsp_mult_unit_t))) {
			EPRINT("failed to copy of vsp_mult_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.mult = &alpha->mult;
	}

	return 0;
}

static int set_compat_vsp_src_par(
	struct vspm_entry_vsp_in *in,
	unsigned int src,
	struct vspm_if_work_buff_t *work_buff)
{
	struct compat_vsp_src_t compat_vsp_src;
	int ercd;

	/* copy vsp_src_t parameter */
	if (copy_from_user(
			&compat_vsp_src,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_src_t))) {
		EPRINT("failed to copy of vsp_src_t\n");
		return -EFAULT;
	}

	in->in.addr = compat_vsp_src.addr;
	in->in.addr_c0 = compat_vsp_src.addr_c0;
	in->in.addr_c1 = compat_vsp_src.addr_c1;
	in->in.stride = compat_vsp_src.stride;
	in->in.stride_c = compat_vsp_src.stride_c;
	in->in.width = compat_vsp_src.width;
	in->in.height = compat_vsp_src.height;
	in->in.width_ex = compat_vsp_src.width_ex;
	in->in.height_ex = compat_vsp_src.height_ex;
	in->in.x_offset = compat_vsp_src.x_offset;
	in->in.y_offset = compat_vsp_src.y_offset;
	in->in.format = compat_vsp_src.format;
	in->in.swap = compat_vsp_src.swap;
	in->in.x_position = compat_vsp_src.x_position;
	in->in.y_position = compat_vsp_src.y_position;
	in->in.pwd = compat_vsp_src.pwd;
	in->in.cipm = compat_vsp_src.cipm;
	in->in.cext = compat_vsp_src.cext;
	in->in.csc = compat_vsp_src.csc;
	in->in.iturbt = compat_vsp_src.iturbt;
	in->in.clrcng = compat_vsp_src.clrcng;
	in->in.vir = compat_vsp_src.vir;
	in->in.vircolor = (unsigned long)compat_vsp_src.vircolor;
	in->in.connect = (unsigned long)compat_vsp_src.connect;

	/* copy vsp_dl_t parameter */
	if (compat_vsp_src.clut) {
		ercd = set_compat_vsp_src_clut_par(
			&in->clut, compat_vsp_src.clut, work_buff);
		if (ercd)
			return ercd;
		in->in.clut = &in->clut;
	}

	/* copy vsp_alpha_unit_t parameter */
	if (compat_vsp_src.alpha) {
		ercd = set_compat_vsp_src_alpha_par(
			&in->alpha, compat_vsp_src.alpha);
		if (ercd)
			return ercd;
		in->in.alpha = &in->alpha.alpha;
	}

	return 0;
}

static int set_compat_vsp_dst_par(
	struct vspm_entry_vsp_out *out, unsigned int src)
{
	struct compat_vsp_dst_t compat_vsp_dst;

	/* copy vsp_dst_t parameter */
	if (copy_from_user(
			&compat_vsp_dst,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_dst_t))) {
		EPRINT("failed to copy of vsp_dst_t\n");
		return -EFAULT;
	}

	out->out.addr = compat_vsp_dst.addr;
	out->out.addr_c0 = compat_vsp_dst.addr_c0;
	out->out.addr_c1 = compat_vsp_dst.addr_c1;
	out->out.stride = compat_vsp_dst.stride;
	out->out.stride_c = compat_vsp_dst.stride_c;
	out->out.width = compat_vsp_dst.width;
	out->out.height = compat_vsp_dst.height;
	out->out.x_offset = compat_vsp_dst.x_offset;
	out->out.y_offset = compat_vsp_dst.y_offset;
	out->out.format = compat_vsp_dst.format;
	out->out.swap = compat_vsp_dst.swap;
	out->out.pxa = compat_vsp_dst.pxa;
	out->out.pad = compat_vsp_dst.pad;
	out->out.x_coffset = compat_vsp_dst.x_coffset;
	out->out.y_coffset = compat_vsp_dst.y_coffset;
	out->out.csc = compat_vsp_dst.csc;
	out->out.iturbt = compat_vsp_dst.iturbt;
	out->out.clrcng = compat_vsp_dst.clrcng;
	out->out.cbrm = compat_vsp_dst.cbrm;
	out->out.abrm = compat_vsp_dst.abrm;
	out->out.athres = compat_vsp_dst.athres;
	out->out.clmd = compat_vsp_dst.clmd;
	out->out.dith = compat_vsp_dst.dith;
	out->out.rotation = compat_vsp_dst.rotation;

	/* copy fcp_info_t parameter */
	if (compat_vsp_dst.fcp) {
		if (copy_from_user(
				&out->fcp,
				VSPM_IF_INT_TO_UP(compat_vsp_dst.fcp),
				sizeof(struct fcp_info_t))) {
			EPRINT("failed to copy to fcp_info_t\n");
			return -EFAULT;
		}
		out->out.fcp = &out->fcp;
	}

	return 0;
}

static int set_compat_vsp_sru_par(struct vsp_sru_t *sru, unsigned int src)
{
	struct compat_vsp_sru_t compat_sru;

	/* copy */
	if (copy_from_user(
			&compat_sru,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_sru_t))) {
		EPRINT("failed to copy of vsp_sru_t\n");
		return -EFAULT;
	}

	/* set */
	sru->mode = compat_sru.mode;
	sru->param = compat_sru.param;
	sru->enscl = compat_sru.enscl;
	sru->fxa = compat_sru.fxa;
	sru->connect = (unsigned long)compat_sru.connect;

	return 0;
}

static int set_compat_vsp_uds_par(struct vsp_uds_t *uds, unsigned int src)
{
	struct compat_vsp_uds_t compat_uds;

	/* copy */
	if (copy_from_user(
			&compat_uds,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_uds_t))) {
		EPRINT("failed to copy of vsp_uds_t\n");
		return -EFAULT;
	}

	/* set */
	uds->amd = compat_uds.amd;
	uds->clip = compat_uds.clip;
	uds->alpha = compat_uds.alpha;
	uds->complement = compat_uds.complement;
	uds->athres0 = compat_uds.athres0;
	uds->athres1 = compat_uds.athres1;
	uds->anum0 = compat_uds.anum0;
	uds->anum1 = compat_uds.anum1;
	uds->anum2 = compat_uds.anum2;
	uds->x_ratio = compat_uds.x_ratio;
	uds->y_ratio = compat_uds.y_ratio;
	uds->connect = (unsigned long)compat_uds.connect;

	return 0;
}

static int set_compat_vsp_lut_par(struct vsp_lut_t *lut, unsigned int src)
{
	struct compat_vsp_lut_t compat_lut;

	/* copy */
	if (copy_from_user(
			&compat_lut,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_lut_t))) {
		EPRINT("failed to copy of vsp_lut_t\n");
		return -EFAULT;
	}

	/* set */
	lut->lut.hard_addr = compat_lut.lut.hard_addr;
	lut->lut.virt_addr = VSPM_IF_INT_TO_VP(compat_lut.lut.virt_addr);
	lut->lut.tbl_num = compat_lut.lut.tbl_num;
	lut->fxa = compat_lut.fxa;
	lut->connect = (unsigned long)compat_lut.connect;

	return 0;
}

static int set_compat_vsp_clu_par(struct vsp_clu_t *clu, unsigned int src)
{
	struct compat_vsp_clu_t compat_clu;

	/* copy */
	if (copy_from_user(
			&compat_clu,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_clu_t))) {
		EPRINT("failed to copy of vsp_clu_t\n");
		return -EFAULT;
	}

	/* set */
	clu->mode = compat_clu.mode;
	clu->clu.hard_addr = compat_clu.clu.hard_addr;
	clu->clu.virt_addr = VSPM_IF_INT_TO_VP(compat_clu.clu.virt_addr);
	clu->clu.tbl_num = compat_clu.clu.tbl_num;
	clu->fxa = compat_clu.fxa;
	clu->connect = (unsigned long)compat_clu.connect;

	return 0;
}

static int set_compat_vsp_hst_par(struct vsp_hst_t *hst, unsigned int src)
{
	struct compat_vsp_hst_t compat_hst;

	/* copy */
	if (copy_from_user(
			&compat_hst,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_hst_t))) {
		EPRINT("failed to copy of vsp_hst_t\n");
		return -EFAULT;
	}

	/* set */
	hst->fxa = compat_hst.fxa;
	hst->connect = (unsigned long)compat_hst.connect;

	return 0;
}

static int set_compat_vsp_hsi_par(struct vsp_hsi_t *hsi, unsigned int src)
{
	struct compat_vsp_hsi_t compat_hsi;

	/* copy */
	if (copy_from_user(
			&compat_hsi,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_hsi_t))) {
		EPRINT("failed to copy of vsp_hsi_t\n");
		return -EFAULT;
	}

	/* set */
	hsi->fxa = compat_hsi.fxa;
	hsi->connect = (unsigned long)compat_hsi.connect;

	return 0;
}

static int set_compat_vsp_bru_vir_par(
	struct vsp_bld_vir_t *vir, unsigned int src)
{
	struct compat_vsp_bld_vir_t compat_vir;

	/* copy */
	if (copy_from_user(
			&compat_vir,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_bld_vir_t))) {
		EPRINT("failed to copy of vsp_bld_vir_t\n");
		return -EFAULT;
	}

	/* set */
	vir->width = compat_vir.width;
	vir->height = compat_vir.height;
	vir->x_position = compat_vir.x_position;
	vir->y_position = compat_vir.y_position;
	vir->pwd = compat_vir.pwd;
	vir->color = (unsigned long)compat_vir.color;

	return 0;
}

static int set_compat_vsp_bru_par(
	struct vspm_entry_vsp_bru *bru, unsigned int src)
{
	struct vsp_bld_ctrl_t **src_blend[5];
	struct compat_vsp_bru_t compat_bru;
	int ercd;
	int i;

	/* copy vsp_bru_t parameter */
	if (copy_from_user(
			&compat_bru,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_bru_t))) {
		EPRINT("failed to copy of vsp_bru_t\n");
		return -EFAULT;
	}

	bru->bru.lay_order = (unsigned long)compat_bru.lay_order;
	bru->bru.adiv = compat_bru.adiv;
	bru->bru.connect = (unsigned long)compat_bru.connect;

	/* copy vsp_bld_dither_t parameter */
	for (i = 0; i < 5; i++) {
		if (compat_bru.dither_unit[i]) {
			if (copy_from_user(
					&bru->dither_unit[i],
					VSPM_IF_INT_TO_UP(
						compat_bru.dither_unit[i]),
					sizeof(struct vsp_bld_dither_t))) {
				EPRINT("failed to copy of vsp_bld_dither_t\n");
				return -EFAULT;
			}
			bru->bru.dither_unit[i] = &bru->dither_unit[i];
		}
	}

	/* copy vsp_bld_vir_t parameter */
	if (compat_bru.blend_virtual) {
		ercd = set_compat_vsp_bru_vir_par(
			&bru->blend_virtual, compat_bru.blend_virtual);
		if (ercd)
			return ercd;
		bru->bru.blend_virtual = &bru->blend_virtual;
	}

	/* copy vsp_bld_ctrl_t parameter */
	src_blend[0] = &bru->bru.blend_unit_a;
	src_blend[1] = &bru->bru.blend_unit_b;
	src_blend[2] = &bru->bru.blend_unit_c;
	src_blend[3] = &bru->bru.blend_unit_d;
	src_blend[4] = &bru->bru.blend_unit_e;

	for (i = 0; i < 5; i++) {
		if (compat_bru.blend_unit[i]) {
			if (copy_from_user(
					&bru->blend_unit[i],
				    VSPM_IF_INT_TO_UP(compat_bru.blend_unit[i]),
					sizeof(struct vsp_bld_ctrl_t))) {
				EPRINT("failed to copy of vsp_bld_ctrl_t\n");
				return -EFAULT;
			}
			*src_blend[i] = &bru->blend_unit[i];
		}
	}

	/* copy vsp_bld_rop_t parameter */
	if (compat_bru.rop_unit) {
		if (copy_from_user(
				&bru->rop_unit,
				VSPM_IF_INT_TO_UP(compat_bru.rop_unit),
				sizeof(struct vsp_bld_rop_t))) {
			EPRINT("failed to copy of vsp_bld_rop_t\n");
			return -EFAULT;
		}
		bru->bru.rop_unit = &bru->rop_unit;
	}

	return 0;
}

static int set_compat_vsp_hgo_par(
	struct vspm_entry_vsp_hgo *hgo,
	unsigned int src,
	struct vspm_if_work_buff_t *work_buff)
{
	struct compat_vsp_hgo_t compat_hgo;
	unsigned long tmp_addr;

	/* copy */
	if (copy_from_user(
			&compat_hgo,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_hgo_t))) {
		EPRINT("failed to copy of vsp_hgo_t\n");
		return -EFAULT;
	}

	/* set */
	tmp_addr =
		(unsigned long)work_buff->hard_addr +
		(unsigned long)work_buff->offset;
	hgo->hgo.hard_addr = (unsigned int)tmp_addr;
	tmp_addr =
		(unsigned long)work_buff->virt_addr +
		(unsigned long)work_buff->offset;
	hgo->hgo.virt_addr = (void *)tmp_addr;

	hgo->hgo.width = compat_hgo.width;
	hgo->hgo.height = compat_hgo.height;
	hgo->hgo.x_offset = compat_hgo.x_offset;
	hgo->hgo.y_offset = compat_hgo.y_offset;
	hgo->hgo.binary_mode = compat_hgo.binary_mode;
	hgo->hgo.maxrgb_mode = compat_hgo.maxrgb_mode;
	hgo->hgo.step_mode = compat_hgo.step_mode;
	hgo->hgo.x_skip = compat_hgo.x_skip;
	hgo->hgo.y_skip = compat_hgo.y_skip;
	hgo->hgo.sampling = (unsigned long)compat_hgo.sampling;

	hgo->user_addr = VSPM_IF_INT_TO_VP(compat_hgo.virt_addr);

	/* increment memory offset */
	work_buff->offset += VSPM_IF_HGO_SIZE;

	return 0;
}

static int set_compat_vsp_hgt_par(
	struct vspm_entry_vsp_hgt *hgt,
	unsigned int src,
	struct vspm_if_work_buff_t *work_buff)
{
	struct compat_vsp_hgt_t compat_hgt;
	unsigned long tmp_addr;

	int i;

	/* copy */
	if (copy_from_user(
			&compat_hgt,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_hgt_t))) {
		EPRINT("failed to copy of vsp_hgt_t\n");
		return -EFAULT;
	}

	/* set */
	tmp_addr =
		(unsigned long)work_buff->hard_addr +
		(unsigned long)work_buff->offset;
	hgt->hgt.hard_addr = (unsigned int)tmp_addr;
	tmp_addr =
		(unsigned long)work_buff->virt_addr +
		(unsigned long)work_buff->offset;
	hgt->hgt.virt_addr = (void *)tmp_addr;

	hgt->hgt.width = compat_hgt.width;
	hgt->hgt.height = compat_hgt.height;
	hgt->hgt.x_offset = compat_hgt.x_offset;
	hgt->hgt.y_offset = compat_hgt.y_offset;
	hgt->hgt.x_skip = compat_hgt.x_skip;
	hgt->hgt.y_skip = compat_hgt.y_skip;
	for (i = 0; i < 6; i++) {
		hgt->hgt.area[i].lower = compat_hgt.area[i].lower;
		hgt->hgt.area[i].upper = compat_hgt.area[i].upper;
	}
	hgt->hgt.sampling = (unsigned long)compat_hgt.sampling;

	hgt->user_addr = VSPM_IF_INT_TO_VP(compat_hgt.virt_addr);

	/* increment memory offset */
	work_buff->offset += VSPM_IF_HGT_SIZE;

	return 0;
}

static int set_compat_vsp_shp_par(struct vsp_shp_t *shp, unsigned int src)
{
	struct compat_vsp_shp_t compat_shp;

	/* copy */
	if (copy_from_user(
			&compat_shp,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_shp_t))) {
		EPRINT("failed to copy of vsp_shp_t\n");
		return -EFAULT;
	}

	/* set */
	shp->mode = compat_shp.mode;
	shp->gain0 = compat_shp.gain0;
	shp->limit0 = compat_shp.limit0;
	shp->gain10 = compat_shp.gain10;
	shp->limit10 = compat_shp.limit10;
	shp->gain11 = compat_shp.gain11;
	shp->limit11 = compat_shp.limit11;
	shp->gain20 = compat_shp.gain20;
	shp->limit20 = compat_shp.limit20;
	shp->gain21 = compat_shp.gain21;
	shp->limit21 = compat_shp.limit21;
	shp->fxa = compat_shp.fxa;
	shp->connect = (unsigned long)compat_shp.connect;

	return 0;
}

static int set_compat_vsp_ctrl_par(
	struct vspm_entry_vsp_ctrl *ctrl,
	unsigned int src,
	struct vspm_if_work_buff_t *work_buff)
{
	struct compat_vsp_ctrl_t compat_vsp_ctrl;
	int ercd;

	/* copy vsp_ctrl_t parameter */
	if (copy_from_user(
			&compat_vsp_ctrl,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_ctrl_t))) {
		EPRINT("failed to copy of vsp_ctrl_t\n");
		return -EFAULT;
	}

	/* copy vsp_sru_t parameter */
	if (compat_vsp_ctrl.sru) {
		ercd = set_compat_vsp_sru_par(&ctrl->sru, compat_vsp_ctrl.sru);
		if (ercd)
			return ercd;
		ctrl->ctrl.sru = &ctrl->sru;
	}

	/* copy vsp_uds_t parameter */
	if (compat_vsp_ctrl.uds) {
		ercd = set_compat_vsp_uds_par(&ctrl->uds, compat_vsp_ctrl.uds);
		if (ercd)
			return ercd;
		ctrl->ctrl.uds = &ctrl->uds;
	}

	/* copy vsp_lut_t parameter */
	if (compat_vsp_ctrl.lut) {
		ercd = set_compat_vsp_lut_par(&ctrl->lut, compat_vsp_ctrl.lut);
		if (ercd)
			return ercd;
		ctrl->ctrl.lut = &ctrl->lut;
	}

	/* copy vsp_clu_t parameter */
	if (compat_vsp_ctrl.clu) {
		ercd = set_compat_vsp_clu_par(&ctrl->clu, compat_vsp_ctrl.clu);
		if (ercd)
			return ercd;
		ctrl->ctrl.clu = &ctrl->clu;
	}

	/* copy vsp_hst_t parameter */
	if (compat_vsp_ctrl.hst) {
		ercd = set_compat_vsp_hst_par(&ctrl->hst, compat_vsp_ctrl.hst);
		if (ercd)
			return ercd;
		ctrl->ctrl.hst = &ctrl->hst;
	}

	/* copy vsp_hsi_t parameter */
	if (compat_vsp_ctrl.hsi) {
		ercd = set_compat_vsp_hsi_par(&ctrl->hsi, compat_vsp_ctrl.hsi);
		if (ercd)
			return ercd;
		ctrl->ctrl.hsi = &ctrl->hsi;
	}

	/* copy vsp_bru_t parameter */
	if (compat_vsp_ctrl.bru) {
		ercd = set_compat_vsp_bru_par(&ctrl->bru, compat_vsp_ctrl.bru);
		if (ercd)
			return ercd;
		ctrl->ctrl.bru = &ctrl->bru.bru;
	}

	/* copy vsp_hgo_t parameter */
	if (compat_vsp_ctrl.hgo) {
		ercd = set_compat_vsp_hgo_par(
			&ctrl->hgo, compat_vsp_ctrl.hgo, work_buff);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgo = &ctrl->hgo.hgo;
	}

	/* copy vsp_hgt_t parameter */
	if (compat_vsp_ctrl.hgt) {
		ercd = set_compat_vsp_hgt_par(
			&ctrl->hgt, compat_vsp_ctrl.hgt, work_buff);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgt = &ctrl->hgt.hgt;
	}

	/* copy vsp_shp_t parameter */
	if (compat_vsp_ctrl.shp) {
		ercd = set_compat_vsp_shp_par(&ctrl->shp, compat_vsp_ctrl.shp);
		if (ercd)
			return ercd;
		ctrl->ctrl.shp = &ctrl->shp;
	}

	return 0;
}

int set_compat_vsp_par(
	struct vspm_if_entry_data_t *entry, unsigned int src)
{
	struct vspm_entry_vsp *vsp = &entry->ip_par.vsp;
	struct compat_vsp_start_t compat_vsp_par;
	unsigned long tmp_addr;

	int ercd;

	int i;

	/* copy vsp_start_t parameter */
	if (copy_from_user(
			&compat_vsp_par,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_vsp_start_t))) {
		EPRINT("failed to copy of vsp_start_t\n");
		return -EFAULT;
	}

	vsp->par.rpf_num = compat_vsp_par.rpf_num;
	vsp->par.rpf_order = 0;	/* not used */
	vsp->par.use_module = (unsigned long)compat_vsp_par.use_module;

	/* get work buffer */
	vsp->work_buff = get_work_buffer(entry->priv);
	if (!vsp->work_buff)
		return -EFAULT;

	/* copy vsp_src_t parameter */
	for (i = 0; i < 5; i++) {
		if (compat_vsp_par.src_par[i]) {
			ercd = set_compat_vsp_src_par(
				&vsp->in[i],
				compat_vsp_par.src_par[i],
				vsp->work_buff);
			if (ercd)
				goto err_exit;
			vsp->par.src_par[i] = &vsp->in[i].in;
		}
	}

	/* copy vsp_dst_t parameter */
	if (compat_vsp_par.dst_par) {
		ercd = set_compat_vsp_dst_par(
			&vsp->out, compat_vsp_par.dst_par);
		if (ercd)
			goto err_exit;
		vsp->par.dst_par = &vsp->out.out;
	}

	/* copy vsp_ctrl_t parameter */
	if (compat_vsp_par.ctrl_par) {
		ercd = set_compat_vsp_ctrl_par(
			&vsp->ctrl, compat_vsp_par.ctrl_par, vsp->work_buff);
		if (ercd)
			goto err_exit;
		vsp->par.ctrl_par = &vsp->ctrl.ctrl;
	}

	/* assign memory for display list */
	tmp_addr =
		(unsigned long)vsp->work_buff->hard_addr +
		(unsigned long)vsp->work_buff->offset;
	vsp->par.dl_par.hard_addr = (unsigned int)tmp_addr;
	tmp_addr =
		(unsigned long)vsp->work_buff->virt_addr +
		(unsigned long)vsp->work_buff->offset;
	vsp->par.dl_par.virt_addr = (void *)tmp_addr;
	vsp->par.dl_par.tbl_num =
		(VSPM_IF_MEM_SIZE - vsp->work_buff->offset) >> 3;

	return 0;

err_exit:
	free_vsp_par(vsp);
	return ercd;
}

/*ISU 32bits */

static int set_compat_isu_alpha_par(
	struct isu_alpha_unit_t *alpha, unsigned int src)
{
	struct compat_isu_alpha_unit_t compat_alpha;

	/* copy isu_alpha_unit_t parameter */
	if (copy_from_user(
		&compat_alpha,
		VSPM_IF_INT_TO_UP(src),
		sizeof(struct compat_isu_alpha_unit_t))) {
		EPRINT("failed to copy of isu_alpha_unit_t\n");
		return -EFAULT;
	}

	alpha->asel 	= compat_alpha.asel;
	alpha->anum0 	= compat_alpha.anum0;
	alpha->anum1 	= compat_alpha.anum1;
	alpha->anum2 	= compat_alpha.anum2;
	alpha->athres0 	= compat_alpha.athres0;
	alpha->athres1 	= compat_alpha.athres1;

	return 0;
}

static int set_compat_isu_src_td_par(
	struct isu_td_unit_t *td, unsigned int src)
{
	struct compat_isu_td_unit_t compat_td;

	/* copy isu_alpha_unit_t parameter */
	if (copy_from_user(
		&compat_td,
		VSPM_IF_INT_TO_UP(src),
		sizeof(struct compat_isu_td_unit_t))) {
		EPRINT("failed to copy of isu_td_unit_t\n");
		return -EFAULT;
	}

	td->grada_mode 	= compat_td.grada_mode;
	td->grada_step	= compat_td.grada_step;
	td->init_val 	= compat_td.init_val;

	return 0;
}

static int set_compat_isu_csc_par(struct isu_csc_t *csc, unsigned int src)
{
	struct compat_isu_csc_t compat_csc;
	int i,j;
	/* copy isu_csc_t parameter */
	if (copy_from_user(
		&compat_csc,
		VSPM_IF_INT_TO_UP(src),
		sizeof(struct compat_isu_csc_t))) {
		EPRINT("failed to copy of isu_td_unit_t\n");
                return -EFAULT;
	}
	csc->csc     = compat_csc.csc;
	for (i=0;i<3;i++){
		for(j=0;j<3;j++){
			if(j<2){
				csc->clip[i][j]=compat_csc.clip[i][j];
				csc->offset[i][j]=compat_csc.offset[i][j];
			}
			csc->k_matrix[i][j]=compat_csc.k_matrix[i][j];
		}
	}
	return 0;
}

static int set_compat_isu_src_par(
	struct vspm_entry_isu_in *in,
	unsigned int src,
	struct vspm_if_work_buff_t *work_buff)
{
	struct compat_isu_src_t compat_isu_src;
	int ercd;

	/* copy isu_src_t parameter */
	if (copy_from_user(
			&compat_isu_src,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_isu_src_t))) {
		EPRINT("failed to copy of isu_src_t\n");
		return -EFAULT;
	}

	in->in.addr 	= compat_isu_src.addr;
	in->in.addr_c 	= compat_isu_src.addr_c;
	in->in.stride 	= compat_isu_src.stride;
	in->in.stride_c = compat_isu_src.stride_c;
	in->in.width 	= compat_isu_src.width;
	in->in.height 	= compat_isu_src.height;
	in->in.format 	= compat_isu_src.format;
	in->in.swap 	= compat_isu_src.swap;
	in->in.uv_conv 	= compat_isu_src.uv_conv;

	/* copy isu_alpha_unit_t parameter */
	if (compat_isu_src.alpha) {
		ercd = set_compat_isu_alpha_par(
                        &in->alpha, compat_isu_src.alpha);
                if (ercd)
                        return ercd;
                in->in.alpha = &in->alpha;
	}
	/* copy isu_td_unit_t parameter */
	if (compat_isu_src.td){
		ercd = set_compat_isu_src_td_par(
			&in->td, compat_isu_src.td);
		if (ercd)
			return ercd;
		in->in.td = &in->td;
	}

	return 0;
}

static int set_compat_isu_dst_par(
	struct vspm_entry_isu_out *out, unsigned int src)
{
	struct compat_isu_dst_t compat_isu_dst;
	int ercd;
	/* copy isu_dst_t parameter */
	if (copy_from_user(
		&compat_isu_dst,
		VSPM_IF_INT_TO_UP(src),
		sizeof(struct compat_isu_dst_t))) {
		EPRINT("failed to copy of isu_dst_t\n");
		return -EFAULT;
	}

	out->out.addr 		= compat_isu_dst.addr;
	out->out.addr_c 	= compat_isu_dst.addr_c;
	out->out.stride 	= compat_isu_dst.stride;
	out->out.stride_c 	= compat_isu_dst.stride_c;
	out->out.format 	= compat_isu_dst.format;
	out->out.swap 		= compat_isu_dst.swap;
	/* copy isu_csc_t parameter */
	if (compat_isu_dst.csc){
		ercd = set_compat_isu_csc_par(
			&out->csc,compat_isu_dst.csc);
		if (ercd)
			return ercd;
		out->out.csc = &out->csc;
	}

	/* copy isu_alpha_unit_t parameter */
	if (compat_isu_dst.alpha) {
		ercd = set_compat_isu_alpha_par(
                        &out->alpha, compat_isu_dst.alpha);
                if (ercd)
                        return ercd;
                out->out.alpha = &out->alpha;
	}
	return 0;
}

static int set_compat_isu_rs_par(struct isu_rs_t *rs, unsigned int src)
{
	struct compat_isu_rs_t compat_rs;

	/* copy */
	if (copy_from_user(
		&compat_rs,
		VSPM_IF_INT_TO_UP(src),
		sizeof(struct compat_isu_rs_t))) {
			EPRINT("failed to copy of isu_rs_t\n");
			return -EFAULT;
	}

	/* set */
	rs->start_x 	= compat_rs.start_x;
	rs->start_y 	= compat_rs.start_y;
	rs->tune_x 	= compat_rs.tune_x;
	rs->tune_y 	= compat_rs.tune_y;
	rs->crop_w 	= compat_rs.crop_w;
	rs->crop_h 	= compat_rs.crop_h;
	rs->pad_mode 	= compat_rs.pad_mode;
	rs->pad_val 	= compat_rs.pad_val;
	rs->x_ratio 	= compat_rs.x_ratio;
	rs->y_ratio 	= compat_rs.y_ratio;

	return 0;
}

int set_compat_isu_par(
	struct vspm_if_entry_data_t *entry, unsigned int src)
{
	struct vspm_entry_isu *isu = &entry->ip_par.isu;
	struct compat_isu_start_t compat_isu_par;

	int ercd;

	/* copy isu_start_t parameter */
	if (copy_from_user(
			&compat_isu_par,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_isu_start_t))) {
		EPRINT("failed to copy of vsp_start_t\n");
		return -EFAULT;
	}

	/* copy isu_src_t parameter */
	if (compat_isu_par.src_par) {
		ercd = set_compat_isu_src_par(
			&isu->in,
			compat_isu_par.src_par,
			isu->work_buff);
		if (ercd)
			goto err_exit;
		isu->par.src_par = &isu->in.in;
	}

	/* copy isu_dst_t parameter */
	if (compat_isu_par.dst_par) {
		ercd = set_compat_isu_dst_par(
			&isu->out, compat_isu_par.dst_par);
		if (ercd)
			goto err_exit;
		isu->par.dst_par = &isu->out.out;
	}

	/* copy isu_rs_t parameter */
	if (compat_isu_par.rs_par) {
		ercd = set_compat_isu_rs_par(
			&isu->rs, compat_isu_par.rs_par);
		if (ercd)
			goto err_exit;
		isu->par.rs_par = &isu->rs;
	}

	return 0;
err_exit:
	return ercd;
}



static int set_compat_fdp_pic_par(struct fdp_pic_t *in_pic, unsigned int src)
{
	struct compat_fdp_pic_t compat_fdp_pic;

	/* copy */
	if (copy_from_user(
			&compat_fdp_pic,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_fdp_pic_t))) {
		EPRINT("failed to copy of fdp_pic_t\n");
		return -EFAULT;
	}

	/* set */
	in_pic->picid = (unsigned long)compat_fdp_pic.picid;
	in_pic->chroma_format = compat_fdp_pic.chroma_format;
	in_pic->width = compat_fdp_pic.width;
	in_pic->height = compat_fdp_pic.height;
	in_pic->progressive_sequence = compat_fdp_pic.progressive_sequence;
	in_pic->progressive_frame = compat_fdp_pic.progressive_frame;
	in_pic->picture_structure = compat_fdp_pic.picture_structure;
	in_pic->repeat_first_field = compat_fdp_pic.repeat_first_field;
	in_pic->top_field_first = compat_fdp_pic.top_field_first;

	return 0;
}

static int set_compat_fdp_ref_par(
	struct vspm_entry_fdp_ref *ref, unsigned int src)
{
	struct compat_fdp_refbuf_t compat_fdp_refbuf;

	/* copy fdp_refbuf_t parameter */
	if (copy_from_user(
			&compat_fdp_refbuf,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_fdp_refbuf_t))) {
		EPRINT("failed to copy of fdp_refbuf_t\n");
		return -EFAULT;
	}

	if (compat_fdp_refbuf.next_buf) {
		if (copy_from_user(
				&ref->ref[0],
				VSPM_IF_INT_TO_UP(compat_fdp_refbuf.next_buf),
				sizeof(struct fdp_imgbuf_t))) {
			EPRINT("failed to copy to fdp_imgbuf_t\n");
			return -EFAULT;
		}
		ref->ref_buf.next_buf = &ref->ref[0];
	}

	if (compat_fdp_refbuf.cur_buf) {
		if (copy_from_user(
				&ref->ref[1],
				VSPM_IF_INT_TO_UP(compat_fdp_refbuf.cur_buf),
				sizeof(struct fdp_imgbuf_t))) {
			EPRINT("failed to copy to fdp_imgbuf_t\n");
			return -EFAULT;
		}
		ref->ref_buf.cur_buf = &ref->ref[1];
	}

	if (compat_fdp_refbuf.prev_buf) {
		if (copy_from_user(
				&ref->ref[2],
				VSPM_IF_INT_TO_UP(compat_fdp_refbuf.prev_buf),
				sizeof(struct fdp_imgbuf_t))) {
			EPRINT("failed to copy to fdp_imgbuf_t\n");
			return -EFAULT;
		}
		ref->ref_buf.prev_buf = &ref->ref[2];
	}

	return 0;
}

static int set_compat_fdp_fproc_par(
	struct vspm_entry_fdp_fproc *fproc, unsigned int src)
{
	struct compat_fdp_fproc_t compat_fdp_fproc;
	int ercd;

	/* copy fdp_fproc_t parameter */
	if (copy_from_user(
			&compat_fdp_fproc,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_fdp_fproc_t))) {
		EPRINT("failed to copy of fdp_fproc_t\n");
		return -EFAULT;
	}

	fproc->fproc.last_seq_indicator = compat_fdp_fproc.last_seq_indicator;
	fproc->fproc.current_field = compat_fdp_fproc.current_field;
	fproc->fproc.interpolated_line = compat_fdp_fproc.interpolated_line;
	fproc->fproc.out_format = compat_fdp_fproc.out_format;

	/* copy fdp_seq_t parameter */
	if (compat_fdp_fproc.seq_par) {
		if (copy_from_user(
				&fproc->seq,
				VSPM_IF_INT_TO_UP(compat_fdp_fproc.seq_par),
				sizeof(struct fdp_seq_t))) {
			EPRINT("failed to copy to fdp_seq_t\n");
			return -EFAULT;
		}
		fproc->fproc.seq_par = &fproc->seq;
	}

	/* copy fdp_pic_t parameter */
	if (compat_fdp_fproc.in_pic) {
		ercd = set_compat_fdp_pic_par(
			&fproc->in_pic, compat_fdp_fproc.in_pic);
		if (ercd)
			return ercd;
		fproc->fproc.in_pic = &fproc->in_pic;
	}

	/* copy fdp_imgbuf_t parameter */
	if (compat_fdp_fproc.out_buf) {
		if (copy_from_user(
				&fproc->out_buf,
				VSPM_IF_INT_TO_UP(compat_fdp_fproc.out_buf),
				sizeof(struct fdp_imgbuf_t))) {
			EPRINT("failed to copy to fdp_imgbuf_t\n");
			return -EFAULT;
		}
		fproc->fproc.out_buf = &fproc->out_buf;
	}

	/* copy fdp_refbuf_t parameter */
	if (compat_fdp_fproc.ref_buf) {
		ercd = set_compat_fdp_ref_par(
			&fproc->ref, compat_fdp_fproc.ref_buf);
		if (ercd)
			return ercd;
		fproc->fproc.ref_buf = &fproc->ref.ref_buf;
	}

	/* copy fcp_info_t parameter */
	if (compat_fdp_fproc.fcp_par) {
		if (copy_from_user(
				&fproc->fcp,
				VSPM_IF_INT_TO_UP(compat_fdp_fproc.fcp_par),
				sizeof(struct fcp_info_t))) {
			EPRINT("failed to copy to fcp_info_t\n");
			return -EFAULT;
		}
		fproc->fproc.fcp_par = &fproc->fcp;
	}

	/* copy fdp_ipc_t parameter */
	if (compat_fdp_fproc.ipc_par) {
		if (copy_from_user(
				&fproc->ipc,
				VSPM_IF_INT_TO_UP(compat_fdp_fproc.ipc_par),
				sizeof(struct fdp_ipc_t))) {
			EPRINT("failed to copy to fdp_ipc_t\n");
			return -EFAULT;
		}
		fproc->fproc.ipc_par = &fproc->ipc;
	}

	return 0;
}

int set_compat_fdp_par(
	struct vspm_if_entry_data_t *entry, unsigned int src)
{
	struct vspm_entry_fdp *fdp = &entry->ip_par.fdp;
	struct compat_fdp_start_t compat_fdp_par;
	int ercd;

	/* copy fdp_start_t parameter */
	if (copy_from_user(
			&compat_fdp_par,
			VSPM_IF_INT_TO_UP(src),
			sizeof(struct compat_fdp_start_t))) {
		EPRINT("failed to copy of fdp_start_t\n");
		return -EFAULT;
	}

	fdp->par.fdpgo = compat_fdp_par.fdpgo;

	/* copy fdp_fproc_t parameter */
	if (compat_fdp_par.fproc_par) {
		ercd = set_compat_fdp_fproc_par(
			&fdp->fproc, compat_fdp_par.fproc_par);
		if (ercd)
			return ercd;
		fdp->par.fproc_par = &fdp->fproc.fproc;
	}

	return 0;
}
