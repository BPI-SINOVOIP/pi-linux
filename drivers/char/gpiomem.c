/*
 * linux/drivers/char/gpiomem.c
 *
 * GPIO memory device driver
 *
 * Creates a chardev /dev/gpiomem which will provide user access to
 * the GPIO registers when it is mmap()'d.
 * No longer need root for user GPIO access, but without relaxing permissions
 * on /dev/mem.
 *
 * This driver is based on bcm2835-gpiomem.c in Raspberrypi's linux kernel 4.4:
 *	Written by Luke Wren <luke@raspberrypi.org>
 *	Copyright (c) 2015, Raspberry Pi (Trading) Ltd.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/pagemap.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/io.h>

#define DEVICE_NAME "sunxi-gpiomem"
#define DRIVER_NAME "sunxi-gpiomem"
#define DEVICE_MINOR 0

struct regs_phys {
	unsigned long start;
	unsigned long end;
};

struct sunxi_gpiomem_instance {
	struct regs_phys gpio_regs_phys[32];
	int gpio_area_count;
	struct device *dev;
};

static struct cdev sunxi_gpiomem_cdev;
static dev_t sunxi_gpiomem_devid;
static struct class *sunxi_gpiomem_class;
static struct device *sunxi_gpiomem_dev;
static struct sunxi_gpiomem_instance *inst;

static int sunxi_gpiomem_open(struct inode *inode, struct file *file)
{
	int dev = iminor(inode);
	int ret = 0;

	dev_info(inst->dev, "gpiomem device opened.");

	if (dev != DEVICE_MINOR) {
		dev_err(inst->dev, "Unknown minor device: %d", dev);
		ret = -ENXIO;
	}
	return ret;
}

static int sunxi_gpiomem_release(struct inode *inode, struct file *file)
{
	int dev = iminor(inode);
	int ret = 0;

	if (dev != DEVICE_MINOR) {
		dev_err(inst->dev, "Unknown minor device %d", dev);
		ret = -ENXIO;
	}
	return ret;
}

static const struct vm_operations_struct sunxi_gpiomem_vm_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int sunxi_gpiomem_mmap(struct file *file, struct vm_area_struct *vma)
{
	int gpio_area = 0;
	unsigned long start = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long end   = start + vma->vm_end - vma->vm_start;

	while (gpio_area < inst->gpio_area_count) {
		if ((inst->gpio_regs_phys[gpio_area].start >= start) &&
		    (inst->gpio_regs_phys[gpio_area].end   <= end))
			goto found;
		gpio_area++;
	}

	return -EACCES;

found:
	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);

	vma->vm_ops = &sunxi_gpiomem_vm_ops;

	if (remap_pfn_range(vma, vma->vm_start,
				vma->vm_pgoff,
				vma->vm_end - vma->vm_start,
				vma->vm_page_prot)) {
		return -EAGAIN;
	}

	return 0;
}

static const struct file_operations
sunxi_gpiomem_fops = {
	.owner = THIS_MODULE,
	.open = sunxi_gpiomem_open,
	.release = sunxi_gpiomem_release,
	.mmap = sunxi_gpiomem_mmap,
};

static int sunxi_gpiomem_probe(struct platform_device *pdev)
{
	int err = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *res = NULL;
	int i = 0;

	/* Allocate buffers and instance data */
	inst = kzalloc(sizeof(struct sunxi_gpiomem_instance), GFP_KERNEL);

	if (!inst) {
		err = -ENOMEM;
		goto failed_inst_alloc;
	}

	inst->dev = dev;
	inst->gpio_area_count = of_property_count_elems_of_size(np, "reg",
				sizeof(u32)) / 4;

	if (inst->gpio_area_count > 32 || inst->gpio_area_count <= 0) {
		dev_err(inst->dev, "failed to get gpio register area.");
		err = -EINVAL;
		goto failed_inst_alloc;
	}

	dev_info(inst->dev, "Initialised: GPIO register area is %d",
			inst->gpio_area_count);

	for (i = 0; i < inst->gpio_area_count; ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res) {
			inst->gpio_regs_phys[i].start = res->start;
			inst->gpio_regs_phys[i].end   = res->end;
		} else {
			dev_err(inst->dev, "failed to get IO resource area %d", i);
			err = -ENOENT;
			goto failed_get_resource;
		}
	}

	/* Create character device entries */
	err = alloc_chrdev_region(&sunxi_gpiomem_devid,
				  DEVICE_MINOR, 1, DEVICE_NAME);
	if (err != 0) {
		dev_err(inst->dev, "unable to allocate device number");
		goto failed_alloc_chrdev;
	}
	cdev_init(&sunxi_gpiomem_cdev, &sunxi_gpiomem_fops);
	sunxi_gpiomem_cdev.owner = THIS_MODULE;
	err = cdev_add(&sunxi_gpiomem_cdev, sunxi_gpiomem_devid, 1);
	if (err != 0) {
		dev_err(inst->dev, "unable to register device");
		goto failed_cdev_add;
	}

	/* Create sysfs entries */
	sunxi_gpiomem_class = class_create(THIS_MODULE, DEVICE_NAME);
	err = IS_ERR(sunxi_gpiomem_class);
	if (err)
		goto failed_class_create;

	sunxi_gpiomem_dev = device_create(sunxi_gpiomem_class, NULL,
					sunxi_gpiomem_devid, NULL,
					"gpiomem");
	err = IS_ERR(sunxi_gpiomem_dev);
	if (err)
		goto failed_device_create;

	for (i = 0; i < inst->gpio_area_count; ++i) {
		dev_info(inst->dev,
			"Initialised: Registers at start:0x%08lx end:0x%08lx size:0x%08lx",
			inst->gpio_regs_phys[i].start,
			inst->gpio_regs_phys[i].end,
			inst->gpio_regs_phys[i].end - inst->gpio_regs_phys[i].start);
	}

	return 0;

failed_device_create:
	class_destroy(sunxi_gpiomem_class);
failed_class_create:
	cdev_del(&sunxi_gpiomem_cdev);
failed_cdev_add:
	unregister_chrdev_region(sunxi_gpiomem_devid, 1);
failed_alloc_chrdev:
failed_get_resource:
	kfree(inst);
failed_inst_alloc:
	dev_err(inst->dev, "could not load sunxi_gpiomem");
	return err;
}

static int sunxi_gpiomem_remove(struct platform_device *pdev)
{
	struct device *dev = inst->dev;

	kfree(inst);
	device_destroy(sunxi_gpiomem_class, sunxi_gpiomem_devid);
	class_destroy(sunxi_gpiomem_class);
	cdev_del(&sunxi_gpiomem_cdev);
	unregister_chrdev_region(sunxi_gpiomem_devid, 1);

	dev_info(dev, "GPIO mem driver removed - OK");
	return 0;
}

static const struct of_device_id sunxi_gpiomem_of_match[] = {
	{.compatible = "allwinner, gpiomem",},
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_gpiomem_of_match);

static struct platform_driver sunxi_gpiomem_driver = {
	.driver			= {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_gpiomem_of_match,
	},
	.probe			= sunxi_gpiomem_probe,
	.remove		= sunxi_gpiomem_remove,
};

module_platform_driver(sunxi_gpiomem_driver);

MODULE_ALIAS("platform:gpiomem");
MODULE_DESCRIPTION("Allwinner gpiomem driver for accessing GPIO from userspace");
MODULE_LICENSE("GPL");
