// SPDX-License-Identifier: GPL-2.0
/*
 * Synaptics AS370 ADC driver
 *
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/driver.h>
#include <linux/iio/machine.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>

#define CTRL			0x0
#define  CTRL_SEL_CHA_SHIFT	0
#define  CTRL_SEL_CHA_MASK	(7 << 0)
#define  CTRL_PD_ADC		BIT(3)
#define  CTRL_MODE_SHIFT	4
#define  CTRL_MODE_MASK		(0x3 << 4)
#define  CTRL_SOC		BIT(6)
#define  CTRL_EN_VCM		BIT(7)
#define  CTRL_SEL_CMP		BIT(8)
#define  CTRL_INTR_MASK		BIT(12)
#define  CTRL_INTR_CLR		BIT(13)
#define STS			0x4
#define  STS_DATA_MASK		0xFFF
#define  STS_EOC		BIT(13)

struct as370_adc_priv {
	void __iomem		*base;
	u32			vref_mv;
	struct mutex		lock;
	wait_queue_head_t	wq;
	int			data;
	bool			data_available;
};

#define ADC_CHANNEL(n, t)						\
	{								\
		.channel		= n,				\
		.datasheet_name		= "channel"#n,			\
		.type			= t,				\
		.indexed		= 1,				\
		.info_mask_separate	= BIT(IIO_CHAN_INFO_RAW),	\
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
	}

static const struct iio_chan_spec as370_adc_channels[] = {
	ADC_CHANNEL(0, IIO_VOLTAGE),
	ADC_CHANNEL(1, IIO_VOLTAGE),
	ADC_CHANNEL(2, IIO_VOLTAGE),
	ADC_CHANNEL(3, IIO_VOLTAGE),
	ADC_CHANNEL(4, IIO_VOLTAGE),
	ADC_CHANNEL(5, IIO_VOLTAGE),
	ADC_CHANNEL(7, IIO_VOLTAGE),
	IIO_CHAN_SOFT_TIMESTAMP(8),
};

static inline u32 rdl(struct as370_adc_priv *priv, u32 offset)
{
	return readl_relaxed(priv->base + offset);
}

static inline void wrl(struct as370_adc_priv *priv, u32 offset, u32 data)
{
	writel_relaxed(data, priv->base + offset);
}

static int as370_adc_read(struct iio_dev *indio_dev, int channel)
{
	u32 val;
	int data, ret;
	struct as370_adc_priv *priv = iio_priv(indio_dev);

	mutex_lock(&priv->lock);

	val = 0;
	val &= ~CTRL_SEL_CHA_MASK;
	val |= (channel << CTRL_SEL_CHA_SHIFT);
	val |= CTRL_SEL_CMP;
	val |= CTRL_EN_VCM;
	val &= ~CTRL_INTR_MASK;
	wrl(priv, CTRL, val);

	local_irq_disable();
	val |= CTRL_SOC;
	wrl(priv, CTRL, val);
	udelay(1);
	val &= ~CTRL_SOC;
	wrl(priv, CTRL, val);
	local_irq_enable();

	ret = wait_event_interruptible_timeout(priv->wq, priv->data_available,
					       msecs_to_jiffies(1000));

	val |= CTRL_INTR_MASK;
	wrl(priv, CTRL, val);

	if (ret == 0)
		ret = -ETIMEDOUT;
	if (ret < 0) {
		mutex_unlock(&priv->lock);
		return ret;
	}

	data = priv->data;
	priv->data_available = false;

	mutex_unlock(&priv->lock);

	return data;
}

static int as370_adc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int *val,
				int *val2, long mask)
{
	int tmp;
	struct as370_adc_priv *priv = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (chan->type != IIO_VOLTAGE)
			return -EINVAL;

		tmp = as370_adc_read(indio_dev, chan->channel);
		if (tmp < 0)
			return tmp;

		*val = tmp;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		/*
		 * vref_mv / 4095 * raw
		 * The max value of vref_mv is 1800
		 */
		*val = 0;
		*val2 = priv->vref_mv * 244200;
		return IIO_VAL_INT_PLUS_NANO;
	default:
		break;
	}

	return -EINVAL;
}

static irqreturn_t as370_adc_irq(int irq, void *private)
{
	u32 val;
	struct as370_adc_priv *priv = iio_priv(private);

	val = rdl(priv, CTRL);
	val |= CTRL_INTR_CLR;
	wrl(priv, CTRL, val);

	val = rdl(priv, STS);
	if (val & STS_EOC) {
		priv->data = val & STS_DATA_MASK;
		priv->data_available = true;
		wake_up_interruptible(&priv->wq);
	}

	return IRQ_HANDLED;
}

static const struct iio_info as370_adc_info = {
	.driver_module	= THIS_MODULE,
	.read_raw	= as370_adc_read_raw,
};

static int as370_adc_probe(struct platform_device *pdev)
{
	u32 val;
	int irq, ret;
	struct as370_adc_priv *priv;
	struct iio_dev *indio_dev;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*priv));
	if (!indio_dev)
		return -ENOMEM;

	priv = iio_priv(indio_dev);
	platform_set_drvdata(pdev, indio_dev);

	if (of_property_read_u32(np, "syna,adc-vref", &val)) {
		dev_err(dev, "Missing adc-vref property in the DT.\n");
		return -EINVAL;
	}
	priv->vref_mv = val;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	val = rdl(priv, CTRL);
	val |= CTRL_PD_ADC;
	wrl(priv, CTRL, val);
	val &= ~CTRL_PD_ADC;
	val |= CTRL_INTR_MASK;
	val |= CTRL_INTR_CLR;
	wrl(priv, CTRL, val);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	ret = devm_request_irq(dev, irq, as370_adc_irq, 0,
			       dev->driver->name, indio_dev);
	if (ret)
		return ret;

	init_waitqueue_head(&priv->wq);
	mutex_init(&priv->lock);

	indio_dev->dev.parent = dev;
	indio_dev->name = dev_name(dev);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &as370_adc_info;

	indio_dev->channels = as370_adc_channels;
	indio_dev->num_channels = ARRAY_SIZE(as370_adc_channels);

	return devm_iio_device_register(dev, indio_dev);
}

static int as370_adc_remove(struct platform_device *pdev)
{
	u32 val;
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct as370_adc_priv *priv = iio_priv(indio_dev);

	/* Power down ADC */
	val = rdl(priv, CTRL);
	val |= CTRL_PD_ADC;
	wrl(priv, CTRL, val);

	return 0;
}

static const struct of_device_id as370_adc_match[] = {
	{ .compatible = "syna,as370-adc", },
	{ },
};
MODULE_DEVICE_TABLE(of, as370_adc_match);

static struct platform_driver as370_adc_driver = {
	.driver	= {
		.name		= "as370-adc",
		.of_match_table	= as370_adc_match,
	},
	.probe	= as370_adc_probe,
	.remove	= as370_adc_remove,
};
module_platform_driver(as370_adc_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics AS370 ADC driver");
MODULE_LICENSE("GPL v2");
