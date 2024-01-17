/*
 * GPIO Init driver for the vs680 board
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

static int vs680_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gpio_descs *out_high_gpios = NULL;
	struct gpio_descs *out_low_gpios = NULL;
	int ret;
	// int i = 0;

	printk("vs680_gpio_probe: start...\n");
	out_high_gpios = devm_gpiod_get_array_optional(
		dev, "out-high", GPIOD_OUT_HIGH);

	if (IS_ERR(out_high_gpios)) {
		ret = PTR_ERR(out_high_gpios);
		printk("vs680_gpio_probe: no out high gpios\n");
	} else {
		// for(i = 0; i < out_high_gpios->ndescs; i++) {
		// 	gpiod_set_value(out_high_gpios->desc[i], 1);
		// }
		printk("vs680_gpio_probe: %d out high gpios value set\n",
			out_high_gpios->ndescs);
	}

	out_low_gpios = devm_gpiod_get_array_optional(
		dev, "out-low", GPIOD_OUT_LOW);

	if (IS_ERR(out_low_gpios)) {
		ret = PTR_ERR(out_low_gpios);
		printk("vs680_gpio_probe: no out low gpios\n");
	} else {
		// for(i = 0; i < out_low_gpios->ndescs; i++) {
		// 	gpiod_set_value(out_low_gpios->desc[i], 1);
		// }
		printk("vs680_gpio_probe: %d out low gpios value set\n",
			out_low_gpios->ndescs);
	}

	printk("vs680_gpio_probe: done\n");

	return 0;
}

static const struct of_device_id vs680_gpio_of_match[] = {
	{ .compatible = "syna,uboot_gpio", },
	{},
};
MODULE_DEVICE_TABLE(of, vs680_gpio_of_match);

static struct platform_driver vs680_gpio_driver = {
	.driver = {
		   .name = "vs680-gpio",
		   .of_match_table = vs680_gpio_of_match,
		   },
	.probe = vs680_gpio_probe,
};

module_platform_driver_probe(vs680_gpio_driver, vs680_gpio_probe);

MODULE_AUTHOR("Yingfeng Xiang <xiangyingfeng@senarytech.com>");
MODULE_DESCRIPTION("VS680 Uboot boot GPIO Init driver");
MODULE_LICENSE("GPL v2");
