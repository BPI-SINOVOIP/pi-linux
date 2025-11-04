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
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/ioctl.h>

#include "vspm_public.h"
#include "vspm_if.h"
#include "vspm_if_local.h"

struct platform_device *g_vspmif_pdev;

static int open(struct inode *inode, struct file *file)
{
	struct vspm_if_private_t *priv;

	/* allocate memory */
	priv = kzalloc(sizeof(struct vspm_if_private_t), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* init */
	spin_lock_init(&priv->lock);
	init_completion(&priv->wait_interrupt);
	init_completion(&priv->wait_thread);
	INIT_LIST_HEAD(&priv->entry_data.list);
	INIT_LIST_HEAD(&priv->cb_data.list);
	sema_init(&priv->sem, 1);

	file->private_data = priv;
	return 0;
}

static int close(struct inode *inode, struct file *file)
{
	struct vspm_if_private_t *priv =
		(struct vspm_if_private_t *)file->private_data;

	if (priv) {
		if (priv->handle) {
			(void)vspm_quit_driver(priv->handle);
			priv->handle = NULL;
		}

		/* release entry data */
		release_all_entry_data(priv);

		/* release callback data */
		release_all_cb_data(priv);

		/* release work buffer */
		release_work_buffers(priv);

		/* release memory */
		kfree(priv);
	}

	file->private_data = NULL;
	return 0;
}

static long vspm_ioctl_init(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	struct vspm_init_t init_par;
	struct vspm_init_fdp_t init_fdp_par;

	void *handle;
	long ercd;

	/* copy initialize parameter */
	if (copy_from_user(
			&init_par, (void __user *)arg, _IOC_SIZE(cmd))) {
		EPRINT("INIT: failed to copy from user!!\n");
		return -EFAULT;
	}

	switch (init_par.type) {
	case VSPM_TYPE_VSP_AUTO:
		/* initialize parameter is not used */
		init_par.par.vsp = NULL;
		break;
	case VSPM_TYPE_FDP_AUTO:
		/* copy initialize parameter of FDP */
		if (init_par.par.fdp) {
			if (copy_from_user(
					&init_fdp_par,
					(void __user *)init_par.par.fdp,
					sizeof(struct vspm_init_fdp_t))) {
				EPRINT("INIT: failed to copy FDP param\n");
				return -EFAULT;
			}
			init_par.par.fdp = &init_fdp_par;
		}
		break;
	case VSPM_TYPE_ISU_AUTO:
		/* initialize parameter is not used */
		init_par.par.isu=NULL;
		break;
	default:
		break;
	}

	/* initialize VSP manager */
	ercd = vspm_init_driver(&handle, &init_par);
	switch (ercd) {
	case R_VSPM_OK:
		break;
	case R_VSPM_PARAERR:
		return -EINVAL;
	case R_VSPM_ALREADY_USED:
		return -EBUSY;
	default:
		return -EFAULT;
	}
	priv->handle = handle;
	return 0;
}

static long vspm_ioctl_quit(struct vspm_if_private_t *priv)
{
	long ercd;

	/* finalize VSP manager */
	ercd = vspm_quit_driver(priv->handle);
	if (ercd != R_VSPM_OK)
		return -EFAULT;

	priv->handle = NULL;

	/* release entry data */
	release_all_entry_data(priv);

	return 0;
}

static void vspm_cb_func(
	unsigned long job_id, long result, void *user_data)
{
	struct vspm_if_entry_data_t *entry_data =
		(struct vspm_if_entry_data_t *)user_data;

	struct vspm_if_private_t *priv;
	struct vspm_if_cb_data_t *cb_data;
	unsigned long lock_flag;

	if (!entry_data)
		return;

	priv = entry_data->priv;

	/* del list */
	spin_lock_irqsave(&priv->lock, lock_flag);
	list_del(&entry_data->list);
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	/* allocate callback data */
	cb_data = kzalloc(sizeof(struct vspm_if_cb_data_t), GFP_ATOMIC);
	if (!cb_data) {
		EPRINT("CB: failed to allocate memory\n");
		/* release memory */
		if (entry_data->job.type == VSPM_TYPE_VSP_AUTO)
			free_vsp_par(&entry_data->ip_par.vsp);
		kfree(entry_data);
		return;
	}

	/* make response data */
	cb_data->rsp.ercd = 0;
	cb_data->rsp.cb_func = entry_data->entry.req.cb_func;
	cb_data->rsp.job_id = job_id;
	cb_data->rsp.result = result;
	cb_data->rsp.user_data = entry_data->entry.req.user_data;

	if (entry_data->job.type == VSPM_TYPE_VSP_AUTO) {
		/* set callback response of vsp */
		set_cb_rsp_vsp(cb_data, entry_data);
	}

	/* addition list */
	spin_lock_irqsave(&priv->lock, lock_flag);
	list_add_tail(&cb_data->list, &priv->cb_data.list);
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	complete(&priv->wait_interrupt);
	kfree(entry_data);
}

static long vspm_ioctl_entry(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	struct vspm_if_entry_data_t *entry_data;
	struct vspm_if_entry_req_t *entry_req;
	struct vspm_if_entry_t entry;

	unsigned long lock_flag;
	int ercd = 0;

	/* allocate entry data */
	entry_data = kzalloc(sizeof(struct vspm_if_entry_data_t), GFP_KERNEL);
	if (!entry_data)
		return -ENOMEM;
	entry_data->priv = priv;

	/* add list */
	spin_lock_irqsave(&priv->lock, lock_flag);
	list_add_tail(&entry_data->list, &priv->entry_data.list);
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	/* copy entry parameter */
	if (copy_from_user(
			&entry_data->entry,
			(void __user *)arg,
			_IOC_SIZE(cmd))) {
		EPRINT("ENTRY: failed to copy the entry parameter\n");
		ercd = -EFAULT;
		goto err_exit;
	}

	entry_req = &entry_data->entry.req;

	if (entry_req->job_param) {
		/* copy job parameter */
		if (copy_from_user(
				&entry_data->job,
				(void __user *)entry_req->job_param,
				sizeof(struct vspm_job_t))) {
			EPRINT("ENTRY: failed to copy the job parameter\n");
			ercd = -EFAULT;
			goto err_exit;
		}
		entry_req->job_param = &entry_data->job;

		switch (entry_data->job.type) {
		case VSPM_TYPE_VSP_AUTO:
			if (entry_data->job.par.vsp) {
				/* copy start parameter of VSP */
				ercd = set_vsp_par(
					entry_data, entry_data->job.par.vsp);
				if (ercd)
					goto err_exit;

				entry_req->job_param->par.vsp =
					&entry_data->ip_par.vsp.par;
			}
			break;
		case VSPM_TYPE_FDP_AUTO:
			if (entry_data->job.par.fdp) {
				/* copy start parameter of FDP */
				ercd = set_fdp_par(
					entry_data, entry_data->job.par.fdp);
				if (ercd)
					goto err_exit;

				entry_req->job_param->par.fdp =
					&entry_data->ip_par.fdp.par;
			}
			break;
		case VSPM_TYPE_ISU_AUTO:
			if (entry_data->job.par.isu) {
				/* copy start parameter of ISU */
				ercd = set_isu_par(
					entry_data, entry_data->job.par.isu);
				if (ercd)
					goto err_exit;

				entry_req->job_param->par.isu =
					&entry_data->ip_par.isu.par;
			}
			break;
		default:
			break;
		}
	}

	/* entry job */
	entry.rsp.ercd = vspm_entry_job(
		priv->handle,
		&entry.rsp.job_id,
		entry_req->priority,
		entry_req->job_param,
		(void *)entry_data,
		vspm_cb_func);

	/* copy result to user */
	if (copy_to_user(
			(void __user *)arg, &entry, _IOC_SIZE(cmd)))
		APRINT("ENTRY: failed to copy the result\n");

	if (entry.rsp.ercd != R_VSPM_OK)
		goto err_exit;

	return 0;

err_exit:
	spin_lock_irqsave(&priv->lock, lock_flag);
	list_del(&entry_data->list);
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	if (entry_data->job.type == VSPM_TYPE_VSP_AUTO)
		free_vsp_par(&entry_data->ip_par.vsp);
	kfree(entry_data);

	return ercd;
}

static long vspm_ioctl_cancel(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	unsigned long job_id = 0;
	long ercd;

	/* copy cancel parameter */
	if (copy_from_user(&job_id, (void __user *)arg, _IOC_SIZE(cmd))) {
		EPRINT("CANCEL: failed to copy the request data\n");
		return -EFAULT;
	}

	/* cancel job */
	ercd = vspm_cancel_job(priv->handle, job_id);
	switch (ercd) {
	case R_VSPM_OK:
		break;
	case VSPM_STATUS_ACTIVE:
		return -EBUSY;
	case VSPM_STATUS_NO_ENTRY:
		return -ENOENT;
	default:
		return -EFAULT;
	}

	return 0;
}

static long vspm_ioctl_get_status(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	struct vspm_status_t status;
	struct fdp_status_t fdp_status;

	long ercd;

	struct vspm_status_t user_status;

	/* copy status parameter from user */
	if (copy_from_user(&user_status, (void __user *)arg, _IOC_SIZE(cmd))) {
		EPRINT("GET: failed to copy from user\n");
		return -EFAULT;
	}

	status.fdp = &fdp_status;

	/* get a status */
	ercd = vspm_get_status(priv->handle, &status);
	switch (ercd) {
	case R_VSPM_OK:
		/* copy status parameter to user */
		if (copy_to_user(
				(void __user *)user_status.fdp,
				&fdp_status,
				sizeof(struct fdp_status_t))) {
			EPRINT("GET: failed to copy to user\n");
			return -EFAULT;
		}
		break;
	case R_VSPM_PARAERR:
		return -EINVAL;
	default:
		return -EFAULT;
	}

	return 0;
}

static long vspm_ioctl_wait_interrupt(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	struct vspm_if_cb_data_t *cb_data;
	unsigned long lock_flag;
	long ercd = 0;

	/* get user process information */
	priv->thread = current;
	complete(&priv->wait_thread);

	/* wait process end */
	if (wait_for_completion_interruptible(&priv->wait_interrupt))
		return -EINTR;

	/* get response data */
	spin_lock_irqsave(&priv->lock, lock_flag);
	if (list_empty(&priv->cb_data.list)) {
		cb_data = NULL;
	} else {
		cb_data = list_first_entry(
			&priv->cb_data.list, struct vspm_if_cb_data_t, list);
		list_del(&cb_data->list);
	}
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	if (!cb_data) {
		struct vspm_if_cb_rsp_t rsp;

		/* set response data (ercd = -1) */
		memset(&rsp, 0, sizeof(struct vspm_if_cb_rsp_t));
		rsp.ercd = -1;

		/* copy response data to user */
		if (copy_to_user((void __user *)arg, &rsp, _IOC_SIZE(cmd))) {
			EPRINT("CB: failed to copy the response\n");
			return -EFAULT;
		}
	} else {
		/* HGO result */
		if (cb_data->vsp_hgo.virt_addr) {
			unsigned long tmp_addr =
				(unsigned long)(cb_data->vsp_hgo.virt_addr);
			tmp_addr = (tmp_addr + 255) >> 8;
			/* copy to user area */
			if (cb_data->vsp_hgo.user_addr) {
				if (copy_to_user((void __user *)
						cb_data->vsp_hgo.user_addr,
						(void *)(tmp_addr << 8),
						1088)) {
					APRINT("CB: failed to copy HGO data\n");
				}
			}
		}

		/* HGT result */
		if (cb_data->vsp_hgt.virt_addr) {
			unsigned long tmp_addr =
				(unsigned long)(cb_data->vsp_hgt.virt_addr);
			tmp_addr = (tmp_addr + 255) >> 8;
			/* copy to user area */
			if (cb_data->vsp_hgt.user_addr) {
				if (copy_to_user((void __user *)
						cb_data->vsp_hgt.user_addr,
						(void *)(tmp_addr << 8),
						800)) {
					APRINT("CB: failed to copy HGT data\n");
				}
			}
		}

		/* copy response data to user */
		if (copy_to_user(
				(void __user *)arg,
				&cb_data->rsp,
				_IOC_SIZE(cmd))) {
			EPRINT("CB: failed to copy the response\n");
			ercd = -EFAULT;
		}

		/* release memory */
		free_cb_vsp_par(cb_data);
		kfree(cb_data);
	}

	return ercd;
}

static long vspm_ioctl_wait_thread(struct vspm_if_private_t *priv)
{
	/* wait for callback thread of user */
	if (wait_for_completion_interruptible(&priv->wait_thread)) {
		APRINT("CB_START: INTR\n");
		return -EINTR;
	}

	/* wait for running state of callback thread  */
	while ((!priv->thread) || task_is_running(priv->thread))
		schedule();

	return 0;
}

static long vspm_ioctl_stop_thread(struct vspm_if_private_t *priv)
{
	/* release callback data */
	release_all_cb_data(priv);

	complete(&priv->wait_interrupt);

	return 0;
}

static long unlocked_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	struct vspm_if_private_t *priv =
		(struct vspm_if_private_t *)file->private_data;

	long ercd = 0;

	/* check parameter */
	if (!priv) {
		EPRINT("IOCTL: invalid private data!!\n");
		return -EFAULT;
	}

	switch (cmd) {
	case VSPM_IOC_CMD_INIT:
		ercd = vspm_ioctl_init(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_QUIT:
		ercd = vspm_ioctl_quit(priv);
		break;
	case VSPM_IOC_CMD_ENTRY:
		ercd = vspm_ioctl_entry(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_CANCEL:
		ercd = vspm_ioctl_cancel(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_GET_STATUS:
		ercd = vspm_ioctl_get_status(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_WAIT_INTERRUPT:
		ercd = vspm_ioctl_wait_interrupt(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_WAIT_THREAD:
		ercd = vspm_ioctl_wait_thread(priv);
		break;
	case VSPM_IOC_CMD_STOP_THREAD:
		ercd = vspm_ioctl_stop_thread(priv);
		break;
	default:
		ercd = -ENOTTY;
		break;
	}

	return ercd;
}

static long vspm_ioctl_init32(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	/* for 64bit */
	struct vspm_init_t init_par;
	struct vspm_init_fdp_t init_fdp_par;

	void *handle;
	long ercd;

	/* for 32bit */
	struct vspm_compat_init_t compat_init_par;

	/* copy initialize parameter */
	if (copy_from_user(
			&compat_init_par, (void __user *)arg, _IOC_SIZE(cmd))) {
		EPRINT("INIT32: failed to copy from user!!\n");
		return -EFAULT;
	}
	init_par.use_ch = compat_init_par.use_ch;
	init_par.mode = compat_init_par.mode;
	init_par.type = compat_init_par.type;

	switch (init_par.type) {
	case VSPM_TYPE_VSP_AUTO:
		/* initialize parameter is not used */
		init_par.par.vsp = NULL;
		break;
	case VSPM_TYPE_FDP_AUTO:
		/* copy initialize parameter of FDP */
		if (compat_init_par.par.fdp != 0) {
			if (copy_from_user(
					&init_fdp_par,
				    VSPM_IF_INT_TO_UP(compat_init_par.par.fdp),
				    sizeof(struct vspm_init_fdp_t))) {
				EPRINT("INIT32: failed to copy FDP param\n");
				return -EFAULT;
			}
			init_par.par.fdp = &init_fdp_par;
		} else {
			init_par.par.fdp = NULL;
		}
		break;
	case VSPM_TYPE_ISU_AUTO:
		/* initialize parameter is not used */
		init_par.par.vsp = NULL;
		break;
	default:
		break;
	}

	/* initialize VSP manager */
	ercd = vspm_init_driver(&handle, &init_par);
	switch (ercd) {
	case R_VSPM_OK:
		break;
	case R_VSPM_PARAERR:
		return -EINVAL;
	case R_VSPM_ALREADY_USED:
		return -EBUSY;
	default:
		return -EFAULT;
	}

	priv->handle = handle;
	return 0;
}

static long vspm_ioctl_entry32(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	/* for 64bit */
	struct vspm_if_entry_data_t *entry_data;
	struct vspm_if_entry_req_t *entry_req;
	struct vspm_if_entry_rsp_t entry_rsp;

	/* for 32bit */
	struct vspm_compat_entry_t compat_entry;
	struct vspm_compat_job_t compat_job;
	struct vspm_compat_entry_req_t *compat_req = &compat_entry.req;
	struct vspm_compat_entry_rsp_t *compat_rsp = &compat_entry.rsp;

	unsigned long lock_flag;
	int ercd = 0;

	/* allocate entry data */
	entry_data = kzalloc(sizeof(struct vspm_if_entry_data_t), GFP_KERNEL);
	if (!entry_data)
		return -ENOMEM;
	entry_data->priv = priv;

	/* add list */
	spin_lock_irqsave(&priv->lock, lock_flag);
	list_add_tail(&entry_data->list, &priv->entry_data.list);
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	entry_req = &entry_data->entry.req;

	/* copy entry parameter */
	if (copy_from_user(
			&compat_entry, (void __user *)arg, _IOC_SIZE(cmd))) {
		EPRINT("ENTRY32: failed to copy the entry parameter\n");
		ercd = -EFAULT;
		goto err_exit;
	}

	entry_req->priority = compat_req->priority;
	entry_req->user_data = VSPM_IF_INT_TO_UP(compat_req->user_data);
	entry_req->cb_func = VSPM_IF_INT_TO_UP(compat_req->cb_func);

	if (compat_req->job_param != 0) {
		/* copy job parameter */
		if (copy_from_user(
				&compat_job,
				VSPM_IF_INT_TO_UP(compat_req->job_param),
				sizeof(struct vspm_compat_job_t))) {
			EPRINT("ENTRY32: failed to copy the job parameter\n");
			ercd = -EFAULT;
			goto err_exit;
		}
		entry_data->job.type = compat_job.type;

		switch (compat_job.type) {
		case VSPM_TYPE_VSP_AUTO:
			/* copy start parameter of VSP */
			if (compat_job.par.vsp) {
				ercd = set_compat_vsp_par(
					entry_data, compat_job.par.vsp);
				if (ercd)
					goto err_exit;

				entry_data->job.par.vsp =
					&entry_data->ip_par.vsp.par;
			}
			break;
		case VSPM_TYPE_FDP_AUTO:
			/* copy start parameter of FDP */
			if (compat_job.par.fdp) {
				ercd = set_compat_fdp_par(
					entry_data, compat_job.par.fdp);
				if (ercd)
					goto err_exit;

				entry_data->job.par.fdp =
					&entry_data->ip_par.fdp.par;
			}
			break;
		case VSPM_TYPE_ISU_AUTO:
			/* copy start parameter of VSP */
			if (compat_job.par.isu) {
				ercd = set_compat_isu_par(
				entry_data, compat_job.par.isu);
				if (ercd)
					goto err_exit;

				entry_data->job.par.isu =
					&entry_data->ip_par.isu.par;
			}
			break;
		default:
			break;
		}

		entry_req->job_param = &entry_data->job;
	}

	/* entry job */
	entry_rsp.ercd = vspm_entry_job(
		priv->handle,
		&entry_rsp.job_id,
		entry_req->priority,
		entry_req->job_param,
		(void *)entry_data,
		vspm_cb_func);

	/* copy result to user */
	compat_rsp->ercd = (int)entry_rsp.ercd;
	compat_rsp->job_id = (unsigned int)entry_rsp.job_id;
	if (copy_to_user(
			(void __user *)arg, &compat_entry, _IOC_SIZE(cmd))) {
		APRINT("ENTRY32: failed to copy the result\n");
	}

	if (entry_rsp.ercd != R_VSPM_OK)
		goto err_exit;

	return 0;

err_exit:
	spin_lock_irqsave(&priv->lock, lock_flag);
	list_del(&entry_data->list);
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	if (entry_data->job.type == VSPM_TYPE_VSP_AUTO)
		free_vsp_par(&entry_data->ip_par.vsp);
	kfree(entry_data);

	return ercd;
}

static long vspm_ioctl_get_status32(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	/* for 64bit */
	struct vspm_status_t status;
	struct fdp_status_t fdp_status;

	long ercd;

	/* for 32bit */
	struct vspm_compat_status_t compat_status;
	struct compat_fdp_status_t compat_fdp_status;

	int i;

	/* copy status parameter from user */
	if (copy_from_user(
			&compat_status, (void __user *)arg, _IOC_SIZE(cmd))) {
		EPRINT("GET32: failed to copy from user\n");
		return -EFAULT;
	}

	status.fdp = &fdp_status;

	/* get a status */
	ercd = vspm_get_status(priv->handle, &status);
	switch (ercd) {
	case R_VSPM_OK:
		/* convert parameter */
		compat_fdp_status.picid = (unsigned int)fdp_status.picid;
		compat_fdp_status.vcycle = fdp_status.vcycle;
		for (i = 0; i < 18; i++)
			compat_fdp_status.sensor[i] = fdp_status.sensor[i];

		/* copy status parameter to user */
		if (copy_to_user(
				VSPM_IF_INT_TO_UP(compat_status.fdp),
				&compat_fdp_status,
				sizeof(struct compat_fdp_status_t))) {
			EPRINT("GET32: failed to copy to user\n");
			return -EFAULT;
		}
		break;
	case R_VSPM_PARAERR:
		return -EINVAL;
	default:
		return -EFAULT;
	}

	return 0;
}

static long vspm_ioctl_wait_interrupt32(
	struct vspm_if_private_t *priv, unsigned int cmd, unsigned long arg)
{
	struct vspm_if_cb_data_t *cb_data;
	unsigned long lock_flag;
	long ercd = 0;

	/* for 32bit */
	struct vspm_compat_cb_rsp_t compat_rsp;

	/* get user process information */
	priv->thread = current;
	complete(&priv->wait_thread);

	/* wait process end */
	if (wait_for_completion_interruptible(
			&priv->wait_interrupt)) {
		return -EINTR;
	}

	/* get response data */
	spin_lock_irqsave(&priv->lock, lock_flag);
	if (list_empty(&priv->cb_data.list)) {
		cb_data = NULL;
	} else {
		cb_data = list_first_entry(
			&priv->cb_data.list, struct vspm_if_cb_data_t, list);
		list_del(&cb_data->list);
	}
	spin_unlock_irqrestore(&priv->lock, lock_flag);

	if (!cb_data) {
		/* set response data (ercd = -1) */
		memset(&compat_rsp, 0, sizeof(struct vspm_compat_cb_rsp_t));
		compat_rsp.ercd = -1;

		/* copy response data to user */
		if (copy_to_user(
				(void __user *)arg,
				&compat_rsp,
				_IOC_SIZE(cmd))) {
			EPRINT("CB32: failed to copy the response\n");
			return -EFAULT;
		}
	} else {
		/* HGO result */
		if (cb_data->vsp_hgo.virt_addr) {
			unsigned long tmp_addr =
				(unsigned long)(cb_data->vsp_hgo.virt_addr);
			tmp_addr = (tmp_addr + 255) >> 8;
			/* copy to user area */
			if (cb_data->vsp_hgo.user_addr) {
				if (copy_to_user((void __user *)
						cb_data->vsp_hgo.user_addr,
						(void *)(tmp_addr << 8),
						1088)) {
					APRINT(
						"CB32: failed to copy HGO data\n");
				}
			}
		}

		/* HGT result */
		if (cb_data->vsp_hgt.virt_addr) {
			unsigned long tmp_addr =
				(unsigned long)(cb_data->vsp_hgt.virt_addr);
			tmp_addr = (tmp_addr + 255) >> 8;
			/* copy to user area */
			if (cb_data->vsp_hgt.user_addr) {
				if (copy_to_user((void __user *)
						cb_data->vsp_hgt.user_addr,
						(void *)(tmp_addr << 8),
						800)) {
					APRINT(
						"CB32: failed to copy HGT data\n");
				}
			}
		}

		compat_rsp.ercd = (int)cb_data->rsp.ercd;
		compat_rsp.cb_func = VSPM_IF_UP_TO_INT(cb_data->rsp.cb_func);
		compat_rsp.job_id = (unsigned int)cb_data->rsp.job_id;
		compat_rsp.result = (int)cb_data->rsp.result;
		compat_rsp.user_data =
			(unsigned int)(unsigned long)cb_data->rsp.user_data;

		/* copy response data to user */
		if (copy_to_user(
				(void __user *)arg,
				&compat_rsp,
				_IOC_SIZE(cmd))) {
			EPRINT("CB32: failed to copy the response\n");
			ercd = -EFAULT;
		}

		/* release memory */
		free_cb_vsp_par(cb_data);
		kfree(cb_data);
	}

	return ercd;
}

static long compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct vspm_if_private_t *priv =
		(struct vspm_if_private_t *)file->private_data;

	long ercd = 0;

	/* check parameter */
	if (!priv) {
		EPRINT("IOCTL32: invalid private data!!\n");
		return -EFAULT;
	}

	switch (cmd) {
	case VSPM_IOC_CMD_INIT32:
		ercd = vspm_ioctl_init32(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_QUIT:
		ercd = vspm_ioctl_quit(priv);
		break;
	case VSPM_IOC_CMD_ENTRY32:
		ercd = vspm_ioctl_entry32(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_CANCEL32:
		ercd = vspm_ioctl_cancel(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_GET_STATUS32:
		ercd = vspm_ioctl_get_status32(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_WAIT_INTERRUPT32:
		ercd = vspm_ioctl_wait_interrupt32(priv, cmd, arg);
		break;
	case VSPM_IOC_CMD_WAIT_THREAD:
		ercd = vspm_ioctl_wait_thread(priv);
		break;
	case VSPM_IOC_CMD_STOP_THREAD:
		ercd = vspm_ioctl_stop_thread(priv);
		break;
	default:
		ercd = -ENOTTY;
		break;
	}

	return ercd;
}

static const struct file_operations fops = {
	.owner   = THIS_MODULE,
	.open    = open,
	.release = close,
	.unlocked_ioctl = unlocked_ioctl,
	.compat_ioctl = compat_ioctl,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVFILE,
	.fops = &fops
};

static int vspm_if_probe(struct platform_device *pdev)
{
	if (g_vspmif_pdev)
		return -1;

	g_vspmif_pdev = pdev;
	return 0;
}

static int vspm_if_remove(struct platform_device *pdev)
{
	g_vspmif_pdev = NULL;
	return 0;
}

static const struct of_device_id vspm_if_of_match[] = {
	{ .compatible = "renesas,vspm_if" },
	{ },
};

static struct platform_driver vspm_if_driver = {
	.driver = {
		.name = DEVFILE,
		.owner = THIS_MODULE,
		.of_match_table = vspm_if_of_match,
	},
	.probe = vspm_if_probe,
	.remove = vspm_if_remove,
};

static int vspm_if_init(void)
{
	g_vspmif_pdev = NULL;

	platform_driver_register(&vspm_if_driver);
	if (!g_vspmif_pdev) {
		platform_driver_unregister(&vspm_if_driver);
		return -ENODEV;
	}

	misc_register(&misc);

	return 0;
}

static void vspm_if_exit(void)
{
	misc_deregister(&misc);

	platform_driver_unregister(&vspm_if_driver);
}

module_init(vspm_if_init);
module_exit(vspm_if_exit);

MODULE_AUTHOR("Renesas Electronics Corporation");
MODULE_LICENSE("Dual MIT/GPL");
