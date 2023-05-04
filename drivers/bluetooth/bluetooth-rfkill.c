/* Copyright (C) 2020 Synaptics.                                       */
/* This software is licensed under the terms of the GNU General Public */
/* License version 2, as published by the Free Software Foundation, and*/
/* may be copied, distributed, and modified under those terms.         */
/*                                                                     */
/* This program is distributed in the hope that it will be useful,     */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the        */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/slab.h>


#define BT_PWR_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n", __func__, ## arg)
#define BT_PWR_INFO(fmt, arg...) pr_info("%s: " fmt "\n", __func__, ## arg)
#define BT_PWR_ERR(fmt, arg...)  pr_err("%s: " fmt "\n", __func__, ## arg)

struct bluetooth_plat_data {
	int power_gpio;
	struct rfkill *rfkill;
};

static struct bluetooth_plat_data *get_dt_data(struct device *dev)
{
	struct bluetooth_plat_data *dt_pdata;
	int power_gpio;

	power_gpio = of_get_named_gpio(dev->of_node,
				"bt-power-gpio", 0);
	if (power_gpio < 0)
		return ERR_PTR(power_gpio);

	dt_pdata = devm_kzalloc(dev, sizeof(*dt_pdata), GFP_KERNEL);
	if (!dt_pdata)
		return ERR_PTR(-ENOMEM);

	dt_pdata->power_gpio = power_gpio;

	BT_PWR_INFO("[BT] bt power gpio is %d", dt_pdata->power_gpio);

	return dt_pdata;
}

static int bluetooth_set_power(void *data, bool blocked)
{
	struct bluetooth_plat_data *pdata = data;
	if (!blocked) {
		gpio_direction_output(pdata->power_gpio, 0);
		mdelay(10);
		BT_PWR_INFO("%s: power up = %d\n", __func__, blocked);
		gpio_direction_output(pdata->power_gpio, 1);
		mdelay(150);
	} else {
		gpio_direction_output(pdata->power_gpio, 0);
		BT_PWR_INFO("%s: power down = %d\n", __func__, blocked);
		mdelay(10);
	}

	BT_PWR_INFO("%s: onoff = %d\n", __func__, blocked);

	return 0;
}

static struct rfkill_ops rfkill_bluetooth_ops = {
	.set_block = bluetooth_set_power,
};

static int rfkill_bluetooth_probe(struct platform_device *pdev)
{
	int ret;
	bool default_state = true;
	struct bluetooth_plat_data *pdata;

	BT_PWR_INFO("%s\n", __func__);

	pdata = get_dt_data(&pdev->dev);
	if (IS_ERR(pdata))
		return PTR_ERR(pdata);

	ret = gpio_request(pdata->power_gpio, "bt_power");
	if (ret)
		return ret;

	ret = gpio_direction_output(pdata->power_gpio, 0);
	if (ret) {
		gpio_free(pdata->power_gpio);
		return ret;
	}

	pdata->rfkill = rfkill_alloc("bt_power", &pdev->dev,
				     RFKILL_TYPE_BLUETOOTH,
				     &rfkill_bluetooth_ops, pdata);

	rfkill_init_sw_state(pdata->rfkill, 0);

	ret = rfkill_register(pdata->rfkill);
	if (ret) {
		gpio_free(pdata->power_gpio);
		rfkill_destroy(pdata->rfkill);
		return ret;
	}

	platform_set_drvdata(pdev, pdata);
	bluetooth_set_power(pdata, default_state);

	return 0;
}

static int rfkill_bluetooth_remove(struct platform_device *pdev)
{
	struct bluetooth_plat_data *pdata = platform_get_drvdata(pdev);

	BT_PWR_INFO("%s\n", __func__);

	rfkill_unregister(pdata->rfkill);
	rfkill_destroy(pdata->rfkill);
	platform_set_drvdata(pdev, NULL);
	gpio_free(pdata->power_gpio);

	return 0;
}

static const struct of_device_id rfkill_of_match[] = {
	{.compatible = "syna,rfkill"},
	{ }
};
MODULE_DEVICE_TABLE(of, rfkill_of_match);

static struct platform_driver rfkill_bluetooth_driver = {
	.probe  = rfkill_bluetooth_probe,
	.remove = rfkill_bluetooth_remove,
	.driver = {
		.name = "bluetooth-rfkill",
		.of_match_table = rfkill_of_match,
	},
};
module_platform_driver(rfkill_bluetooth_driver);
MODULE_DESCRIPTION("synaptics bluetooth rfkill driver");
MODULE_LICENSE("GPL v2");
