#include <linux/module.h>
#include <linux/irq.h>
#include <linux/reset.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/i2c.h>

#include "isp_driver.h"
#include "reg_if.h"
#include "drv_isp.h"
#include "drv_debug.h"
#include "drv_ispdhub.h"

#define ISP_MAX_DEVS    ISP_MODULE_MAX

typedef struct isp_clk_params_t {
	int clk_id;
	unsigned long clk_rate;
} ISP_CLK_PARAMS;

static const isp_module_fops module_list_fops[] = {
	//ISP_CORE
	{
	.probe	= isp_core_mod_probe,
	.close	= isp_core_mod_close,
	.ioctl	= isp_core_mod_ioctl,
	},
	//ISP_DHUB
	{
	.probe		= isp_dhub_mod_probe,
	.exit		= isp_dhub_mod_exit,
	.open		= isp_dhub_mod_open,
	.close		= isp_dhub_mod_close,
	.ioctl		= isp_dhub_mod_ioctl,
	.suspend	= isp_dhub_mod_suspend,
	.resume		= isp_dhub_mod_resume,
	},
};

static const char *isp_clock_list[] = {
	"txescclk",
	"ispclk",
	"isssysclk",
	"ispbeclk",
	"ispdscclk",
	"ispcsi0clk",
	"ispcsi1clk"
};

static const struct of_device_id isp_match[] = {
	{.compatible = "syna,vs680-isp"},
	{},
};

#define isp_module_fops_iter(fops_func)					\
static int isp_invoke_mod_##fops_func(isp_device *isp_dev,		\
				isp_module_ctx *mod_ctx)		\
{									\
	int i, retVal = 0;						\
	char *mod_name;							\
									\
	for (i = 0; i < ISP_MODULE_MAX; i++) {				\
		isp_mod_gen_fops fops = module_list_fops[i].fops_func;	\
									\
		if (!fops) {						\
			continue;					\
		}							\
									\
		retVal = fops(isp_dev, &mod_ctx[i]);			\
									\
		mod_name = mod_ctx[i].mod_name;				\
		isp_error(isp_dev->dev, "%s:%s %s!\n",			\
			mod_name ? mod_name : "Unknown", #fops_func,	\
			isp_module_err_str(retVal));			\
		if (retVal) {						\
			break;						\
		}							\
	}								\
									\
	return retVal;							\
}									\

isp_module_fops_iter(probe)
isp_module_fops_iter(open)
isp_module_fops_iter(close)
isp_module_fops_iter(exit)
#ifdef CONFIG_PM
isp_module_fops_iter(suspend)
isp_module_fops_iter(resume)
#endif

static int isp_driver_open(struct inode *inode, struct file *filp)
{
	isp_device *isp_dev;
	isp_drv_data *drv_data;

	isp_dev = container_of(inode->i_cdev, isp_device, c_dev);

	if (!isp_dev) {
		isp_error(isp_dev->dev,
			"Failed to get driver data\n");
		return -ENOMEM;
	}

	drv_data = kmalloc(sizeof(isp_drv_data), GFP_KERNEL);

	if (!drv_data) {
		isp_error(isp_dev->dev,
			"Failed to allocate memory\n");
		return -ENOMEM;
	}

	drv_data->isp_dev = isp_dev;
	drv_data->mod_ctx = NULL;
	drv_data->mod_type = ISP_MODULE_MAX;
	filp->private_data = drv_data;

	mutex_lock(&isp_dev->isp_mutex);

	//Open all ISP modules
	isp_invoke_mod_open(isp_dev, isp_dev->mod_ctx);

	mutex_unlock(&isp_dev->isp_mutex);

	return 0;
}

static int isp_driver_release(struct inode *inode, struct file *filp)
{
	isp_drv_data *drv_data = (isp_drv_data *)filp->private_data;
	isp_device *isp_dev = drv_data->isp_dev;

	mutex_lock(&isp_dev->isp_mutex);

	//Close all ISP modules
	isp_invoke_mod_close(isp_dev, isp_dev->mod_ctx);

	mutex_unlock(&isp_dev->isp_mutex);

	drv_data->isp_dev = NULL;
	kfree(drv_data);

	return 0;
}

static int isp_i2c_write(isp_device *isp, u8 dev_addr, void *buf, size_t len)
{
	struct i2c_msg msgs[1];
	int ret;

	msgs[0].addr = isp->dev_addr;
	msgs[0].flags = 0;
	msgs[0].len = 1 + len;
	msgs[0].buf = kmalloc(1 + len, GFP_KERNEL);
	if (!msgs[0].buf)
		return -ENOMEM;

	msgs[0].buf[0] = dev_addr;
	memcpy(&msgs[0].buf[1], buf, len);

	ret = i2c_transfer(isp->i2c, msgs, ARRAY_SIZE(msgs));

	kfree(msgs[0].buf);

	if (ret < 0)
		return ret;

	return ret == ARRAY_SIZE(msgs) ? len : 0;
}

static long isp_driver_ioctl(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	void __user *pInArg      = NULL;
	void __user *pOutArg     = NULL;
	isp_drv_data *drv_data = (isp_drv_data *)filp->private_data;
	struct device *dev = drv_data->isp_dev->dev;
	struct clk **isp_clks = drv_data->isp_dev->isp_clks;
	isp_device *isp_dev = drv_data->isp_dev;

	pInArg = (void __user *)arg;
	pOutArg = (void __user *)arg;

	//Remove module offset
	cmd = cmd & 0xFFFF;

	switch (cmd & 0xFF) {
	case ISP_SET_CLK:
	{
		int ret;
		ISP_CLK_PARAMS clk_par;

		if (!copy_from_user(&clk_par, pInArg, sizeof(ISP_CLK_PARAMS))) {
			if ((clk_par.clk_id > -1) && (clk_par.clk_id < ARRAY_SIZE(isp_clock_list))) {
				if (clk_par.clk_rate == (clk_round_rate(isp_clks[clk_par.clk_id],
									clk_par.clk_rate))) {
					ret = clk_set_rate(isp_clks[clk_par.clk_id],
									clk_par.clk_rate);
					if (ret != 0) {
						isp_error(dev,
							"Failed to set clock %s\n",
							isp_clock_list[clk_par.clk_id]);
							return ret;
					}
				} else {
					isp_error(dev,
						"Requested clock not supported for %s\n",
							isp_clock_list[clk_par.clk_id]);
					return -EINVAL;
				}
			} else {
				isp_error(dev,
					"Invalid Clock ID. Check params\n");
				return -EINVAL;
			}
		} else {
			isp_error(dev,
				"IOCTL: Clock configuration failed\n");
			return -EINVAL;
		}
		break;
	}

	case ISP_SELECT_SENSOR:
	{
		char sensor;
		#ifdef CONFIG_ISP_CAMERA_C05
		char channel;
		#endif

		if (!copy_from_user(&sensor, pInArg, sizeof(sensor))) {
			#ifdef CONFIG_ISP_CAMERA_C05
			/* Using two identical sensor profiles requires an i2c switching IC */
			if(sensor == 1)
				channel = 8;//csi0
			else
				channel = 4;//csi1
			if(isp_dev->i2c)
				/* Configure the i2c switch IC */
				isp_i2c_write(isp_dev, 1, &channel, sizeof(channel));
			#else
			if(isp_dev->i2c)
				/* Configure the sensor IC */
				isp_i2c_write(isp_dev, 1, &sensor, sizeof(sensor));
			#endif
		}
	}
	break;

	case ISP_SET_POWER:
	{
		int enable;

		if (!copy_from_user(&enable, pInArg, sizeof(enable))) {
			if (!IS_ERR_OR_NULL(isp_dev->enable_gpios)) {

				if (enable)
					bitmap_fill(isp_dev->gpio_values.values,
							isp_dev->gpio_values.nvalues);
				else
					bitmap_zero(isp_dev->gpio_values.values,
							isp_dev->gpio_values.nvalues);

				gpiod_set_array_value_cansleep(isp_dev->gpio_values.nvalues,
								isp_dev->enable_gpios->desc,
								isp_dev->enable_gpios->info,
								isp_dev->gpio_values.values);

			} else {
				isp_error(dev,
					"IOCTL: Failed to find GPIO for setting power\n");
				return -EINVAL;
			}
		} else {
			isp_error(dev,
				"IOCTL: Failed to set power\n");
			return -EINVAL;
		}
		break;
	}

	case ISP_MODULE_CONFIG:
	{
		uint8_t sub_module;

		if (!copy_from_user(&sub_module, pInArg, sizeof(sub_module))) {
			if (isp_module_is_invalid(sub_module)) {
				isp_error(dev, "Invalid Sub Module - %d\n", sub_module);
				return -EINVAL;
			}

			drv_data->mod_type = sub_module;
			drv_data->mod_ctx = &isp_dev->mod_ctx[sub_module];
			isp_info(dev,
				"IOCTL: module configuration done for - %s...!\n",
				drv_data->mod_ctx->mod_name);
		} else {
			isp_error(dev,
				"IOCTL: module configuration failed...!\n");
			return -EINVAL;
		}
		//Follow through to pass CONFIG cmd to respective module
	}

	default:
	{
		isp_module_ctx *p_mod_ctx;
		isp_mod_ioctl ioctl_func;
		uint8_t sub_module = drv_data->mod_type;

		if (isp_module_is_invalid(sub_module))
			return -EINVAL;

		p_mod_ctx = drv_data->mod_ctx;
		ioctl_func = module_list_fops[sub_module].ioctl;
		if (ioctl_func)
			return ioctl_func(isp_dev, p_mod_ctx, cmd, arg);

		//ioctl not supported by module
		return -EINVAL;
	}

	}
	return 0;
}

static int isp_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	isp_drv_data *drv_data = (isp_drv_data *)file->private_data;
	isp_device *isp_dev = drv_data->isp_dev;
	struct device *dev = isp_dev->dev;
	size_t size = vma->vm_end - vma->vm_start;

	unsigned long addr =
		isp_dev->reg_base_phys + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= isp_dev->reg_base_phys &&
		(addr + size) <=
		isp_dev->reg_base_phys +
		isp_dev->reg_size)) {
		isp_error(dev,
			"mmap: Invalid address, start=0x%lx, end=0x%lx\n",
			addr, addr + size);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma, vma->vm_start,
		((isp_dev->reg_base_phys) >> PAGE_SHIFT) + vma->vm_pgoff,
		size, vma->vm_page_prot);
}

static const struct file_operations isp_ops = {
	.open		= isp_driver_open,
	.release	= isp_driver_release,
	.unlocked_ioctl	= isp_driver_ioctl,
	.compat_ioctl	= isp_driver_ioctl,
	.mmap		= isp_driver_mmap,
	.owner		= THIS_MODULE,
};

static int isp_device_init(isp_device *isp_dev)
{
	int ret = 0;
	struct device *class_dev, *dev;
	dev_t dev_n;

	dev = isp_dev->dev;
	ret = alloc_chrdev_region(&dev_n, 0,
				ISP_MAX_DEVS, ISP_DEVICE_NAME);
	if (ret) {
		isp_error(dev, "alloc_chrdev_region failed...!\n");
		return ret;
	}

	isp_dev->dev_class = class_create(THIS_MODULE, ISP_DEVICE_NAME);
	if (IS_ERR(isp_dev->dev_class)) {
		isp_error(dev, "class_create failed...!\n");
		ret = -ENOMEM;
		goto err_class_create;
	}

	class_dev = device_create(isp_dev->dev_class, NULL,
			dev_n, NULL, ISP_DEVICE_NAME);
	if (IS_ERR(class_dev)) {
		isp_error(dev, "device_create failed...!\n");
		ret =  -ENOMEM;
		goto err_dev_create;
	}

	cdev_init(&isp_dev->c_dev, &isp_ops);
	ret = cdev_add(&isp_dev->c_dev, dev_n, 1);
	if (ret) {
		isp_error(dev, "cdev_add failed...!\n");
		goto err_cdev_add;
	}

	mutex_init(&isp_dev->isp_mutex);

	isp_info(dev, "Initialization complete\n");

	return ret;

err_cdev_add:
	device_destroy(isp_dev->dev_class, dev_n);
err_dev_create:
	class_destroy(isp_dev->dev_class);
err_class_create:
	unregister_chrdev_region(dev_n, ISP_MAX_DEVS);

	return ret;
}

static int isp_enable_sensor(isp_device *isp_dev)
{
	struct device *dev;

	dev = isp_dev->dev;
	isp_dev->enable_gpios = devm_gpiod_get_array_optional(dev, "enable", GPIOD_OUT_LOW);

	if (IS_ERR(isp_dev->enable_gpios))
		return PTR_ERR(isp_dev->enable_gpios);
	return 0;
}

static void isp_device_exit(isp_device *isp_dev)
{
	int i;

	if (!IS_ERR_OR_NULL(isp_dev->enable_gpios))
		for (i = 0; i < isp_dev->enable_gpios->ndescs; i++)
			gpiod_set_value_cansleep(isp_dev->enable_gpios->desc[i], 0);

	//Exit all ISP modules
	isp_invoke_mod_exit(isp_dev, isp_dev->mod_ctx);

	cdev_del(&isp_dev->c_dev);
	device_destroy(isp_dev->dev_class, isp_dev->c_dev.dev);
	class_destroy(isp_dev->dev_class);
	unregister_chrdev_region(isp_dev->c_dev.dev, ISP_MAX_DEVS);
}

static int isp_fetch_clocks(struct device *dev, struct clk **isp_clks)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(isp_clock_list); i++) {
		isp_clks[i] = devm_clk_get(dev, isp_clock_list[i]);
		if (IS_ERR(isp_clks[i])) {
			isp_error(dev, "failed to get %s ...!\n", isp_clock_list[i]);
			return PTR_ERR(isp_clks[i]);
		}
	}

	return 0;
}

static int isp_enable_clocks(struct device *dev, struct clk **isp_clks)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(isp_clock_list); i++) {
		ret = clk_prepare_enable(isp_clks[i]);
		if (ret < 0) {
			isp_error(dev, "%s prepare failed..!\n", isp_clock_list[i]);
			goto prepare_failure;
		}
	}
	return 0;
prepare_failure:
	while (--i >= 0)
		clk_disable_unprepare(isp_clks[i]);

	return ret;
}

static int isp_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	struct reset_control *rst;
	struct device *dev;
	static isp_device *isp_dev;
	static struct clk **isp_clks;
	struct i2c_adapter *i2c;
	u32 dev_addr;
	char sensor;

	dev = &pdev->dev;
	isp_dev = devm_kzalloc(&pdev->dev, sizeof(*isp_dev), GFP_KERNEL);
	if (IS_ERR(isp_dev)) {
		isp_error(dev, "failed to allocate memory for drv_data...!\n");
		return PTR_ERR(isp_dev);
	}

	isp_dev->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	isp_dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(isp_dev->reg_base)) {
		isp_error(dev, "ioremap failed...!\n");
		return PTR_ERR(isp_dev->reg_base);
	}

	isp_dev->reg_base_phys = res->start;
	isp_dev->reg_size = resource_size(res);

	rst = devm_reset_control_get_optional(&pdev->dev, "isprst");
	if (IS_ERR(rst) && PTR_ERR(rst) == -EPROBE_DEFER) {
		isp_error(dev, "isprst reset failed...!\n");
		return -EPROBE_DEFER;
	}

	reset_control_deassert(rst);

	ret = isp_device_init(isp_dev);
	if (ret <  0) {
		isp_error(dev, "character device registration failed...!\n");
		return ret;
	}

	isp_clks = devm_kzalloc(dev,
		sizeof(struct clk *) * ARRAY_SIZE(isp_clock_list), GFP_KERNEL);
	if (IS_ERR(isp_clks)) {
		isp_error(dev, "failed to allocate memory for isp_clocks...!\n");
		return PTR_ERR(isp_clks);
	}

	ret = isp_enable_sensor(isp_dev);
	if (ret) {
		isp_error(dev, "failed to enable isp sensor device..!\n");
		return ret;
	}

	if (pdev->dev.of_node) {
		struct device_node *node = pdev->dev.of_node;
		struct device_node *np;

		np = of_parse_phandle(node, "i2c-bus", 0);
		if (np) {
			i2c = of_find_i2c_adapter_by_node(np);

			if (!i2c) {
				dev_err(dev, "failed to find i2c adapter\n");
				of_node_put(np);
				return -ENODEV;
			}

			isp_dev->i2c = i2c;

			if (!of_property_read_u32(node, "dev-addr", &dev_addr)) {
				isp_dev->dev_addr = (u8)dev_addr;
			} else {
				dev_err(dev, "failed to read 'dev-addr' property\n");
				of_node_put(np);
				return -ENODEV;
			}

			of_node_put(np);
			#ifdef CONFIG_ISP_CAMERA_C05
			sensor = 8; /* select sensor connected in CSI0 by default */
			isp_i2c_write(isp_dev, 1, &sensor, sizeof(sensor));
			#else
			sensor = 1; /* select sensor connected in CSI0 by default */
			isp_i2c_write(isp_dev, 1, &sensor, sizeof(sensor));
			#endif
		}
	}

	isp_dev->gpio_values.nvalues = isp_dev->enable_gpios->ndescs;
	isp_dev->gpio_values.values = bitmap_alloc(isp_dev->gpio_values.nvalues, GFP_KERNEL);
	if (!isp_dev->gpio_values.values) {
		isp_error(dev, "failed to allocate memory for gpio_values...!\n");
		return -ENOMEM;
	}

	ret = isp_fetch_clocks(dev, isp_clks);
	if (ret) {
		isp_error(dev, "isp clock fetch failed...!\n");
		bitmap_free(isp_dev->gpio_values.values);
		return ret;
	}
	ret = isp_enable_clocks(dev, isp_clks);
	if (ret) {
		isp_error(dev, "isp clock enable failed...!\n");
		bitmap_free(isp_dev->gpio_values.values);
		return ret;
	}
	isp_dev->isp_clks = isp_clks;

	dev_set_drvdata(dev, isp_dev);

	writel(ISP_CORE_TOP_CTRL_INIT, isp_dev->reg_base +
				ISP_CORE_BASE + ISP_CORE_TOP_CTRL_REG);

	/* create PE device proc file */
	isp_dev->dev_procdir = proc_mkdir(ISP_DEVICE_NAME, NULL);

	//Probe & init for all isp modules
	ret = isp_invoke_mod_probe(isp_dev, isp_dev->mod_ctx);
	if (ret) {
		isp_error(dev, "isp driver probe/init failed...!\n");
		bitmap_free(isp_dev->gpio_values.values);
		return ret;
	}

	return 0;
}

static int isp_remove(struct platform_device *pdev)
{
	int i;
	isp_device *isp_dev = dev_get_drvdata(&pdev->dev);
	struct clk **isp_clks = isp_dev->isp_clks;

	bitmap_free(isp_dev->gpio_values.values);

	for (i = 0; i < ARRAY_SIZE(isp_clock_list); i++)
		clk_disable_unprepare(isp_clks[i]);

	isp_device_exit(isp_dev);

	return 0;
}

static void isp_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	isp_trace(dev, "%s\n", __func__);

#ifdef CONFIG_PM_SLEEP
	{
		isp_device *isp_dev = dev_get_drvdata(dev);

		//Suspend all ISP modules
		isp_invoke_mod_suspend(isp_dev, isp_dev->mod_ctx);
	}
#endif //CONFIG_PM_SLEEP
}

#ifdef CONFIG_PM_SLEEP
static int isp_suspend(struct device *dev)
{
	isp_device *isp_dev = dev_get_drvdata(dev);

	isp_trace(dev, "%s\n", __func__);

	//Suspend all ISP modules
	isp_invoke_mod_suspend(isp_dev, isp_dev->mod_ctx);

	return 0;
}

static int isp_resume(struct device *dev)
{
	isp_device *isp_dev = dev_get_drvdata(dev);

	isp_trace(dev, "%s\n", __func__);

	//Resume all ISP modules
	isp_invoke_mod_resume(isp_dev, isp_dev->mod_ctx);

	return 0;
}
#endif //CONFIG_PM_SLEEP

static SIMPLE_DEV_PM_OPS(isp_pmops, isp_suspend, isp_resume);

static struct platform_driver isp_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	.shutdown = isp_shutdown,
	.driver = {
		.name = ISP_DEVICE_NAME,
		.of_match_table = isp_match,
		.pm = &isp_pmops,
	},
};

module_platform_driver(isp_driver);

MODULE_AUTHOR("Synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ISP module driver");
