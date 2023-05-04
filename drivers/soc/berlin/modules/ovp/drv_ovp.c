// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/proc_fs.h>
#include <linux/io.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/platform_device.h>

#include <linux/of_platform.h>
#include <linux/of_device.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

/*************************************************************************
 * Local head files
 */
#include "ovp_debug.h"
#include "ovp_ioctl.h"
#include "drv_ovp.h"
#include "tz_utils.h"
#include "tee_ca_ovp.h"

/***********************************************************************
 * Module Variable
 */
#define OVP_NODE_NAME		"syna,berlin-ovp"
#define OVP_INTR_NAME		"ovp_intr"
#define OVP_MODULE_NAME	"ovp_module"
#define OVP_DEVICE_NAME	"ovp"
#define OVP_MODULE_CLK		"ovp_coreclk"
#define OVP_DEVICE_PATH	("/dev/" OVP_DEVICE_NAME)
#define OVP_MAX_DEVS	8
#define OVP_MINOR	0
#define OVP_INTR_STS	0x40

#define OVP_ENABLE_FASTCALL_FOR_REG_ACCESS

static OVP_CTX ovp_ctx;
static atomic_t ovp_dev_refcnt = ATOMIC_INIT(0);

/**********************************************************************
 * Module API
 */
#ifdef OVP_ENABLE_FASTCALL_FOR_REG_ACCESS

static tz_secure_reg ovp_tz_phy_to_secure_reg(unsigned int regAddr)
{
	tz_secure_reg secureReg = TZ_REG_MAX;

	if (regAddr == OVP_INTR_STS)
		secureReg = TZ_REG_OVP_INTR_STATUS;

	return secureReg;
}

static int ovp_wrap_register_read(unsigned int regAddr, unsigned int *pRegVal)
{
	tz_secure_reg secureReg = ovp_tz_phy_to_secure_reg(regAddr);

	return tz_secure_reg_rw(secureReg, TZ_SECURE_REG_READ, pRegVal);
}

static int ovp_wrap_register_write(unsigned int regAddr, unsigned int regVal)
{
	tz_secure_reg secureReg = ovp_tz_phy_to_secure_reg(regAddr);

	return tz_secure_reg_rw(secureReg, TZ_SECURE_REG_WRITE, &regVal);
}

#else //!OVP_ENABLE_FASTCALL_FOR_REG_ACCESS

static int ovp_wrap_register_read(unsigned int regAddr, unsigned int *pRegVal)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;

	*pRegVal = readl_relaxed(ovp_ctx->ovp_virt_base + regAddr);

	return 0;
}

static int ovp_wrap_register_write(unsigned int regAddr, unsigned int regVal)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;

	writel_relaxed(regVal, ovp_ctx->ovp_virt_base + regAddr);

	return 0;
}

#endif //!OVP_ENABLE_FASTCALL_FOR_REG_ACCESS

static irqreturn_t ovp_drv_isr(int irq, void *dev_id)
{
	OVP_CTX *hOvpCtx = (OVP_CTX *) dev_id;
	unsigned int ovp_intr = 0;
	int ret = S_OK;

	//Read interrupt status
	ovp_wrap_register_read(OVP_INTR_STS, &ovp_intr);

	//Clear all interrupts
	ovp_wrap_register_write(OVP_INTR_STS, 0x3F);


	if (ovp_intr && hOvpCtx->ovp_intr_status) {
		MV_CC_MSG_t msg = { OVP_CC_MSG, ovp_intr };

		spin_lock(&hOvpCtx->ovp_msg_spinlock);
		ret = AMPMsgQ_Add(&hOvpCtx->hOVPMsgQ, &msg);
		spin_unlock(&hOvpCtx->ovp_msg_spinlock);
		if (ret == S_OK)
			up(&hOvpCtx->ovp_sem);
	}

	return IRQ_HANDLED;
}

static int ovp_device_init(struct ovp_device_t *ovp_dev, unsigned int user)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;
	unsigned int err;

	mutex_init(&hOvpCtx->ovp_mutex);

	sema_init(&hOvpCtx->ovp_sem, 0);
	spin_lock_init(&hOvpCtx->ovp_msg_spinlock);

	err = AMPMsgQ_Init(&hOvpCtx->hOVPMsgQ, OVP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		ovp_error("%s: hOVPMsgQ init failed, err:%8x\n", __func__, err);
		return -1;
	}

	ovp_trace("%s ok\n", __func__);

	return S_OK;
}

static int ovp_device_exit(struct ovp_device_t *ovp_dev, unsigned int user)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;
	unsigned int err;

	err = AMPMsgQ_Destroy(&hOvpCtx->hOVPMsgQ);
	if (unlikely(err != S_OK)) {
		ovp_error("ovp MsgQ Destroy: failed, err:%8x\n", err);
		return -1;
	}
	ovp_trace("%s ok\n", __func__);

	return S_OK;
}

static int ovp_drv_open(struct inode *inode, struct file *filp)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;
	struct ovp_device_t *dev;
	unsigned int vec_num;
	int err = 0;

	mutex_lock(&hOvpCtx->ovp_mutex);

	ovp_trace("Start open ovp driver!\n");

	if (atomic_inc_return(&ovp_dev_refcnt) > 1) {
		ovp_trace("ovp driver reference count %d!\n",
			  atomic_read(&ovp_dev_refcnt));
		goto err_exit;
	}

	dev = container_of(inode->i_cdev, struct ovp_device_t, cdev);
	filp->private_data = dev;

	/* register and enable OVP ISR  */
	vec_num = hOvpCtx->irq_num;
	err = request_irq(vec_num, ovp_drv_isr, 0,
			  OVP_MODULE_NAME, hOvpCtx);
	if (unlikely(err < 0)) {
		ovp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		goto err_exit;
	} else
		ovp_trace("%s ok\n", __func__);

	err = tz_ovp_initialize();
	if (err) {
		ovp_trace("TZ OVP initialize failed.\n");
		/* unregister OVP interrupt */
		free_irq(vec_num, (void *)hOvpCtx);
	}

err_exit:
	mutex_unlock(&hOvpCtx->ovp_mutex);

	return err;
}

static int ovp_drv_release(struct inode *inode, struct file *filp)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;

	mutex_lock(&hOvpCtx->ovp_mutex);

	if (atomic_read(&ovp_dev_refcnt) == 0) {
		ovp_trace("ovp driver already released!\n");
		goto err_exit;
	}

	if (atomic_dec_return(&ovp_dev_refcnt)) {
		ovp_trace("ovp dev ref cnt after this release: %d!\n",
				atomic_read(&ovp_dev_refcnt));
		goto err_exit;
	}

	/* unregister OVP interrupt */
	free_irq(hOvpCtx->irq_num, (void *) hOvpCtx);

	tz_ovp_finalize();

	ovp_trace("%s ok\n", __func__);

err_exit:
	mutex_unlock(&hOvpCtx->ovp_mutex);

	return 0;
}

static long ovp_drv_ioctl_unlocked(struct file *filp, unsigned int cmd, unsigned long arg)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;
	unsigned long flags;
	int ret;

	switch (cmd) {
	case OVP_IOCTL_GET_MSG:
	{
		MV_CC_MSG_t msg = { 0 };
		int rc = S_OK;

		rc = down_interruptible(&hOvpCtx->ovp_sem);
		if (rc < 0)
			return rc;

		// only send latest message to task.
		if (AMPMsgQ_Fullness(&hOvpCtx->hOVPMsgQ) <= 0) {
			ovp_trace(" E/[ovp isr task]  message queue empty\n");
			return -EFAULT;
		}

		spin_lock_irqsave(&hOvpCtx->ovp_msg_spinlock, flags);
		AMPMsgQ_DequeueRead(&hOvpCtx->hOVPMsgQ, &msg);
		spin_unlock_irqrestore(&hOvpCtx->ovp_msg_spinlock, flags);

		if (copy_to_user
		    ((void __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
			return -EFAULT;
		break;
	}

	case OVP_IOCTL_INTR:
	{
		INTR_MSG ovp_intr_info = { 0, 0 };

		if (copy_from_user
		    (&ovp_intr_info, (void __user *) arg,
		     sizeof(INTR_MSG)))
			return -EFAULT;

		hOvpCtx->ovp_intr_status = ovp_intr_info.Enable;

		break;
	}
	case OVP_IOCTL_CLKCONTROL:
	{
		bool bOVPClockControl;

		if (copy_from_user
			(&bOVPClockControl, (void __user *)arg,
				sizeof(bool)))
			return -EFAULT;
		if (bOVPClockControl) {
			ret = clk_prepare_enable(hOvpCtx->ovp_clk);
			if (ret < 0) {
				ovp_error("%s prepare failed..!\n", OVP_MODULE_CLK);
				return ret;
			}
		} else {
			clk_disable_unprepare(hOvpCtx->ovp_clk);
		}
		break;
	}
	case OVP_IOCTL_DUMMY_INTR:
	{
		up(&hOvpCtx->ovp_sem);

		break;
	}

	} //Switch end

	return 0;
}

/*********************************************************************
 * Module Register API
 */

static const struct file_operations ovp_ops = {
	.open = ovp_drv_open,
	.release = ovp_drv_release,
	.unlocked_ioctl = ovp_drv_ioctl_unlocked,
	.compat_ioctl = ovp_drv_ioctl_unlocked,
	.owner = THIS_MODULE,
};

static struct ovp_device_t ovp_device = {
	.dev_name = OVP_DEVICE_NAME,
	.minor = OVP_MINOR,
	.dev_init = ovp_device_init,
	.dev_exit = ovp_device_exit,
	.fops = &ovp_ops,
};

static const struct of_device_id ovp_match[] = {
	{
		.compatible = OVP_NODE_NAME,
	},
	{},
};

static int ovp_drv_init(struct ovp_device_t *ovp_device)
{
	struct cdev *dev = &ovp_device->cdev;
	int res;

	/* Now setup cdevs. */
	cdev_init(dev, ovp_device->fops);
	dev->owner = THIS_MODULE;
	res = cdev_add(dev, MKDEV(ovp_device->major, ovp_device->minor), 1);
	if (res) {
		ovp_error("ovp_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}
	ovp_trace("setup cdevs device minor [%d]\n", ovp_device->minor);

	/* add PE devices to sysfs */
	ovp_device->dev_class =
		class_create(THIS_MODULE, ovp_device->dev_name);
	if (IS_ERR(ovp_device->dev_class)) {
		ovp_error("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(ovp_device->dev_class, NULL,
			  MKDEV(ovp_device->major, ovp_device->minor), NULL,
			  ovp_device->dev_name);
	ovp_trace("create device sysfs [%s]\n", ovp_device->dev_name);

	/* create hw device */
	if (ovp_device->dev_init) {
		res = ovp_device->dev_init(ovp_device, 0);
		if (res != 0) {
			ovp_error("ovp_int_init failed !!! res = 0x%08X\n",
				  res);
			res = -ENODEV;
			goto err_add_device;
		}
	}

	return 0;

err_add_device:
	if (ovp_device->dev_class) {
		device_destroy(ovp_device->dev_class, MKDEV(ovp_device->major,
			ovp_device->minor));
		class_destroy(ovp_device->dev_class);
	}

	cdev_del(&ovp_device->cdev);

	return res;
}

static int ovp_drv_exit(struct ovp_device_t *ovp_device)
{
	int res;

	ovp_trace("%s [%s] enter\n", __func__, ovp_device->dev_name);

	/* destroy kernel API */
	if (ovp_device->dev_exit) {
		res = ovp_device->dev_exit(ovp_device, 0);
		if (res != 0)
			ovp_error("%s failed !!! res = 0x%08X\n",
				__func__, res);
	}

	if (ovp_device->dev_class) {
		/* del sysfs entries */
		device_destroy(ovp_device->dev_class, MKDEV(ovp_device->major,
			ovp_device->minor));
		ovp_trace("delete device sysfs [%s]\n", ovp_device->dev_name);

		class_destroy(ovp_device->dev_class);
	}
	/* del cdev */
	cdev_del(&ovp_device->cdev);

	return 0;
}

static int ovp_drv_read_cfg(OVP_CTX *hOvpCtx,
			struct platform_device *pdev)
{
	hOvpCtx->irq_num =
		platform_get_irq_byname(pdev, OVP_INTR_NAME);

	if (hOvpCtx->irq_num <= 0) {
		ovp_error("Failed to get irq(%s) for OVP\n", OVP_INTR_NAME);
		return -EINVAL;
	}

	ovp_trace("%s:%d: irq - %s:%x\n",
		__func__, __LINE__,
		OVP_INTR_NAME, hOvpCtx->irq_num);

	return 0;
}

static int ovp_drv_create_devioremap(OVP_CTX *hOvpCtx,
			struct platform_device *pdev)
{
	int ret = 0;

	hOvpCtx->ovp_virt_base = NULL;

	hOvpCtx->pOvpRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hOvpCtx->ovp_base = hOvpCtx->pOvpRes->start;
	hOvpCtx->ovp_size = resource_size(hOvpCtx->pOvpRes);
	hOvpCtx->ovp_virt_base = devm_ioremap_resource(&pdev->dev, hOvpCtx->pOvpRes);

	if (IS_ERR_OR_NULL(hOvpCtx->ovp_virt_base)) {
		ovp_trace("Fail to map address before it is used!\n");
		ret = -1;
	} else {
		ovp_trace("ioremap %s: vir_addr: %p, size: 0x%x, phy_addr: %x!\n",
			ret ? "failed" : "success",
		  hOvpCtx->ovp_virt_base, hOvpCtx->ovp_size, hOvpCtx->ovp_base);
	}

	return ret;
}

static int ovp_drv_probe(struct platform_device *pdev)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;
	int ret;
	dev_t pedev;

	ovp_device.dev = &pdev->dev;

	//Read/Parse configuration for all modules from DTS file
	ret = ovp_drv_read_cfg(hOvpCtx, pdev);
	if (ret < 0) {
		ovp_error("OVP config read fail!\n");
		goto err_fail1;
	}

	//Map only OVP space for direct reigster access
	ret = ovp_drv_create_devioremap(hOvpCtx, pdev);
	if (ret < 0) {
		ovp_error("OVP ioremap fail!\n");
		goto err_fail1;
	}

	hOvpCtx->ovp_clk = devm_clk_get(ovp_device.dev, OVP_MODULE_CLK);
	if (!IS_ERR(hOvpCtx->ovp_clk)) {
		ret = clk_prepare_enable(hOvpCtx->ovp_clk);
		if (ret < 0) {
			ovp_error("%s prepare failed..!\n", OVP_MODULE_CLK);
			goto err_fail2;
		}
	} else {
		ovp_error("failed to get %s ...!\n", OVP_MODULE_CLK);
	}

	ret = alloc_chrdev_region(&pedev, 0, OVP_MAX_DEVS, OVP_DEVICE_NAME);
	if (ret < 0) {
		ovp_error("alloc_chrdev_region() failed for ovp\n");
		goto err_fail2;
	}
	ovp_device.major = MAJOR(pedev);
	ovp_trace("register cdev device major [%d]\n", ovp_device.major);

	ret = ovp_drv_init(&ovp_device);
	if (ret)
		goto err_drv_init;

	ovp_trace("%s OK\n", __func__);

	return 0;

err_drv_init:
	unregister_chrdev_region(MKDEV(ovp_device.major, 0), OVP_MAX_DEVS);
err_fail2:
	clk_disable_unprepare(hOvpCtx->ovp_clk);
err_fail1:
	ovp_trace("%s failed !!! (%d)\n", __func__, ret);

	return ret;
}

static int ovp_drv_remove(struct platform_device *pdev)
{
	ovp_trace("%s\n", __func__);

	ovp_drv_exit(&ovp_device);

	unregister_chrdev_region(MKDEV(ovp_device.major, 0), OVP_MAX_DEVS);
	ovp_trace("unregister cdev device major [%d]\n", ovp_device.major);
	ovp_device.major = 0;

	ovp_trace("%s OK\n", __func__);

	return 0;
}

static void ovp_drv_disable_irq(void)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;

	/* disable OVP interrupt */
	disable_irq_nosync(hOvpCtx->irq_num);
}

static void ovp_drv_shutdown(struct platform_device *pdev)
{
	ovp_trace("%s\n", __func__);

	ovp_drv_disable_irq();
}

#ifdef CONFIG_PM_SLEEP
static void ovp_drv_enable_irq(void)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;

	/* disable OVP interrupt */
	enable_irq(hOvpCtx->irq_num);
}

static int ovp_drv_suspend(struct device *dev)
{
	int ret = 0;

	ovp_trace("%s\n", __func__);

	ovp_drv_disable_irq();

	ret = tz_ovp_invoke_cmd(OVP_SUSPEND);
	if (ret) {
		ovp_error("%s OVP Suspend failed\n", __func__);
		/*
		 * Kernel will stop suspend and resume other modules back.
		 * so enable irq
		 */
		ovp_drv_enable_irq();
	}

	return ret;
}

static int ovp_drv_resume(struct device *dev)
{
	int ret = 0;

	ovp_trace("%s\n", __func__);

	ret = tz_ovp_invoke_cmd(OVP_RESUME);
	if (ret) {
		ovp_error("%s OVP Resume failed\n", __func__);
		return ret;
	}

	ovp_drv_enable_irq();

	return ret;
}
#endif //CONFIG_PM_SLEEP

static SIMPLE_DEV_PM_OPS(ovp_pmops, ovp_drv_suspend, ovp_drv_resume);

static struct platform_driver ovp_driver = {
	.probe = ovp_drv_probe,
	.remove = ovp_drv_remove,
	.shutdown = ovp_drv_shutdown,
	.driver = {
		.name = OVP_DEVICE_NAME,
		.of_match_table = ovp_match,
		.pm = &ovp_pmops,
	},
};
module_platform_driver(ovp_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("OVP module driver");
