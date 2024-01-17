// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/kobject.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/ide.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include "msg.h"

#define DEVICE_SNAME "s_syna_msg_1"
#define DEVICE_DNAME "d_syna_msg_1"
#define DEVICE_MINOR_NUMBER 0
#define DEV_MA 300
#define DEV_MI 0
#define DEV_NUM 1


#define syna_msg_t 'H'
#define MSG_HPD_SET_CMD _IOW(syna_msg_t, 0x88, int)
dev_t num_dev;
static int num_dev_ma = DEV_MA;
static int num_dev_mi = DEV_MI;
module_param(num_dev_ma, int, S_IWUSR);
module_param(num_dev_mi, int, S_IWUSR);
struct class *cls;
struct device *dev;
struct cdev cdevm;

typedef int (*SYNA_MSG_CALLBACK)(unsigned int EventCode, void *EventInfo, void *Context);
SYNA_MSG_CALLBACK msg_callback = NULL;


int syna_msg_callback_register(void* func)
{
	msg_callback = (SYNA_MSG_CALLBACK)func;
	printk("syna_msg_callback_register:%p\n", msg_callback);
	return 0;
}
EXPORT_SYMBOL(syna_msg_callback_register);

int syna_msg_callback_unregister(void* func)
{
	msg_callback = NULL;
	return 0;
}
EXPORT_SYMBOL(syna_msg_callback_unregister);


static long ioctl_msg_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case MSG_HPD_SET_CMD:
		printk("ioctl_msg_unlocked_ioctl address:%ld  value:%d\n", arg, *((int *)arg));
		if (msg_callback != NULL)
			msg_callback(0, arg, NULL);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ioctl_msg_unlocked_ioctl,
	.compat_ioctl = ioctl_msg_unlocked_ioctl,
};

static int syna_msg_probe(struct platform_device *pdev)
{
	printk("syna_msg_probe\n");
	int ret = 0;

	num_dev = MKDEV(num_dev_ma, num_dev_mi);

	ret = register_chrdev_region(num_dev, DEV_NUM, DEVICE_SNAME);
	if (ret < 0) {
		printk("register_chrdev_region error!\n");
		ret = alloc_chrdev_region(&num_dev, DEVICE_MINOR_NUMBER, DEV_NUM, DEVICE_DNAME);
		if (ret < 0)
			printk("allo_chrdev_region error!\n");
		else {
			num_dev_ma = MAJOR(num_dev);
			num_dev_mi = MINOR(num_dev);
			printk("allo_chrdev_region ok!\n");
		}
	} else
		printk("register_chrdev_region ok!\n");

	cdev_init(&cdevm, &fops);
	cdevm.owner = THIS_MODULE;
	ret = cdev_add(&cdevm, num_dev, DEV_NUM);

	//create node
	cls = class_create(THIS_MODULE, "cls_syna_msg");
	if (IS_ERR(cls)) {
		printk("class create error\n");
		goto err;
	}

	dev = device_create(cls, NULL, num_dev, NULL, "dev_syna_msg");
	if (IS_ERR(dev)) {
		printk("device create error\n");
		goto err;
	}
	printk(KERN_INFO "syna_msg_probe: Interrupt registered\n");

	return ret;

err:
	if (dev == NULL)
		device_destroy(cls, num_dev);
	if (cls == NULL)
		class_destroy(cls);
	cdev_del(&cdevm);
	unregister_chrdev_region(num_dev, DEV_NUM);
	return ret;
}

static int syna_msg_remove(struct platform_device *pdev)
{
	device_destroy(cls, num_dev);
	class_destroy(cls);
	cdev_del(&cdevm);
	unregister_chrdev_region(num_dev, DEV_NUM);

	printk("chrdev ioctl_demo is rmmod and unregister success\n");
	return 0;
}

static void syna_msg_shutdown(struct platform_device *pdev)
{
}

static const struct of_device_id syna_msg_match[] = {
	{.compatible = "syna,berlin-msg",},
	{},
};

static struct platform_driver syna_msg_driver = {
	.driver = {
		.name   	= "msg-dev",
		.of_match_table = syna_msg_match,
	},
	.probe          = syna_msg_probe,
	.remove         = syna_msg_remove,
	.shutdown	    = syna_msg_shutdown,
};

static int __init syna_msg_init(void)
{
	return platform_driver_register(&syna_msg_driver);
}

static void __exit syna_msg_exit(void)
{
	platform_driver_unregister(&syna_msg_driver);
}

module_init(syna_msg_init);
module_exit(syna_msg_exit);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MSG module driver");
