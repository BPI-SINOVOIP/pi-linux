// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#define pr_fmt(fmt) "[cec kernel driver]" fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/mod_devicetable.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include "drv_msg.h"
#include "cec.h"

#define CEC_DEVICE_NAME			"cec"
#define CEC_DEVICE_PATH			("/dev/" CEC_DEVICE_NAME)
#define CEC_MAX_DEVS			2
#define CEC_MINOR				0

/*******************************************************************************
  Static Variables
  */
static struct cec_device_t cec_dev = {
	.dev_name = CEC_DEVICE_NAME,
	.minor = CEC_MINOR,
};

/*******************************************************************************
  Module internal function
  */
#define CEC_REG_BYTE_WRITE(addr, data) \
	writeb_relaxed(data, ((addr << 2) + cec_dev.cec_virt_addr))
#define CEC_REG_BYTE_READ(offset, holder)	\
	(*(holder) = readb_relaxed((offset << 2) + cec_dev.cec_virt_addr))

static irqreturn_t cec_devices_isr(int irq, void *dev_id)
{
	struct cec_context *hCecCtx = (struct cec_context *)dev_id;
	unsigned short value = 0;
	unsigned short reg = 0;
	int intr;
	int i;
	int dptr_len = 0;
	int ret = S_OK;

	MV_CC_MSG_t msg = { CEC_CC_MSG_TYPE, 0, 0 };

	// Read CEC status register
	CEC_REG_BYTE_READ(CEC_INTR_STATUS0_REG_ADDR, &value);
	reg = (unsigned short) value;
	CEC_REG_BYTE_READ(CEC_INTR_STATUS1_REG_ADDR, &value);
	reg |= ((unsigned short) value << 8);

	// Clear CEC interrupta
	if (reg & BE_CEC_INTR_TX_FAIL) {
		intr = BE_CEC_INTR_TX_FAIL;
		CEC_REG_BYTE_WRITE(CEC_RDY_ADDR, 0x0);
		CEC_REG_BYTE_READ(CEC_INTR_ENABLE0_REG_ADDR, &value);
		value &= ~(intr & 0x00ff);
		CEC_REG_BYTE_WRITE(CEC_INTR_ENABLE0_REG_ADDR, value);
	}
	if (reg & BE_CEC_INTR_TX_COMPLETE) {
		intr = BE_CEC_INTR_TX_COMPLETE;
		CEC_REG_BYTE_WRITE(CEC_RDY_ADDR, 0x0);
		CEC_REG_BYTE_READ(CEC_INTR_ENABLE0_REG_ADDR, &value);
		value &= ~(intr & 0x00ff);
		CEC_REG_BYTE_WRITE(CEC_INTR_ENABLE0_REG_ADDR, value);
	}
	if (reg & BE_CEC_INTR_RX_FAIL) {
		intr = BE_CEC_INTR_RX_FAIL;
		CEC_REG_BYTE_READ(CEC_INTR_ENABLE0_REG_ADDR, &value);
		value &= ~(intr & 0x00ff);
		CEC_REG_BYTE_WRITE(CEC_INTR_ENABLE0_REG_ADDR, value);
	}
	if (reg & BE_CEC_INTR_RX_COMPLETE) {
		intr = BE_CEC_INTR_RX_COMPLETE;
		CEC_REG_BYTE_READ(CEC_INTR_ENABLE0_REG_ADDR, &value);
		value &= ~(intr & 0x00ff);
		CEC_REG_BYTE_WRITE(CEC_INTR_ENABLE0_REG_ADDR, value);
		// read cec mesg from rx buffer
		CEC_REG_BYTE_READ(CEC_RX_FIFO_DPTR, &dptr_len);
		hCecCtx->rx_buf.len = dptr_len;
		for (i = 0; i < dptr_len; i++) {
			CEC_REG_BYTE_READ(CEC_RX_BUF_READ_REG_ADDR, &hCecCtx->rx_buf.buf[i]);
			CEC_REG_BYTE_WRITE(CEC_TOGGLE_FOR_READ_REG_ADDR, 0x01);
		}
		CEC_REG_BYTE_WRITE(CEC_RX_RDY_ADDR, 0x00);
		CEC_REG_BYTE_WRITE(CEC_RX_RDY_ADDR, 0x01);
		CEC_REG_BYTE_READ(CEC_INTR_ENABLE0_REG_ADDR, &value);
		value |= (intr & 0x00ff);
		CEC_REG_BYTE_WRITE(CEC_INTR_ENABLE0_REG_ADDR, value);
	}

	msg.m_Param1 = reg;
	ret = AMPMsgQ_Add(&hCecCtx->hCECMsgQ, &msg);
	if (likely(ret == S_OK)) {
		up(&hCecCtx->cec_sem);
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_PM_SLEEP
static void cec_disable_irq(void)
{
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);
	/* disable all enabled interrupt */
	if (pCecCtx->gCecIsrEnState && (atomic_dec_if_positive(&pCecCtx->irq_stat) == 0)) {
		/* disable CEC interrupt */
		disable_irq_nosync(pCecCtx->cec_irq);
	}
}

static void cec_enable_irq(void)
{
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);
	/* disable all enabled interrupt */
	if (pCecCtx->gCecIsrEnState && atomic_add_unless(&pCecCtx->irq_stat, 1, 1)) {
		/* disable CEC interrupt */
		enable_irq(pCecCtx->cec_irq);
	}
}

static int cec_suspend(struct device *dev)
{
	pr_info("cec_suspend\n");

	cec_disable_irq();

	return 0;
}

static int cec_resume(struct device *dev)
{
	pr_info("cec_resume\n");

	cec_enable_irq();
	return 0;
}
#endif

static int cec_device_init(struct cec_device_t *cec_dev, unsigned int user)
{
	unsigned int err;

	sema_init(&(cec_dev->CecCtx.cec_sem), 0);
	err = AMPMsgQ_Init(&(cec_dev->CecCtx.hCECMsgQ), CEC_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		pr_err("drv_cec_init: hCECMsgQ init failed, err:%8x\n", err);
		return -1;
	}

	return S_OK;
}

static int cec_device_exit(struct cec_device_t *cec_dev, unsigned int user)
{
	unsigned int err;

	err = AMPMsgQ_Destroy(&(cec_dev->CecCtx.hCECMsgQ));
	if (unlikely(err != S_OK)) {
		pr_err("drv_cec_exit: CEC MsgQ Destroy failed, err:%8x\n",
			err);
		return -1;
	}

	return S_OK;
}

/*******************************************************************************
  Module Register API
  */
static atomic_t cec_dev_refcnt = ATOMIC_INIT(0);
static int cec_driver_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);

	mutex_lock(&pCecCtx->cec_mutex);
	if (atomic_inc_return(&cec_dev_refcnt) > 1) {
		pr_err("cec driver reference count %d!\n",
			 atomic_read(&cec_dev_refcnt));
		goto err_driver_open;
	}
	err = request_irq(pCecCtx->cec_irq, cec_devices_isr, 0,
			  "cec_module", (void *)pCecCtx);
	if (unlikely(err < 0)) {
		pr_err("cec_irq:%5d, err:%8x\n", pCecCtx->cec_irq, err);
		goto err_driver_open;
	}
	pCecCtx->gCecIsrEnState = 1;
	atomic_set(&pCecCtx->irq_stat, 1);

err_driver_open:
	mutex_unlock(&pCecCtx->cec_mutex);
	return err;
}

static int cec_driver_release(struct inode *inode, struct file *filp)
{
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);

	mutex_lock(&pCecCtx->cec_mutex);
	if (atomic_read(&cec_dev_refcnt) == 0) {
		pr_err("cec driver already released!\n");
		goto err_driver_release;
	}

	if (atomic_dec_return(&cec_dev_refcnt)) {
		pr_err("cec dev ref cnt after this release: %d!\n",
			 atomic_read(&cec_dev_refcnt));
		goto err_driver_release;
	}

	free_irq(pCecCtx->cec_irq, (void *)pCecCtx);
	pCecCtx->gCecIsrEnState = 0;
	atomic_set(&pCecCtx->irq_stat, 0);

err_driver_release:
	mutex_unlock(&pCecCtx->cec_mutex);
	return 0;
}

static int cec_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long addr =
	    pCecCtx->pCecRes->start + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= pCecCtx->pCecRes->start &&
	      (addr + size) <= pCecCtx->pCecRes->start + resource_size(pCecCtx->pCecRes))) {
		pr_err("Invalid address, start=0x%lx, end=0x%lx\n", addr,
			addr + size);
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma,
			      vma->vm_start,
			      (pCecCtx->pCecRes->start >> PAGE_SHIFT) +
			      vma->vm_pgoff, size, vma->vm_page_prot);
	if (ret)
		pr_err("cec_driver_mmap failed, ret = %d\n", ret);
	return ret;
}

static long cec_driver_ioctl_unlocked(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);
	switch (cmd) {
	case CEC_IOCTL_INTR_MSG:
	{
		int intr_en = 0;

		if (unlikely(copy_from_user
			(&intr_en, (void __user *) arg,
			sizeof(int)) > 0)) {
			return -EFAULT;
		}

		if (pCecCtx->gCecIsrEnState) {
			/* disable CEC interrupt */
			if (intr_en) {
				if (atomic_add_unless(&pCecCtx->irq_stat, 1, 1))
					enable_irq(pCecCtx->cec_irq);
			} else {
				if (atomic_dec_if_positive(&pCecCtx->irq_stat) == 0)
					disable_irq(pCecCtx->cec_irq);
			}
		}
		break;
	}

	case CEC_IOCTL_GET_MSG:
	{
		MV_CC_MSG_t msg = { 0 };
		int rc = S_OK;
		rc = down_interruptible(&pCecCtx->cec_sem);
		if (unlikely(rc < 0)) {
			return rc;
		}
		rc = AMPMsgQ_ReadTry(&pCecCtx->hCECMsgQ, &msg);
		if (unlikely(rc != S_OK)) {
			pr_err("CEC read message queue failed\n");
			return -EFAULT;
		}
		AMPMsgQ_ReadFinish(&pCecCtx->hCECMsgQ);
		if (unlikely(copy_to_user
				((void __user *) arg, &msg,
				sizeof(MV_CC_MSG_t)) > 0)) {
			return -EFAULT;
		}
		break;
	}

	case CEC_IOCTL_RX_MSG_BUF:  // copy cec rx message to user space buffer
		if (unlikely(copy_to_user
			((void __user *) arg, &pCecCtx->rx_buf,
			sizeof(CEC_RX_MSG_BUF)) > 0))
			return -EFAULT;
		break;
	default:
		break;
	}
	return 0;
}

static const struct file_operations cec_ops = {
	.open = cec_driver_open,
	.release = cec_driver_release,
	.unlocked_ioctl = cec_driver_ioctl_unlocked,
	.compat_ioctl = cec_driver_ioctl_unlocked,
	.mmap = cec_driver_mmap,
	.owner = THIS_MODULE,
};

static int
cec_driver_setup_cdev(struct cdev *dev, int major, int minor,
		      const struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static int cec_drv_init(struct cec_device_t *cec_device)
{
	int res;

	/* Now setup cdevs. */
	res = cec_driver_setup_cdev(&cec_device->cdev, cec_device->major,
				  cec_device->minor, &cec_ops);
	if (res) {
		pr_err("cec_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	/* add CEC devices to sysfs */
	cec_device->dev_class = class_create(THIS_MODULE, cec_device->dev_name);
	if (IS_ERR(cec_device->dev_class)) {
		pr_err("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(cec_device->dev_class, NULL,
		      MKDEV(cec_device->major, cec_device->minor), NULL,
		      cec_device->dev_name);

	/* create hw device */
	res = cec_device_init(cec_device, 0);
	if (res != 0) {
		pr_err("cec_int_init failed !!! res = 0x%08X\n",
			res);
		res = -ENODEV;
		goto err_add_device;
	}

	return 0;
err_add_device:
	if (cec_device->dev_class) {
		device_destroy(cec_device->dev_class,
			       MKDEV(cec_device->major, cec_device->minor));
		class_destroy(cec_device->dev_class);
	}

	cdev_del(&cec_device->cdev);

	return res;
}

static int cec_drv_exit(struct cec_device_t *cec_device)
{
	int res;

	/* destroy kernel API */
	res = cec_device_exit(cec_device, 0);
	if (res != 0)
		pr_err("dev_exit failed !!! res = 0x%08X\n", res);
	if (cec_device->dev_class) {
		/* del sysfs entries */
		device_destroy(cec_device->dev_class,
			       MKDEV(cec_device->major, cec_device->minor));

		class_destroy(cec_device->dev_class);
	}
	/* del cdev */
	cdev_del(&cec_device->cdev);

	return 0;
}

static int cec_probe(struct platform_device *pdev)
{
	int res;
	dev_t dev;
	struct cec_context *pCecCtx = &(cec_dev.CecCtx);

	pCecCtx->cec_irq = platform_get_irq(pdev, 0);
	pCecCtx->pCecRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cec_dev.cec_virt_addr = devm_ioremap_resource(&pdev->dev, pCecCtx->pCecRes);
	if (IS_ERR(cec_dev.cec_virt_addr)){
		res = PTR_ERR(cec_dev.cec_virt_addr);
		goto err_prob_device_1;
	}

	res = alloc_chrdev_region(&dev, 0, CEC_MAX_DEVS, CEC_DEVICE_NAME);
	cec_dev.major = MAJOR(dev);
	if (res < 0) {
		pr_err("alloc_chrdev_region() failed for cec\n");
		goto err_prob_device_1;
	}

	mutex_init(&pCecCtx->cec_mutex);

	res = cec_drv_init(&cec_dev);
	if (res < 0) {
		pr_err("cec_drv_init fail!\n");
		goto err_prob_device_0;
	}

	return 0;

err_prob_device_0:
	unregister_chrdev_region(MKDEV(cec_dev.major, 0), CEC_MAX_DEVS);
err_prob_device_1:
	pr_err("cec_probe failed !!! (%d)\n", res);
	return res;
}

static int cec_remove(struct platform_device *pdev)
{
	cec_drv_exit(&cec_dev);
	unregister_chrdev_region(MKDEV(cec_dev.major, 0), CEC_MAX_DEVS);
	cec_dev.major = 0;
	return 0;
}

static const struct of_device_id cec_match[] = {
	{.compatible = "syna,berlin-cec",},
	{},
};

static SIMPLE_DEV_PM_OPS(cec_pmops, cec_suspend,
			 cec_resume);

static struct platform_driver berlin_cec_driver = {
	.probe = cec_probe,
	.remove = cec_remove,
	.driver = {
		   .name = CEC_DEVICE_NAME,
		   .of_match_table = cec_match,
		   .pm = &cec_pmops,
	},
};
module_platform_driver(berlin_cec_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("cec module driver");
