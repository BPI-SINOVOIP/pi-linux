/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include "drv_msg.h"
#include "tee_client_api.h"

#define VPU_INT_TIMEOUT				2000
#define VDEC_ISR_MSGQ_SIZE			16
#define VPU_DEVICE_TAG    "[vpu kernel driver] "
#define vpu_debug(...)		pr_dbg( VPU_DEVICE_TAG __VA_ARGS__)
#define vpu_trace(...)		pr_info( VPU_DEVICE_TAG __VA_ARGS__)
#define vpu_error(...)		pr_err( VPU_DEVICE_TAG __VA_ARGS__)

enum {
	VPU_IOCTL_CMD_MSG = 0,
	VDEC_IOCTL_POLL_INT,
	VPU_IOCTL_DISABLE_INT,
	VPU_IOCTL_ENABLE_INT,
	VPU_IOCTL_SET_INT_AFFINITY,
	VPU_IOCTL_WAKEUP,
};

enum VPU_HW_ID {
	VPU_VMETA = 0,
	VPU_V2G = 1,
	VPU_G2 = 2,
	VPU_G1 = 3,
	VPU_H1_0 = 4,
	VPU_H1_1 = 5,
	VPU_HW_IP_NUM,
};

struct VPU_NAME_ID {
	const char *hw_name;
	const u32 hw_id;
};

typedef struct _VPU_HW_IP_CTX_ {
	u32 hw_id;
	const char *hw_name;
	int irq_num;
	int irq_state;
	struct semaphore vdec_sem;
	struct semaphore resume_sem;
	resource_size_t iobase;
	resource_size_t iosize;
	u32 vdec_int_cnt;
	u32 vdec_enable_int_cnt;
	AMPMsgQ_t hVPUMsgQ;
	atomic_t vpu_dev_refcnt;
	atomic_t vdec_isr_msg_err_cnt;
	struct clk *vpuclk;
	struct cdev cdev;
	struct class *dev_class;
	dev_t vpu_devt;
	struct mutex vpu_mutex;
#ifdef CONFIG_PM_SLEEP
	TEEC_Context context;
	TEEC_Session session;
	bool session_opened;
	bool suspended;
#endif
} VPU_HW_IP_CTX;

#define VDEC_SUSPEND		44
#define VDEC_RESUME			45
#define VENC_SUSPEND		46
#define VENC_RESUME			47
static const TEEC_UUID TAVmeta_UUID = {0x1316a183, 0x894d, 0x43fe, \
	{0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf0}};
static int tz_vpu_initialize(VPU_HW_IP_CTX *pVpuHwCtx);
static void tz_vpu_finalize(VPU_HW_IP_CTX *pVpuHwCtx);

/*******************************************************************************
  Module internal function
  */
static irqreturn_t vpu_devices_isr(int irq, void *dev_id)
{
	/* disable interrupt */
	VPU_HW_IP_CTX *pVpuHwCtx = (VPU_HW_IP_CTX*)dev_id;
	/* disable interrupt */
	disable_irq_nosync(irq);
	pVpuHwCtx->irq_state = 0;

	if (pVpuHwCtx->hw_id == VPU_VMETA) {
		int ret = S_OK;
		MV_CC_MSG_t msg = { 0 };
		msg.m_Param1 = ++pVpuHwCtx->vdec_int_cnt;
		ret = AMPMsgQ_Add(&pVpuHwCtx->hVPUMsgQ, &msg);
		if (ret != S_OK) {
			if (!atomic_read(&pVpuHwCtx->vdec_isr_msg_err_cnt)) {
				vpu_error("[vdec isr] MsgQ full\n");
			}
			atomic_inc(&pVpuHwCtx->vdec_isr_msg_err_cnt);
			return IRQ_HANDLED;
		}
	}

	up(&pVpuHwCtx->vdec_sem);
	return IRQ_HANDLED;
}

/*******************************************************************************
  Module Register API
  */
static int vpu_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	unsigned long addr = 0;
	VPU_HW_IP_CTX *pVpuHwCtx = (VPU_HW_IP_CTX*)file->private_data;
	size_t size = vma->vm_end - vma->vm_start;

	addr = pVpuHwCtx->iobase + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= pVpuHwCtx->iobase &&
	      (addr + size) <= (pVpuHwCtx->iobase + pVpuHwCtx->iosize))) {
		vpu_error("Mmap invalid address %s, start=0x%lx, end=0x%lx\n",
			  pVpuHwCtx->hw_name, addr, addr + size);
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma,
			      vma->vm_start,
			      (pVpuHwCtx->iobase >> PAGE_SHIFT) +
			      vma->vm_pgoff, size, vma->vm_page_prot);
	if (ret)
		vpu_trace("%s mmap failed, ret = %d\n",
			  pVpuHwCtx->hw_name, ret);
	return ret;
}

static long vpu_driver_ioctl_unlocked(struct file *filp, unsigned int cmd,
				      unsigned long arg)
{
	int rc = S_OK;
	VPU_HW_IP_CTX *pVpuHwCtx = (VPU_HW_IP_CTX*)filp->private_data;

	switch (cmd) {
	case VDEC_IOCTL_POLL_INT:
		{
			pVpuHwCtx->vdec_enable_int_cnt++;
			enable_irq(pVpuHwCtx->irq_num);
			pVpuHwCtx->irq_state = 1;
			/* VDEC userspace will reset VPU if timeout here */
			rc = down_timeout(&pVpuHwCtx->vdec_sem, msecs_to_jiffies(VPU_INT_TIMEOUT));
			if (rc < 0) {
				vpu_error("%s isr timeout after %dms, ret = %d\n",
					pVpuHwCtx->hw_name, VPU_INT_TIMEOUT, rc);
				return rc;
			}

			if (pVpuHwCtx->hw_id == VPU_VMETA) {
				MV_CC_MSG_t msg = { 0 };
				// check fullness, clear message queue once.
				// only send latest message to task.
				if (AMPMsgQ_Fullness(&pVpuHwCtx->hVPUMsgQ) <= 0) {
					//vpu_trace(" E/[vdec isr task]  message queue empty\n");
					return -EFAULT;
				}
				AMPMsgQ_DequeueRead(&pVpuHwCtx->hVPUMsgQ, &msg);
				if (!atomic_read
				    (&pVpuHwCtx->vdec_isr_msg_err_cnt)) {
					atomic_set(&pVpuHwCtx->
						   vdec_isr_msg_err_cnt, 0);
				}
			}
			break;
		}

	case VPU_IOCTL_DISABLE_INT:
		{
			if (pVpuHwCtx->irq_state) {
				disable_irq(pVpuHwCtx->irq_num);
				pVpuHwCtx->irq_state = 0;
			}
			break;
		}

	case VPU_IOCTL_ENABLE_INT:
		{
			if (!pVpuHwCtx->irq_state) {
				enable_irq(pVpuHwCtx->irq_num);
				pVpuHwCtx->irq_state = 1;
			}
			break;
		}
	case VPU_IOCTL_SET_INT_AFFINITY:
		{
			unsigned int affinity = 0;
			if (copy_from_user
				(&affinity, (void __user *) arg, sizeof(unsigned int)))
				return -EFAULT;
			irq_set_affinity_hint(pVpuHwCtx->irq_num, get_cpu_mask(affinity));
			break;
		}

	case VPU_IOCTL_WAKEUP:
		{
			up(&pVpuHwCtx->vdec_sem);
			break;
		}

	default:
		break;
	}
	return 0;
}

static int vpu_driver_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	VPU_HW_IP_CTX *pVpuHwCtx = NULL;
	pVpuHwCtx = container_of(inode->i_cdev, struct _VPU_HW_IP_CTX_, cdev);
	filp->private_data = pVpuHwCtx;

	mutex_lock(&pVpuHwCtx->vpu_mutex);
	if (atomic_inc_return(&pVpuHwCtx->vpu_dev_refcnt) > 1)
		goto exit;

	if (pVpuHwCtx->irq_num > 0) {
		err = request_irq(pVpuHwCtx->irq_num, vpu_devices_isr, IRQF_ONESHOT,
					  pVpuHwCtx->hw_name, (void *) pVpuHwCtx);
		if (unlikely(err < 0)) {
			vpu_error("request irq fail, irq_num:%5d, %s, err:%8x\n",
				  pVpuHwCtx->irq_num, pVpuHwCtx->hw_name, err);
			goto exit;
		}
		disable_irq(pVpuHwCtx->irq_num);
		pVpuHwCtx->irq_state = 0;
	}

exit:
	mutex_unlock(&pVpuHwCtx->vpu_mutex);
	return err;
}

static int vpu_driver_release(struct inode *inode, struct file *filp)
{
	VPU_HW_IP_CTX *pVpuHwCtx = NULL;
	pVpuHwCtx = container_of(inode->i_cdev, struct _VPU_HW_IP_CTX_, cdev);

	mutex_lock(&pVpuHwCtx->vpu_mutex);
	if (atomic_dec_return(&pVpuHwCtx->vpu_dev_refcnt))
		goto exit;

	if (pVpuHwCtx->irq_num > 0) {
		irq_set_affinity_hint(pVpuHwCtx->irq_num, NULL);
		free_irq(pVpuHwCtx->irq_num, (void *)(pVpuHwCtx));
		pVpuHwCtx->irq_state = 0;
	}

exit:
	mutex_unlock(&pVpuHwCtx->vpu_mutex);
	return 0;
}

static const struct file_operations vpu_ops = {
	.open = vpu_driver_open,
	.release = vpu_driver_release,
	.unlocked_ioctl = vpu_driver_ioctl_unlocked,
	.compat_ioctl = vpu_driver_ioctl_unlocked,
	.mmap = vpu_driver_mmap,
	.owner = THIS_MODULE,
};

static const struct VPU_NAME_ID vpu_name_id[] = {
	{"vmeta", VPU_VMETA},
	{"vxg", VPU_V2G},
	{"g2", VPU_G2},
	{"g1", VPU_G1},
	{"h1", VPU_H1_0},
	{"h1_1", VPU_H1_1},
};

static const struct of_device_id vpu_match[] = {
	{
		.compatible = "marvell,berlin-vmeta",
		.data = (void*)&vpu_name_id[VPU_VMETA],
	},
	{
		.compatible = "marvell,berlin-vxg",
		.data = (void*)&vpu_name_id[VPU_V2G],
	},
	{
		.compatible = "marvell,berlin-h1",
		.data = (void*)&vpu_name_id[VPU_H1_0],
	},
	{
		.compatible = "syna,berlin-vxg",
		.data = (void*)&vpu_name_id[VPU_V2G],
	},
	{
		.compatible = "syna,berlin-h1",
		.data = (void*)&vpu_name_id[VPU_H1_0],
	},
	{},
};
MODULE_DEVICE_TABLE(of, vpu_match);

static int berlin_vpu_probe(struct platform_device *pdev)
{
	int err;
	struct device *dev = NULL;
	const struct of_device_id *of_id = NULL;
	VPU_HW_IP_CTX *pVpuHwCtx = NULL;
	struct resource *pVpuRes = NULL;
	struct VPU_NAME_ID *pNameId = NULL;

	vpu_trace("vpu probe enter\n");

	of_id = of_match_device(vpu_match, &pdev->dev);
	if (of_id) {
		pVpuHwCtx = devm_kzalloc(&pdev->dev, sizeof(VPU_HW_IP_CTX), GFP_KERNEL);
		pNameId = (struct VPU_NAME_ID*)of_id->data;
	} else
		return -ENXIO;

	if (!pVpuHwCtx)
		return -ENOMEM;

	pVpuHwCtx->hw_name = pNameId->hw_name;
	pVpuHwCtx->hw_id = pNameId->hw_id;
	vpu_trace("find vpu device %s\n",pVpuHwCtx->hw_name);

	pVpuHwCtx->irq_num = platform_get_irq(pdev, 0);
	vpu_trace("request irq:%d\n", pVpuHwCtx->irq_num);

	pVpuRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!pVpuRes) {
		vpu_error("failed to get io resource\n");
		return -ENXIO;
	}
	pVpuHwCtx->iobase = pVpuRes->start;
	pVpuHwCtx->iosize = resource_size(pVpuRes);
        vpu_trace("register space %pR\n", pVpuRes);
	sema_init(&pVpuHwCtx->vdec_sem, 0);
	sema_init(&pVpuHwCtx->resume_sem, 0);
	mutex_init(&pVpuHwCtx->vpu_mutex);
	err = AMPMsgQ_Init(&pVpuHwCtx->hVPUMsgQ, VDEC_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		vpu_error("drv_vpu_init: hVPUMsgQ init failed, err:%8x\n", err);
		return err;
	}

	pVpuHwCtx->vpuclk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(pVpuHwCtx->vpuclk))
		clk_prepare_enable(pVpuHwCtx->vpuclk);

	err = alloc_chrdev_region(&pVpuHwCtx->vpu_devt, 0, 1, pVpuHwCtx->hw_name);
	if(err < 0) {
		vpu_error("failed to allocate dev num\n");
		goto err_prob_device_0;
	}

	cdev_init(&pVpuHwCtx->cdev, &vpu_ops);
	pVpuHwCtx->cdev.owner = THIS_MODULE;
	/* Add the device */
	err = cdev_add(&pVpuHwCtx->cdev, MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), 1);
	if (err)
		goto err_prob_device_1;

	/* add vpu devices to sysfs */
	pVpuHwCtx->dev_class = class_create(THIS_MODULE, pVpuHwCtx->hw_name);
	if (IS_ERR(pVpuHwCtx->dev_class)) {
		vpu_error("class_create failed.\n");
		err = PTR_ERR(pVpuHwCtx->dev_class);
		goto err_prob_device_2;
	}

	dev = device_create(pVpuHwCtx->dev_class, NULL,
		      MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), NULL,
		      pVpuHwCtx->hw_name);
	if (IS_ERR(dev)) {
		vpu_error("device_create failed.\n");
		err = PTR_ERR(dev);
		goto err_prob_device_3;
	}

	platform_set_drvdata(pdev, pVpuHwCtx);

	vpu_trace("vpu:%s probe ok\n", pVpuHwCtx->hw_name);
	return 0;

err_prob_device_3:
	class_destroy(pVpuHwCtx->dev_class);
err_prob_device_2:
	cdev_del(&pVpuHwCtx->cdev);
err_prob_device_1:
	unregister_chrdev_region(MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), 1);
err_prob_device_0:
	if (!IS_ERR(pVpuHwCtx->vpuclk)) {
		clk_disable_unprepare(pVpuHwCtx->vpuclk);
	}
	AMPMsgQ_Destroy(&pVpuHwCtx->hVPUMsgQ);
	return err;
}

static int berlin_vpu_remove(struct platform_device *pdev)
{
	int err;
	VPU_HW_IP_CTX *pVpuHwCtx = NULL;
	vpu_trace("vpu remove enter\n");

	pVpuHwCtx = platform_get_drvdata(pdev);

	vpu_trace("revomve vpu:%s\n", pVpuHwCtx->hw_name);

	if (pVpuHwCtx->dev_class) {
		device_destroy(pVpuHwCtx->dev_class,
			       MKDEV(MAJOR(pVpuHwCtx->vpu_devt),
				     0));
		class_destroy(pVpuHwCtx->dev_class);
	}

	cdev_del(&pVpuHwCtx->cdev);
	unregister_chrdev_region(MKDEV(MAJOR(pVpuHwCtx->vpu_devt), 0), 1);

	err = AMPMsgQ_Destroy(&pVpuHwCtx->hVPUMsgQ);
	if (unlikely(err != S_OK)) {
		vpu_error("drv_app_exit: failed, err:%8x\n", err);
		return err;
	}

	if (!IS_ERR(pVpuHwCtx->vpuclk))
		clk_disable_unprepare(pVpuHwCtx->vpuclk);

#ifdef CONFIG_PM_SLEEP
	tz_vpu_finalize(pVpuHwCtx);
#endif

	vpu_trace("vpu:%s remove ok\n", pVpuHwCtx->hw_name);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int tz_vpu_retcode_translate(TEEC_Result result)
{
	int ret;

	switch (result) {
	case TEEC_SUCCESS:
		ret = 0;
		break;
	case TEEC_ERROR_ACCESS_DENIED:
		ret = -ENOTSUPP;
		break;
	case TEEC_ERROR_BAD_PARAMETERS:
		ret = -EINVAL;
		break;
	default:
		ret = -EPERM;
		break;
	}
	return ret;
}

static int tz_vpu_initialize(VPU_HW_IP_CTX *pVpuHwCtx)
{
	TEEC_Result result;

	if (pVpuHwCtx->session_opened)
		return 0;

	/* [1] Connect to TEE */
	result = TEEC_InitializeContext(
				NULL,
				&pVpuHwCtx->context);
	if (result != TEEC_SUCCESS) {
		vpu_error("TEEC_InitializeContext ret=0x%08x\n", result);
		goto cleanup;
	}

	/* [2] Open session with TEE application */
	result = TEEC_OpenSession(
			&pVpuHwCtx->context,
			&pVpuHwCtx->session,
			&TAVmeta_UUID,
			TEEC_LOGIN_USER,
			NULL,    /* No connection data needed for TEEC_LOGIN_USER. */
			NULL,    /* No payload, and do not want cancellation. */
			NULL);
	if (result != TEEC_SUCCESS) {
		vpu_error("TEEC_OpenSession ret=0x%08x\n", result);
		goto cleanup1;
	}

	pVpuHwCtx->session_opened = 1;

	return 0;

cleanup1:
	TEEC_FinalizeContext(&pVpuHwCtx->context);
cleanup:
	return tz_vpu_retcode_translate(result);
}

static void tz_vpu_finalize(VPU_HW_IP_CTX *pVpuHwCtx)
{
	if (pVpuHwCtx->session_opened) {
		TEEC_CloseSession(&pVpuHwCtx->session);
		TEEC_FinalizeContext(&pVpuHwCtx->context);
		pVpuHwCtx->session_opened = 0;
	}
}

static int tz_vpu_suspend(VPU_HW_IP_CTX *pVpuHwCtx)
{
	TEEC_Result result;
	TEEC_Operation operation;
	int cmd, dret, ret;

	if (!pVpuHwCtx->session_opened) {
		ret = tz_vpu_initialize(pVpuHwCtx);
		if (ret) {
			vpu_error("TZ VPU initialize failed.\n");
			return ret;
		}
	}

	if (pVpuHwCtx->hw_id == VPU_V2G) {
		cmd = VDEC_SUSPEND;
	} else if (pVpuHwCtx->hw_id == VPU_H1_0) {
		cmd = VENC_SUSPEND;
	} else {
		vpu_error("invalid hw id %d, return\n", pVpuHwCtx->hw_id);
		return -EINVAL;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	result = TEEC_InvokeCommand(
			&pVpuHwCtx->session,
			cmd,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS) {
		vpu_error("[%d: %s]VPU_SUSPEND error: 0x%x\n",
			pVpuHwCtx->hw_id, pVpuHwCtx->hw_name, result);
		return tz_vpu_retcode_translate(result);
	}

	dret = operation.params[0].value.a;
	if (dret < 0) {
		result = TEEC_ERROR_CANCEL;
	}

	return tz_vpu_retcode_translate(result);
}

static int tz_vpu_resume(VPU_HW_IP_CTX *pVpuHwCtx)
{
	TEEC_Result result;
	TEEC_Operation operation;
	int cmd, dret, ret;

	if (!pVpuHwCtx->session_opened) {
		ret = tz_vpu_initialize(pVpuHwCtx);
		if (ret) {
			vpu_error("TZ VPU initialize failed.\n");
			return ret;
		}
	}

	if (pVpuHwCtx->hw_id == VPU_V2G) {
		cmd = VDEC_RESUME;
	} else if (pVpuHwCtx->hw_id == VPU_H1_0) {
		cmd = VENC_RESUME;
	} else {
		vpu_error("invalid hw id %d, return\n", pVpuHwCtx->hw_id);
		return -EINVAL;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	result = TEEC_InvokeCommand(
			&pVpuHwCtx->session,
			cmd,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS) {
		vpu_error("[%d: %s]VPU_RESUME error: 0x%x\n",
			pVpuHwCtx->hw_id, pVpuHwCtx->hw_name, result);
		return tz_vpu_retcode_translate(result);
	}

	dret = operation.params[0].value.a;
	if (dret < 0) {
		result = TEEC_ERROR_CANCEL;
	}

	return tz_vpu_retcode_translate(result);
}

static int berlin_vpu_suspend(struct device *dev)
{
	int timeout = 0, ret;
	VPU_HW_IP_CTX *pVpuHwCtx;

	pVpuHwCtx = dev_get_drvdata(dev);

	/* nothing to do if HW inactive */
	if (!pVpuHwCtx->vdec_enable_int_cnt)
		return 0;

	/* wait VPU IRQ */
	while (pVpuHwCtx->irq_state && timeout < 500) {
		usleep_range(1000, 1200);
		timeout++;
	}

	if (timeout >= 500) {
		vpu_error("[%d: %s]irq_state: %d, waiting timeout: %d, return -EBUSY\n",
			pVpuHwCtx->hw_id, pVpuHwCtx->hw_name, pVpuHwCtx->irq_state, timeout);
		return -EBUSY;
	}

	/* disable IRQ */
	disable_irq_nosync(pVpuHwCtx->irq_num);

	/* Notify VPU to suspend */
	ret = tz_vpu_suspend(pVpuHwCtx);
	if (ret) {
		vpu_error("[%d: %s]vpu suspend error(%d), return\n",
			pVpuHwCtx->hw_id, pVpuHwCtx->hw_name, ret);
		goto cleanup;
	}

	pVpuHwCtx->vdec_enable_int_cnt = 0;
	pVpuHwCtx->vdec_int_cnt = 0;
	pVpuHwCtx->suspended = true;

	return 0;

cleanup:
	enable_irq(pVpuHwCtx->irq_num);
	return ret;
}

static int berlin_vpu_resume(struct device *dev)
{
	int ret = 0;
	VPU_HW_IP_CTX *pVpuHwCtx;

	pVpuHwCtx = dev_get_drvdata(dev);

	if (pVpuHwCtx->suspended) {

		/* enable IRQ */
		enable_irq(pVpuHwCtx->irq_num);

		/* Notify VPU to resume */
		ret = tz_vpu_resume(pVpuHwCtx);
		if (ret) {
			vpu_error("[%d: %s]vpu resume error(%d), return\n",
				pVpuHwCtx->hw_id, pVpuHwCtx->hw_name, ret);
		}
		pVpuHwCtx->suspended = false;
	}

	return ret;

}

static SIMPLE_DEV_PM_OPS(berlin_vpu_pmops, berlin_vpu_suspend,
			 berlin_vpu_resume);

#endif

static struct platform_driver berlin_vpu_driver = {
	.probe = berlin_vpu_probe,
	.remove = berlin_vpu_remove,
	.driver = {
		   .name = "marvell,berlin-vpu",
		   .of_match_table = vpu_match,
#ifdef CONFIG_PM_SLEEP
		   .pm = &berlin_vpu_pmops,
#endif
	},
};
module_platform_driver(berlin_vpu_driver);

MODULE_AUTHOR("marvell");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vpu module template");
