// SPDX-License-Identifier: GPL-2.0
//
// HL7593 regulator driver
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

#define HL7593_SEL0		0
#define   HL7593_BUCK_EN	(1 << 7)
#define HL7593_SEL1		1
#define HL7593_CTRL		2
#define   HL7593_MODE0		(1 << 0)
#define   HL7593_MODE1		(1 << 1)
#define HL7593_ID1		3
#define HL7593_ID2		4
#define HL7593_MONITOR		5
#define HL7593_MAX		(HL7593_MONITOR + 1)

#define HL7593_NVOLTAGES	128
#define HL7593_VSELMIN		600000
#define HL7593_VSELSTEP		6250

struct hl7593_device_info {
	struct device *dev;
	struct regulator_desc desc;
	struct regulator_init_data *regulator;
	unsigned int vsel_reg;
	unsigned int vsel_step;
};

static int hl7593_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	struct hl7593_device_info *di = rdev_get_drvdata(rdev);
	u32 val =  di->vsel_reg == HL7593_SEL0 ? HL7593_MODE0 : HL7593_MODE1;

	switch (mode) {
	case REGULATOR_MODE_FAST:
		regmap_update_bits(rdev->regmap, HL7593_CTRL, val, val);
		break;
	case REGULATOR_MODE_NORMAL:
		regmap_update_bits(rdev->regmap, HL7593_CTRL, val, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static unsigned int hl7593_get_mode(struct regulator_dev *rdev)
{
	struct hl7593_device_info *di = rdev_get_drvdata(rdev);
	u32 mode =  di->vsel_reg == HL7593_SEL0 ? HL7593_MODE0 : HL7593_MODE1;
	u32 val;
	int ret = 0;

	ret = regmap_read(rdev->regmap, HL7593_CTRL, &val);
	if (ret < 0)
		return ret;
	if (val & mode)
		return REGULATOR_MODE_FAST;
	else
		return REGULATOR_MODE_NORMAL;
}

static const struct regulator_ops hl7593_regulator_ops = {
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.map_voltage = regulator_map_voltage_linear,
	.list_voltage = regulator_list_voltage_linear,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.set_mode = hl7593_set_mode,
	.get_mode = hl7593_get_mode,
};

static int hl7593_regulator_register(struct hl7593_device_info *di,
			struct regulator_config *config)
{
	struct regulator_desc *rdesc = &di->desc;
	struct regulator_dev *rdev;

	rdesc->name = "hl7593-reg";
	rdesc->supply_name = "vin";
	rdesc->ops = &hl7593_regulator_ops;
	rdesc->type = REGULATOR_VOLTAGE;
	rdesc->n_voltages = HL7593_NVOLTAGES;
	rdesc->enable_reg = di->vsel_reg;
	rdesc->enable_mask = HL7593_BUCK_EN;
	rdesc->min_uV = HL7593_VSELMIN;
	rdesc->uV_step = HL7593_VSELSTEP;
	rdesc->vsel_reg = di->vsel_reg;
	rdesc->vsel_mask = rdesc->n_voltages - 1;
	rdesc->vsel_step = di->vsel_step;
	rdesc->owner = THIS_MODULE;

	rdev = devm_regulator_register(di->dev, &di->desc, config);
	return PTR_ERR_OR_ZERO(rdev);
}

static bool hl7593_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case HL7593_MONITOR:
		return true;
	}
	return false;
}

static const struct regmap_config hl7593_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = hl7593_volatile_reg,
	.num_reg_defaults_raw = HL7593_MAX,
	.cache_type = REGCACHE_FLAT,
};

static int hl7593_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	struct hl7593_device_info *di;
	struct regulator_config config = { };
	struct regmap *regmap;
	int ret;

	di = devm_kzalloc(dev, sizeof(struct hl7593_device_info), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->regulator = of_get_regulator_init_data(dev, np, &di->desc);
	if (!di->regulator) {
		dev_err(dev, "Platform data not found!\n");
		return -EINVAL;
	}

	if (of_property_read_bool(np, "halo,vsel-state-high"))
		di->vsel_reg = HL7593_SEL1;
	else
		di->vsel_reg = HL7593_SEL0;

	di->dev = dev;

	regmap = devm_regmap_init_i2c(client, &hl7593_regmap_config);
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

	of_property_read_u32(np, "halo,vsel-step", &di->vsel_step);

	ret = hl7593_regulator_register(di, &config);
	if (ret < 0)
		dev_err(dev, "Failed to register regulator!\n");
	return ret;
}

static const struct of_device_id hl7593_dt_ids[] = {
	{
		.compatible = "halo,hl7593",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, hl7593_dt_ids);

static const struct i2c_device_id hl7593_id[] = {
	{ "hl7593", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, hl7593_id);

static struct i2c_driver hl7593_regulator_driver = {
	.driver = {
		.name = "hl7593-regulator",
		.of_match_table = of_match_ptr(hl7593_dt_ids),
	},
	.probe_new = hl7593_i2c_probe,
	.id_table = hl7593_id,
};
module_i2c_driver(hl7593_regulator_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("HL7593 regulator driver");
MODULE_LICENSE("GPL v2");
