/*
 *  Copyright (C) 2016 Broadcom Limited.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

/**
 * DOC: FXL6408 I2C to GPIO expander.
 *
 * This chip has has 8 GPIO lines out of it, and is controlled by an
 * I2C bus (a pair of lines), providing 4x expansion of GPIO lines.
 * It also provides an interrupt line out for notifying of
 * statechanges.
 *
 * Any preconfigured state will be left in place until the GPIO lines
 * get activated.  At power on, everything is treated as an input.
 *
 * Documentation can be found at:
 * https://www.fairchildsemi.com/datasheets/FX/FXL6408.pdf
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>

#define FXL6408_DEVICE_ID		0x01
#define FXL6408_RST_INT			BIT(1)
#define FXL6408_SW_RST			BIT(0)

/* Bits set here indicate that the GPIO is an output. */
#define FXL6408_IO_DIR			0x03
/* Bits set here, when the corresponding bit of IO_DIR is set, drive
 * the output high instead of low.
 */
#define FXL6408_OUTPUT			0x05
/* Bits here make the output High-Z, instead of the OUTPUT value. */
#define FXL6408_OUTPUT_HIGH_Z		0x07
/* Bits here define the expected input state of the GPIO.
 * INTERRUPT_STAT bits will be set when the INPUT transitions away
 * from this value.
 */
#define FXL6408_INPUT_DEFAULT_STATE	0x09
/* Bits here enable either pull up or pull down according to
 * FXL6408_PULL_DOWN.
 */
#define FXL6408_PULL_ENABLE		0x0b
/* Bits set here (when the corresponding PULL_ENABLE is set) enable a
 * pull-up instead of a pull-down.
 */
#define FXL6408_PULL_UP			0x0d
/* Returns the current status (1 = HIGH) of the input pins. */
#define FXL6408_INPUT_STATUS		0x0f
/* Mask of pins which can generate interrupts. */
#define FXL6408_INTERRUPT_MASK		0x11
/* Mask of pins which have generated an interrupt.  Cleared on read. */
#define FXL6408_INTERRUPT_STAT		0x13

#define FXL6408_PIN_NUM			8
#define FXL_NO_SUSPEND_RESUME		BIT(0)

struct fxl6408_chip {
	struct gpio_chip gpio_chip;
	struct i2c_client *client;
	struct mutex i2c_lock;
	struct mutex irq_lock;
	u32 flags;
	u8 irq_mask;
	u8 irq_stat;
	u8 irq_trig_raise;
	u8 irq_trig_fall;
	u8 def_state;

	/* Caches of register values so we don't have to read-modify-write. */
	u8 reg_io_dir;
	u8 reg_output;
	u8 shutdown_mask;
	u8 shutdown_val;
};

static int fxl6408_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);
	chip->reg_io_dir &= ~BIT(off);
	i2c_smbus_write_byte_data(chip->client, FXL6408_IO_DIR,
				  chip->reg_io_dir);
	mutex_unlock(&chip->i2c_lock);

	return 0;
}

static int fxl6408_gpio_direction_output(struct gpio_chip *gc,
		unsigned off, int val)
{
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);

	if (val)
		chip->reg_output |= BIT(off);
	else
		chip->reg_output &= ~BIT(off);
	i2c_smbus_write_byte_data(chip->client, FXL6408_OUTPUT,
				  chip->reg_output);

	chip->reg_io_dir |= BIT(off);
	i2c_smbus_write_byte_data(chip->client, FXL6408_IO_DIR,
				  chip->reg_io_dir);
	mutex_unlock(&chip->i2c_lock);

	return 0;
}

static int fxl6408_gpio_get_direction(struct gpio_chip *gc, unsigned off)
{
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	return (chip->reg_io_dir & BIT(off)) == 0;
}

static int fxl6408_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct fxl6408_chip *chip = gpiochip_get_data(gc);
	u8 reg;

	mutex_lock(&chip->i2c_lock);
	reg = i2c_smbus_read_byte_data(chip->client, FXL6408_INPUT_STATUS);
	mutex_unlock(&chip->i2c_lock);

	return (reg & BIT(off)) != 0;
}

static void fxl6408_gpio_set_value(struct gpio_chip *gc, unsigned off, int val)
{
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);

	if (val)
		chip->reg_output |= BIT(off);
	else
		chip->reg_output &= ~BIT(off);

	i2c_smbus_write_byte_data(chip->client, FXL6408_OUTPUT,
				  chip->reg_output);
	mutex_unlock(&chip->i2c_lock);
}


static void fxl6408_gpio_set_multiple(struct gpio_chip *gc,
		unsigned long *mask, unsigned long *bits)
{
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->i2c_lock);
	chip->reg_output = (chip->reg_output & ~mask[0]) | bits[0];
	i2c_smbus_write_byte_data(chip->client, FXL6408_OUTPUT,
				  chip->reg_output);
	mutex_unlock(&chip->i2c_lock);
}

static void fxl6408_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	if (d->hwirq >= FXL6408_PIN_NUM)
		return;

	chip->irq_mask &= ~(1 << d->hwirq);
}

static void fxl6408_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	if (d->hwirq >= FXL6408_PIN_NUM)
		return;

	chip->irq_mask |= 1 << d->hwirq;
}

static int fxl6408_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fxl6408_chip *chip = gpiochip_get_data(gc);
	u8 mask;

	if (d->hwirq >= FXL6408_PIN_NUM)
		return -EINVAL;

	if (!(type & IRQ_TYPE_EDGE_BOTH)) {
		dev_err(&chip->client->dev, "irq %d: unsupported type %d\n",
			d->irq, type);
		return -EINVAL;
	}

	mask = 1 << d->hwirq;

	if (type & IRQ_TYPE_EDGE_FALLING)
		chip->irq_trig_fall |= mask;
	else
		chip->irq_trig_fall &= ~mask;

	if (type & IRQ_TYPE_EDGE_RISING)
		chip->irq_trig_raise |= mask;
	else
		chip->irq_trig_raise &= ~mask;

	return 0;
}

static void fxl6408_irq_bus_lock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fxl6408_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->irq_lock);
}

static void fxl6408_irq_bus_sync_unlock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fxl6408_chip *chip = gpiochip_get_data(gc);
	u8 def_state, both;

	/* Unmask enabled interrupts */
	i2c_smbus_write_byte_data(chip->client, FXL6408_INTERRUPT_MASK,
				  ~chip->irq_mask);

	/* set the interrupt polarity*/
	def_state = chip->irq_trig_fall;
	both = chip->irq_trig_fall & chip->irq_trig_raise;
	if (both) {
		u8 input;
		input = i2c_smbus_read_byte_data(chip->client,
						FXL6408_INPUT_STATUS);
		def_state &= ~both;
		def_state |= input & both;
	}
	i2c_smbus_write_byte_data(chip->client, FXL6408_INPUT_DEFAULT_STATE,
				  def_state);
	chip->def_state = def_state;

	mutex_unlock(&chip->irq_lock);
}

static struct irq_chip fxl6408_irq_chip = {
	.name			= "fxl6408",
	.irq_mask		= fxl6408_irq_mask,
	.irq_unmask		= fxl6408_irq_unmask,
	.irq_bus_lock		= fxl6408_irq_bus_lock,
	.irq_bus_sync_unlock	= fxl6408_irq_bus_sync_unlock,
	.irq_set_type		= fxl6408_irq_set_type,
};

static u8 fxl6408_irq_pending(struct fxl6408_chip *chip)
{
	uint8_t status;

	status = i2c_smbus_read_byte_data(chip->client, FXL6408_INTERRUPT_STAT);
	return status & chip->irq_mask;
}

static irqreturn_t fxl6408_irq_handler(int irq, void *devid)
{
	struct fxl6408_chip *chip = devid;
	u8 pending, both, input, bit;
	int pos, def_update = 0;
	unsigned nhandled = 0;

	pending = fxl6408_irq_pending(chip);

	if (!pending)
		return IRQ_NONE;

	both = chip->irq_trig_fall & chip->irq_trig_raise;
	if (both) {
		input = i2c_smbus_read_byte_data(chip->client,
						FXL6408_INPUT_STATUS);
	}

	while (pending) {
		pos = __ffs(pending);
		bit = (1 << pos);
		handle_nested_irq(irq_find_mapping(chip->gpio_chip.irq.domain,
						pos));
		pending &= ~bit;
		if (both & bit) {
			if (input & bit) {
				if (!(chip->def_state & bit)) {
					chip->def_state |= bit;
					def_update = 1;
				}
			} else if (chip->def_state & bit) {
				chip->def_state &= ~bit;
				def_update = 1;
			}
		}
		nhandled++;
	}

	if (def_update)
		i2c_smbus_write_byte_data(chip->client,
					FXL6408_INPUT_DEFAULT_STATE,
					chip->def_state);

	return (nhandled > 0) ? IRQ_HANDLED : IRQ_NONE;
}

static int fxl6408_irq_setup(struct fxl6408_chip *chip)
{
	struct i2c_client *client = chip->client;
	int ret;

	if (client->irq) {
		chip->irq_mask = 0;
		mutex_init(&chip->irq_lock);

		ret = devm_request_threaded_irq(&client->dev,
					   client->irq,
					   NULL,
					   fxl6408_irq_handler,
					   IRQF_TRIGGER_LOW | IRQF_ONESHOT |
						   IRQF_SHARED,
					   dev_name(&client->dev), chip);
		if (ret) {
			dev_err(&client->dev, "failed to request irq %d\n",
				client->irq);
			return ret;
		}

		ret =  gpiochip_irqchip_add_nested(&chip->gpio_chip,
						   &fxl6408_irq_chip,
						   0,
						   handle_simple_irq,
						   IRQ_TYPE_NONE);
		if (ret) {
			dev_err(&client->dev,
				"could not connect irqchip to gpiochip\n");
			return ret;
		}

		gpiochip_set_nested_irqchip(&chip->gpio_chip,
					    &fxl6408_irq_chip,
					    client->irq);
		dev_info(&client->dev, "%s done with irq %d.\n", __FUNCTION__,
					client->irq);
	}

	return 0;
}

static int fxl6408_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct fxl6408_chip *chip;
	struct gpio_chip *gc;
	int ret;
	u8 device_id;

	/* Check the device ID register to see if it's responding.
	 * This also clears RST_INT as a side effect, so we won't get
	 * the "we've been power cycled" interrupt once we enable
	 * interrupts.
	 */
	device_id = i2c_smbus_read_byte_data(client, FXL6408_DEVICE_ID);
	if (device_id < 0) {
		dev_err(dev, "FXL6408 probe returned %d\n", device_id);
		return device_id;
	} else if (device_id >> 5 != 5) {
		dev_err(dev, "FXL6408 probe returned DID: 0x%02x\n", device_id);
		return -ENODEV;
	}

	/* Disable High-Z of outputs, so that our OUTPUT updates
	 * actually take effect.
	 */
	i2c_smbus_write_byte_data(client, FXL6408_OUTPUT_HIGH_Z, 0);

	chip = devm_kzalloc(dev, sizeof(struct fxl6408_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	if (device_property_read_bool(dev, "no-suspend-resume"))
		chip->flags |= FXL_NO_SUSPEND_RESUME;

	if (!device_property_read_u8(dev, "shutdown-mask", &chip->shutdown_mask))
		device_property_read_u8(dev, "shutdown-value", &chip->shutdown_val);

	chip->client = client;
	mutex_init(&chip->i2c_lock);
	chip->reg_io_dir = i2c_smbus_read_byte_data(client, FXL6408_IO_DIR);
	chip->reg_output = i2c_smbus_read_byte_data(client, FXL6408_OUTPUT);

	gc = &chip->gpio_chip;
	gc->direction_input  = fxl6408_gpio_direction_input;
	gc->direction_output = fxl6408_gpio_direction_output;
	gc->get_direction = fxl6408_gpio_get_direction;
	gc->get = fxl6408_gpio_get_value;
	gc->set = fxl6408_gpio_set_value;
	gc->set_multiple = fxl6408_gpio_set_multiple;
	gc->can_sleep = true;

	gc->base =-1;
	gc->ngpio = 8;
	gc->label = chip->client->name;
	gc->parent = dev;
	gc->owner = THIS_MODULE;

	ret = gpiochip_add_data(gc, chip);
	if (ret)
		return ret;

	ret = fxl6408_irq_setup(chip);
	if (ret)
		return ret;

	i2c_set_clientdata(client, chip);
	return 0;
}

static int fxl6408_remove(struct i2c_client *client)
{
	struct fxl6408_chip *chip = i2c_get_clientdata(client);

	gpiochip_remove(&chip->gpio_chip);

	return 0;
}

static void fxl6408_shutdown(struct i2c_client *client)
{
	struct fxl6408_chip *chip = i2c_get_clientdata(client);
	u8 val;

	if (chip->shutdown_mask) {
		val = chip->reg_output & ~chip->shutdown_mask;
		val |= chip->shutdown_mask & chip->shutdown_val;
		i2c_smbus_write_byte_data(chip->client, FXL6408_OUTPUT, val);
	}
}

#ifdef CONFIG_PM_SLEEP
static int fxl6408_gpio_suspend(struct device *dev)
{
	return 0;
}

static int fxl6408_gpio_resume(struct device *dev)
{
	struct fxl6408_chip *chip;

	chip = dev_get_drvdata(dev);

	if (chip->flags & FXL_NO_SUSPEND_RESUME)
		return 0;

	mutex_lock(&chip->i2c_lock);

	i2c_smbus_write_byte_data(chip->client, FXL6408_OUTPUT,
				  chip->reg_output);
	i2c_smbus_write_byte_data(chip->client, FXL6408_IO_DIR,
				  chip->reg_io_dir);
	i2c_smbus_write_byte_data(chip->client, FXL6408_INPUT_DEFAULT_STATE,
				  chip->def_state);
	i2c_smbus_write_byte_data(chip->client, FXL6408_INTERRUPT_MASK,
				  ~chip->irq_mask);
	i2c_smbus_write_byte_data(chip->client, FXL6408_OUTPUT_HIGH_Z, 0);

	mutex_unlock(&chip->i2c_lock);

	return 0;
}
#else
#define fxl6408_gpio_suspend	NULL
#define fxl6408_gpio_resume	NULL
#endif

static const struct dev_pm_ops fxl6408_gpio_pm_ops = {
	.suspend_noirq = fxl6408_gpio_suspend,
	.resume_noirq = fxl6408_gpio_resume,
};

static const struct of_device_id fxl6408_dt_ids[] = {
	{ .compatible = "fcs,fxl6408" },
	{ }
};

MODULE_DEVICE_TABLE(of, fxl6408_dt_ids);

static const struct i2c_device_id fxl6408_id[] = {
	{ "fxl6408", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, fxl6408_id);

static struct i2c_driver fxl6408_driver = {
	.driver = {
		.name	= "fxl6408",
		.pm	= &fxl6408_gpio_pm_ops,
		.of_match_table = fxl6408_dt_ids,
	},
	.probe		= fxl6408_probe,
	.remove		= fxl6408_remove,
	.shutdown	= fxl6408_shutdown,
	.id_table	= fxl6408_id,
};

module_i2c_driver(fxl6408_driver);

MODULE_AUTHOR("Eric Anholt <eric@anholt.net>");
MODULE_DESCRIPTION("GPIO expander driver for FXL6408");
MODULE_LICENSE("GPL");
