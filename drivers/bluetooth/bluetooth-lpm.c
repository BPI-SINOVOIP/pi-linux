/* Copyright (C) 2020 Synaptics.                                       */
/* This software is licensed under the terms of the GNU General Public */
/* License version 2, as published by the Free Software Foundation, and*/
/* may be copied, distributed, and modified under those terms.         */
/*                                                                     */
/* This program is distributed in the hope that it will be useful,     */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the        */
/* GNU General Public License for more details.                        */
/*bt-host-wake-gpio is connected into SM_GPIO[6]                       */
/*which is handled in bootloader                                       */
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <linux/irq.h>
#include <linux/ioport.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/serial_core.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "hci_uart.h"

#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>

#undef  BT_DBG
#undef BT_ERR
#define BT_DBG(fmt, arg...) pr_debug(fmt " [BT]\n", ##arg)
#define BT_ERR(fmt, arg...) pr_err(fmt " [BT]\n", ##arg)

#define VERSION	 "1.1"
#define PROC_DIR	"bluetooth/sleep"

#define POLARITY_LOW 0
#define POLARITY_HIGH 1

struct bluesleep_info {
	unsigned int host_wake;
	int ext_wake;
	unsigned int host_wake_irq;
	struct wakeup_source *p_wake_lock;
	int irq_polarity;
	int has_ext_wake;
};

static const struct of_device_id bt_bluesleep_table[] = {
	{	.compatible = "syna,bluesleep" },
	{}
};

#define BT_PROTO	 0x01
#define BT_TXDATA	 0x02
#define BT_ASLEEP	 0x04
#define BT_EXT_WAKE	 0x08
#define BT_SUSPEND	 0x10

static bool bt_enabled;

static struct bluesleep_info *bsi;

static unsigned long flags;

struct proc_dir_entry *bluetooth_dir, *sleep_dir;


static ssize_t bluesleep_read_proc_lpm(struct file *file,
	char __user *userbuf, size_t bytes, loff_t *off)
{
	int ret;

	ret = copy_to_user(userbuf,
		bt_enabled?"lpm: 1\n":"lpm: 0\n", sizeof("lpm: 0\n"));
	if (ret) {
		BT_ERR("Failed to %s : %d", __func__, ret);
		return ret;
	}

	return sizeof("lpm: 0\n");
}

static ssize_t bluesleep_write_proc_lpm(struct file *file,
	const char __user *buffer, size_t count, loff_t *pos)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	if (b == '0') {
		BT_DBG("(%s) Unreg HCI notifier.", __func__);
		bt_enabled = false;
		return count;
	} else if (b == '1') {
		BT_DBG("(%s) Reg HCI notifier.", __func__);
		if (!bt_enabled) {
			bt_enabled = true;
			return count;
		}
	} else if (b == '2') {
		BT_DBG("(%s) don`t control ext_wake & uart clk", __func__);
		if (bt_enabled) {
			bt_enabled = false;
			return count;
		}
	}

	BT_ERR("(%s) Reg HCI notifier, unknown state", __func__);

	return -EINVAL;
}

static ssize_t bluesleep_read_proc_btwrite(struct file *file,
				char __user *userbuf, size_t bytes, loff_t *off)
{
	int ret;
	int bt_dev_wake_value = 0;

	if (bsi->has_ext_wake == 1) {
		if (test_bit(BT_EXT_WAKE, &flags)) {
			BT_DBG("BT_EXT_WAKE is set!");
			bt_dev_wake_value = 1;
		} else {
			BT_DBG("BT_EXT_WAKE is zero!");
			bt_dev_wake_value = 0;
		}
	}

	ret = copy_to_user(userbuf, bt_dev_wake_value
		? "bt_dev_wake_value: 1\n":"bt_dev_wake_value: 0\n",
		sizeof("bt_dev_wake_value: 0\n"));
	if (ret) {
		BT_ERR("Failed to %s : %d", __func__, ret);
		return ret;
	}

	return sizeof("bt_dev_wake_value: 0\n");
}

static ssize_t bluesleep_write_proc_btwrite(struct file *file,
	const char __user *buffer, size_t count, loff_t *pos)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	return count;
}

static const struct file_operations proc_fops_lpm = {
	.owner = THIS_MODULE,
	.read = bluesleep_read_proc_lpm,
	.write = bluesleep_write_proc_lpm,
};
static const struct file_operations proc_fops_btwrite = {
	.owner = THIS_MODULE,
	.read = bluesleep_read_proc_btwrite,
	.write = bluesleep_write_proc_btwrite,
};

static int bluesleep_make_node(void)
{
	struct proc_dir_entry *ent;

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		return -EINVAL;
	}

	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
		goto fail_mk_sleep;
	}

	ent = proc_create("lpm", 0660, sleep_dir, &proc_fops_lpm);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/lpm entry", PROC_DIR);
		goto fail_mk_lpm;
	}

	ent = proc_create("btwrite", 0660, sleep_dir, &proc_fops_btwrite);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_DIR);
		goto fail_mk_btwrite;
	}

	return 0;

fail_mk_btwrite:
	remove_proc_entry("lpm", sleep_dir);
fail_mk_lpm:
	remove_proc_entry("sleep", bluetooth_dir);
fail_mk_sleep:
	remove_proc_entry("bluetooth", 0);
	return -EINVAL;
}

static void bluesleep_rm_node(void)
{
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

static int bluesleep_probe(struct platform_device *pdev)
{
	int ret, gpio_ext_wake;

	BT_DBG("bluesleep probe!\n");

	gpio_ext_wake = of_get_named_gpio(pdev->dev.of_node,
		"bt-dev-wake-gpio", 0);
	if (gpio_ext_wake < 0)
		return gpio_ext_wake;

	bsi = devm_kzalloc(&pdev->dev, sizeof(struct bluesleep_info), GFP_KERNEL);
	if (!bsi) {
		BT_ERR("failed to allocate memory to bsi\n");
		return -ENOMEM;
	}

	bsi->ext_wake = gpio_ext_wake;
	BT_DBG("[BT] bt device wake gpio is %d", bsi->ext_wake);

	ret = gpio_request(bsi->ext_wake, "bt_ext_wake");
	if (ret) {
		BT_ERR("%s gpio_request for bt_wake is failed, ret = %d",
			__func__, ret);
		return ret;
	}
	bsi->has_ext_wake = 1;

	ret = gpio_direction_output(bsi->ext_wake, 1);
	if (ret) {
		BT_ERR("%s set input for bt_wake is failed, ret = %d",
			__func__, ret);
		gpio_free(bsi->ext_wake);
		return ret;
	}

	ret = bluesleep_make_node();
	if (ret) {
		gpio_free(bsi->ext_wake);
		BT_ERR("%s proc node create failed, ret = %d",
			__func__, ret);
		return ret;
	}

	set_bit(BT_EXT_WAKE, &flags);
	BT_DBG("[BT] Bluesleep probe success");
	return 0;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	bluesleep_rm_node();
	gpio_free(bsi->ext_wake);
	return 0;
}


static int bluesleep_resume(struct platform_device *pdev)
{
	if (test_bit(BT_SUSPEND, &flags))
		clear_bit(BT_SUSPEND, &flags);

	return 0;
}

static int bluesleep_suspend(struct platform_device *pdev, pm_message_t state)
{
	set_bit(BT_SUSPEND, &flags);
	return 0;
}

static struct platform_driver bluesleep_driver = {
	.probe = bluesleep_probe,
	.remove = bluesleep_remove,
	.suspend = bluesleep_suspend,
	.resume = bluesleep_resume,
	.driver = {
		.name = "bluetooth-lpm",
		.owner = THIS_MODULE,
		.of_match_table = bt_bluesleep_table,
	},
};


/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	BT_DBG("BlueSleep Mode Driver Ver %s", VERSION);

	bt_enabled = false;
	flags = 0; /* clear all status bits */

	return platform_driver_register(&bluesleep_driver);
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{
	if (bsi) {
		/* assert bt wake */
		if (bsi->has_ext_wake == 1)
			gpio_direction_output(bsi->ext_wake, 1);

		set_bit(BT_EXT_WAKE, &flags);
	}

	platform_driver_unregister(&bluesleep_driver);

	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Low Power Mode Driver ver %s " VERSION);
MODULE_LICENSE("GPL v2");
