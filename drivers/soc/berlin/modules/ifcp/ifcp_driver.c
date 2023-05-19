// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <soc/berlin/berlin_sip.h>
#include <linux/arm-smccc.h>

#define IFCP_ENABLE_SMCCC_FOR_REG_ACCESS

#define IFCP_DEVICE_NAME			"ifcp"
#define IFCP_MAX_DEVS				1

#define IFCP_ENABLE_INTR			0
#define IFCP_DISABLE_INTR			1
#define IFCP_WAIT_INTR				2

#define IFCP_INTERRUPT_TEE_MB_REG	(0x0020 + 0x0004) //TEE_MB+I2H_CSR_Host

struct ifcp_interrupts {
	unsigned int intr_status;
	struct semaphore sem;
	spinlock_t lock;
	bool enabled;
};

struct ifcp_device {
	struct cdev c_dev;
	struct class *dev_class;
	void __iomem *reg_base;
	resource_size_t reg_base_phys;
	resource_size_t reg_size;
	struct device *dev;
	int irq_num;
	struct ifcp_interrupts interruptMb;
};

struct ifcp_event_params {
	unsigned int intr_status;
};

#ifdef IFCP_ENABLE_SMCCC_FOR_REG_ACCESS

static u32 ifcp_wrap_register_read(void *virt_base, unsigned int regAddr)
{
	struct arm_smccc_res res = {};

	arm_smccc_smc(SYNA_SIP_SMC64_SREGISTER_OP,
				  SYNA_SREGISTER_READ,
				  IFCP_I2H_CTRL_STATUS,
				  0, 0, 0, 0, 0,
				  &res);
	if (res.a0) {
		pr_warn("ifcp sec reg read failed id(%u)", IFCP_I2H_CTRL_STATUS);
		res.a1 = 0;
	}
	return (u32)res.a1;
}

#else

static u32 ifcp_wrap_register_read(void *virt_base, unsigned int regAddr)
{
	return readl(virt_base + regAddr);
}

#endif


static irqreturn_t ifcp_irq_handler(int irq, void *dev_id)
{
	struct ifcp_device *ifcp_dev = (struct ifcp_device *)dev_id;
	unsigned int intr_status = 0;

	intr_status = ifcp_wrap_register_read(ifcp_dev->reg_base, IFCP_INTERRUPT_TEE_MB_REG);
	if (true == ifcp_dev->interruptMb.enabled) {
		spin_lock(&ifcp_dev->interruptMb.lock);
		ifcp_dev->interruptMb.intr_status = intr_status;
		spin_unlock(&ifcp_dev->interruptMb.lock);
		up(&ifcp_dev->interruptMb.sem);
	}
	return IRQ_HANDLED;
}

static int ifcp_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ifcp_device *ifcp_dev = filp->private_data;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma,
					vma->vm_start,
					(ifcp_dev->reg_base_phys >> PAGE_SHIFT) + vma->vm_pgoff,
					vma->vm_end - vma->vm_start,
					vma->vm_page_prot);
}

static int ifcp_open(struct inode *inode, struct file *file)
{
	struct ifcp_device *ifcp_dev = container_of(inode->i_cdev, struct ifcp_device, c_dev);

	file->private_data = ifcp_dev;

	return 0;
}

static long ifcp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ifcp_device *ifcp_dev = filp->private_data;

	switch (cmd & 0xFF) {
	case  IFCP_ENABLE_INTR:
	{
		sema_init(&ifcp_dev->interruptMb.sem, 0);
		spin_lock_init(&ifcp_dev->interruptMb.lock);
		ifcp_dev->interruptMb.enabled = true;
	}
	break;
	case  IFCP_DISABLE_INTR:
	{
		ifcp_dev->interruptMb.enabled = false;
		if (down_trylock(&ifcp_dev->interruptMb.sem))
			up(&ifcp_dev->interruptMb.sem);
	}
	break;
	case  IFCP_WAIT_INTR:
	{
		struct ifcp_event_params params;
		unsigned long flags;

		if (!copy_from_user(&params, (void __user *)arg, sizeof(params))) {
			if (true == ifcp_dev->interruptMb.enabled) {
				if (down_interruptible(&ifcp_dev->interruptMb.sem))
					return -EINTR;

				spin_lock_irqsave(&ifcp_dev->interruptMb.lock, flags);
				params.intr_status = ifcp_dev->interruptMb.intr_status;
				spin_unlock_irqrestore(&ifcp_dev->interruptMb.lock, flags);
				if (copy_to_user((void __user *)arg, &params, sizeof(params))) {
					dev_err(ifcp_dev->dev, "iotcl: wait failed in data copy\n");
					return -EINVAL;
				}
			}
		} else {
			return -EINVAL;
		}
	}
		break;
	}
	return 0;
}

static const struct file_operations ifcp_fops = {
	.owner = THIS_MODULE,
	.open = ifcp_open,
	.mmap = ifcp_mmap,
	.unlocked_ioctl = ifcp_ioctl,
	.compat_ioctl = ifcp_ioctl,
};

static int ifcp_device_init(struct ifcp_device *ifcp_dev)
{
	struct device *class_dev;
	struct device *dev = ifcp_dev->dev;
	dev_t ifcp_device_num;
	int ret = 0;

	ret = alloc_chrdev_region(&ifcp_device_num, 0, IFCP_MAX_DEVS,
							  IFCP_DEVICE_NAME);
	if (ret < 0) {
		dev_err(dev, "alloc_chrdev_region failed %d\n", ret);
		return ret;
	}

	ifcp_dev->dev_class = class_create(THIS_MODULE, IFCP_DEVICE_NAME);
	if (IS_ERR(ifcp_dev->dev_class)) {
		ret = -ENOMEM;
		dev_err(dev, "class_create failed %d\n", ret);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(ifcp_dev->dev_class, NULL, ifcp_device_num,
							  NULL, IFCP_DEVICE_NAME);
	if (!class_dev) {
		ret = -ENOMEM;
		dev_err(dev, "class_device_create failed %d\n", ret);
		goto class_destroy;
	}

	cdev_init(&ifcp_dev->c_dev, &ifcp_fops);

	ret = cdev_add(&ifcp_dev->c_dev,
				   MKDEV(MAJOR(ifcp_device_num), 0), 1);
	if (ret < 0) {
		dev_err(dev, "cdev_add failed %d\n", ret);
		goto class_device_destroy;
	}

	return ret;

class_device_destroy:
	device_destroy(ifcp_dev->dev_class, ifcp_device_num);
class_destroy:
	class_destroy(ifcp_dev->dev_class);
unregister_chrdev_region:
	unregister_chrdev_region(ifcp_device_num, IFCP_MAX_DEVS);

	return ret;
}

static int ifcp_device_exit(struct ifcp_device *ifcp_dev)
{
	free_irq(ifcp_dev->irq_num, ifcp_dev);
	cdev_del(&ifcp_dev->c_dev);
	device_destroy(ifcp_dev->dev_class, ifcp_dev->c_dev.dev);
	class_destroy(ifcp_dev->dev_class);
	unregister_chrdev_region(ifcp_dev->c_dev.dev, IFCP_MAX_DEVS);
	return 0;
}

static int ifcp_probe(struct platform_device *pdev)
{
	struct resource *res;
	static struct ifcp_device *ifcp_dev;
	struct device *dev = &pdev->dev;
	int ret = 0;

	ifcp_dev = devm_kzalloc(&pdev->dev, sizeof(struct ifcp_device), GFP_KERNEL);
	if (IS_ERR(ifcp_dev)) {
		dev_err(dev, "failed to allocate memory for drv data...!\n");
		return PTR_ERR(ifcp_dev);
	}

	ifcp_dev->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ifcp_dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ifcp_dev->reg_base)) {
		dev_err(dev, "failed to get remap\n");
		return PTR_ERR(ifcp_dev->reg_base);
	}

	ifcp_dev->reg_base_phys = res->start;
	ifcp_dev->reg_size = resource_size(res);

	ifcp_dev->irq_num = platform_get_irq(pdev, 0);
	if (ifcp_dev->irq_num < 0) {
		dev_err(dev, "failed to get interrupt for ifcp\n");
		return -EINVAL;
	}

	ret = request_irq(ifcp_dev->irq_num, ifcp_irq_handler, 0, IFCP_DEVICE_NAME, ifcp_dev);
	if (ret) {
		dev_err(ifcp_dev->dev, "ifcp irq request failed %d\n", ret);
		return ret;
	}

	ret = ifcp_device_init(ifcp_dev);
	if (ret <  0) {
		dev_err(dev, "character device registration failed...!\n");
		return ret;
	}

	dev_set_drvdata(&pdev->dev, ifcp_dev);
	return ret;
}

static int ifcp_remove(struct platform_device *pdev)
{
	struct ifcp_device *ifcp_dev = dev_get_drvdata(&pdev->dev);

	ifcp_device_exit(ifcp_dev);
	return 0;
}

static const struct of_device_id ifcp_match[] = {
	{
		.compatible = "syna,berlin-ifcp",
	},
	{},
};

static struct platform_driver ifcp_driver = {
	.probe      = ifcp_probe,
	.remove     = ifcp_remove,
	.driver = {
		.name   = IFCP_DEVICE_NAME,
		.of_match_table = ifcp_match,
	},
};

module_platform_driver(ifcp_driver);

MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IFCP module driver");
