/*
 *  Copyright (C) 2022 SeeVision Limited.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

/**
 * DOC: ET6416 I2C to GPIO expander.
 *
 * This chip has has 16 GPIO lines out of it, and is controlled by an
 * I2C bus (a pair of lines).
 *
 * Any preconfigured state will be left in place until the GPIO lines
 * get activated.  At power on, everything is treated as an input.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>

#define ET6416_INPUT          			(0x00)
#define ET6416_OUTPUT         			(0x02)
#define ET6416_INVERT         			(0x04)
#define ET6416_DIRECTION      			(0x06)

#define ET6416_PIN_NUM					(16)
#define ET6416_NO_SUSPEND_RESUME		BIT(0)

struct et6416_chip {
	struct gpio_chip gpio_chip;
	struct i2c_client *client;
	struct mutex i2c_lock;
	struct mutex irq_lock;
	u32 flags;
	u16 def_state;

	/* Caches of register values so we don't have to read-modify-write. */
	u16 reg_io_dir; // 0:Output 1:input Hz
	u16 reg_output;

	int irqnum;
	u16 pinmask;
};

static int et6416_write_reg(struct et6416_chip *chip, int reg, u16 val)
{
	int error;

	error =	i2c_smbus_write_word_data(chip->client, reg, val);
	if (error < 0) {
		dev_err(&chip->client->dev,
			"%s failed, reg: %d, val: %d, error: %d\n",
			__func__, reg, val, error);
		return error;
	}

	return 0;
}

static int et6416_read_reg(struct et6416_chip *chip, int reg, u16 *val)
{
	int retval;

	retval = i2c_smbus_read_word_data(chip->client, reg);
	if (retval < 0) {
		dev_err(&chip->client->dev, "%s failed, reg: %d, error: %d\n",
			__func__, reg, retval);
		return retval;
	}

	*val = (u16)retval;
	return 0;
}

static int et6416_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct et6416_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);
	chip->reg_io_dir |= BIT(off);

	et6416_write_reg(chip,ET6416_DIRECTION,chip->reg_io_dir);

	mutex_unlock(&chip->i2c_lock);

	return 0;
}

static int et6416_gpio_direction_output(struct gpio_chip *gc,
		unsigned off, int val)
{
	struct et6416_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);

	if (val)
		chip->reg_output |= BIT(off);
	else
		chip->reg_output &= ~BIT(off);

	et6416_write_reg(chip,ET6416_OUTPUT,chip->reg_output);

	chip->reg_io_dir &= ~BIT(off);
	et6416_write_reg(chip,ET6416_DIRECTION,chip->reg_io_dir);

	mutex_unlock(&chip->i2c_lock);

	return 0;
}

static int et6416_gpio_get_direction(struct gpio_chip *gc, unsigned off)
{
	struct et6416_chip *chip = gpiochip_get_data(gc);

	return (chip->reg_io_dir & BIT(off)) != 0;
}

static int et6416_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct et6416_chip *chip = gpiochip_get_data(gc);
	u16 reg = 0;

	mutex_lock(&chip->i2c_lock);

	et6416_read_reg(chip, ET6416_INPUT,&reg);

	mutex_unlock(&chip->i2c_lock);

	return (reg & BIT(off)) != 0;
}

static void et6416_gpio_set_value(struct gpio_chip *gc, unsigned off, int val)
{
	struct et6416_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);

	if (val)
		chip->reg_output |= BIT(off);
	else
		chip->reg_output &= ~BIT(off);

	et6416_write_reg(chip, ET6416_OUTPUT, chip->reg_output);

	mutex_unlock(&chip->i2c_lock);
}

/*
 * This is threaded IRQ handler and this can (and will) sleep.
 */
static irqreturn_t et6416_chip_input_isr(int irq, void *dev_id)
{
	struct et6416_chip *chip = dev_id;

	dev_info(&chip->client->dev, "ET6416 input isr ...\n");

	return IRQ_HANDLED;
}

static void et6416_gpio_set_multiple(struct gpio_chip *gc,
		unsigned long *mask, unsigned long *bits)
{
	struct et6416_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);

	chip->reg_output = (chip->reg_output & ~mask[0]) | bits[0];

	et6416_write_reg(chip, ET6416_OUTPUT, chip->reg_output);

	mutex_unlock(&chip->i2c_lock);
}

static int et6416_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct et6416_chip *chip;
	struct gpio_chip *gc;
	int ret;
	u16 val = 0;

	chip = devm_kzalloc(dev, sizeof(struct et6416_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	if (device_property_read_bool(dev, "no-suspend-resume"))
		chip->flags |= ET6416_NO_SUSPEND_RESUME;

	chip->client = client;
	mutex_init(&chip->i2c_lock);
	ret = et6416_read_reg(chip, ET6416_DIRECTION,&val);
	chip->reg_io_dir = val;
	ret = et6416_read_reg(chip, ET6416_OUTPUT,&val);
	chip->reg_output = val;
	if(ret != 0){
		dev_err(dev, "et6416 not found !\n");
		//goto exit_00;
	}

	gc = &chip->gpio_chip;
	gc->direction_input  = et6416_gpio_direction_input;
	gc->direction_output = et6416_gpio_direction_output;
	gc->get_direction = et6416_gpio_get_direction;
	gc->get = et6416_gpio_get_value;
	gc->set = et6416_gpio_set_value;
	gc->set_multiple = et6416_gpio_set_multiple;
	gc->can_sleep = true;

	gc->base =-1;
	gc->ngpio = ET6416_PIN_NUM;
	gc->label = chip->client->name;
	gc->parent = dev;
	gc->owner = THIS_MODULE;

	if (client->irq != 0x00) {
		chip->irqnum = gpio_to_irq(client->irq);

		ret = request_threaded_irq(chip->irqnum, NULL,
					     et6416_chip_input_isr,
					     IRQ_TYPE_EDGE_BOTH,
					     "et6416_input_isr", chip);
		if (ret) {
			dev_err(&client->dev,
				"Unable to claim irq %d; error %d\n",
				chip->irqnum, ret);
			goto exit_00;
		}
		disable_irq(chip->irqnum);
	}

	ret = gpiochip_add_data(gc, chip);
	if(ret != 0){
		dev_err(dev, "et6416 gpiochip add failed!\n");
		goto exit_00;
	}

	dev_err(dev, "label = %s base = %d .\n",gc->label,gc->base);

	i2c_set_clientdata(client, chip);

	if (client->irq != 0x00)
		enable_irq(chip->irqnum);
	return 0;
exit_00:
	if(chip != NULL)
		devm_kfree(dev, chip);
	return ret;
}

static int et6416_remove(struct i2c_client *client)
{
	struct et6416_chip *chip = i2c_get_clientdata(client);

	gpiochip_remove(&chip->gpio_chip);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int et6416_gpio_suspend(struct device *dev)
{
	return 0;
}

static int et6416_gpio_resume(struct device *dev)
{
	struct et6416_chip *chip;

	chip = dev_get_drvdata(dev);

	if (chip->flags & ET6416_NO_SUSPEND_RESUME)
		return 0;

	mutex_lock(&chip->i2c_lock);

	et6416_write_reg(chip, ET6416_OUTPUT,
				  chip->reg_output);
	et6416_write_reg(chip, ET6416_DIRECTION,
				  chip->reg_io_dir);
	mutex_unlock(&chip->i2c_lock);

	return 0;
}
#else
#define et6416_gpio_suspend	NULL
#define et6416_gpio_resume	NULL
#endif

static const struct dev_pm_ops et6416_gpio_pm_ops = {
	.suspend_noirq = et6416_gpio_suspend,
	.resume_noirq = et6416_gpio_resume,
};

static const struct of_device_id et6416_dt_ids[] = {
	{ .compatible = "etek,et6416" },
	{ }
};

MODULE_DEVICE_TABLE(of, et6416_dt_ids);

static const struct i2c_device_id et6416_id[] = {
	{ "et6416", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, et6416_id);

static struct i2c_driver et6416_driver = {
	.driver = {
		.name	= "et6416",
		.pm	= &et6416_gpio_pm_ops,
		.of_match_table = et6416_dt_ids,
	},
	.probe		= et6416_probe,
	.remove		= et6416_remove,
	.id_table	= et6416_id,
};

module_i2c_driver(et6416_driver);

MODULE_AUTHOR("Chwei <chenwei@seevision.cn>");
MODULE_DESCRIPTION("GPIO expander driver for ET6416");
MODULE_LICENSE("GPL");

