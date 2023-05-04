/*
 * Platform Dependent file for Hikey
 *
 * Copyright (C) 2020, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id$
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/skbuff.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_WIFI_CONTROL_FUNC
#include <linux/wlan_plat.h>
#else
#include <dhd_plat.h>
#endif /* CONFIG_WIFI_CONTROL_FUNC */
#include <dhd_dbg.h>
#include <dhd.h>

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
extern int dhd_init_wlan_mem(void);
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

static int wlan_reg_on = -1;
#define DHD_DT_COMPAT_ENTRY		"android,bcmdhd_wlan"
#define WIFI_WL_REG_ON_PROPNAME		"wl_reg_on"

static int wlan_host_wake_up = -1;
static int wlan_host_wake_irq = 0;
#define WIFI_WLAN_HOST_WAKE_PROPNAME    "wl_host_wake"

int
dhd_wifi_init_gpio(void)
{
	int gpio_reg_on_val;
	int err;
	/* ========== WLAN_PWR_EN ============ */
	char *wlan_node = DHD_DT_COMPAT_ENTRY;
	struct device_node *root_node = NULL;

	root_node = of_find_compatible_node(NULL, NULL, wlan_node);
	if (NULL == root_node) {
		DHD_ERROR(("%s: fail to find bcmdhd_wlan device node!\n", __FUNCTION__));
		return 0;
	}

	wlan_reg_on = of_get_named_gpio(root_node, WIFI_WL_REG_ON_PROPNAME, 0);
	if (!gpio_is_valid(wlan_reg_on)) {
		DHD_ERROR(("%s: fail to get wlan_reg_on : %d\n", __FUNCTION__, wlan_reg_on));
	}
	wlan_host_wake_up = of_get_named_gpio(root_node, WIFI_WLAN_HOST_WAKE_PROPNAME, 0);
	if (!gpio_is_valid(wlan_host_wake_up)) {
		DHD_ERROR(("%s: fail to get wlan_host_wake_up : %d\n", __FUNCTION__, wlan_host_wake_up));
	}
	/* ========== WLAN_PWR_EN ============ */
	DHD_INFO(("%s: gpio_wlan_power : %d\n", __FUNCTION__, wlan_reg_on));

	/*
	 * For reg_on, gpio_request will fail if the gpio is configured to output-high
	 * in the dts using gpio-hog, so do not return error for failure.
	 */
	if (gpio_is_valid(wlan_reg_on)) {
		err = gpio_request_one(wlan_reg_on, GPIOF_OUT_INIT_HIGH, "WL_REG_ON");
		if (err) {
			DHD_ERROR(("%s: Failed to request gpio %d for WL_REG_ON, "
				"might have configured in the dts\n",
				__FUNCTION__, wlan_reg_on));
			return err;

		} else {
			DHD_ERROR(("%s: gpio_request WL_REG_ON done - WLAN_EN: GPIO %d\n",
				__FUNCTION__, wlan_reg_on));
		}

		gpio_reg_on_val = gpio_get_value_cansleep(wlan_reg_on);
		DHD_INFO(("%s: Initial WL_REG_ON: [%d]\n",
			__FUNCTION__, gpio_get_value_cansleep(wlan_reg_on)));

		if (gpio_reg_on_val == 0) {
			DHD_INFO(("%s: WL_REG_ON is LOW, drive it HIGH\n", __FUNCTION__));
			err = gpio_direction_output(wlan_reg_on, 1);
			if (err) {
				DHD_ERROR(("%s: WL_REG_ON is failed to pull up\n", __FUNCTION__));
				return err;
			}
		}
	}

	/* Wait for WIFI_TURNON_DELAY due to power stability */
	msleep(WIFI_TURNON_DELAY);
	if (gpio_is_valid(wlan_host_wake_up)) {
	/* ========== WLAN_HOST_WAKE ============ */
		DHD_INFO(("%s: gpio_wlan_host_wake : %d\n", __FUNCTION__, wlan_host_wake_up));

		err = gpio_request_one(wlan_host_wake_up, GPIOF_IN, "WLAN_HOST_WAKE");
		if (err) {
			DHD_ERROR(("%s: Failed to request gpio %d for WLAN_HOST_WAKE\n",
				__FUNCTION__, wlan_host_wake_up));
			return err;

		} else {
			DHD_ERROR(("%s: gpio_request WLAN_HOST_WAKE done"
				" - WLAN_HOST_WAKE: GPIO %d\n",
				__FUNCTION__, wlan_host_wake_up));
		}

		err = gpio_direction_input(wlan_host_wake_up);
		if (err) {
			DHD_ERROR(("%s: Failed to set WL_HOST_WAKE gpio direction\n", __FUNCTION__));
			return err;
		}
		wlan_host_wake_irq = gpio_to_irq(wlan_host_wake_up);
		if (wlan_host_wake_irq < 0) {
			DHD_ERROR(("%s: Failed to get wlan host wake irq!!\n", __FUNCTION__));
			return wlan_host_wake_irq;
		}
	}

	return 0;
}

int
dhd_wlan_power(int onoff)
{
	int err;
	DHD_INFO(("------------------------------------------------"));
	DHD_INFO(("------------------------------------------------\n"));
	DHD_INFO(("%s Enter: power %s\n", __func__, onoff ? "on" : "off"));

	if (!gpio_is_valid(wlan_reg_on)) {
		DHD_ERROR(("%s: wlan_reg_on no found!\n", __FUNCTION__));
		return 0;
	}

	if (onoff) {
		err = gpio_direction_output(wlan_reg_on, 1);
		if (err) {
			DHD_ERROR(("%s: WL_REG_ON is failed to pull up\n", __FUNCTION__));
			return err;
		}
		if (gpio_get_value_cansleep(wlan_reg_on)) {
			DHD_INFO(("WL_REG_ON on-step-2 : [%d]\n",
				gpio_get_value_cansleep(wlan_reg_on)));
		} else {
			DHD_ERROR(("[%s] gpio value is 0. We need reinit.\n", __func__));
			err = gpio_direction_output(wlan_reg_on, 1);
			if (err){
				DHD_ERROR(("%s: WL_REG_ON is "
					"failed to pull up\n", __func__));
				return err;
			}
		}

		/* Wait for WIFI_TURNON_DELAY due to power stability */
		msleep(WIFI_TURNON_DELAY);

	} else {
		/* Disable ASPM before powering off */
		err = gpio_direction_output(wlan_reg_on, 0);
		if (err) {
			DHD_ERROR(("%s: WL_REG_ON is failed to pull up\n", __FUNCTION__));
			return err;
		}
		if (gpio_get_value_cansleep(wlan_reg_on)) {
			DHD_INFO(("WL_REG_ON on-step-2 : [%d]\n",
				gpio_get_value_cansleep(wlan_reg_on)));
		}
	}

	return 0;
}
EXPORT_SYMBOL(dhd_wlan_power);

static int
dhd_wlan_reset(int onoff)
{
	return 0;
}

static int
dhd_wlan_set_carddetect(int val)
{
	struct pci_dev *pci_wifi_dev = NULL;
	struct pci_bus *b = NULL;

	DHD_ERROR(("%s: Entry: val=%d\n", __FUNCTION__, val));

#ifdef BCMPCIE
	if (val) {
		DHD_ERROR(("======== Card detection to detect PCIE WiFi card! ========\n"));
		pci_lock_rescan_remove();
		while ((b = pci_find_next_bus(b)) != NULL)
		{
			pci_rescan_bus(b);
		}
		pci_unlock_rescan_remove();
	} else {
		DHD_ERROR(("======== Card detection to remove PCIE WiFi card! ========\n"));
		pci_wifi_dev = pci_get_device(0x14e4, 0x4362, NULL);
		if(pci_wifi_dev != NULL)
		{
			pci_stop_and_remove_bus_device(pci_wifi_dev);
		}

		pci_wifi_dev = pci_get_device(0x16c3, 0xabcd, NULL);
		if(pci_wifi_dev != NULL)
		{
			pci_stop_and_remove_bus_device(pci_wifi_dev);
		}
	}
#endif /* BCMPCIE */

	return 0;
}

#ifdef BCMSDIO
static int dhd_wlan_get_wake_irq(void)
{
	return gpio_to_irq(wlan_host_wake_up);
}
#endif /* BCMSDIO */

#if defined(CONFIG_BCMDHD_OOB_HOST_WAKE) && defined(CONFIG_BCMDHD_GET_OOB_STATE)
int
dhd_get_wlan_oob_gpio(void)
{
	return gpio_is_valid(wlan_host_wake_up) ?
		gpio_get_value(wlan_host_wake_up) : -1;
}
EXPORT_SYMBOL(dhd_get_wlan_oob_gpio);
#endif /* CONFIG_BCMDHD_OOB_HOST_WAKE && CONFIG_BCMDHD_GET_OOB_STATE */

struct resource dhd_wlan_resources = {
	.name	= "bcmdhd_wlan_irq",
	.start	= 0, /* Dummy */
	.end	= 0, /* Dummy */
	.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE |
	IORESOURCE_IRQ_HIGHEDGE,
};
EXPORT_SYMBOL(dhd_wlan_resources);

struct wifi_platform_data dhd_wlan_control = {
	.set_power	= dhd_wlan_power,
	.set_reset	= dhd_wlan_reset,
	.set_carddetect	= dhd_wlan_set_carddetect,
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc	= dhd_wlan_mem_prealloc,
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */
#ifdef BCMSDIO
	.get_wake_irq   = dhd_wlan_get_wake_irq,
#endif
};
EXPORT_SYMBOL(dhd_wlan_control);

int
dhd_wlan_init(void)
{
	int ret;

	DHD_INFO(("%s: START.......\n", __FUNCTION__));
	ret = dhd_wifi_init_gpio();
	if (ret < 0) {
		DHD_ERROR(("%s: failed to initiate GPIO, ret=%d\n",
			__FUNCTION__, ret));
		goto fail;
	}

	dhd_wlan_resources.start = wlan_host_wake_irq;
	dhd_wlan_resources.end = wlan_host_wake_irq;

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	ret = dhd_init_wlan_mem();
	if (ret < 0) {
		DHD_ERROR(("%s: failed to alloc reserved memory,"
				" ret=%d\n", __FUNCTION__, ret));
	} else {
		DHD_ERROR(("%s: Allocate reserved memory successfully,"
				" ret=%d\n", __FUNCTION__, ret));
	}

#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

fail:
	DHD_INFO(("%s: FINISH.......\n", __FUNCTION__));
	return ret;
}

int
dhd_wlan_deinit(void)
{
	if (gpio_is_valid(wlan_host_wake_up))
		gpio_free(wlan_host_wake_up);

	if (gpio_is_valid(wlan_reg_on))
		gpio_free(wlan_reg_on);
	return 0;
}
#ifndef BCMDHD_MODULAR
/* Required only for Built-in DHD */
device_initcall(dhd_wlan_init);
#endif /* BOARD_HIKEY_MODULAR */
