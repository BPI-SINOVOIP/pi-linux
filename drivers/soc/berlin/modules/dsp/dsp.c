// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */
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
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/idr.h>
#include <linux/arm-smccc.h>
#include <soc/berlin/berlin_sip.h>

#include "dsp_def.h"
#include "drv_msg.h"

#define DSP_DEVICE_TAG			"[dsp kernel driver] "
#define DSP_DEVICE_NAME			"dsp"

#define DSP_DEV_MAX			2
static DEFINE_IDA(dsp_minors);

#define EVENT_QUEUE_SIZE		(8)
#define MAX_DEV_NAME_LEN		(0x10)

/* local Register addr */
#define RA_hifiWrapper_IPC_int_to_acpu		(0x000C)

static dev_t dsp_major;
static struct class *dsp_class;

/**
 * struct dsp_drv - DSP driver structure
 * @dev: cached device pointer
 * @reg_base: register base virtual address
 * @irq_num: IRQ number when DSP sends nofitifaction to ARM
 * @msg_q: DSP driver will push the notification into this queue in ISR
 * @dsp_sem: User-space thread waits on this semaphore to get msg from DSP
 * @dsp_dev_refcnt: referene cnt of this device
 * @dsp_mutex:
 * @cdev:
 * @dev_class:
 * @devno: device number
 * @hw_name: device name
 * @hw_id: device id
 */
struct dsp_drv {
	struct device *dev;
	void __iomem *reg_base;
	int irq_num;
	AMPMsgQ_t msg_q;
	struct semaphore dsp_sem;
	atomic_t dsp_dev_refcnt;
	struct mutex dsp_mutex;
	struct cdev cdev;
	struct class *dev_class;
	dev_t devno;
	char hw_name[MAX_DEV_NAME_LEN];
	int hw_id;
};

static u32 dsp_tz_phy_to_secure_reg(unsigned int hw_id)
{
	u32 secureReg = -1;

	if (hw_id == 0) {
		secureReg = DSP0_INTR_STATUS;
	} else if (hw_id == 1) {
		secureReg = DSP1_INTR_STATUS;
	}
	return secureReg;
}

static void dsp_secure_register_write(unsigned int hw_id, unsigned int regVal)
{
	struct arm_smccc_res res;

	u32 secureReg = dsp_tz_phy_to_secure_reg(hw_id);

	arm_smccc_smc(SYNA_SIP_SMC64_SREGISTER_OP,
			SYNA_SREGISTER_WRITE,
			secureReg,
			regVal,
			0, 0, 0, 0,
			&res);
	if (res.a0) {
		pr_err("[dsp driver]sec reg write failed id(%u)\n", secureReg);
	}
}

static irqreturn_t dsp_receive(int irq, void *dev_id)
{
	struct dsp_drv *p_dsp_drv = (struct dsp_drv *)dev_id;

	up(&p_dsp_drv->dsp_sem);
	/* clear INT: RA_hifiWrapper_IPC_int_to_acpu */
	dsp_secure_register_write(p_dsp_drv->hw_id, 0);

	return IRQ_HANDLED;
}

static int dsp_driver_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct dsp_drv *p_dsp_drv = container_of(inode->i_cdev,
		struct dsp_drv, cdev);

	filp->private_data = p_dsp_drv;

	mutex_lock(&p_dsp_drv->dsp_mutex);
	if (atomic_inc_return(&p_dsp_drv->dsp_dev_refcnt) > 1) {
		goto exit;
	}

	err = request_irq(p_dsp_drv->irq_num, dsp_receive, IRQF_ONESHOT,
		p_dsp_drv->hw_name, (void *) p_dsp_drv);
	if (unlikely(err < 0)) {
		dev_err(p_dsp_drv->dev, "irq fail,irq_num:%5d, %s, err:%8x\n"
			, p_dsp_drv->irq_num, p_dsp_drv->hw_name
			, err);
		goto exit;
	}

exit:
	mutex_unlock(&p_dsp_drv->dsp_mutex);
	return err;
}

/**
 * clean the items in queue when close /dev/dsp
 * Otherwise, when open /dev/dsp for the second time,
 * it may contains histroy invalid data.
 * This case happens when /dev/dsp is closed, however, driver queue is not empty
 */
static void dsp_driver_reset_queue(struct dsp_drv *p_dsp_drv)
{
	int rc;
	int times = 0;
	MV_CC_MSG_t msg = { 0 };

	/* Clean queue and sema to make sure there is no invalid histroy data */
	times = 0;
	while (times < EVENT_QUEUE_SIZE) {
		rc = AMPMsgQ_ReadTry(&p_dsp_drv->msg_q, &msg);
		if (unlikely(rc != S_OK))
			break;
		times++;
		AMPMsgQ_ReadFinish(&p_dsp_drv->msg_q);
	}
	if (times > 0) {
		dev_info(p_dsp_drv->dev, "%s: Still %d notification in the msg_q\n",
			__func__, times);
	}
	sema_init(&p_dsp_drv->dsp_sem, 0);
}

static int dsp_driver_release(struct inode *inode, struct file *filp)
{
	struct dsp_drv *p_dsp_drv;

	p_dsp_drv = container_of(inode->i_cdev, struct dsp_drv, cdev);

	mutex_lock(&p_dsp_drv->dsp_mutex);
	if (atomic_dec_return(&p_dsp_drv->dsp_dev_refcnt)) {
		goto exit;
	}

	free_irq(p_dsp_drv->irq_num, (void *)(p_dsp_drv));
	dsp_driver_reset_queue(p_dsp_drv);
exit:
	mutex_unlock(&p_dsp_drv->dsp_mutex);
	return 0;
}

static long dsp_driver_ioctl_unlocked(struct file *filp, unsigned int ioctl_cmd,
	unsigned long arg)
{
	struct dsp_drv *p_dsp_drv = (struct dsp_drv *)filp->private_data;
	MV_CC_MSG_t msg = { 0 };
	int rc = S_OK;

	switch (ioctl_cmd) {
	case DSP_IOCTL_RECEIVE_NOTIFIER:
	{
		rc = down_interruptible(&p_dsp_drv->dsp_sem);
		if (rc < 0) {
			return rc;
		}
		/*only care about msg UNBLOCK_USER_ISR*/
		if (AMPMsgQ_Fullness(&p_dsp_drv->msg_q) <= 0) {
			return rc;
		}
		AMPMsgQ_DequeueRead(&p_dsp_drv->msg_q, &msg);
		if (copy_to_user((void __user *) arg, &msg,
			sizeof(MV_CC_MSG_t))) {
			return -EFAULT;
		}
		break;
	}
	case DSP_IOCTL_UNBLOCK_USER_ISR:
	{
		/**
		 * the ISR thread is blocked in down_interruptible, to delete
		 * this ISR thread, fake a notification to unblock it.
		 * msg value is a different from normal msg case,
		 * so ISR can know it's a msg to *terminate itself
		 */
		msg.m_Param1 = UNBLOCK_USER_ISR;
		AMPMsgQ_Add(&p_dsp_drv->msg_q, &msg);
		up(&p_dsp_drv->dsp_sem);
		break;
	}
	default:
		dev_err(p_dsp_drv->dev, "Unsupported IOCTL command\n");
		break;
	}
	return rc;
}

static const struct file_operations dsp_ops = {
	.open = dsp_driver_open,
	.release = dsp_driver_release,
	.unlocked_ioctl = dsp_driver_ioctl_unlocked,
	.compat_ioctl = dsp_driver_ioctl_unlocked,
	.owner = THIS_MODULE,
};

static int syna_dsp_probe(struct platform_device *pdev)
{
	int err;
	struct device *dev = &pdev->dev;
	struct dsp_drv *p_dsp_drv;
	int dsp_id;
	const char * const template = "dsp%d";

	p_dsp_drv = devm_kzalloc(&pdev->dev, sizeof(struct dsp_drv), GFP_KERNEL);
	if (!p_dsp_drv)
		return -ENOMEM;

	/* get dsp id and set hw_name */
	dsp_id = ida_simple_get(&dsp_minors, 0, DSP_DEV_MAX, GFP_KERNEL);
	if (dsp_id < 0) {
		dev_err(p_dsp_drv->dev, "failed to get dsp id");
		return -ERANGE;
	}
	snprintf(p_dsp_drv->hw_name, MAX_DEV_NAME_LEN, template, dsp_id);
	p_dsp_drv->hw_id = dsp_id;
	p_dsp_drv->dev = &pdev->dev;

	/*Get register address*/
	p_dsp_drv->reg_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(p_dsp_drv->reg_base)) {
		dev_err(p_dsp_drv->dev, "failed to get io base resource");
		err = PTR_ERR(p_dsp_drv->reg_base);
		goto free_ida;
	}

	/* Get IRQ */
	p_dsp_drv->irq_num = platform_get_irq_byname(pdev, "dsp2acpu");
	if (p_dsp_drv->irq_num <= 0) {
		dev_err(p_dsp_drv->dev, "failed to get irq resource\n");
		err = -ENXIO;
		goto free_ida;
	}

	sema_init(&p_dsp_drv->dsp_sem, 0);
	mutex_init(&p_dsp_drv->dsp_mutex);

	err = AMPMsgQ_Init(&p_dsp_drv->msg_q, EVENT_QUEUE_SIZE);
	if (unlikely(err != S_OK)) {
		dev_err(p_dsp_drv->dev, "msg_q init failed, err:%8x\n", err);
		goto free_ida;
	}

	p_dsp_drv->dev_class = dsp_class;
	p_dsp_drv->devno = MKDEV(MAJOR(dsp_major), dsp_id);

	cdev_init(&p_dsp_drv->cdev, &dsp_ops);
	err = cdev_add(&p_dsp_drv->cdev, p_dsp_drv->devno, 1);
	if (err)
		goto err_prob_device_0;
	p_dsp_drv->cdev.owner = THIS_MODULE;

	/* creates device and registers it with sysfs */
	dev = device_create(p_dsp_drv->dev_class, NULL,
				p_dsp_drv->devno, NULL, p_dsp_drv->hw_name);
	if (IS_ERR(dev)) {
		dev_err(p_dsp_drv->dev, "device_create failed.\n");
		err = PTR_ERR(dev);
		goto err_prob_device_1;
	}
	platform_set_drvdata(pdev, p_dsp_drv);
	return 0;

err_prob_device_1:
	cdev_del(&p_dsp_drv->cdev);
err_prob_device_0:
	AMPMsgQ_Destroy(&p_dsp_drv->msg_q);
free_ida:
	ida_simple_remove(&dsp_minors, p_dsp_drv->hw_id);
	return err;
}

static int syna_dsp_remove(struct platform_device *pdev)
{
	int err;
	struct dsp_drv *p_dsp_drv;

	p_dsp_drv = platform_get_drvdata(pdev);

	cdev_del(&p_dsp_drv->cdev);
	device_destroy(p_dsp_drv->dev_class, p_dsp_drv->devno);
	ida_simple_remove(&dsp_minors, p_dsp_drv->hw_id);

	err = AMPMsgQ_Destroy(&p_dsp_drv->msg_q);
	if (unlikely(err != S_OK)) {
		dev_err(p_dsp_drv->dev, "drv_app_exit: failed, err:%8x\n", err);
		return err;
	}
	return 0;
}

static const struct of_device_id dsp_match[] = {
	{ .compatible = "syna,hifi-dsp" },
	{}

};
MODULE_DEVICE_TABLE(of, dsp_match);

static struct platform_driver syna_dsp_driver = {
	.probe = syna_dsp_probe,
	.remove = syna_dsp_remove,
	.driver = {
		.name = "hifi-dsp",
		.of_match_table = dsp_match,
	},
};

static int __init syna_dsp_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&dsp_major, 0, DSP_DEV_MAX, DSP_DEVICE_NAME);
	if (ret) {
		pr_err("dsp: failed to allocate char dev region\n");
		return ret;
	}
	dsp_class = class_create(THIS_MODULE, DSP_DEVICE_NAME);
	if (IS_ERR(dsp_class)) {
		pr_err("dsp: failed to create class\n");
		ret = PTR_ERR(dsp_class);
		goto failed0;
	}
	ret = platform_driver_register(&syna_dsp_driver);
	if (ret) {
		pr_err("dsp: driver register faild\n");
		goto failed1;
	}
	return ret;

failed1:
	class_destroy(dsp_class);
failed0:
	unregister_chrdev_region(dsp_major, DSP_DEV_MAX);
	return ret;
}
module_init(syna_dsp_init);

static void __exit syna_dsp_exit(void)
{
	platform_driver_unregister(&syna_dsp_driver);
	class_destroy(dsp_class);
	unregister_chrdev_region(dsp_major, DSP_DEV_MAX);
	ida_destroy(&dsp_minors);
}
module_exit(syna_dsp_exit);

MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DSP module driver");
