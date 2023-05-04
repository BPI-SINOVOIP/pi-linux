// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */


#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/clk.h>
#include "drv_msg.h"

#define	SCRD_ISR_MSGQ_SIZE						32
#define SCRD_PIN_HIGH							1
#define SCRD_PIN_LOW							0
#define SCRD_VOLT_CFG_DIV_SIZE					3
#define SCRD_CLK_CFG_DIV_SIZE					4

#define	RA_USIM_IER								0x0008
#define	RA_USIM_IIR								0x000C
#define	RA_USIM_FSR								0x0014
#define	RA_USIM_DLR								0x0038

#define USIM_FSR_TX_LEN_MASK					0x3E0
#define USIM_FSR_RX_LEN_MASK					0x1F
#define USIM_FIFO_RX_LEN(_X_)					_X_ = (_X_ & USIM_FSR_RX_LEN_MASK)
#define USIM_FIFO_TX_LEN(_X_)					_X_ = (_X_ & USIM_FSR_TX_LEN_MASK) >> 5

// interrupt sources masks:
#define	USIM_RX_OVERRUN_MASK					0x01
#define	USIM_PARITY_ERR_MASK					0x02
#define	USIM_T0_ERR_MASK						0x04
#define	USIM_FRAMING_ERR_MASK					0x08
#define	USIM_RX_TIMEOUT_MASK					0x10
#define	USIM_CWT_MASK							0x20
#define	USIM_BWT_MASK							0x40
#define	USIM_RX_DATA_READY_MASK					0x100
#define	USIM_TX_DATA_REFILL_MASK				0x200
#define	USIM_CARD_DETECT_MASK					0x400
#define	USIM_IIR_ALL_BITS_MASK					\
					(USIM_CARD_DETECT_MASK|USIM_TX_DATA_REFILL_MASK|\
					USIM_RX_DATA_READY_MASK|USIM_BWT_MASK|USIM_CWT_MASK|\
					USIM_RX_TIMEOUT_MASK|USIM_FRAMING_ERR_MASK|\
					USIM_T0_ERR_MASK|USIM_PARITY_ERR_MASK|USIM_RX_OVERRUN_MASK)

#define	USIM_CLEARABLE_INTERRUPT_SOURCES_MASK	\
					(USIM_CARD_DETECT_MASK|USIM_BWT_MASK|USIM_CWT_MASK|\
					USIM_FRAMING_ERR_MASK|USIM_PARITY_ERR_MASK|USIM_RX_OVERRUN_MASK)

#define SCRD_DEVICE_NAME			"scrd"
#define SCRD_DEVICE_PATH			("/dev/" SCRD_DEVICE_NAME)
#define SCRD_MAX_DEVS				1
#define SCRD_MINOR					0

#define SCRD_IOCTL_CMD_MSG			_IOW('t', 1, int[2])
#define SCRD_IOCTL_GET_MSG			_IOR('t', 2, MV_CC_MSG_t)

#define SCRD_REG_WORD32_WRITE(addr, data) \
	writel_relaxed(((unsigned int)(data)), ((addr) + scrd_dev->reg_base))
#define SCRD_REG_WORD32_READ(offset, holder) \
	(*(holder) = readl_relaxed((offset) + scrd_dev->reg_base))

typedef enum _scrd_ioctol_cmd {
	SCRD_ISR_WAKEUP,
	SCRD_CTRL_CMDVCC,
	SCRD_SET_CLK,
	SCRD_SET_EXT_CLK_DIV,
	SCRD_SET_EXT_VSEL,
	SCRD_SET_EXT_CS
}scrd_ioctl_cmd_t;

typedef enum _scrd_ext_volt_cfg {
	SCRD_EXT_VOLT_CFG_1_8V,
	SCRD_EXT_VOLT_CFG_3_3V,
	SCRD_EXT_VOLT_CFG_5_0V,
}scrd_ext_volt_cfg_t;

typedef enum _scrd_ext_clk_div {
	SCRD_EXT_DIV_XTAL_BY_8,
	SCRD_EXT_DIV_XTAL_BY_4,
	SCRD_EXT_DIV_XTAL_BY_1,
	SCRD_EXT_DIV_XTAL_BY_2,
}scrd_ext_clk_div_t;

struct scrd_context {
	struct clk *core;
	struct gpio_desc *cmdvcc_gpio;
	struct gpio_desc *clk_gpio;
	struct gpio_desc *voltcfg_gpio[2];
	struct gpio_desc *clkdiv_gpio[2];
	struct gpio_desc *chipsel_gpio;
	int ext_volt_cfg_div[SCRD_VOLT_CFG_DIV_SIZE];
	int ext_clk_cfg_div[SCRD_CLK_CFG_DIV_SIZE];
	AMPMsgQ_t scrd_msgq;
	struct semaphore scrd_sem;
};

typedef struct scrd_device {
	struct scrd_context scrd_ctx;
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	struct device *dev;
	int major;
	int minor;
	void __iomem *reg_base;
	resource_size_t reg_base_phys;
	resource_size_t reg_size;
	int irq_num;
}scrd_device_t;

static atomic_t scrd_dev_refcnt = ATOMIC_INIT(0);
static DEFINE_MUTEX(scrd_dev_mutex);

static irqreturn_t scrd_devices_isr(int irq, void *dev_id)
{
	scrd_device_t *scrd_dev = (scrd_device_t *)dev_id;
	struct scrd_context *pScrdCtx = &scrd_dev->scrd_ctx;
	MV_CC_MSG_t msg = { 0 };
	u32 addr;
	u32 status_ints = 0;
	u32 enabled_ints = 0;
	u32 divisor, rxdata, txdata;
	int rc;

	addr = RA_USIM_IIR;
	SCRD_REG_WORD32_READ(addr, &status_ints);

	/* Clear all Clearable interrupts indications */
	SCRD_REG_WORD32_WRITE(addr, USIM_CLEARABLE_INTERRUPT_SOURCES_MASK);

	addr = RA_USIM_IIR;
	SCRD_REG_WORD32_READ(addr, &enabled_ints);

	addr = RA_USIM_IER;
	SCRD_REG_WORD32_WRITE(addr, 0);

	if ((status_ints & USIM_IIR_ALL_BITS_MASK) == 0) {
		addr = RA_USIM_FSR;
		SCRD_REG_WORD32_READ(addr, &rxdata);
		USIM_FIFO_RX_LEN(rxdata);
		addr = RA_USIM_FSR;
		SCRD_REG_WORD32_READ(addr, &txdata);
		USIM_FIFO_TX_LEN(txdata);
		if ((enabled_ints & USIM_TX_DATA_REFILL_MASK) && txdata)
			status_ints |= USIM_TX_DATA_REFILL_MASK;
		if ((enabled_ints & USIM_RX_DATA_READY_MASK) && rxdata)
			status_ints |= USIM_RX_DATA_READY_MASK;
		if ((enabled_ints & USIM_IIR_ALL_BITS_MASK) == 0) {
			addr = RA_USIM_IER;
			SCRD_REG_WORD32_WRITE(addr, enabled_ints);
			addr = RA_USIM_DLR;
			SCRD_REG_WORD32_READ(addr, &divisor);
			SCRD_REG_WORD32_WRITE(addr, divisor);
			return IRQ_NONE;
		}
	}

	if (likely(status_ints != 0)) {
		msg.m_Param1 = status_ints;
		rc = AMPMsgQ_Add(&pScrdCtx->scrd_msgq, &msg);
		if (likely(rc == S_OK)) {
			up(&pScrdCtx->scrd_sem);
		}
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_PM_SLEEP

static int scrd_suspend(struct device *dev)
{
	//TODO: need to handle smartcard activation
	return 0;
}

static int scrd_resume(struct device *dev)
{
	//TODO: need to handle smartcard activation
	return 0;
}
#endif //CONFIG_PM_SLEEP

static void scrd_device_cmdvcc(struct scrd_context *pScrdCtx, int value)
{
	gpiod_set_value_cansleep(pScrdCtx->cmdvcc_gpio, value);
}

static void scrd_device_set_vsel(struct scrd_context *pScrdCtx, int value)
{
	switch (value) {
		case SCRD_EXT_VOLT_CFG_1_8V:
		case SCRD_EXT_VOLT_CFG_3_3V:
		case SCRD_EXT_VOLT_CFG_5_0V:
		{
			gpiod_set_value_cansleep(pScrdCtx->voltcfg_gpio[0], (pScrdCtx->ext_volt_cfg_div[value] & 0x02));
			gpiod_set_value_cansleep(pScrdCtx->voltcfg_gpio[1], (pScrdCtx->ext_volt_cfg_div[value] & 0x01));
			break;
		}
		default:
			break;
	}
}

static void scrd_device_set_clk_div(struct scrd_context *pScrdCtx, int value)
{
	switch (value) {
		case SCRD_EXT_DIV_XTAL_BY_8:
		case SCRD_EXT_DIV_XTAL_BY_4:
		case SCRD_EXT_DIV_XTAL_BY_1:
		case SCRD_EXT_DIV_XTAL_BY_2:
		{
			gpiod_set_value_cansleep(pScrdCtx->clkdiv_gpio[0], (pScrdCtx->ext_clk_cfg_div[value] & 0x02));
			gpiod_set_value_cansleep(pScrdCtx->clkdiv_gpio[1], (pScrdCtx->ext_clk_cfg_div[value] & 0x01));
			break;
		}
		default:
			break;
	}
}

static void scrd_device_set_ext_cs(struct scrd_context *pScrdCtx, int value)
{
	gpiod_set_value_cansleep(pScrdCtx->chipsel_gpio, value);
}

static void scrd_device_clk_set(struct scrd_context *pScrdCtx, int clk_rate)
{
	clk_set_rate(pScrdCtx->core, clk_rate);
}

static int scrd_device_init(scrd_device_t *scrd_dev, unsigned int user)
{
	unsigned int err;

	sema_init(&(scrd_dev->scrd_ctx.scrd_sem), 0);
	err = AMPMsgQ_Init(&(scrd_dev->scrd_ctx.scrd_msgq), SCRD_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		pr_err("drv_scrd_init: scrd_msgq init failed, err:%8x\n", err);
		return -1;
	}
	return S_OK;
}

static void scrd_device_exit(scrd_device_t *scrd_dev, unsigned int user)
{
	struct scrd_context *pScrdCtx = &(scrd_dev->scrd_ctx);

	/** EXPANDER.V1P8.SM-TW3.GPIO0_3/7, Required LOW in Standby */
	scrd_device_set_vsel(pScrdCtx, 0x00);
	scrd_device_cmdvcc(pScrdCtx, SCRD_PIN_LOW);
	scrd_device_set_ext_cs(pScrdCtx, SCRD_PIN_LOW);
}

static int scrd_driver_open(struct inode *inode, struct file *filp)
{
	scrd_device_t *scrd_dev;
	int err = 0;

	scrd_dev = container_of(inode->i_cdev, scrd_device_t, cdev);
	if (!atomic_inc_not_zero(&scrd_dev_refcnt)) {
		mutex_lock(&scrd_dev_mutex);
		if ((atomic_read(&scrd_dev_refcnt) == 0) &&
			(err = request_irq(scrd_dev->irq_num, scrd_devices_isr, 0,
				"scrd_module", (void *)(scrd_dev)))) {
				pr_err("irq_num:%5d, err:%8x\n", scrd_dev->irq_num, err);
		}
		else
			atomic_inc(&scrd_dev_refcnt);
		mutex_unlock(&scrd_dev_mutex);
	}
	filp->private_data = scrd_dev;
	return err;
}

static int scrd_driver_release(struct inode *inode, struct file *filp)
{
	scrd_device_t *scrd_dev = (scrd_device_t *)filp->private_data;

	if (!atomic_add_unless(&scrd_dev_refcnt, -1, 1)) {
		mutex_lock(&scrd_dev_mutex);
		if (atomic_dec_return(&scrd_dev_refcnt) == 0)
			free_irq(scrd_dev->irq_num, (void *)&(scrd_dev->scrd_ctx));
		mutex_unlock(&scrd_dev_mutex);
	}
	return 0;
}

static int scrd_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	scrd_device_t *scrd_dev = (scrd_device_t *)file->private_data;

	size_t size = vma->vm_end - vma->vm_start;
	unsigned long addr =
		scrd_dev->reg_base_phys + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= scrd_dev->reg_base_phys &&
		(addr + size) <= scrd_dev->reg_base_phys + scrd_dev->reg_size)) {
		pr_err("Invalid address, start=0x%lx, end=0x%lx\n", addr,
			addr + size);
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma,
				vma->vm_start,
				(scrd_dev->reg_base_phys >> PAGE_SHIFT) +
				vma->vm_pgoff, size, vma->vm_page_prot);
	if (ret)
		pr_err("scrd_driver_mmap failed, ret = %d\n", ret);
	return ret;
}

static long scrd_driver_ioctl_unlocked(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = S_OK;
	scrd_device_t *scrd_dev = (scrd_device_t *)filp->private_data;

	switch (cmd) {
	case SCRD_IOCTL_CMD_MSG:
	{
		int io_info[2];
		struct scrd_context *pScrdCtx = &(scrd_dev->scrd_ctx);
		if (unlikely(copy_from_user
				(io_info, (void __user *) arg,
				2 * sizeof(int)) > 0)) {
			return -EFAULT;
		}
		switch (io_info[0]) {
		case SCRD_CTRL_CMDVCC:
			scrd_device_cmdvcc(pScrdCtx, io_info[1]);
			break;
		case SCRD_SET_CLK:
			scrd_device_clk_set(pScrdCtx, io_info[1]);
			break;
		case SCRD_SET_EXT_CLK_DIV:
			scrd_device_set_clk_div(pScrdCtx, io_info[1]);
			break;
		case SCRD_SET_EXT_VSEL:
			scrd_device_set_vsel(pScrdCtx, io_info[1]);
			break;
		case SCRD_SET_EXT_CS:
			scrd_device_set_ext_cs(pScrdCtx, io_info[1]);
			break;
		case SCRD_ISR_WAKEUP:
			up(&pScrdCtx->scrd_sem);
			break;
		default:
			break;
		}
		break;
	}

	case SCRD_IOCTL_GET_MSG:
	{
		MV_CC_MSG_t msg = { 0 };
		struct scrd_context *pScrdCtx = &(scrd_dev->scrd_ctx);
		rc = down_interruptible(&pScrdCtx->scrd_sem);
		if (unlikely(rc < 0)) {
			return rc;
		}
		rc = AMPMsgQ_ReadTry(&pScrdCtx->scrd_msgq, &msg);
		if (unlikely(rc != S_OK)) {
			pr_info("SCRD read message queue failed\n");
			return -EFAULT;
		}
		AMPMsgQ_ReadFinish(&pScrdCtx->scrd_msgq);
		if (unlikely(copy_to_user
				((void __user *) arg, &msg,
				sizeof(MV_CC_MSG_t)) > 0)) {
			return -EFAULT;
		}
		break;
	}
	default:
		break;
	}
	return rc;
}

static const struct file_operations scrd_ops = {
	.open = scrd_driver_open,
	.release = scrd_driver_release,
	.unlocked_ioctl = scrd_driver_ioctl_unlocked,
	.compat_ioctl = scrd_driver_ioctl_unlocked,
	.mmap = scrd_driver_mmap,
	.owner = THIS_MODULE,
};

static int
scrd_driver_setup_cdev(struct cdev *dev, int major, int minor,
			const struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static int scrd_drv_init(scrd_device_t *scrd_device)
{
	int res;
	dev_t dev_n;

	res = alloc_chrdev_region(&dev_n, 0, SCRD_MAX_DEVS, SCRD_DEVICE_NAME);
	scrd_device->minor = SCRD_MINOR;
	scrd_device->major = MAJOR(dev_n);
	if (res < 0) {
		pr_err("alloc_chrdev_region() failed for scrd\n");
		return res;
	}

	/* Now setup cdevs. */
	res = scrd_driver_setup_cdev(&scrd_device->cdev, scrd_device->major,
				scrd_device->minor, &scrd_ops);
	if (res) {
		pr_err("scrd_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	/* add SCRD devices to sysfs */
	scrd_device->dev_class = class_create(THIS_MODULE, SCRD_DEVICE_NAME);
	if (IS_ERR(scrd_device->dev_class)) {
		pr_err("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(scrd_device->dev_class, NULL,
			MKDEV(scrd_device->major, scrd_device->minor), NULL,
			SCRD_DEVICE_NAME);

	/* create hw device */
	res = scrd_device_init(scrd_device, 0);
	if (res != 0) {
		pr_err("scrd_int_init failed !!! res = 0x%08X\n",
			res);
		res = -ENODEV;
		goto err_add_device;
	}

	return 0;
err_add_device:
	if (scrd_device->dev_class) {
		device_destroy(scrd_device->dev_class,
				MKDEV(scrd_device->major, scrd_device->minor));
		class_destroy(scrd_device->dev_class);
	}

	cdev_del(&scrd_device->cdev);

	return res;
}

static int scrd_drv_exit(scrd_device_t *scrd_device)
{
	unsigned int err;

	/* destroy kernel API */
	scrd_device_exit(scrd_device, 0);

	err = AMPMsgQ_Destroy(&(scrd_device->scrd_ctx.scrd_msgq));
	if (unlikely(err != S_OK)) {
		pr_err("drv_scrd_exit: SCRD MsgQ Destroy failed, err:%8x\n", err);
	}
	if (scrd_device->dev_class) {
		/* del sysfs entries */
		device_destroy(scrd_device->dev_class,
				MKDEV(scrd_device->major, scrd_device->minor));
		class_destroy(scrd_device->dev_class);
	}
	/* del cdev */
	cdev_del(&scrd_device->cdev);

	return 0;
}

static int scrd_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	struct device *dev;
	struct device_node *np;
	scrd_device_t * scrd_dev;
	dev = &pdev->dev;
	np = dev->of_node;

	scrd_dev = devm_kzalloc(&pdev->dev, sizeof(scrd_device_t), GFP_KERNEL);
	if (IS_ERR(scrd_dev)) {
		pr_err("scrd failed to allocate memory for drv_data...!\n");
		return PTR_ERR(scrd_dev);
	}

	scrd_dev->dev = &pdev->dev;
	scrd_dev->irq_num = platform_get_irq(pdev, 0);
	if (scrd_dev->irq_num  < 0) {
		pr_err("scrd failed to get interrupt num\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	scrd_dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(scrd_dev->reg_base))
		return PTR_ERR(scrd_dev->reg_base);

	scrd_dev->reg_base_phys = res->start;
	scrd_dev->reg_size = resource_size(res);

	scrd_dev->scrd_ctx.core = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR_OR_NULL(scrd_dev->scrd_ctx.core))
		return PTR_ERR(scrd_dev->scrd_ctx.core);

	/** Filling Default value to use, in absence of of_property from device tree */
	scrd_dev->scrd_ctx.ext_volt_cfg_div[0] = 0b00; /* VOLT_CFG_1_8V */
	scrd_dev->scrd_ctx.ext_volt_cfg_div[1] = 0b01; /* VOLT_CFG_3_3V */
	scrd_dev->scrd_ctx.ext_volt_cfg_div[2] = 0b11; /* VOLT_CFG_5_0V */

	scrd_dev->scrd_ctx.ext_clk_cfg_div[0] = 0b00; /* FXTAL_DIV_8 */
	scrd_dev->scrd_ctx.ext_clk_cfg_div[1] = 0b01; /* FXTAL_DIV_2 */
	scrd_dev->scrd_ctx.ext_clk_cfg_div[2] = 0b10; /* FXTAL_DIV_1 */
	scrd_dev->scrd_ctx.ext_clk_cfg_div[3] = 0b11; /* FXTAL_DIV_4 */

	of_property_read_u32_array(np, "ext-volt-cfg-div",
			scrd_dev->scrd_ctx.ext_volt_cfg_div, SCRD_VOLT_CFG_DIV_SIZE);
	of_property_read_u32_array(np, "ext-clk-cfg-div",
			scrd_dev->scrd_ctx.ext_clk_cfg_div, SCRD_CLK_CFG_DIV_SIZE);

	scrd_dev->scrd_ctx.chipsel_gpio = devm_gpiod_get_optional(&pdev->dev, "chipsel", GPIOD_OUT_HIGH);
	if (IS_ERR(scrd_dev->scrd_ctx.chipsel_gpio))
		return PTR_ERR(scrd_dev->scrd_ctx.chipsel_gpio);

	scrd_dev->scrd_ctx.cmdvcc_gpio = devm_gpiod_get_optional(&pdev->dev, "cmdvcc", GPIOD_OUT_HIGH);
	if (IS_ERR(scrd_dev->scrd_ctx.cmdvcc_gpio))
		return PTR_ERR(scrd_dev->scrd_ctx.cmdvcc_gpio);

	scrd_dev->scrd_ctx.voltcfg_gpio[0] = devm_gpiod_get_optional(&pdev->dev, "voltcfg1", GPIOD_OUT_HIGH);
	if (IS_ERR(scrd_dev->scrd_ctx.voltcfg_gpio[0]))
		return PTR_ERR(scrd_dev->scrd_ctx.voltcfg_gpio[0]);

	scrd_dev->scrd_ctx.voltcfg_gpio[1] = devm_gpiod_get_optional(&pdev->dev, "voltcfg2", GPIOD_OUT_HIGH);
	if (IS_ERR(scrd_dev->scrd_ctx.voltcfg_gpio[1]))
		return PTR_ERR(scrd_dev->scrd_ctx.voltcfg_gpio[1]);

	scrd_dev->scrd_ctx.clkdiv_gpio[0] = devm_gpiod_get_optional(&pdev->dev, "clkdiv1", GPIOD_OUT_HIGH);
	if (IS_ERR(scrd_dev->scrd_ctx.clkdiv_gpio[0]))
		return PTR_ERR(scrd_dev->scrd_ctx.clkdiv_gpio[0]);

	scrd_dev->scrd_ctx.clkdiv_gpio[1] = devm_gpiod_get_optional(&pdev->dev, "clkdiv2", GPIOD_OUT_HIGH);
	if (IS_ERR(scrd_dev->scrd_ctx.clkdiv_gpio[1]))
		return PTR_ERR(scrd_dev->scrd_ctx.clkdiv_gpio[1]);

	clk_prepare_enable(scrd_dev->scrd_ctx.core);

	ret = scrd_drv_init(scrd_dev);
	if (ret < 0) {
		pr_err("scrd_drv_init fail!\n");
		goto err_prob_device_0;
	}

	dev_set_drvdata(dev, scrd_dev);
	return 0;

err_prob_device_0:
	unregister_chrdev_region(MKDEV(scrd_dev->major, 0), SCRD_MAX_DEVS);
	clk_disable_unprepare(scrd_dev->scrd_ctx.core);
	return ret;
}

static int scrd_remove(struct platform_device *pdev)
{
	scrd_device_t *scrd_dev = dev_get_drvdata(&pdev->dev);
	scrd_drv_exit(scrd_dev);
	unregister_chrdev_region(MKDEV(scrd_dev->major, 0), SCRD_MAX_DEVS);
	clk_disable_unprepare(scrd_dev->scrd_ctx.core);
	scrd_dev->major = 0;
	return 0;
}

static void scrd_shutdown(struct platform_device *pdev)
{
	scrd_device_t *scrd_dev = dev_get_drvdata(&pdev->dev);
	scrd_device_exit(scrd_dev, 0);
}

static const struct of_device_id scrd_match[] = {
	{.compatible = "syna,berlin-scrd",},
	{},
};

static SIMPLE_DEV_PM_OPS(scrd_pmops, scrd_suspend,
			scrd_resume);

static struct platform_driver scrd_driver = {
	.probe = scrd_probe,
	.remove = scrd_remove,
	.shutdown = scrd_shutdown,
	.driver = {
		.name = SCRD_DEVICE_NAME,
		.of_match_table = scrd_match,
		.pm = &scrd_pmops,
	},
};
module_platform_driver(scrd_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("scrd module template");
