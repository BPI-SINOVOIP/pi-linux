// SPDX-License-Identifier: GPL-2.0
/*
 * Synaptics AS470 ADC driver
 *
 * Copyright (C) 2020 Synaptics Incorporated
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
#define  ADC_ENCTR_SHIFT	0
#define  ADC_ENCTR_MASK		(7 << 0)
#define  ADC_SELIN_SHIFT	3
#define  ADC_SELIN_MASK		(7 << 3)
#define  ADC_EN			BIT(6)
#define  ADC_SEL_VREF		BIT(7)
#define  ADC_SELBG		BIT(8)
#define  ADC_SELDIFF		BIT(9)
#define  ADC_SELRES_SHIFT	10
#define  ADC_SELRES_MASK	(3 << 10)
#define  ADC_START		BIT(12)
#define  ADC_RESET		BIT(13)
#define  ADC_CONT		BIT(14)
#define  ADC_DAT_LT_SHIFT	15
#define  ADC_DAT_LT_MASK	(0x1f << 15)
#define  INT_EN			BIT(20)
#define  OUTOFRANGE_INT_EN	BIT(21)
#define STATUS			0x4
#define  DATA_RDY		BIT(0)
#define  ADC_TEST_FAIL		BIT(1)
#define  ADC_MAX_FAIL		BIT(2)
#define  ADC_MIN_FAIL		BIT(3)
#define DATA			0x8
#define  ADC_DATA_MASK		0xFFF
#define DATA_CHK_CTRL		0xc

struct as470_adc_priv {
	void __iomem		*base;
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

static const struct iio_chan_spec as470_adc_channels[] = {
	ADC_CHANNEL(0, IIO_VOLTAGE),
	ADC_CHANNEL(1, IIO_VOLTAGE),
	ADC_CHANNEL(2, IIO_VOLTAGE),
	ADC_CHANNEL(3, IIO_VOLTAGE),
	IIO_CHAN_SOFT_TIMESTAMP(5),
};

static inline u32 rdl(struct as470_adc_priv *priv, u32 offset)
{
	return readl_relaxed(priv->base + offset);
}

static inline void wrl(struct as470_adc_priv *priv, u32 offset, u32 data)
{
	writel_relaxed(data, priv->base + offset);
}

static int as470_adc_read(struct iio_dev *indio_dev, int channel)
{
	u32 val;
	int data, ret;
	struct as470_adc_priv *priv = iio_priv(indio_dev);

	mutex_lock(&priv->lock);

	val = rdl(priv, CTRL);
	val &= ~ADC_SELIN_MASK;
	val |= (channel << ADC_SELIN_SHIFT);
	val |= INT_EN | ADC_START;
	wrl(priv, CTRL, val);

	ret = wait_event_interruptible_timeout(priv->wq, priv->data_available,
					       msecs_to_jiffies(1000));

	val = rdl(priv, CTRL);
	val &= ~INT_EN;
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

static int as470_adc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int *val,
				int *val2, long mask)
{
	int tmp;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (chan->type != IIO_VOLTAGE)
			return -EINVAL;

		tmp = as470_adc_read(indio_dev, chan->channel);
		if (tmp < 0)
			return tmp;

		*val = tmp;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		/*
		 * vrefint / 4095 * raw
		 * The typical value of vrefint is 1200
		 */
		*val = 0;
		*val2 = 1200 * 244200;
		return IIO_VAL_INT_PLUS_NANO;
	default:
		break;
	}

	return -EINVAL;
}

static irqreturn_t as470_adc_irq(int irq, void *private)
{
	u32 val, status;
	struct as470_adc_priv *priv = iio_priv(private);

	status = rdl(priv, STATUS);
	if (status & DATA_RDY) {
		status &= ~DATA_RDY;
		wrl(priv, STATUS, status);
		val = rdl(priv, DATA);
		priv->data = val & ADC_DATA_MASK;
		priv->data_available = true;
		wake_up_interruptible(&priv->wq);
	}

	return IRQ_HANDLED;
}

static int as470_adc_startup(struct as470_adc_priv *priv)
{
	int ret;
	u32 val;

	val = ADC_SELBG | ADC_SEL_VREF | ADC_EN;
	wrl(priv, CTRL, val);
	ndelay(800);

	val |= ADC_RESET;
	wrl(priv, CTRL, val);
	ndelay(20);
	val &= ~ADC_RESET;
	wrl(priv, CTRL, val);

	wrl(priv, STATUS, 0);
	val |= INT_EN | ADC_START;
	val &= ~ADC_SELRES_MASK;
	val |= (3 << ADC_SELRES_SHIFT);
	val &= ~ADC_SELIN_MASK;
	wrl(priv, CTRL, val);

	ret = wait_event_interruptible_timeout(priv->wq, priv->data_available,
					       msecs_to_jiffies(1000));
	val = rdl(priv, CTRL);
	val &= ~INT_EN;
	wrl(priv, CTRL, val);

	if (ret == 0)
		ret = -ETIMEDOUT;
	if (ret < 0)
		return ret;
	return 0;
}

static const struct iio_info as470_adc_info = {
	.driver_module	= THIS_MODULE,
	.read_raw	= as470_adc_read_raw,
};

static int as470_adc_probe(struct platform_device *pdev)
{
	int irq, ret;
	struct as470_adc_priv *priv;
	struct iio_dev *indio_dev;
	struct resource *res;
	struct device *dev = &pdev->dev;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*priv));
	if (!indio_dev)
		return -ENOMEM;

	priv = iio_priv(indio_dev);
	platform_set_drvdata(pdev, indio_dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	ret = devm_request_irq(dev, irq, as470_adc_irq, 0,
			       dev->driver->name, indio_dev);
	if (ret)
		return ret;

	init_waitqueue_head(&priv->wq);
	mutex_init(&priv->lock);

	ret = as470_adc_startup(priv);
	if (ret)
		return ret;

	indio_dev->dev.parent = dev;
	indio_dev->name = dev_name(dev);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &as470_adc_info;

	indio_dev->channels = as470_adc_channels;
	indio_dev->num_channels = ARRAY_SIZE(as470_adc_channels);

	return devm_iio_device_register(dev, indio_dev);
}

static const struct of_device_id as470_adc_match[] = {
	{ .compatible = "syna,as470-adc", },
	{ },
};
MODULE_DEVICE_TABLE(of, as470_adc_match);

static struct platform_driver as470_adc_driver = {
	.driver	= {
		.name		= "as470-adc",
		.of_match_table	= as470_adc_match,
	},
	.probe	= as470_adc_probe,
};
module_platform_driver(as470_adc_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics AS470 ADC driver");
MODULE_LICENSE("GPL v2");
