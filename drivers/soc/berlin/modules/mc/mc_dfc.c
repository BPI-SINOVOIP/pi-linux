// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated
*
* Author: Benson Gui <Benson.Gui@synaptics.com>
*
*/

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mc_dfc.h>

#define DEVICE_NAME	"mc_dfc"

#define CTRLA					0x0
#define CTRLB					0x4
#define CTRLC					0x8
#define CTRLD					0xc
#define CTRLE					0x10
#define CTRLF					0x14
#define CTRLG					0x18

#define MC_REGS_HWFFCCTL			0x3c
#define MC_REGS_DFIMISC				0x1b0
#define MC_REGS_SWCTL				0x320
#define MC_REGS_SWSTAT				0x324

#define RA_mc_wrap_ddrc_low_pwr_ifc_ch0		0x0380
#define RA_mc_wrap_ddrc_low_pwr_ifc_ch1		0x0384
#define RA_mc_wrap_DFC_PMU_CTRL			0x0388
#define RA_mc_wrap_DFC_PMU_CTRL1		0x038C
#define RA_mc_wrap_DFC_PMU_CTRL2		0x0390

#define MEMPLL_OFFSET				0x8
#define PMU_OFFSET				0x28

enum {
	CTRLA_BK = 0,
	CTRLB_BK,
	CTRLC_BK,
	CTRLD_BK,
	CTRLE_BK,
	CTRLF_BK,
	CTRLG_BK,
	CTRLBK_MAX,
};

struct mc_dfc_priv {
	struct device *dev;
	const char *dev_name;
	void __iomem *ddrc_base;
	void __iomem *mcss_base;
	void __iomem *pmu_base;
	void __iomem *mempll_base;
	struct completion comp;
	u32 pmu_def[CTRLBK_MAX];
	u32 mempll_def[CTRLBK_MAX];
	struct mutex mc_lock;
	int irq;
	bool down;

	/* cdev */
	struct class *drv_class;
	dev_t dev_num;
	struct cdev cdev;
};

static const struct of_device_id syna_mc_dfc_dt_ids[] = {
	{ .compatible = "syna,vs680-mc-dfc" },
	{}
};
MODULE_DEVICE_TABLE(of, syna_mc_dfc_dt_ids);

static irqreturn_t syna_mc_dfc_irq(int irq, void *irq_data)
{
	struct mc_dfc_priv *mc_dfc = irq_data;
	u32 val;

	complete(&mc_dfc->comp);

	val = readl_relaxed(mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);
	val |= 0x8;
	writel_relaxed(val, mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);

	return IRQ_HANDLED;
}

static void update_pll_setting(void __iomem *pllbase, u32 *setting)
{
	writel_relaxed(setting[CTRLA_BK], pllbase + CTRLA);
	writel_relaxed(setting[CTRLB_BK], pllbase + CTRLB);
	writel_relaxed(setting[CTRLC_BK], pllbase + CTRLC);
	writel_relaxed(setting[CTRLD_BK], pllbase + CTRLD);
	writel_relaxed(setting[CTRLE_BK], pllbase + CTRLE);
	writel_relaxed(setting[CTRLF_BK], pllbase + CTRLF);
	writel_relaxed(setting[CTRLG_BK], pllbase + CTRLG);
}

static int syna_mc_dfc_switch(struct mc_dfc_priv *mc_dfc, bool down)
{
	u32 val;

	mutex_lock(&mc_dfc->mc_lock);
	if (mc_dfc->down == down) {
		mutex_unlock(&mc_dfc->mc_lock);
		return 0;
	}

	/* enable Quasi-Dynamic reg programming */
	writel_relaxed(0, mc_dfc->ddrc_base + MC_REGS_SWCTL);
	val = readl_relaxed(mc_dfc->ddrc_base + MC_REGS_DFIMISC);
	val &= ~(0x1f << 8);
	if (down)
		val |= 0x11 << 8;
	else
		val |= 0x10 << 8;
	writel_relaxed(val, mc_dfc->ddrc_base + MC_REGS_DFIMISC);

	/* disable Quasi-Dynamic reg programming */
	writel_relaxed(1, mc_dfc->ddrc_base + MC_REGS_SWCTL);
	do {
		val = readl_relaxed(mc_dfc->ddrc_base + MC_REGS_SWSTAT);
	} while ( val != 0x1);

	/* HWFFCCTL.hwffc_en = 2'b11 */
	val = readl_relaxed(mc_dfc->ddrc_base + MC_REGS_HWFFCCTL);
	val |= 0x3;
	writel_relaxed(val, mc_dfc->ddrc_base + MC_REGS_HWFFCCTL);

	if (down) {
		update_pll_setting(mc_dfc->pmu_base, mc_dfc->pmu_def);
	} else
		update_pll_setting(mc_dfc->pmu_base, mc_dfc->mempll_def);

	if (down)
		val = 0x3;
	else
		val = 0x2;
	writel_relaxed(val, mc_dfc->mcss_base + RA_mc_wrap_ddrc_low_pwr_ifc_ch0);
	writel_relaxed(val, mc_dfc->mcss_base + RA_mc_wrap_ddrc_low_pwr_ifc_ch1);
	writel_relaxed(0x100, mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);
	writel_relaxed(0x00100010, mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL1);
	writel_relaxed(0x4E20ffff, mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL2);
	dsb(ish);

	init_completion(&mc_dfc->comp);
	/* Start DFC */
	val = readl_relaxed(mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);
	val |= 0x7;
	writel_relaxed(val, mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);

	if (!wait_for_completion_timeout(&mc_dfc->comp, msecs_to_jiffies(100)))
		dev_err(mc_dfc->dev, "dfc interrupt timeout and continue\n");

	if (down)
		update_pll_setting(mc_dfc->mempll_base, mc_dfc->pmu_def);
	else
		update_pll_setting(mc_dfc->mempll_base, mc_dfc->mempll_def);

	/* Disable DFC PMU */
	val = readl_relaxed(mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);
	val &= ~0x7;
	writel_relaxed(val, mc_dfc->mcss_base + RA_mc_wrap_DFC_PMU_CTRL);

	mc_dfc->down = down;
	mutex_unlock(&mc_dfc->mc_lock);

	return 0;
}

static void syna_mc_dfc_get_def(struct mc_dfc_priv *mc_dfc)
{
	mc_dfc->pmu_def[CTRLA_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLA);
	mc_dfc->pmu_def[CTRLB_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLB);
	mc_dfc->pmu_def[CTRLC_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLC);
	mc_dfc->pmu_def[CTRLD_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLD);
	mc_dfc->pmu_def[CTRLE_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLE);
	mc_dfc->pmu_def[CTRLF_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLF);
	mc_dfc->pmu_def[CTRLG_BK] = readl_relaxed(mc_dfc->pmu_base + CTRLG);

	mc_dfc->mempll_def[CTRLA_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLA);
	mc_dfc->mempll_def[CTRLB_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLB);
	mc_dfc->mempll_def[CTRLC_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLC);
	mc_dfc->mempll_def[CTRLD_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLD);
	mc_dfc->mempll_def[CTRLE_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLE);
	mc_dfc->mempll_def[CTRLF_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLF);
	mc_dfc->mempll_def[CTRLG_BK] = readl_relaxed(mc_dfc->mempll_base + CTRLG);
}

static long syna_mc_dfc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct mc_dfc_priv *mc_dfc = filp->private_data;

	switch (cmd) {
	case DFC_IOC_PMU_SW:
	{
		ret = syna_mc_dfc_switch(mc_dfc, !!arg);
		break;
	}
	default:
		return -EINVAL;
	}

	return ret;
}

static int syna_mc_dfc_open(struct inode *inode, struct file *fd)
{
	struct mc_dfc_priv *mc_dfc =
		container_of(inode->i_cdev, struct mc_dfc_priv, cdev);

	fd->private_data = mc_dfc;

	return 0;
}

static const struct file_operations mc_dfc_fops = {
	.owner          = THIS_MODULE,
	.open		= syna_mc_dfc_open,
	.unlocked_ioctl = syna_mc_dfc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= syna_mc_dfc_ioctl,
#endif
};

static int syna_mc_dfc_create_dev(struct mc_dfc_priv *mc_dfc)
{
	int ret;
	struct device *dev = mc_dfc->dev;
	struct device *class_dev;

	ret = alloc_chrdev_region(&mc_dfc->dev_num, 0, 1,
				DEVICE_NAME);
	if (ret < 0) {
		dev_err(dev, "alloc_chrdev_region failed %d\n", ret);
		return ret;
	}

	mc_dfc->drv_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mc_dfc->drv_class)) {
		dev_err(dev, "class create failed\n");
		ret = PTR_ERR(mc_dfc->drv_class);
		goto err_unregister_chrdev_region;
	}

	class_dev = device_create(mc_dfc->drv_class,
					NULL, mc_dfc->dev_num,
					NULL, DEVICE_NAME);
	if (IS_ERR(class_dev)) {
		dev_err(dev, "device create failed\n");
		ret = PTR_ERR(class_dev);
		goto err_class_destroy;
	}

	cdev_init(&mc_dfc->cdev, &mc_dfc_fops);
	mc_dfc->cdev.owner = THIS_MODULE;

	ret = cdev_add(&mc_dfc->cdev,
			MKDEV(MAJOR(mc_dfc->dev_num), 0), 1);
	if (ret < 0) {
		dev_err(dev, "cdev_add failed %d\n", ret);
		goto err_class_dev_destroy;
	}

	return 0;

err_class_dev_destroy:
	device_destroy(mc_dfc->drv_class, mc_dfc->dev_num);
err_class_destroy:
	class_destroy(mc_dfc->drv_class);
err_unregister_chrdev_region:
	unregister_chrdev_region(mc_dfc->dev_num, 1);
	return ret;

}

static int syna_mc_dfc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mc_dfc_priv *mc_dfc;
	struct resource *res;
	int ret;

	mc_dfc = devm_kzalloc(dev, sizeof(*mc_dfc), GFP_KERNEL);
	if (WARN_ON(!mc_dfc))
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mc_dfc->ddrc_base = devm_ioremap(dev, res->start, resource_size(res));
	if (WARN_ON(!mc_dfc->ddrc_base))
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	mc_dfc->mcss_base = devm_ioremap(dev, res->start, resource_size(res));
	if (WARN_ON(!mc_dfc->mcss_base))
		return -ENOMEM;

	mc_dfc->pmu_base = mc_dfc->mcss_base + PMU_OFFSET;
	mc_dfc->mempll_base = mc_dfc->mcss_base + MEMPLL_OFFSET;

	mc_dfc->irq = platform_get_irq(pdev, 0);
	if (WARN_ON(mc_dfc->irq < 0))
		return mc_dfc->irq;

	ret = devm_request_irq(dev, mc_dfc->irq,
				  syna_mc_dfc_irq, IRQF_SHARED,
				  dev_name(dev), mc_dfc);
	if (WARN_ON(ret))
		return ret;

	mutex_init(&mc_dfc->mc_lock);
	mc_dfc->dev = dev;
	mc_dfc->dev_name = dev_name(dev);
	mc_dfc->down = false;

	syna_mc_dfc_get_def(mc_dfc);
	ret = syna_mc_dfc_create_dev(mc_dfc);
	if (ret) {
		dev_err(dev, "create mc_dfc dev failed!\n");
		return ret;
	}

	dev_set_drvdata(dev, mc_dfc);
	return 0;
}

static int syna_mc_dfc_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mc_dfc_priv *mc_dfc = dev_get_drvdata(dev);

	device_destroy(mc_dfc->drv_class, mc_dfc->dev_num);
	class_destroy(mc_dfc->drv_class);
	unregister_chrdev_region(mc_dfc->dev_num, 1);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int syna_mc_dfc_suspend(struct device *dev)
{
	return 0;
}

static int syna_mc_dfc_resume(struct device *dev)
{
	struct mc_dfc_priv *mc_dfc = dev_get_drvdata(dev);

	mc_dfc->down = false;

	return 0;
}

static const struct dev_pm_ops syna_mc_dfc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(syna_mc_dfc_suspend, syna_mc_dfc_resume)
};

#define DEV_PM_OPS	(&syna_mc_dfc_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver syna_mc_dfc_driver = {
	.probe = syna_mc_dfc_probe,
	.remove = syna_mc_dfc_remove,
	.driver = {
		.name = "syna-mc-dfc",
		.of_match_table = syna_mc_dfc_dt_ids,
		.pm	= DEV_PM_OPS,
	},
};
module_platform_driver(syna_mc_dfc_driver);

MODULE_DESCRIPTION("Synaptics mc dfc pmu controller");
MODULE_ALIAS("platform:syna-mc-dfc");
MODULE_LICENSE("GPL v2");
