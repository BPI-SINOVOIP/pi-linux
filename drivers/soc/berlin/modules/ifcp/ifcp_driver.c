// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/of.h>

#define IFCP_DEVICE_NAME	"ifcp"
#define IFCP_MAX_DEVS		1

typedef struct ifcp_device_t {
	struct cdev c_dev;
	struct class *dev_class;
	resource_size_t reg_base_phys;
	resource_size_t reg_size;
	struct device *dev;
} ifcp_device;

static int ifcp_mmap(struct file *filp, struct vm_area_struct *vma)
{
	ifcp_device *ifcp_dev = filp->private_data;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma,
					vma->vm_start,
					(ifcp_dev->reg_base_phys >> PAGE_SHIFT) + vma->vm_pgoff,
					vma->vm_end - vma->vm_start,
					vma->vm_page_prot);
}

static int ifcp_open(struct inode *inode, struct file *file)
{
	ifcp_device *ifcp_dev = container_of(inode->i_cdev, ifcp_device, c_dev);

	file->private_data = ifcp_dev;

	return 0;
}

static const struct file_operations ifcp_fops = {
	.owner = THIS_MODULE,
	.open = ifcp_open,
	.mmap = ifcp_mmap
};

static int ifcp_device_init(ifcp_device *ifcp_dev)
{
	struct device *class_dev;
	struct device *dev = ifcp_dev->dev;
	dev_t ifcp_device_num;
	int ret = 0;

	ret = alloc_chrdev_region(&ifcp_device_num, 0, IFCP_MAX_DEVS,
							  IFCP_DEVICE_NAME);
	if (ret < 0) {
		dev_err(dev, "alloc_chrdev_region failed %d", ret);
		return ret;
	}

	ifcp_dev->dev_class = class_create(THIS_MODULE, IFCP_DEVICE_NAME);
	if (IS_ERR(ifcp_dev->dev_class)) {
		ret = -ENOMEM;
		dev_err(dev, "class_create failed %d", ret);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(ifcp_dev->dev_class, NULL, ifcp_device_num,
							  NULL, IFCP_DEVICE_NAME);
	if (!class_dev) {
		dev_err(dev, "class_device_create failed %d", ret);
		ret = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&ifcp_dev->c_dev, &ifcp_fops);

	ret = cdev_add(&ifcp_dev->c_dev,
				   MKDEV(MAJOR(ifcp_device_num), 0), 1);
	if (ret < 0) {
		dev_err(dev, "cdev_add failed %d", ret);
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

static int ifcp_device_exit(ifcp_device *ifcp_dev)
{
	cdev_del(&ifcp_dev->c_dev);
	device_destroy(ifcp_dev->dev_class, ifcp_dev->c_dev.dev);
	class_destroy(ifcp_dev->dev_class);
	unregister_chrdev_region(ifcp_dev->c_dev.dev, IFCP_MAX_DEVS);
	return 0;
}
static int ifcp_probe(struct platform_device *pdev)
{
	struct resource *res;
	static ifcp_device *ifcp_dev;
	struct device *dev = &pdev->dev;
	int ret = 0;

	ifcp_dev = devm_kzalloc(&pdev->dev, sizeof(ifcp_device), GFP_KERNEL);
	if (!ifcp_dev) {
		dev_err(dev, "failed to allocate memory for ifcp_dev");
		return -ENOMEM;
	}

	ifcp_dev->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ifcp_dev->reg_base_phys = res->start;
	ifcp_dev->reg_size = resource_size(res);

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
	ifcp_device *ifcp_dev = dev_get_drvdata(&pdev->dev);

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
