// SPDX-License-Identifier: GPL-2.0
/*
 * Synaptics VS680 SoC Hardware Monitoring Driver
 *
 * Copyright (C) 2020 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/bitops.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>

#define CTRL		0x0
#define  ENA		BIT(0)
#define  CLK_EN		BIT(1)
#define  PSAMPLE_SFT	2
#define  PSAMPLE_MSK	0x3
#define  VSAMPLE	BIT(4)
#define  TRIMG_SFT	5
#define  TRIMG_MSK	0x1f
#define  TRIMO_SFT	10
#define  TRIMO_MSK	0x3f
#define  DAT_LT_SFT	16
#define  DAT_LT_MSK	0x1f
#define STATUS		0x4
#define  DATA_RDY	BIT(0)
#define  INT_EN		BIT(1)
#define DATA		0x8
#define CHK_CTRL	0xc
#define DATA_STATUS	0x10

#define AS470_CTRL	0x0
#define AS470_STATUS	0x8
#define AS470_DATA	0xc

struct vs680_hwmon_cfg {
	u8 ctrl;
	u8 status;
	u8 data;
};

struct vs680_hwmon {
	void __iomem *base;
	const struct vs680_hwmon_cfg *cfg;
	int irq;
	struct completion read_completion;
	struct mutex lock;
};

static irqreturn_t vs680_hwmon_irq(int irq, void *data)
{
	u32 val;
	struct vs680_hwmon *hwmon = data;
	const struct vs680_hwmon_cfg *cfg = hwmon->cfg;

	val = readl_relaxed(hwmon->base + cfg->status);
	val &= ~INT_EN;
	writel_relaxed(val, hwmon->base + cfg->status);
	complete(&hwmon->read_completion);
	return IRQ_HANDLED;
}

static int vs680_read_temp(struct vs680_hwmon *hwmon, long *temp)
{
	u32 val;
	long t;
	struct completion *completion = &hwmon->read_completion;
	const struct vs680_hwmon_cfg *cfg = hwmon->cfg;

	mutex_lock(&hwmon->lock);

	reinit_completion(completion);

	val = readl_relaxed(hwmon->base + cfg->status);
	val |= INT_EN;
	writel_relaxed(val, hwmon->base + cfg->status);

	val = readl_relaxed(hwmon->base + cfg->ctrl);
	val |= ENA;
	writel_relaxed(val, hwmon->base + cfg->ctrl);
	val = readl_relaxed(hwmon->base + cfg->ctrl);
	val |= CLK_EN;
	writel_relaxed(val, hwmon->base + cfg->ctrl);

	t = wait_for_completion_interruptible_timeout(completion, HZ);
	if (t <= 0) {
		mutex_unlock(&hwmon->lock);
		return t ? t : -ETIMEDOUT;
	}

	val = readl_relaxed(hwmon->base + cfg->data);
	t = 18439 * val;
	t /= 1000;
	t = 80705 - t;
	t *= val;
	t /= 1000;
	t = 185010 - t;
	t *= val;
	t /= 1000;
	t = 328430 - t;
	t *= val;
	t /= 1000;
	t -= 48690;
	*temp = t;

	val = readl_relaxed(hwmon->base + cfg->status);
	val &= ~DATA_RDY;
	writel_relaxed(val, hwmon->base + cfg->status);

	val = readl_relaxed(hwmon->base + cfg->ctrl);
	val &= ~ENA;
	writel_relaxed(val, hwmon->base + cfg->ctrl);
	val = readl_relaxed(hwmon->base + cfg->ctrl);
	val &= ~CLK_EN;
	writel_relaxed(val, hwmon->base + cfg->ctrl);

	mutex_unlock(&hwmon->lock);

	return 0;
}

static int vs680_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			    u32 attr, int channel, long *temp)
{
	struct vs680_hwmon *hwmon = dev_get_drvdata(dev);

	switch (attr) {
	case hwmon_temp_input:
		return vs680_read_temp(hwmon, temp);
	default:
		return -EOPNOTSUPP;
	}
}

static umode_t
vs680_hwmon_is_visible(const void *data, enum hwmon_sensor_types type,
		       u32 attr, int channel)
{
	if (type != hwmon_temp)
		return 0;

	switch (attr) {
	case hwmon_temp_input:
		return 0444;
	default:
		return 0;
	}
}

static const u32 vs680_hwmon_temp_config[] = {
	HWMON_T_INPUT,
	0
};

static const struct hwmon_channel_info vs680_hwmon_temp = {
	.type = hwmon_temp,
	.config = vs680_hwmon_temp_config,
};

static const struct hwmon_channel_info *vs680_hwmon_info[] = {
	&vs680_hwmon_temp,
	NULL
};

static const struct hwmon_ops vs680_hwmon_ops = {
	.is_visible = vs680_hwmon_is_visible,
	.read = vs680_hwmon_read,
};

static const struct hwmon_chip_info vs680_chip_info = {
	.ops = &vs680_hwmon_ops,
	.info = vs680_hwmon_info,
};

static int vs680_hwmon_probe(struct platform_device *pdev)
{
	int ret;
	u32 val;
	struct resource *res;
	struct device *hwmon_dev;
	struct vs680_hwmon *hwmon;
	struct device *dev = &pdev->dev;

	hwmon = devm_kzalloc(dev, sizeof(*hwmon), GFP_KERNEL);
	if (!hwmon)
		return -ENOMEM;

	hwmon->cfg = of_device_get_match_data(dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hwmon->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hwmon->base))
		return PTR_ERR(hwmon->base);

	hwmon->irq = platform_get_irq(pdev, 0);
	if (hwmon->irq < 0) {
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n",
			hwmon->irq);
		return hwmon->irq;
	}

	init_completion(&hwmon->read_completion);
	mutex_init(&hwmon->lock);

	val = readl_relaxed(hwmon->base + hwmon->cfg->status);
	val &= ~INT_EN;
	writel_relaxed(val, hwmon->base + hwmon->cfg->status);

	ret = devm_request_irq(dev, hwmon->irq, vs680_hwmon_irq, 0,
			       pdev->name, hwmon);
	if (ret) {
		dev_err(dev, "Failed to request irq: %d\n", ret);
		return ret;
	}

	hwmon_dev = devm_hwmon_device_register_with_info(dev,
							 "vs680",
							 hwmon,
							 &vs680_chip_info,
							 NULL);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct vs680_hwmon_cfg vs680_cfg = {
	.ctrl = CTRL,
	.status = STATUS,
	.data = DATA,
};

static const struct vs680_hwmon_cfg as470_cfg = {
	.ctrl = AS470_CTRL,
	.status = AS470_STATUS,
	.data = AS470_DATA,
};

static const struct of_device_id vs680_hwmon_match[] = {
	{
		.compatible = "syna,vs680-hwmon",
		.data = &vs680_cfg
	},
	{
		.compatible = "syna,as470-hwmon",
		.data = &as470_cfg
	},
	{},
};
MODULE_DEVICE_TABLE(of, vs680_hwmon_match);

static struct platform_driver vs680_hwmon_driver = {
	.probe = vs680_hwmon_probe,
	.driver = {
		.name = "vs680-hwmon",
		.of_match_table = vs680_hwmon_match,
	},
};
module_platform_driver(vs680_hwmon_driver);

MODULE_AUTHOR("Jisheng Zhang<jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics VS680 SoC hardware monitor");
MODULE_LICENSE("GPL v2");
