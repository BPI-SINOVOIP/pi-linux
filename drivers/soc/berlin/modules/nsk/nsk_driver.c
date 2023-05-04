// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/arm-smccc.h>
#include <soc/berlin/berlin_sip.h>

#define NSK_ENABLE_SMCCC_FOR_REG_ACCESS

#define NSK_DEVICE_NAME			"syna-nsk"
#define NSK_INTERRUPT_NAME		"nsk_core"
#define NSK_MAX_DEVS			1

/*Interrupt Registers*/
#define NSK_INTERRUPT_STATUS_REG_ADDR		0x10020
#define NSK_INTERRUPT_MASK_REG_ADDR			0x10024
#define NSK_CORE_INTERRUPT_STATUS_REG_ADDR	0x0FC08
#define NSK_CORE_ERROR_STATUS_REG_ADDR		0x0FC10
#define NSK_INTERRUPT_STATUS_NSK_BIT		0x01
/*Interrupt bit definitions*/
#define NSK2_INTERRUPT_WRITE_ENABLE_BIT		0x80000000
#define NSK2_INTERRUPT_KTE_VALID_BIT		0x00000040
#define NSK2_INTERRUPT_HANG_BIT				0x00000020
#define NSK2_INTERRUPT_RESET_BIT			0x00000010
#define NSK2_INTERRUPT_ILLEGAL_ACCESS_BIT	0x00000008
#define NSK2_INTERRUPT_ILLEGAL_COMMAND_BIT	0x00000004
#define NSK2_INTERRUPT_ASYNC_EVENT_BIT		0x00000002
#define NSK2_INTERRUPT_COMMAND_EXIT_BIT		0x00000001

#define nsk_info(...)		dev_info(__VA_ARGS__)
#define nsk_error(...)		dev_err(__VA_ARGS__)

typedef enum _nsk_interrupt_types_ {
	NSK_INTERRUPT_TYPE_NSK,
	NSK_INTERRUPT_TYPE_KTE,
	NSK_INTERRUPT_TYPE_MAX
} nsk_interrupt_types;

typedef struct _nsk_interrupts_t {
	unsigned int intr_status;
	struct semaphore sem;
	spinlock_t lock;
	bool enabled;
} nsk_interrupts;

typedef struct nsk_device_t {
	struct cdev c_dev;
	struct class *dev_class;
	void __iomem *reg_base;
	resource_size_t reg_base_phys;
	resource_size_t reg_size;
	struct device *dev;
	int irq_num;
	nsk_interrupts intrs[NSK_INTERRUPT_TYPE_MAX];
} nsk_device;

typedef struct nsk_irq_params_t {
	nsk_interrupt_types intr_type;
	unsigned int intr_status;
} NSK_EVENT_PARAMS;

typedef enum nsk_ioctls_t {
	NSK_WAIT_FOR_EVENT,
	NSK_EVENT_ENABLE,
	NSK_EVENT_DISABLE,
} NSK_IOCTLS;

static atomic_t nsk_dev_refcnt = ATOMIC_INIT(0);

#ifdef NSK_ENABLE_SMCCC_FOR_REG_ACCESS
static u32 nsk_phy_to_secure_reg(unsigned int regAddr)
{
	u32 secureReg = -1;

	if (regAddr == NSK_CORE_INTERRUPT_STATUS_REG_ADDR)
		secureReg = NSK_CORE_INTSTATUS;
	else if (regAddr == NSK_INTERRUPT_STATUS_REG_ADDR)
		secureReg = NSK_SOC_INTSTATUS;
	else if (regAddr == NSK_INTERRUPT_MASK_REG_ADDR)
		secureReg = NSK_SOC_INTCLEAR;

	return secureReg;
}

static u32 nsk_wrap_register_read(void *virt_base,  unsigned int regAddr)
{
	struct arm_smccc_res res = {};
	u32 secureReg = nsk_phy_to_secure_reg(regAddr);

	if ((u32)-1 == secureReg) {
		pr_err("invalid sec reg read req\n");
		return 0;
	}

	arm_smccc_smc(SYNA_SIP_SMC64_SREGISTER_OP,
				SYNA_SREGISTER_READ,
				secureReg,
				0, 0, 0, 0, 0,
				&res);
	if (res.a0) {
		pr_warn("sec reg read failed id(%u)\n", secureReg);
		res.a1 = 0;
	}
	return (u32)res.a1;
}

static void nsk_wrap_register_write(void *virt_base, unsigned int regAddr, unsigned int regVal)
{
	struct arm_smccc_res res = {};
	u32 secureReg = nsk_phy_to_secure_reg(regAddr);

	if ((u32)-1 == secureReg) {
		pr_err("invalid sec reg write req\n");
		return;
	}

	arm_smccc_smc(SYNA_SIP_SMC64_SREGISTER_OP,
				SYNA_SREGISTER_WRITE,
				secureReg,
				regVal,
				0, 0, 0, 0,
				&res);
	if (res.a0)
		pr_warn("sec reg write failed id(%u)\n", secureReg);
}

#else

static u32 nsk_wrap_register_read(void *virt_base, unsigned int regAddr)
{
	return readl(virt_base + regAddr);
}

static void nsk_wrap_register_write(void *virt_base, unsigned int regAddr, unsigned int regVal)
{
	writel(regVal, virt_base + regAddr);
}

#endif //NSK_ENABLE_SMCCC_FOR_REG_ACCESS

static void nsk_irq_handle_intr(int intr_type, void *dev_id)
{
	nsk_device *nsk_dev = (nsk_device *)dev_id;
	int sig_en;
	u32 soc_intr, core_intr, mask;

	/*Masking Interrupt Register, set zero to mask all interrupts */
	mask = 0;
	mask = (1U << 31);
	nsk_wrap_register_write(nsk_dev->reg_base, NSK_CORE_INTERRUPT_STATUS_REG_ADDR, mask);

	soc_intr = nsk_wrap_register_read(nsk_dev->reg_base, NSK_INTERRUPT_STATUS_REG_ADDR);
	core_intr = nsk_wrap_register_read(nsk_dev->reg_base, NSK_CORE_INTERRUPT_STATUS_REG_ADDR);

	mask = core_intr;
	mask &= NSK2_INTERRUPT_KTE_VALID_BIT | NSK2_INTERRUPT_HANG_BIT |
			NSK2_INTERRUPT_RESET_BIT | NSK2_INTERRUPT_ILLEGAL_ACCESS_BIT |
			NSK2_INTERRUPT_ILLEGAL_COMMAND_BIT | NSK2_INTERRUPT_ASYNC_EVENT_BIT |
			NSK2_INTERRUPT_COMMAND_EXIT_BIT;

	/*clear NSK source interrupt*/
	nsk_wrap_register_write(nsk_dev->reg_base, NSK_CORE_INTERRUPT_STATUS_REG_ADDR, mask);

	mask = soc_intr;
	mask &= NSK_INTERRUPT_STATUS_NSK_BIT;
	/*clear SOC interrupt*/
	nsk_wrap_register_write(nsk_dev->reg_base, NSK_INTERRUPT_STATUS_REG_ADDR, mask);

	spin_lock(&nsk_dev->intrs[intr_type].lock);
	sig_en = nsk_dev->intrs[intr_type].intr_status ? 0 : 1;
	nsk_dev->intrs[intr_type].intr_status |= core_intr;
	spin_unlock(&nsk_dev->intrs[intr_type].lock);

	if (nsk_dev->intrs[intr_type].enabled && sig_en)
		up(&(nsk_dev->intrs[intr_type].sem));
}

static irqreturn_t nsk_irq_handler(int irq, void *dev_id)
{
	nsk_device *nsk_dev = (nsk_device *)dev_id;
	unsigned intstat = 0;

	intstat = nsk_wrap_register_read(nsk_dev->reg_base, NSK_INTERRUPT_STATUS_REG_ADDR);

	if (intstat & NSK_INTERRUPT_STATUS_NSK_BIT) {
		nsk_irq_handle_intr(NSK_INTERRUPT_TYPE_NSK, dev_id);
	}

	return IRQ_HANDLED;
}

static int nsk_driver_open(struct inode *inode, struct file *filp)
{
	nsk_device *nsk_dev = container_of(inode->i_cdev, nsk_device, c_dev);
	int ret, i;

	if (atomic_inc_return(&nsk_dev_refcnt) > 1) {
		nsk_info(nsk_dev->dev, "nsk driver reference count %d!\n",
			 atomic_read(&nsk_dev_refcnt));
		return 0;
	}

	for (i = 0; i < NSK_INTERRUPT_TYPE_MAX; ++i)
		nsk_dev->intrs[0].intr_status = 0;

	nsk_wrap_register_write(nsk_dev->reg_base, NSK_INTERRUPT_MASK_REG_ADDR,
							~NSK_INTERRUPT_STATUS_NSK_BIT);

	ret = request_irq(nsk_dev->irq_num, nsk_irq_handler, 0,
				NSK_DEVICE_NAME, nsk_dev);
	if (ret) {
		nsk_error(nsk_dev->dev, "Interrupt registration failed for NSK\n");
		return ret;
	}

	filp->private_data = nsk_dev;

	return 0;
}

static int nsk_driver_release(struct inode *inode, struct file *filp)
{
	nsk_device *nsk_dev = (nsk_device *)filp->private_data;

	if (atomic_read(&nsk_dev_refcnt) == 0) {
		nsk_info(nsk_dev->dev, "nsk driver already released!\n");
		return 0;
	}

	if (atomic_dec_return(&nsk_dev_refcnt)) {
		nsk_info(nsk_dev->dev, "nsk dev ref cnt after this release: %d!\n",
			 atomic_read(&nsk_dev_refcnt));
		return 0;
	}

	free_irq(nsk_dev->irq_num, (void *)nsk_dev);

	return 0;
}

static long nsk_driver_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	void __user *pInArg      = NULL;
	void __user *pOutArg     = NULL;
	nsk_device *nsk_dev = (nsk_device *)filp->private_data;

	pInArg = (void __user *)arg;
	pOutArg = (void __user *)arg;

	switch (cmd & 0xFF) {
	case NSK_WAIT_FOR_EVENT:
	{
		unsigned long flags;
		NSK_EVENT_PARAMS wait;

		if (!copy_from_user(&wait, pInArg, sizeof(wait))) {
			if (true == nsk_dev->intrs[wait.intr_type].enabled) {
				if (down_interruptible(
					&nsk_dev->intrs[wait.intr_type].sem))
					return -EINTR;

				spin_lock_irqsave(&nsk_dev->intrs[wait.intr_type].lock, flags);
				wait.intr_status =
					nsk_dev->intrs[wait.intr_type].intr_status;
				nsk_dev->intrs[wait.intr_type].intr_status = 0;
				spin_unlock_irqrestore(&nsk_dev->intrs[wait.intr_type].lock, flags);

				if (copy_to_user(pOutArg, &wait, sizeof(wait))) {
					nsk_error(nsk_dev->dev, "IOCTL: wait release param failed...!\n");
					return -EINVAL;
				}
			}
		} else {
			nsk_error(nsk_dev->dev, "IOCTL: wait for event failed...!\n");
			return -EINVAL;
		}
	}
	break;

	case NSK_EVENT_ENABLE:
	{
		NSK_EVENT_PARAMS params;

		if (!copy_from_user(&params, pInArg, sizeof(params))) {
			sema_init(&nsk_dev->intrs[params.intr_type].sem, 0);
			spin_lock_init(&nsk_dev->intrs[params.intr_type].lock);
			nsk_dev->intrs[params.intr_type].enabled = true;
		} else {
			nsk_error(nsk_dev->dev, "IOCTL: event enable failed...!\n");
			return -EINVAL;
		}
	}
	break;

	case NSK_EVENT_DISABLE:
	{
		unsigned long flags;
		NSK_EVENT_PARAMS params;

		if (!copy_from_user(&params, pInArg, sizeof(params))) {
			if (true == nsk_dev->intrs[params.intr_type].enabled) {
				spin_lock_irqsave(&nsk_dev->intrs[params.intr_type].lock, flags);
				nsk_dev->intrs[params.intr_type].enabled = false;
				nsk_dev->intrs[params.intr_type].intr_status  = 0;
				spin_unlock_irqrestore(&nsk_dev->intrs[params.intr_type].lock, flags);
				if (down_trylock(&nsk_dev->intrs[params.intr_type].sem))
					up(&nsk_dev->intrs[params.intr_type].sem);
			}
		} else {
			nsk_error(nsk_dev->dev, "IOCTL: event disable failed...!\n");
			return -EINVAL;
		}
	}
	break;
	}

	return 0;
}

static int nsk_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	nsk_device *nsk_dev = (nsk_device *)file->private_data;
	struct device *dev = nsk_dev->dev;
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long addr =
		nsk_dev->reg_base_phys + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= nsk_dev->reg_base_phys &&
		(addr + size) <=
		nsk_dev->reg_base_phys +
		nsk_dev->reg_size)) {
		nsk_error(dev,
			"mmap: Invalid address, start=0x%lx, end=0x%lx\n",
			addr, addr + size);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma, vma->vm_start,
		((nsk_dev->reg_base_phys) >> PAGE_SHIFT) + vma->vm_pgoff,
		size, vma->vm_page_prot);
}

static const struct file_operations nsk_ops = {
	.open		= nsk_driver_open,
	.release	= nsk_driver_release,
	.unlocked_ioctl	= nsk_driver_ioctl,
	.compat_ioctl	= nsk_driver_ioctl,
	.mmap		= nsk_driver_mmap,
	.owner		= THIS_MODULE,
};

static int nsk_device_init(nsk_device *nsk_dev)
{
	int ret = 0;
	struct device *class_dev, *dev;
	dev_t dev_n;

	dev = nsk_dev->dev;
	ret = alloc_chrdev_region(&dev_n, 0,
				NSK_MAX_DEVS, NSK_DEVICE_NAME);
	if (ret) {
		nsk_error(dev, "alloc_chrdev_region failed...!\n");
		return ret;
	}

	nsk_dev->dev_class = class_create(THIS_MODULE, NSK_DEVICE_NAME);
	if (IS_ERR(nsk_dev->dev_class)) {
		nsk_error(dev, "class_create failed...!\n");
		ret = -ENOMEM;
		goto err_class_create;
	}

	class_dev = device_create(nsk_dev->dev_class, NULL,
			dev_n, NULL, NSK_DEVICE_NAME);
	if (IS_ERR(class_dev)) {
		nsk_error(dev, "device_create failed...!\n");
		ret =  -ENOMEM;
		goto err_dev_create;
	}

	cdev_init(&nsk_dev->c_dev, &nsk_ops);
	ret = cdev_add(&nsk_dev->c_dev, dev_n, 1);
	if (ret) {
		nsk_error(dev, "cdev_add failed...!\n");
		goto err_cdev_add;
	}

	return ret;
err_cdev_add:
	device_destroy(nsk_dev->dev_class, dev_n);
err_dev_create:
	class_destroy(nsk_dev->dev_class);
err_class_create:
	unregister_chrdev_region(dev_n, NSK_MAX_DEVS);

	return ret;
}

static void nsk_device_exit(nsk_device *nsk_dev)
{
	cdev_del(&nsk_dev->c_dev);
	device_destroy(nsk_dev->dev_class, nsk_dev->c_dev.dev);
	class_destroy(nsk_dev->dev_class);
	unregister_chrdev_region(nsk_dev->c_dev.dev, NSK_MAX_DEVS);
}

static int nsk_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	struct device *dev;
	static nsk_device *nsk_dev;

	dev = &pdev->dev;
	nsk_dev = devm_kzalloc(&pdev->dev, sizeof(*nsk_dev), GFP_KERNEL);
	if (IS_ERR(nsk_dev)) {
		nsk_error(dev, "failed to allocate memory for drv_data...!\n");
		return PTR_ERR(nsk_dev);
	}

	nsk_dev->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nsk_dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(nsk_dev->reg_base)) {
		nsk_error(dev, "ioremap failed...!\n");
		return PTR_ERR(nsk_dev->reg_base);
	}

	nsk_dev->reg_base_phys = res->start;
	nsk_dev->reg_size = resource_size(res);

	nsk_dev->irq_num = platform_get_irq(pdev, 0);
	if (nsk_dev->irq_num  < 0) {
		nsk_error(dev, "Failed to get interrupt for NSK\n");
		return -EINVAL;
	}

	ret = nsk_device_init(nsk_dev);
	if (ret <  0) {
		nsk_error(dev, "character device registration failed...!\n");
		return ret;
	}

	dev_set_drvdata(dev, nsk_dev);

	return 0;
}

static int nsk_remove(struct platform_device *pdev)
{
	nsk_device *nsk_dev = dev_get_drvdata(&pdev->dev);

	nsk_device_exit(nsk_dev);

	return 0;
}

static const struct of_device_id nsk_match[] = {
	{
		.compatible = "syna,vs640-nsk",
	},
	{},
};

static struct platform_driver nsk_driver = {
	.probe = nsk_probe,
	.remove = nsk_remove,
	.driver = {
		.name = NSK_DEVICE_NAME,
		.of_match_table = nsk_match,
	},
};

module_platform_driver(nsk_driver);

MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NSK module driver");

