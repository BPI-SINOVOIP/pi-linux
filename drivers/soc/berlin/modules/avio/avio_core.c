// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

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
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/gpio.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

/*************************************************************************
 * Local head files
 */
#include "ctypes.h"

#include "avio_core.h"
#include "avio_debug.h"
#include "avio_ioctl.h"

/* Sub Driver Modules */
#include "avio_sub_module.h"
#include "avio_util.h"
#include "drv_dhub.h"
#include "avio_regmap.h"
#include "avio_common.h"
#include "tz_driver.h"
#include "hal_vpp_wrap.h"

#define AVIO_MODULE_NAME	"avio_module_avio"
#define AVIO_DEVICE_NAME	"avio"
#define AVIO_DEVICE_PATH	("/dev/" AVIO_DEVICE_NAME)
#define AVIO_MAX_DEVS		8
#define AVIO_MINOR			0

/***********************************************************************
 * Module Variable
 */

static int avio_device_init(struct avio_device_t *avio_dev, unsigned int user)
{
	avio_sub_module_init();

	avio_trace("%s ok\n", __func__);

	return S_OK;
}

static int avio_device_exit(struct avio_device_t *avio_dev, unsigned int user)
{
	avio_sub_module_exit();

	avio_trace("%s ok\n", __func__);

	return S_OK;
}

/**********************************************************************
 * Module API
 */

static atomic_t avio_dev_refcnt = ATOMIC_INIT(0);

static void avio_disable_irq(void)
{
	/* disable all enabled interrupt */
	avio_sub_module_disable_irq();
}

static int avio_driver_open(struct inode *inode, struct file *filp)
{
	struct avio_device_t *dev;

	int err = 0;

	dev = container_of(inode->i_cdev, struct avio_device_t, cdev);
	filp->private_data = dev;

	mutex_lock(&dev->avio_mutex);

	avio_trace("Start open avio driver!\n");

	if (atomic_inc_return(&avio_dev_refcnt) > 1) {
		avio_trace("avio driver reference count %d!\n",
			  atomic_read(&avio_dev_refcnt));
		goto err_exit;
	}

	err = avio_sub_module_open();

	if (err == 0)
		avio_trace("%s ok\n", __func__);

err_exit:
	mutex_unlock(&dev->avio_mutex);

	return err;
}

static int avio_driver_release(struct inode *inode, struct file *filp)
{
	struct avio_device_t *dev = (struct avio_device_t *)filp->private_data;

	mutex_lock(&dev->avio_mutex);

	if (atomic_read(&avio_dev_refcnt) == 0) {
		avio_trace("avio driver already released!\n");
		goto err_exit;
	}

	if (atomic_dec_return(&avio_dev_refcnt)) {
		avio_trace("avio dev ref cnt after this release: %d!\n",
				atomic_read(&avio_dev_refcnt));
		goto err_exit;
	}

	//Disable irq before close
	avio_disable_irq();

	avio_sub_module_close();

	avio_trace("%s ok\n", __func__);

err_exit:
	mutex_unlock(&dev->avio_mutex);

	return 0;
}

static int avio_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;

	ret = avio_driver_memmap(file, vma);

	if (ret)
		avio_trace("%s failed, ret = %d\n", __func__, ret);
	else
		avio_trace("%s ok\n", __func__);
	return ret;
}

static long avio_driver_ioctl_unlocked(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retVal;

	if (avio_sub_module_ioctl_unlocked(cmd, arg, &retVal)) {
		//If processed by any of the sub module, then return immediately
		return retVal;
	}

	retVal = avio_util_ioctl_unlocked(filp, cmd, arg);

	return retVal;
}

/*********************************************************************
 * Module Register API
 */

static const struct file_operations avio_ops = {
	.open = avio_driver_open,
	.release = avio_driver_release,
	.unlocked_ioctl = avio_driver_ioctl_unlocked,
	.compat_ioctl = avio_driver_ioctl_unlocked,
	.mmap = avio_driver_mmap,
	.owner = THIS_MODULE,
};

static struct avio_device_t avio_device = {
	.dev_name = AVIO_DEVICE_NAME,
	.minor = AVIO_MINOR,
	.dev_init = avio_device_init,
	.dev_exit = avio_device_exit,
	.fops = &avio_ops,
};

static const struct of_device_id avio_match[] = {
	{
		.compatible = "syna,berlin-avio",
	},
	{},
};

static int
avio_driver_setup_cdev(struct cdev *dev, int major, int minor,
			  const struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static int avio_drv_init(struct avio_device_t *avio_device)
{
	int res;

	/* Now setup cdevs. */
	res = avio_driver_setup_cdev(&avio_device->cdev, avio_device->major,
				  avio_device->minor, avio_device->fops);
	if (res) {
		avio_error("avio_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}
	avio_trace("setup cdevs device minor [%d]\n", avio_device->minor);

	/* add PE devices to sysfs */
	avio_device->dev_class =
		class_create(THIS_MODULE, avio_device->dev_name);
	if (IS_ERR(avio_device->dev_class)) {
		avio_error("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(avio_device->dev_class, NULL,
			  MKDEV(avio_device->major, avio_device->minor), NULL,
			  avio_device->dev_name);
	avio_trace("create device sysfs [%s]\n", avio_device->dev_name);

	/* create hw device */
	if (avio_device->dev_init) {
		res = avio_device->dev_init(avio_device, 0);
		if (res != 0) {
			avio_error("avio_int_init failed !!! res = 0x%08X\n",
				  res);
			res = -ENODEV;
			goto err_add_device;
		}
	}

	/* create PE device proc file */
	avio_device->dev_procdir = proc_mkdir(avio_device->dev_name, NULL);
	avio_sub_module_create_proc_file(avio_device->dev_procdir);

	return 0;

err_add_device:
	if (avio_device->dev_class) {
		device_destroy(avio_device->dev_class, MKDEV(avio_device->major,
			avio_device->minor));
		class_destroy(avio_device->dev_class);
	}

	cdev_del(&avio_device->cdev);

	return res;
}

static int avio_drv_exit(struct avio_device_t *avio_device)
{
	int res;

	avio_trace("%s [%s] enter\n", __func__, avio_device->dev_name);

	/* destroy kernel API */
	if (avio_device->dev_exit) {
		res = avio_device->dev_exit(avio_device, 0);
		if (res != 0)
			avio_error("%s failed !!! res = 0x%08X\n",
				__func__, res);
	}

	if (avio_device->dev_class) {
		/* del sysfs entries */
		device_destroy(avio_device->dev_class, MKDEV(avio_device->major,
			avio_device->minor));
		avio_trace("delete device sysfs [%s]\n", avio_device->dev_name);

		class_destroy(avio_device->dev_class);
	}
	/* del cdev */
	cdev_del(&avio_device->cdev);

	return 0;
}

int is_avio_driver_initialized(void)
{
	return avio_device.dev_class ? 1 : 0;
}
EXPORT_SYMBOL(is_avio_driver_initialized);

static int avio_probe(struct platform_device *pdev)
{
	int ret;
	dev_t pedev;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!tzd_get_kernel_dev_file())
		return -EPROBE_DEFER;

	//Initialize/probe sub module framework
	ret = avio_sub_module_probe(pdev);
	if (ret < 0) {
		avio_error("AVIO sub module probe fail!\n");
		goto err_fail1;
	}

	//Read/Parse configuration for all modules from DTS file
	avio_sub_module_config(pdev);

	//Map only AVIO space for direct reigster access
	ret = avio_create_devioremap(pdev);
	if (ret < 0) {
		avio_error("AVIO ioremap fail!\n");
		goto err_fail1;
	}

	ret = alloc_chrdev_region(&pedev, 0, AVIO_MAX_DEVS, AVIO_DEVICE_NAME);
	if (ret < 0) {
		avio_error("alloc_chrdev_region() failed for avio\n");
		goto err_fail1;
	}
	avio_device.major = MAJOR(pedev);
	avio_trace("register cdev device major [%d]\n", avio_device.major);

	ret = avio_drv_init(&avio_device);
	if (ret)
		goto err_drv_init;

	mutex_init(&avio_device.avio_mutex);

	avio_trace("%s OK\n", __func__);

	return 0;

err_drv_init:
	unregister_chrdev_region(MKDEV(avio_device.major, 0), AVIO_MAX_DEVS);
err_fail1:
	avio_trace("%s failed !!! (%d)\n", __func__, ret);

	return ret;
}

static int avio_remove(struct platform_device *pdev)
{
	avio_trace("%s\n", __func__);

	avio_drv_exit(&avio_device);

	unregister_chrdev_region(MKDEV(avio_device.major, 0), AVIO_MAX_DEVS);
	avio_trace("unregister cdev device major [%d]\n", avio_device.major);
	avio_device.major = 0;

	avio_destroy_deviounmap();

	avio_trace("%s OK\n", __func__);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static void avio_enable_irq(void)
{
	/* disable all enabled interrupt */
	avio_sub_module_enable_irq();
}

static int avio_suspend(struct device *dev)
{
	int ret;

	avio_trace("%s\n", __func__);

	avio_disable_irq();

	ret = avio_sub_module_save_state();
	// If fail, kernel will resume all modules back. so enable irq
	if (ret)
		avio_enable_irq();

	return ret;
}

static int avio_resume(struct device *dev)
{
	int ret;

	avio_trace("%s\n", __func__);

	ret = avio_sub_module_restore_state();
	if (ret)
		avio_error("Restoring state failed %s\n", __func__);

	avio_enable_irq();

	return ret;
}
#endif //CONFIG_PM_SLEEP

int avio_module_avio_probe(struct platform_device *pdev)
{
	AVIO_CTX *hAvioCtx;
	int err = 0;

	avio_trace("%s\n", __func__);

	hAvioCtx = devm_kzalloc(&pdev->dev, sizeof(AVIO_CTX), GFP_KERNEL);
	if (!hAvioCtx)
		return -ENOMEM;

	//Always TZ is enabled/supported.
	hAvioCtx->isTeeEnabled = 1;

	sema_init(&hAvioCtx->resume_sem, 0);

	hAvioCtx->pAvioRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hAvioCtx->avio_base = hAvioCtx->pAvioRes->start;
	hAvioCtx->avio_size = resource_size(hAvioCtx->pAvioRes);

	avio_sub_module_register(AVIO_MODULE_TYPE_AVIO, AVIO_MODULE_NAME,
		hAvioCtx, NULL);

	return err;
}

static SIMPLE_DEV_PM_OPS(avio_pmops, avio_suspend, avio_resume);

static struct platform_driver avio_driver = {
	.probe = avio_probe,
	.remove = avio_remove,
	.driver = {
		.name = AVIO_DEVICE_NAME,
		.of_match_table = avio_match,
		.pm = &avio_pmops,
	},
};
module_platform_driver(avio_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AVIO module driver");
