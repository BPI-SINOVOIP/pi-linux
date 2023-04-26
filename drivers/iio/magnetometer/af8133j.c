// SPDX-License-Identifier: GPL-2.0-only
/*
 * af8133j.c - Voltafield AF8133J magnetometer driver
 *
 * Based on mmc35240.c, which is:
 *   Copyright (c) 2015, Intel Corporation.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>

#define AF8133J_DRV_NAME "af8133j"

#define AF8133J_REG_OUT		0x03
/* Little endian */
#define AF8133J_REG_OUT_SIZE	0x06

#define AF8133J_REG_PCODE	0x00
#define AF8133J_REG_PCODE_VAL	0x5e

/* Named STATUS in datasheet, renamed here to prevent confusion with STATE */
#define AF8133J_REG_DRDY	0x02
#define AF8133J_REG_DRDY_ACQ	BIT(0)
/* Named STATE in datasheet, renamed here to prevent confusion */
#define AF8133J_REG_STATE	0x0a
#define AF8133J_REG_STATE_STBY	0x00
#define AF8133J_REG_STATE_WORK	0x01
#define AF8133J_REG_RANGE	0x0b
#define AF8133J_REG_RANGE_22G	0x12
#define AF8133J_REG_RANGE_12G	0x34
/* Software reset */
#define AF8133J_REG_SWR		0x11
#define AF8133J_REG_SWR_PERFORM	BIT(0)

static const char * const af8133j_supply_names[] = {
        "avdd",
        "dvdd",
};

#define AF8133J_NUM_SUPPLIES ARRAY_SIZE(af8133j_supply_names)

struct af8133j_data {
	struct i2c_client *client;
	struct regmap *regmap;
	struct mutex mutex;

	struct gpio_desc *reset_gpiod;
	struct iio_mount_matrix orientation;
        struct regulator_bulk_data supplies[AF8133J_NUM_SUPPLIES];
	bool powered;
};

enum af8133j_axis {
	AXIS_X = 0,
	AXIS_Y,
	AXIS_Z,
};

static struct iio_mount_matrix *
af8133j_get_mount_matrix(struct iio_dev *indio_dev,
			 const struct iio_chan_spec *chan)
{
	struct af8133j_data *data = iio_priv(indio_dev);
	return &data->orientation;
}

static const struct iio_chan_spec_ext_info af8133j_ext_info[] = {
	IIO_MOUNT_MATRIX(IIO_SHARED_BY_DIR, af8133j_get_mount_matrix),
	{ }
};

/*	.info_mask_shared_by_type = , \*/

#define AF8133J_CHANNEL(_axis) { \
	.type = IIO_MAGN, \
	.modified = 1, \
	.channel2 = IIO_MOD_ ## _axis, \
	.address = AXIS_ ## _axis, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE), \
	.ext_info = af8133j_ext_info, \
	.scan_index = AXIS_ ## _axis, \
	.scan_type = { \
		.sign = 's', \
		.realbits = 16, \
		.storagebits = 16, \
		.endianness = IIO_LE, \
	}, \
}

static const struct iio_chan_spec af8133j_channels[] = {
	AF8133J_CHANNEL(X),
	AF8133J_CHANNEL(Y),
	AF8133J_CHANNEL(Z),
	IIO_CHAN_SOFT_TIMESTAMP(3),
};

static int af8133j_power_up(struct af8133j_data *data)
{
	struct device *dev = &data->client->dev;
	unsigned int val;
	int ret;

	if (data->powered)
		return 0;

        ret = regulator_bulk_enable(AF8133J_NUM_SUPPLIES, data->supplies);
        if (ret) {
                dev_err(dev, "Could not enable regulators\n");
		return ret;
	}

	msleep(15);

	gpiod_set_value_cansleep(data->reset_gpiod, 0);

	msleep(1);

	ret = regmap_read(data->regmap, AF8133J_REG_PCODE, &val);
	if (ret < 0) {
		dev_err(dev, "Error reading product code\n");
		goto out_assert_reset;
	}

	if (val != AF8133J_REG_PCODE_VAL) {
		dev_err(dev, "Unknown AF8133J product code 0x%x\n", val);
		ret = -EINVAL;
		goto out_assert_reset;
	}

	/* Reset the chip */
	ret = regmap_write(data->regmap, AF8133J_REG_SWR,
			   AF8133J_REG_SWR_PERFORM);
	if (ret < 0) {
		dev_err(dev, "Failed to reset the chip\n");
		goto out_assert_reset;
	}

	/* Wait for reset finish */
	usleep_range(1000, 1100);

	/* Check whether the reset bit is cleared */
	ret = regmap_read(data->regmap, AF8133J_REG_SWR, &val);
	if (ret < 0) {
		dev_err(dev, "Failed to read reset status\n");
		goto out_assert_reset;
	}
	if (val & AF8133J_REG_SWR_PERFORM) {
		dev_err(dev, "Device is not responding to reset\n");
		ret = -EIO;
		goto out_assert_reset;
	}

	data->powered = true;
	return 0;

out_assert_reset:
	gpiod_set_value_cansleep(data->reset_gpiod, 1);
        regulator_bulk_disable(AF8133J_NUM_SUPPLIES, data->supplies);
	return ret;
}

static void af8133j_power_down(struct af8133j_data *data)
{
//	struct device *dev = &data->client->dev;

	if (!data->powered)
		return;

	gpiod_set_value_cansleep(data->reset_gpiod, 1);
        regulator_bulk_disable(AF8133J_NUM_SUPPLIES, data->supplies);
	data->powered = false;
}

static int af8133j_take_measurement(struct af8133j_data *data)
{
	unsigned int val;
	int ret;

	ret = regmap_write(data->regmap, AF8133J_REG_STATE,
			   AF8133J_REG_STATE_WORK);
	if (ret < 0)
		return ret;

	/* The datasheet says "Mesaure Time <1.5ms" */
	ret = regmap_read_poll_timeout(data->regmap, AF8133J_REG_DRDY, val,
				       val & AF8133J_REG_DRDY_ACQ,
				       100, 1500);
	if (ret < 0)
		return ret;

	ret = regmap_write(data->regmap, AF8133J_REG_STATE,
			   AF8133J_REG_STATE_STBY);
	if (ret < 0)
		return ret;

	return 0;
}

static int af8133j_read_measurement(struct af8133j_data *data, __le16 buf[3])
{
	struct device *dev = &data->client->dev;
	int ret;

	ret = pm_runtime_resume_and_get(dev);
	if (ret) {
		dev_err(dev, "failed to power on\n");
		return ret;
	}

	mutex_lock(&data->mutex);

	ret = af8133j_take_measurement(data);
	if (ret == 0)
		ret = regmap_bulk_read(data->regmap, AF8133J_REG_OUT, buf,
				       3 * sizeof(__le16));

	mutex_unlock(&data->mutex);

	pm_runtime_mark_last_busy(dev);
	if (pm_runtime_put_autosuspend(dev))
		dev_err(dev, "failed to power off\n");

	return ret;
}

static int af8133j_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan, int *val,
			     int *val2, long mask)
{
	struct af8133j_data *data = iio_priv(indio_dev);
	__le16 buf[3];
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = af8133j_read_measurement(data, buf);
		if (ret < 0)
			return ret;

		*val = sign_extend32(le16_to_cpu(buf[chan->address]), 15);
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		/* We only use the default 12G scale now */
		*val = 0;
		*val2 = 12 * 1000000 / 32768;
		return IIO_VAL_INT_PLUS_MICRO;
	default:
		return -EINVAL;
	}
}

static const struct iio_info af8133j_info = {
	.read_raw	= af8133j_read_raw,
};

irqreturn_t af8133j_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct af8133j_data *sdata = iio_priv(indio_dev);
	s64 timestamp = iio_get_time_ns(indio_dev);
	u8 buf[2 * 3 + 8];
	int ret;

	ret = af8133j_read_measurement(sdata, (__le16*)buf);
	if (ret < 0)
		return ret;

	iio_push_to_buffers_with_timestamp(indio_dev, buf, timestamp);
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static const struct regmap_config af8133j_regmap_config = {
	.name = "af8133j_regmap",

	.reg_bits = 8,
	.val_bits = 8,

	.max_register = AF8133J_REG_SWR,
	.cache_type = REGCACHE_NONE,
};

static int af8133j_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct af8133j_data *data;
	struct iio_dev *indio_dev;
	struct regmap *regmap;
	int ret, i;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	regmap = devm_regmap_init_i2c(client, &af8133j_regmap_config);
	if (IS_ERR(regmap))
		return dev_err_probe(dev, PTR_ERR(regmap),
				     "regmap initialization failed\n");

	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	data->client = client;
	data->regmap = regmap;
	mutex_init(&data->mutex);

	data->reset_gpiod = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(data->reset_gpiod))
		return dev_err_probe(dev, PTR_ERR(data->reset_gpiod),
				     "Failed to get reset gpio\n");

        for (i = 0; i < AF8133J_NUM_SUPPLIES; i++)
                data->supplies[i].supply = af8133j_supply_names[i];
        ret = devm_regulator_bulk_get(dev, AF8133J_NUM_SUPPLIES, data->supplies);
        if (ret)
		return ret;

	ret = iio_read_mount_matrix(dev, &data->orientation);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to read mount matrix\n");

	/*
	 * Check if the device is responding.
	 */
	ret = af8133j_power_up(data);
	if (ret)
		return ret;

	af8133j_power_down(data);

	indio_dev->info = &af8133j_info;
	indio_dev->name = AF8133J_DRV_NAME;
	indio_dev->channels = af8133j_channels;
	indio_dev->num_channels = ARRAY_SIZE(af8133j_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
		NULL, &af8133j_trigger_handler, NULL);
	if (ret < 0)
		return ret;

	ret = devm_iio_device_register(dev, indio_dev);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to register iio device");

	pm_runtime_enable(dev);
	pm_runtime_set_autosuspend_delay(dev, 500);
	pm_runtime_use_autosuspend(dev);

	return 0;
}

static void af8133j_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct af8133j_data *data = iio_priv(indio_dev);
	struct device *dev = &data->client->dev;

	pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);
	pm_runtime_put_noidle(dev);

	af8133j_power_down(data);
}

static int __maybe_unused af8133j_runtime_suspend(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct af8133j_data *data = iio_priv(indio_dev);

	af8133j_power_down(data);

	return 0;
}

static int __maybe_unused af8133j_runtime_resume(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct af8133j_data *data = iio_priv(indio_dev);

	return af8133j_power_up(data);
}

const struct dev_pm_ops af8133j_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend, pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(af8133j_runtime_suspend, af8133j_runtime_resume, NULL)
};

static const struct of_device_id af8133j_of_match[] = {
	{ .compatible = "voltafield,af8133j", },
	{ }
};
MODULE_DEVICE_TABLE(of, af8133j_of_match);

static const struct i2c_device_id af8133j_id[] = {
	{"af8133j", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, af8133j_id);

static struct i2c_driver af8133j_driver = {
	.driver = {
		.name = AF8133J_DRV_NAME,
		.of_match_table = af8133j_of_match,
		.pm = &af8133j_pm_ops,
	},
	.probe = af8133j_probe,
	.remove = af8133j_remove,
	.id_table = af8133j_id,
};

module_i2c_driver(af8133j_driver);

MODULE_AUTHOR("Icenowy Zheng <icenowy@aosc.io>");
MODULE_DESCRIPTION("Voltafield AF8133J magnetic sensor driver");
MODULE_LICENSE("GPL v2");
