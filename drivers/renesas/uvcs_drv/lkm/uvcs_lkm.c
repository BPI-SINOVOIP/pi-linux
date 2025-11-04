/*************************************************************************/ /*
 UVCS Driver (kernel module)

 Copyright (C) 2015 - 2024 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

 The contents of this file are subject to the MIT license as set out below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 Alternatively, the contents of this file may be used under the terms of
 the GNU General Public License Version 2 ("GPL") in which case the provisions
 of GPL are applicable instead of those above.

 If you wish to allow use of your version of this file only under the terms of
 GPL, and not to allow others to use your version of this file under the terms
 of the MIT license, indicate your decision by deleting the provisions above
 and replace them with the notice and other provisions required by GPL as set
 out in the file called "GPL-COPYING" included in this distribution. If you do
 not delete the provisions above, a recipient may use your version of this file
 under the terms of either the MIT license or GPL.

 This License is also included in this distribution in the file called
 "MIT-COPYING".

 EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/


/******************************************************************************/
/*                    INCLUDE FILES                                           */
/******************************************************************************/

#include <linux/version.h>
#include <linux/compat.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include "uvcs_types.h"
#include "uvcs_cmn.h"
#include "uvcs_ioctl.h"
#include "uvcs_lkm_internal.h"

/******************************************************************************/
/*                    MODULE PARAMETERS                                       */
/******************************************************************************/
static uint timeout = UVCS_TIMEOUT_DEFAULT;
static uint uvcs_debug;
static uint safe_mode;

module_param(timeout, uint, 0x0);
module_param(uvcs_debug, uint, 0x0);	/* debug mode for UVCS Driver */
module_param(safe_mode, uint, 0x0);

/******************************************************************************/
/*                    MODULE DESCRIPTIONS                                     */
/******************************************************************************/
MODULE_AUTHOR("Renesas Electronics");
MODULE_LICENSE("Dual MIT/GPL");

/******************************************************************************/
/*                    DEFINES                                                 */
/******************************************************************************/

/******************************************************************************/
/*                    VARIABLES                                               */
/******************************************************************************/
static struct uvcs_driver_info *uvcsdrv;

/* open (chrdev) */
static int uvcs_open(struct inode *inode, struct file *filp)
{
	struct uvcs_hdl_info	*hdl;
	UVCS_RESULT				 uvcs_ret;
	UVCS_CMN_OPEN_PARAM_T	 oparam;
	unsigned int			 i;
	int						 result;

	down(&uvcsdrv->cdev_sem);

	/* init hardware */
	if (!safe_mode && !uvcsdrv->counter) {
		result = uvcs_io_init(uvcsdrv);
		if (result) {
			pr_err("uvcs: open failed #%d\n", __LINE__);
			goto err_exit_0;
		}
	}

	/* allocate memory for this handle */
	hdl = vzalloc(sizeof(struct uvcs_hdl_info));
	if (!hdl) {
		pr_err("uvcs: open failed #%d\n", __LINE__);
		result = -ENOMEM;
		goto err_exit_1;
	}

	hdl->uvcs_hdl_work = vzalloc(uvcsdrv->work_size_per_hdl);
	if (!hdl->uvcs_hdl_work) {
		pr_err("uvcs: open failed #%d\n", __LINE__);
		result = -ENOMEM;
		goto err_exit_2;
	}

	/* init handle */
	init_waitqueue_head(&hdl->waitq);
	sema_init(&hdl->sem, 1);	/* semaphore for this handle */

	/* open uvcs-cmn library */
	oparam.struct_size     = sizeof(UVCS_CMN_OPEN_PARAM_T);
	oparam.hdl_udptr       = hdl;
	oparam.hdl_work_0_virt = hdl->uvcs_hdl_work;
	oparam.hdl_work_0_size = uvcsdrv->work_size_per_hdl;
	oparam.ip_binding_enable = UVCS_FALSE;
	oparam.ip_binding_id     = 0;

	uvcs_ret = uvcs_cmn_open(uvcsdrv->uvcs_info, &oparam, &hdl->uvcs_hdl);
	switch (uvcs_ret) {
	/* interrupted */
	case UVCS_RTN_CONTINUE:
		result = -ERESTARTSYS;
		goto err_exit_3;

	case UVCS_RTN_OK:
		uvcsdrv->counter++;
		filp->private_data = hdl;
		hdl->drv = uvcsdrv;
		for (i = 0; i < UVCS_CMN_PROC_REQ_MAX; i++)
			hdl->req_data[i].state = UVCS_REQ_NONE;

		break;

	default:
		pr_err("uvcs: open failed #%d\n", __LINE__);
		result = -EFAULT;
		goto err_exit_3;
	}

	up(&uvcsdrv->cdev_sem);
	return 0;

err_exit_3:
	vfree(hdl->uvcs_hdl_work);

err_exit_2:
	vfree(hdl);

err_exit_1:
	if (!uvcsdrv->counter)
		uvcs_io_deinit(uvcsdrv);

err_exit_0:
	up(&uvcsdrv->cdev_sem);

	return result;
}


/* release (chrdev) */
static int uvcs_release(struct inode *inode, struct file *filp)
{
	struct uvcs_hdl_info *hdl = filp->private_data;
	struct uvcs_driver_info *drv;
	UVCS_RESULT uvcs_ret;
	unsigned int wait_cnt = 0;

	if (hdl) {
		drv = hdl->drv;
		do {
			uvcs_ret = uvcs_cmn_close(drv->uvcs_info,
							hdl->uvcs_hdl, UVCS_FALSE);
			switch (uvcs_ret) {
			/* interrupted */
			case UVCS_RTN_CONTINUE:
				return -ERESTARTSYS;

			/* this handle is working */
			case UVCS_RTN_BUSY:
				if (wait_cnt >= UVCS_CLOSE_WAIT_MAX) {
					uvcs_cmn_close(drv->uvcs_info, hdl->uvcs_hdl, UVCS_TRUE);
					uvcs_ret = UVCS_RTN_OK;
				} else {
					msleep(UVCS_CLOSE_WAIT_TIME);
					wait_cnt++;
				}
				break;

			case UVCS_RTN_OK:
				break;

			default:
				pr_debug("uvcs: close failed #%d\n", __LINE__);
				return -EFAULT;
			}
		} while (uvcs_ret == UVCS_RTN_BUSY);

		down(&drv->cdev_sem);
		filp->private_data = NULL;
		vfree(hdl->uvcs_hdl_work);
		vfree(hdl);

		if (drv->counter > 1u) {
			drv->counter--;

		} else {
			drv->counter = 0;
			if (!safe_mode)
				uvcs_io_deinit(drv);
		}
		up(&drv->cdev_sem);
	}

	return 0;
}


/* read (chrdev) */
static ssize_t uvcs_read(struct file *filp, char __user *buff,
					size_t count, loff_t *offset)
{
	struct uvcs_hdl_info	*hdl = filp->private_data;
	int						 wait_result;
	unsigned int			 i;

	if ((hdl == NULL)
	||	(hdl->drv == NULL)) {
		return -EFAULT;
	}

	if (count != sizeof(UVCS_CMN_HW_PROC_T)) {
		return -EINVAL;
	}

	/* into exclusive section */
	if (down_interruptible(&hdl->sem))
		return -ERESTARTSYS;

	/* waits until a result data becomes receivable */
	while (((hdl->req_data[0].state == UVCS_REQ_END)
		||	(hdl->req_data[1].state == UVCS_REQ_END)
		||	(hdl->req_data[2].state == UVCS_REQ_END)) == 0) {
		up(&hdl->sem);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		wait_result = wait_event_interruptible_timeout(
							hdl->waitq,
							((hdl->req_data[0].state == UVCS_REQ_END)
							|| (hdl->req_data[1].state == UVCS_REQ_END)
							|| (hdl->req_data[2].state == UVCS_REQ_END)),
							timeout
							);

		if (wait_result == 0)
			return -ETIMEDOUT;

		if (wait_result < 0)
			return -ERESTARTSYS;

		if (down_interruptible(&hdl->sem))
			return -ERESTARTSYS;
	}

	/* search finished module number */
	for (i = 0; i < UVCS_CMN_PROC_REQ_MAX; i++) {
		if (hdl->req_data[i].state == UVCS_REQ_END) {
			/* copy result information from kernel memory to userland */
			if (copy_to_user(buff, &hdl->req_data[i].data, count)) {
				/* failed to copy */
				up(&hdl->sem);
				pr_debug("uvcs: read failed #%d\n", __LINE__);
				return -EFAULT;
			}
			hdl->req_data[i].state = UVCS_REQ_NONE;
			up(&hdl->sem);
			return count;
		}
	}

	up(&hdl->sem);
	pr_debug("uvcs: read failed #%d\n", __LINE__);
	return 0;
}


/* write (chrdev) */
static ssize_t uvcs_write(struct file *filp, const char __user *buff,
					size_t count, loff_t *offset)
{
	struct uvcs_hdl_info *hdl = filp->private_data;
	unsigned int temp[2];
	ssize_t result;
	struct timespec64 ts;
	UVCS_RESULT uvcs_ret;

	if ((hdl == NULL)
	||	(hdl->drv == NULL)) {
		pr_debug("uvcs: write failed #%d\n", __LINE__);
		return -EFAULT;
	}

	if (count != sizeof(UVCS_CMN_HW_PROC_T)) {
		pr_debug("uvcs: write failed #%d\n", __LINE__);
		return -EINVAL;
	}

	/* into exclusive section */
	if (down_interruptible(&hdl->sem))
		return -ERESTARTSYS;

	/* check requested data */
	if (copy_from_user(&temp, buff, 8)) {
		pr_debug("uvcs: write failed #%d\n", __LINE__);
		result = -EFAULT;

	} else if ((temp[0] < sizeof(UVCS_CMN_HW_PROC_T))
	|| (temp[1] >= UVCS_CMN_PROC_REQ_MAX)) {
		pr_debug("uvcs: write failed #%d\n", __LINE__);
		result = -EINVAL;

	} else if (hdl->req_data[temp[1]].state != UVCS_REQ_NONE) {
		/* state error */
		pr_debug("uvcs: write failed #%d\n", __LINE__);
		result = -EBUSY;

	} else if (copy_from_user(&hdl->req_data[temp[1]].data, buff, count)) {
		/* copy requested data from user-land */
		pr_debug("uvcs: write failed #%d\n", __LINE__);
		result = -EFAULT;

	} else {
		ktime_get_raw_ts64(&ts);
		hdl->req_data[temp[1]].state = UVCS_REQ_USED;
		hdl->req_data[temp[1]].time = ts.tv_nsec;

		/* request to UVCS-CMN */
		uvcs_ret = uvcs_cmn_request(hdl->drv->uvcs_info, hdl->uvcs_hdl,
						&hdl->req_data[temp[1]].data);
		switch (uvcs_ret) {
		/* interrupted */
		case UVCS_RTN_CONTINUE:
			hdl->req_data[temp[1]].state = UVCS_REQ_NONE;
			result = -ERESTARTSYS;
			break;

		case UVCS_RTN_OK:
			result = count;
			break;

		/* unacceptable request */
		case UVCS_RTN_BUSY:
			pr_debug("uvcs: write failed #%d\n", __LINE__);
			hdl->req_data[temp[1]].state = UVCS_REQ_NONE;
			result = -EBUSY;
			break;

		default:
			pr_debug("uvcs: write failed #%d\n", __LINE__);
			hdl->req_data[temp[1]].state = UVCS_REQ_NONE;
			result = -EFAULT;
			break;
		}
	}

	up(&hdl->sem);

	return result;
}

/* ioctl (chrdev) */
static long uvcs_ioctl(struct file *filp, unsigned int cmd,
							unsigned long arg)
{
	struct uvcs_hdl_info *hdl = filp->private_data;
	long result = 0;
	UVCS_RESULT uvcs_ret;

	if ((hdl == NULL)
	|| (hdl->drv == NULL)) {
		return -ENODEV;
	}

	if (down_interruptible(&hdl->sem))
		return -ERESTARTSYS;

	switch (cmd) {
	case UVCS_IOCTL_DUMP_GET_SIZE:
		if (put_user(hdl->drv->uvcs_init_param.debug_log_size,
					(unsigned int __user *)arg)) {
			pr_debug("uvcs: ioctrl failed #%d", __LINE__);
			result = -EFAULT;
		}
		break;

	case UVCS_IOCTL_DUMP_OUTPUT:
		if (copy_to_user((void __user *)arg,
					hdl->drv->uvcs_init_param.debug_log_buff,
					hdl->drv->uvcs_init_param.debug_log_size)) {
			pr_debug("uvcs: ioctrl failed #%d", __LINE__);
			result = -EFAULT;
		}
		break;

	case UVCS_IOCTL_GET_IP_INFO:
		if (copy_to_user((void __user *)arg,
					&hdl->drv->ip_info, sizeof(UVCS_CMN_IP_INFO_T))) {
			pr_debug("uvcs: ioctrl failed #%d", __LINE__);
			result = -EFAULT;
		}
		break;

	case UVCS_IOCTL_GET_LSI_INFO:
		if (copy_to_user((void __user *)arg,
					&hdl->drv->lsi_info, sizeof(UVCS_CMN_LSI_INFO_T))) {
			pr_debug("uvcs: ioctrl failed #%d", __LINE__);
			result = -EFAULT;
		}
		break;

	case UVCS_IOCTL_GET_IP_CAPABILITY:
		if (copy_to_user((void __user *)arg,
					&hdl->drv->ip_cap, sizeof(UVCS_CMN_IP_CAPABILITY_T))) {
			pr_debug("uvcs: ioctrl failed #%d", __LINE__);
			result = -EFAULT;
		}
		break;

	case UVCS_IOCTL_SET_USE_IP:
		uvcs_ret = uvcs_cmn_set_ip_binding(hdl->drv->uvcs_info,
					hdl->uvcs_hdl, UVCS_TRUE, (UVCS_U32)arg);
		switch (uvcs_ret) {
		case UVCS_RTN_BUSY:
			result = -EBUSY;
			pr_debug("uvcs: ioctrl busy #%d", __LINE__);
			break;

		case UVCS_RTN_OK:
			result = 0;
			break;

		default:
			pr_debug("uvcs: ioctrl failed #%d", __LINE__);
			result = -EINVAL;
			break;
		}
		break;

	case UVCS_IOCTL_CLR_USE_IP:
		uvcs_ret = uvcs_cmn_set_ip_binding(hdl->drv->uvcs_info,
					hdl->uvcs_hdl, UVCS_FALSE, 0);
		if (uvcs_ret) {
			pr_debug("uvcs: ioctrl failed #%d,%d", __LINE__, uvcs_ret);
			result = -EINVAL;
		}
		break;

	default:
		pr_debug("uvcs: ioctrl unknown #%d", __LINE__);
		result = -ENOTTY;
		break;
	}

	up(&hdl->sem);

	return result;
}

#ifdef CONFIG_COMPAT
static long uvcs_compat_ioctl(struct file *filp, unsigned int cmd,
								unsigned long arg)
{
	switch (cmd) {
	case COMPAT_UVCS_IOCTL_DUMP_GET_SIZE:
		cmd = UVCS_IOCTL_DUMP_GET_SIZE;
		break;

	case COMPAT_UVCS_IOCTL_DUMP_OUTPUT:
		cmd = UVCS_IOCTL_DUMP_OUTPUT;
		break;

	case COMPAT_UVCS_IOCTL_GET_IP_INFO:
		cmd = UVCS_IOCTL_GET_IP_INFO;
		break;

	case COMPAT_UVCS_IOCTL_GET_LSI_INFO:
		cmd = UVCS_IOCTL_GET_LSI_INFO;
		break;

	case COMPAT_UVCS_IOCTL_GET_IP_CAPABILITY:
		cmd = UVCS_IOCTL_GET_IP_CAPABILITY;
		break;

	case COMPAT_UVCS_IOCTL_SET_USE_IP:
		cmd = UVCS_IOCTL_SET_USE_IP;
		break;

	case COMPAT_UVCS_IOCTL_CLR_USE_IP:
		cmd = UVCS_IOCTL_CLR_USE_IP;
		break;

	default:
		break;
	}

	return uvcs_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

/* poll (chrdev) */
static unsigned int uvcs_poll(struct file *filp, poll_table *wait)
{
	struct uvcs_hdl_info *hdl = filp->private_data;
	unsigned int mask = 0;

	/* into exclusive section */
	down(&hdl->sem);
	poll_wait(filp, &hdl->waitq, wait);
	if ((hdl->req_data[0].state == UVCS_REQ_END)
	||	(hdl->req_data[1].state == UVCS_REQ_END)
	||	(hdl->req_data[2].state == UVCS_REQ_END))
		mask |= POLLIN | POLLRDNORM;

	up(&hdl->sem);

	return mask;
}

/******************************************************************************/
/*                    power management                                        */
/******************************************************************************/
static int uvcs_runtime_nop(struct device *dev)
{
	(void)dev;
	return 0;
}

/******************************************************************************/
/*                    device probe / remove                                   */
/******************************************************************************/
static const struct file_operations uvcs_fops = {
	.owner = THIS_MODULE,
	.read = uvcs_read,
	.write = uvcs_write,
	.unlocked_ioctl = uvcs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = uvcs_compat_ioctl,
#endif
	.poll = uvcs_poll,
	.open = uvcs_open,
	.release = uvcs_release,
};

static struct miscdevice uvcs_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVNAME,
	.fops = &uvcs_fops
};

static const struct dev_pm_ops uvcs_pm_ops = {
	.runtime_suspend = uvcs_runtime_nop,
	.runtime_resume = uvcs_runtime_nop,
	.suspend = uvcs_runtime_nop,
	.resume = uvcs_runtime_nop,
};


struct uvcs_vcp_of_data {
	u32			 iparch;
	char		*name;
};

static const struct uvcs_vcp_of_data vcp_of_data[] = {
	{
		.iparch = UVCS_CMN_IPARCH_VCPLF,
		.name = UVCS_DT_VCPLF,
	},
	{
		.iparch = UVCS_CMN_IPARCH_VCPL4,
		.name = UVCS_DT_VCPL4,
	},
	{
		.iparch = UVCS_CMN_IPARCH_BELZ,
		.name = UVCS_DT_VDPB,
	},
	{
		.iparch = UVCS_CMN_IPARCH_FCPCS,
		.name = UVCS_DT_FCPCS,
	},
	{
		.iparch = UVCS_CMN_IPARCH_FCPCI,
		.name = UVCS_DT_FCPCI1,
	},
	{
		.iparch = UVCS_CMN_IPARCH_FCPCI,
		.name = UVCS_DT_FCPCI0,
	},
	{ /* end */ }
};

static const struct of_device_id uvcs_of_match[] = {
	{
		.compatible = UVCS_DT_COMPAT_BASE UVCS_DT_VCPLF,
		.data = &vcp_of_data[0],
	},
	{
		.compatible = UVCS_DT_COMPAT_BASE UVCS_DT_VCPL4,
		.data = &vcp_of_data[1],
	},
	{
		.compatible = UVCS_DT_COMPAT_BASE UVCS_DT_VDPB,
		.data = &vcp_of_data[2],
	},
	{
		.compatible = UVCS_DT_COMPAT_BASE UVCS_DT_FCPCS,
		.data = &vcp_of_data[3],
	},
	{
		.compatible = UVCS_DT_COMPAT_BASE UVCS_DT_FCPCI1,
		.data = &vcp_of_data[4],
	},
	{
		.compatible = UVCS_DT_COMPAT_BASE UVCS_DT_FCPCI0,
		.data = &vcp_of_data[5],
	},
	{ /* end */}
};

static int uvcs_dev_probe(struct platform_device *pdev)
{
	struct uvcs_platform_data *pdata = vzalloc(sizeof(struct uvcs_platform_data));
	const struct of_device_id *node;
	const struct uvcs_vcp_of_data *of_data;
	int result = -ENODEV;
	u32 ch;

	if (!pdata)
		return -ENOMEM;

	node = of_match_node(uvcs_of_match, pdev->dev.of_node);
	if ((node == NULL)
	||	(node->data == NULL)) {
		dev_err(&pdev->dev, "of_match_node() failed #%d\n", __LINE__);
		goto err_exit;
	}

	if (of_property_read_u32(pdev->dev.of_node, "renesas,#ch", &ch) < 0) {
		dev_err(&pdev->dev, "of_property_read_u32() failed #%d\n", __LINE__);
		goto err_exit;
	}

	of_data = (const struct uvcs_vcp_of_data *)node->data;
	if (of_data->iparch < UVCS_CMN_IPARCH_FCPCS) {
		/* vcp */
		if ((ch >= UVCS_VCP_DEVNUM)
		||	(uvcsdrv->vcpinf[ch].reg_vlc != NULL)) {
			result = -EINVAL;
			dev_err(&pdev->dev, "property error #%d\n", __LINE__);
			goto err_exit;
		}
		pdata->device_id = uvcsdrv->devnum;
		pdata->device_vcp = true;
		pdata->hwinf = &uvcsdrv->vcpinf[ch];
		result = uvcs_get_vcp_resource(pdev,
				&uvcsdrv->vcpinf[ch], of_data->iparch, of_data->name);
		if (result)
			goto err_exit;
		uvcsdrv->vcp_devnum++;

	} else {
		/* other */
		if ((ch >= UVCS_FCP_DEVNUM)
		||	(uvcsdrv->fcpinf[ch].reg_fcp != NULL)) {
			result = -EINVAL;
			dev_err(&pdev->dev, "property error #%d\n", __LINE__);
			goto err_exit;
		}
		pdata->device_id = uvcsdrv->devnum;
		pdata->device_vcp = false;
		pdata->hwinf = &uvcsdrv->fcpinf[ch];
		result = uvcs_get_fcp_resource(pdev,
				&uvcsdrv->fcpinf[ch], of_data->iparch);

		if (result)
			goto err_exit;
		uvcsdrv->fcp_devnum++;
	}

	pm_suspend_ignore_children(&pdev->dev, true);
	pm_runtime_enable(&pdev->dev);
	pdev->dev.platform_data = pdata;
	uvcsdrv->pdev[uvcsdrv->devnum] = pdev;
	platform_set_drvdata(pdev, uvcsdrv);
	uvcsdrv->devnum++;

	return 0;

err_exit:
	vfree(pdata);
	return result;
}

static int uvcs_dev_remove(struct platform_device *pdev)
{
	struct uvcs_platform_data *pdata = pdev->dev.platform_data;

	if (!pdata)
		return -EFAULT;

	if (pdata->device_vcp)
		uvcs_put_vcp_resource((struct uvcs_vcp_hwinf *)pdata->hwinf);
	else
		uvcs_put_fcp_resource((struct uvcs_fcp_hwinf *)pdata->hwinf);

	pm_runtime_disable(&pdev->dev);
	uvcsdrv->pdev[pdata->device_id] = NULL;
	platform_set_drvdata(pdev, NULL);
	vfree(pdata);

	return 0;
}

/******************************************************************************/
/*                    module init / exit                                      */
/******************************************************************************/
static struct platform_driver uvcs_driver = {
	.driver = {
		.name = DEVNAME,
		.owner = THIS_MODULE,
		.pm = &uvcs_pm_ops,
		.of_match_table = uvcs_of_match,
	},
	.probe = uvcs_dev_probe,
	.remove = uvcs_dev_remove,
};

static int __init uvcs_module_init(void)
{
	int ercd;
	u32 i;

	uvcsdrv = kzalloc(sizeof(struct uvcs_driver_info), GFP_KERNEL);
	if (!uvcsdrv) {
		ercd = -ENOMEM;
		goto err_exit_0;
	}

	/* allocate workarea */
	uvcs_cmn_get_work_size(&uvcsdrv->uvcs_init_param.work_mem_0_size,
			&uvcsdrv->work_size_per_hdl);
	uvcsdrv->uvcs_init_param.work_mem_0_virt = vzalloc(uvcsdrv->uvcs_init_param.work_mem_0_size);
	if (!uvcsdrv->uvcs_init_param.work_mem_0_virt) {
		ercd = -ENOMEM;
		goto err_exit_1;
	}

	/* allocate debug buffer */
	if (uvcs_debug) {
		uvcsdrv->uvcs_init_param.debug_log_size = UVCS_DEBUG_BUFF_SIZE;
		uvcsdrv->uvcs_init_param.debug_log_buff = vzalloc(UVCS_DEBUG_BUFF_SIZE);
		if (!uvcsdrv->uvcs_init_param.debug_log_buff) {
			ercd = -ENOMEM;
			goto err_exit_2;
		}
	}

	/* register driver */
	ercd = platform_driver_register(&uvcs_driver);
	if (ercd)
		goto err_exit_3;

	for (i = 0; i < uvcsdrv->vcp_devnum; i++) {
		if (!uvcsdrv->vcpinf[i].reg_vlc) {
			pr_debug("uvcs: probe error #%d\n", __LINE__);
			ercd = -ENODEV;
			goto err_exit_4;
		}
	}
	for (i = 0; i < uvcsdrv->fcp_devnum; i++) {
		if (!uvcsdrv->fcpinf[i].reg_fcp) {
			pr_debug("uvcs: probe error #%d\n", __LINE__);
			ercd = -ENODEV;
			goto err_exit_4;
		}
	}

	if (safe_mode) {
		ercd = uvcs_io_init(uvcsdrv);
		if (ercd)
			goto err_exit_4;
	}

	ercd = misc_register(&uvcs_misc);
	if (ercd)
		goto err_exit_5;

	sema_init(&uvcsdrv->cdev_sem, 1);

	return 0;

err_exit_5:
	if (safe_mode)
		uvcs_io_deinit(uvcsdrv);

err_exit_4:
	platform_driver_unregister(&uvcs_driver);

err_exit_3:
	if (uvcs_debug)
		vfree(uvcsdrv->uvcs_init_param.debug_log_buff);

err_exit_2:
	vfree(uvcsdrv->uvcs_init_param.work_mem_0_virt);

err_exit_1:
	kfree(uvcsdrv);
	uvcsdrv = NULL;

err_exit_0:
	return ercd;
}

static void __exit uvcs_module_exit(void)
{
	if (safe_mode)
		uvcs_io_deinit(uvcsdrv);

	misc_deregister(&uvcs_misc);
	platform_driver_unregister(&uvcs_driver);

	if (uvcs_debug)
		vfree(uvcsdrv->uvcs_init_param.debug_log_buff);
	vfree(uvcsdrv->uvcs_init_param.work_mem_0_virt);
	kfree(uvcsdrv);
	uvcsdrv = NULL;
}

module_init(uvcs_module_init);
module_exit(uvcs_module_exit);
