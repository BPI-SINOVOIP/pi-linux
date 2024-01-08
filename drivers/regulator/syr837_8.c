// SPDX-License-Identifier: GPL-2.0
//
// SYR837_8 regulator driver
//
// Copyright (C) 2020 Synaptics Incorporated
//
// Author: Jisheng Zhang <jszhang@kernel.org>

#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

#define SYR837_8_VSEL0		0
#define   SYR837_8_BUCK_EN	(1 << 7)
#define   SYR837_8_MODE		(1 << 6)
#define SYR837_8_VSEL1		1
#define SYR837_8_CTRL		2
#define SYR837_8_ID1		3
#define SYR837_8_ID2		4
#define SYR837_8_PGOOD		5
#define SYR837_8_MAX		(SYR837_8_PGOOD + 1)

#define SYR837_8_NVOLTAGES	64
#define SYR837_8_VSELMIN	712500
#define SYR837_8_VSELSTEP	12500

struct syr837_8_device_info {
	struct device *dev;
	struct regulator_desc desc;
	struct regulator_init_data *regulator;
	struct gpio_desc *en_gpio;
	unsigned int vsel_reg;
	unsigned int vsel_step;
};

static int syr837_8_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	struct syr837_8_device_info *di = rdev_get_drvdata(rdev);

	switch (mode) {
	case REGULATOR_MODE_FAST:
		regmap_update_bits(rdev->regmap, di->vsel_reg,
				   SYR837_8_MODE, SYR837_8_MODE);
		break;
	case REGULATOR_MODE_NORMAL:
		regmap_update_bits(rdev->regmap, di->vsel_reg,
				   SYR837_8_MODE, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static unsigned int syr837_8_get_mode(struct regulator_dev *rdev)
{
	struct syr837_8_device_info *di = rdev_get_drvdata(rdev);
	u32 val;
	int ret = 0;

	ret = regmap_read(rdev->regmap, di->vsel_reg, &val);
	if (ret < 0)
		return ret;
	if (val & SYR837_8_MODE)
		return REGULATOR_MODE_FAST;
	else
		return REGULATOR_MODE_NORMAL;
}

static const struct regulator_ops syr837_8_regulator_ops = {
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.map_voltage = regulator_map_voltage_linear,
	.list_voltage = regulator_list_voltage_linear,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.set_mode = syr837_8_set_mode,
	.get_mode = syr837_8_get_mode,
};

static int syr837_8_regulator_register(struct syr837_8_device_info *di,
			struct regulator_config *config)
{
	struct regulator_desc *rdesc = &di->desc;
	struct regulator_dev *rdev;

	rdesc->name = "syr837_8-reg";
	rdesc->supply_name = "vin";
	rdesc->ops = &syr837_8_regulator_ops;
	rdesc->type = REGULATOR_VOLTAGE;
	rdesc->n_voltages = SYR837_8_NVOLTAGES;
	rdesc->enable_reg = di->vsel_reg;
	rdesc->enable_mask = SYR837_8_BUCK_EN;
	rdesc->min_uV = SYR837_8_VSELMIN;
	rdesc->uV_step = SYR837_8_VSELSTEP;
	rdesc->vsel_reg = di->vsel_reg;
	rdesc->vsel_mask = rdesc->n_voltages - 1;
	rdesc->vsel_step = di->vsel_step;
	rdesc->owner = THIS_MODULE;

	rdev = devm_regulator_register(di->dev, &di->desc, config);
	return PTR_ERR_OR_ZERO(rdev);
}

static bool syr837_8_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case SYR837_8_PGOOD:
		return true;
	}
	return false;
}

static const struct regmap_config syr837_8_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = syr837_8_volatile_reg,
	.num_reg_defaults_raw = SYR837_8_MAX,
	.cache_type = REGCACHE_FLAT,
};

static int syr837_8_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	struct syr837_8_device_info *di;
	struct regulator_config config = { };
	struct regmap *regmap;
	int ret;

	di = devm_kzalloc(dev, sizeof(struct syr837_8_device_info), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->regulator = of_get_regulator_init_data(dev, np, &di->desc);
	if (!di->regulator) {
		dev_err(dev, "Platform data not found!\n");
		return -EINVAL;
	}

	di->en_gpio = devm_gpiod_get_optional(dev, "enable",
			GPIOD_OUT_HIGH | GPIOD_FLAGS_BIT_NONEXCLUSIVE);
	if (IS_ERR(di->en_gpio))
		return PTR_ERR(di->en_gpio);

	if (of_property_read_bool(np, "silergy,vsel-state-high"))
		di->vsel_reg = SYR837_8_VSEL1;
	else
		di->vsel_reg = SYR837_8_VSEL0;

	di->dev = dev;

	regmap = devm_regmap_init_i2c(client, &syr837_8_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(dev, "Failed to allocate regmap!\n");
		return PTR_ERR(regmap);
	}
	i2c_set_clientdata(client, di);

	config.dev = di->dev;
	config.init_data = di->regulator;
	config.regmap = regmap;
	config.driver_data = di;
	config.of_node = np;

	of_property_read_u32(np, "silergy,vsel-step", &di->vsel_step);

	ret = syr837_8_regulator_register(di, &config);
	if (ret < 0)
		dev_err(dev, "Failed to register regulator!\n");
	return ret;
}

static const struct of_device_id syr837_8_dt_ids[] = {
	{
		.compatible = "silergy,syr837_8",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, syr837_8_dt_ids);

static const struct i2c_device_id syr837_8_id[] = {
	{ "syr837_8", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, syr837_8_id);

static struct i2c_driver syr837_8_regulator_driver = {
	.driver = {
		.name = "syr837_8-regulator",
		.of_match_table = of_match_ptr(syr837_8_dt_ids),
	},
	.probe_new = syr837_8_i2c_probe,
	.id_table = syr837_8_id,
};
module_i2c_driver(syr837_8_regulator_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("SYR837_8 regulator driver");
MODULE_LICENSE("GPL v2");
