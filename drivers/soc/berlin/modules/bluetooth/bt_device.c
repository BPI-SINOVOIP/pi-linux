/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/rfkill.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/random.h>

#include <linux/interrupt.h>
#include <linux/pm_wakeup.h>
#include <linux/pm_wakeirq.h>
#include <linux/irq.h>

#include <linux/input.h>

#define BT_RFKILL "bt_rfkill"

#define BT_DBG 0
#if BT_DBG
#define BT_INFO(fmt, args...)   \
        pr_info("[%s] " fmt, __func__, ##args)
#else
#define BT_INFO(fmt, args...)
#endif

#define BT_ERR(fmt, args...)    \
        pr_info("[%s] " fmt, __func__, ##args)


struct bt_dev_data {
	int gpio_reset;
	int gpio_en;
	int gpio_hostwake;
	int gpio_btwakeup;
	int power_low_level;
	int power_on_pin_OD;
	int power_off_flag;
	int power_down_disable;
	int irqno_wakeup;
	struct work_struct btwakeup_work;
	struct input_dev *input_dev;
	struct hrtimer timer;
};

struct bt_dev_runtime_data {
        struct rfkill *bt_rfk;
        struct bt_dev_data *pdata;
};

char bt_addr[18] = "";
static int btwake_evt;
static int btirq_flag;
static int btpower_evt;


static void bt_device_off(struct bt_dev_data *pdata)
{
	if (pdata->power_down_disable == 0) {
		if ((btpower_evt == 1) && (pdata->gpio_reset > 0)) {
			if ((pdata->power_on_pin_OD)
				&& (pdata->power_low_level)) {
				gpio_direction_input(pdata->gpio_reset);
			} else {
				gpio_direction_output(pdata->gpio_reset,
					pdata->power_low_level);
			}
		}
		if ((btpower_evt == 1) && (pdata->gpio_en > 0)) {
			if ((pdata->power_on_pin_OD)
				&& (pdata->power_low_level)) {
				gpio_direction_input(pdata->gpio_en);
			} else {
				gpio_direction_output(pdata->gpio_en,
					pdata->power_low_level);
			}
		}

		if ((btpower_evt == 0) && (pdata->gpio_reset > 0)) {
			if ((pdata->power_on_pin_OD)
				&& (pdata->power_low_level)) {
				gpio_direction_input(pdata->gpio_reset);
			} else {
				gpio_direction_output(pdata->gpio_reset,
					pdata->power_low_level);
			}
		}
		if ((btpower_evt == 0) && (pdata->gpio_en > 0)) {
			if ((pdata->power_on_pin_OD)
				&& (pdata->power_low_level)) {
				gpio_direction_input(pdata->gpio_en);
			}
		}
		msleep(20);
	}
}


static void bt_device_init(struct bt_dev_data *pdata)
{
	int tmp = 0;
	btpower_evt = 0;
	btirq_flag = 0;

	if (pdata->gpio_reset > 0)
		gpio_request(pdata->gpio_reset, BT_RFKILL);

	if (pdata->gpio_en > 0)
		gpio_request(pdata->gpio_en, BT_RFKILL);

	if (pdata->gpio_hostwake > 0) {
		gpio_request(pdata->gpio_hostwake, BT_RFKILL);
		gpio_direction_output(pdata->gpio_hostwake, 1);
	}

	if (pdata->gpio_btwakeup > 0) {
		gpio_request(pdata->gpio_btwakeup, BT_RFKILL);
		gpio_direction_input(pdata->gpio_btwakeup);
	}

	tmp = pdata->power_down_disable;
	pdata->power_down_disable = 0;
	bt_device_off(pdata);
	pdata->power_down_disable = tmp;

}

static void bt_device_deinit(struct bt_dev_data *pdata)
{
	if (pdata->gpio_reset > 0)
		gpio_free(pdata->gpio_reset);
	if (pdata->gpio_en > 0)
		gpio_free(pdata->gpio_en);

	btpower_evt = 0;
	if (pdata->gpio_hostwake > 0)
		gpio_free(pdata->gpio_hostwake);

}

static void bt_device_on(struct bt_dev_data *pdata)
{
	if ((btpower_evt == 1) && (pdata->gpio_reset > 0)) {
		if ((pdata->power_on_pin_OD)
			&& (!pdata->power_low_level)) {
			gpio_direction_input(pdata->gpio_reset);
		} else {
			gpio_direction_output(pdata->gpio_reset,
				!pdata->power_low_level);
		}
	}

	if ((btpower_evt == 1) && (pdata->gpio_en > 0)) {
		if ((pdata->power_on_pin_OD)
			&& (!pdata->power_low_level)) {
			gpio_direction_input(pdata->gpio_en);
		} else {
			gpio_direction_output(pdata->gpio_en,
				!pdata->power_low_level);
		}
	}

	if ((btpower_evt == 0) && (pdata->gpio_reset > 0)) {
		if ((pdata->power_on_pin_OD)
			&& (!pdata->power_low_level)) {
			gpio_direction_input(pdata->gpio_reset);
		} else {
			gpio_direction_output(pdata->gpio_reset,
				!pdata->power_low_level);
		}
	}

	if ((btpower_evt == 0) && (pdata->gpio_en > 0)) {
		if ((pdata->power_on_pin_OD)
			&& (!pdata->power_low_level)) {
			gpio_direction_input(pdata->gpio_en);
		}
	}

	msleep(200);
}

static int bt_set_block(void *data, bool blocked)
{
	struct bt_dev_data *pdata = data;

	BT_INFO("BT_RADIO going: %s\n", blocked ? "off" : "on");

	if (!blocked) {
		BT_INFO("SYNA_BT: going ON,btpower_evt=%d %d\n", btpower_evt,pdata->power_down_disable);
		bt_device_on(pdata);
	} else {
		BT_INFO("SYNA_BT: going OFF,btpower_evt=%d %d\n", btpower_evt,pdata->power_down_disable);
		bt_device_off(pdata);
	}
	return 0;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_set_block,
};

static int bt_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct bt_dev_data *pdata = platform_get_drvdata(pdev);

	btwake_evt = 0;
	BT_INFO("bt suspend\n");
	disable_irq(pdata->irqno_wakeup);

	return 0;
}

static int bt_resume(struct platform_device *pdev)
{
	struct bt_dev_data *pdata = platform_get_drvdata(pdev);

	BT_INFO("bt resume\n");
	enable_irq(pdata->irqno_wakeup);
	btwake_evt = 0;
	return 0;
}

static int bt_probe(struct platform_device *pdev)
{
	int ret = 0;
	const void *prop;
	struct rfkill *bt_rfk;
	struct bt_dev_data *pdata = NULL;
	struct bt_dev_runtime_data *prdata;

#ifdef CONFIG_OF
	if (pdev && pdev->dev.of_node) {
		const char *str;
		struct gpio_desc *desc;

		BT_INFO("enter bt_probe of_node\n");
		pdata = kzalloc(sizeof(struct bt_dev_data), GFP_KERNEL);
		ret = of_property_read_string(pdev->dev.of_node,
			"gpio_reset", &str);
		if (ret) {
			pr_warn("not get gpio_reset\n");
			pdata->gpio_reset = 0;
		} else {
			desc = gpiod_get_from_of_node(pdev->dev.of_node,
				"gpio_reset", 0, GPIOD_ASIS, NULL);
			pdata->gpio_reset = desc_to_gpio(desc);
		}

		ret = of_property_read_string(pdev->dev.of_node,
			"gpio_en", &str);
		if (ret) {
			pr_warn("not get gpio_en\n");
			pdata->gpio_en = 0;
		} else {
			desc = gpiod_get_from_of_node(pdev->dev.of_node,
				"gpio_en", 0, GPIOD_ASIS, NULL);
			pdata->gpio_en = desc_to_gpio(desc);
		}

		ret = of_property_read_string(pdev->dev.of_node,
			"gpio_hostwake", &str);
		if (ret) {
			pr_warn("not get gpio_hostwake\n");
			pdata->gpio_hostwake = 0;
		} else {
			desc = gpiod_get_from_of_node(pdev->dev.of_node,
				"gpio_hostwake", 0, GPIOD_ASIS, NULL);
			pdata->gpio_hostwake = desc_to_gpio(desc);
		}

		/*gpio_btwakeup = BT_WAKE_HOST*/
		ret = of_property_read_string(pdev->dev.of_node,
			"gpio_btwakeup", &str);
		if (ret) {
			pr_warn("not get gpio_btwakeup\n");
			pdata->gpio_btwakeup = 0;
		} else {
			desc = gpiod_get_from_of_node(pdev->dev.of_node,
				"gpio_btwakeup", 0, GPIOD_ASIS, NULL);
			pdata->gpio_btwakeup = desc_to_gpio(desc);
		}

		prop = of_get_property(pdev->dev.of_node,
			"power_low_level", NULL);
		if (prop) {
			BT_INFO("power on valid level is low");
			pdata->power_low_level = 1;
		} else {
			BT_INFO("power on valid level is high");
			pdata->power_low_level = 0;
			pdata->power_on_pin_OD = 0;
		}

		ret = of_property_read_u32(pdev->dev.of_node,
			"power_on_pin_OD", &pdata->power_on_pin_OD);
		if (ret)
			pdata->power_on_pin_OD = 0;
		BT_INFO("bt: power_on_pin_OD = %d;\n", pdata->power_on_pin_OD);

		ret = of_property_read_u32(pdev->dev.of_node,
				"power_off_flag", &pdata->power_off_flag);
		if (ret)
			pdata->power_off_flag = 1;/*bt poweroff*/
		BT_INFO("bt: power_off_flag = %d;\n", pdata->power_off_flag);

		ret = of_property_read_u32(pdev->dev.of_node,
			"power_down_disable", &pdata->power_down_disable);
		if (ret)
			pdata->power_down_disable = 0;
		BT_INFO("dis power down = %d;\n", pdata->power_down_disable);

	} else if (pdev) {
		pdata = (struct bt_dev_data *)(pdev->dev.platform_data);
	} else {
		ret = -ENOENT;
		goto err_res;
	}
#else
	pdata = (struct bt_dev_data *)(pdev->dev.platform_data);
#endif

	bt_device_init(pdata);
	if (pdata->power_down_disable == 1) {
		pdata->power_down_disable = 0;
		bt_device_on(pdata);
		pdata->power_down_disable = 1;
	}

	/* default to bluetooth off */
	/* rfkill_switch_all(RFKILL_TYPE_BLUETOOTH, 1); */
	/* bt_device_off(pdata); */

	bt_rfk = rfkill_alloc("bt-dev", &pdev->dev,
		RFKILL_TYPE_BLUETOOTH,
		&bt_rfkill_ops, pdata);

	if (!bt_rfk) {
		BT_ERR("rfk alloc fail\n");
		ret = -ENOMEM;
		goto err_rfk_alloc;
	}

	rfkill_init_sw_state(bt_rfk, false);
	ret = rfkill_register(bt_rfk);
	if (ret) {
		pr_err("rfkill_register fail\n");
		goto err_rfkill;
	}
	prdata = kmalloc(sizeof(struct bt_dev_runtime_data),
	GFP_KERNEL);

	if (!prdata)
		goto err_rfkill;

	prdata->bt_rfk = bt_rfk;
	prdata->pdata = pdata;
	platform_set_drvdata(pdev, prdata);
	return 0;

err_rfkill:
	rfkill_destroy(bt_rfk);
err_rfk_alloc:
	bt_device_deinit(pdata);
err_res:
	return ret;

}

static int bt_remove(struct platform_device *pdev)
{
	struct bt_dev_runtime_data *prdata =
		platform_get_drvdata(pdev);
	struct rfkill *rfk = NULL;
	struct bt_dev_data *pdata = NULL;

	platform_set_drvdata(pdev, NULL);

	if (prdata) {
		rfk = prdata->bt_rfk;
		pdata = prdata->pdata;
	}

	if (pdata) {
		bt_device_deinit(pdata);
		kfree(pdata);
	}

	if (rfk) {
		rfkill_unregister(rfk);
		rfkill_destroy(rfk);
	}
	rfk = NULL;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id bt_dev_dt_match[] = {
	{.compatible = "syna,bt-dev",},
	{},
};
#else
#define bt_dev_dt_match NULL
#endif

static struct platform_driver bt_driver = {
	.driver		= {
		.name	= "bt-dev",
		.of_match_table = bt_dev_dt_match,
	},
	.probe		= bt_probe,
	.remove		= bt_remove,
	.suspend	= bt_suspend,
	.resume		= bt_resume,
};

static int __init bt_init(void)
{
	return platform_driver_register(&bt_driver);
}
static void __exit bt_exit(void)
{
	platform_driver_unregister(&bt_driver);
}

module_param(btpower_evt, int, 0664);
MODULE_PARM_DESC(btpower_evt, "btpower_evt");

module_param(btwake_evt, int, 0664);
MODULE_PARM_DESC(btwake_evt, "btwake_evt");
module_init(bt_init);
module_exit(bt_exit);
MODULE_DESCRIPTION("bt rfkill");
MODULE_AUTHOR("");
MODULE_LICENSE("GPL");
