// SPDX-License-Identifier: GPL-2.0
/*
 * Synaptics VS680 pinctrl driver
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "berlin.h"

static const struct berlin_desc_group vs680_soc_pinctrl_groups[] = {
	BERLIN_PINCTRLCONF_GROUP("SDIO_CDn", 0x0, 0x3, 0x00,
			0x0, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* CDn */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO45 */
			BERLIN_PINCTRL_FUNCTION(0x2, "tw1a")), /* SCL */
	BERLIN_PINCTRLCONF_GROUP("SDIO_WP", 0x0, 0x3, 0x03,
			0x4, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* WP */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO44 */
			BERLIN_PINCTRL_FUNCTION(0x2, "tw1a")), /* SDA */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS0n", 0x0, 0x3, 0x06,
			0x8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SS0n */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO54 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS1n", 0x0, 0x3, 0x09,
			0xc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO53 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS1n */
			BERLIN_PINCTRL_FUNCTION(0x2, "sts7"), /* VALD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG14 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS2n", 0x0, 0x3, 0x0c,
			0x10, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO52 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS2n */
			BERLIN_PINCTRL_FUNCTION(0x2, "sts7"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "tw1b"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG12 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS3n", 0x0, 0x3, 0x0f,
			0x14, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO51 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS3n */
			BERLIN_PINCTRL_FUNCTION(0x2, "sts7"), /* SD */
			BERLIN_PINCTRL_FUNCTION(0x3, "tw1b"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG13 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SDO", 0x0, 0x3, 0x12,
			0x18, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO50 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SCLK", 0x0, 0x3, 0x15,
			0x1c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SCLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO49 */
			BERLIN_PINCTRL_FUNCTION(0x7, "dbg")), /* CLK */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SDI", 0x0, 0x3, 0x18,
			0x20, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO48 */
	BERLIN_PINCTRLCONF_GROUP("TW0_SCL", 0x0, 0x3, 0x1b,
			0x24, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO47 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw0"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG10 */
	BERLIN_PINCTRLCONF_GROUP("TW0_SDA", 0x4, 0x3, 0x00,
			0x28, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO46 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw0"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG11 */
	BERLIN_PINCTRLCONF_GROUP("STS0_CLK", 0x4, 0x3, 0x03,
			0x2c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO43 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts0"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x2, "cpupll"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x4, "uart3"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG0 */
	BERLIN_PINCTRLCONF_GROUP("STS0_SOP", 0x4, 0x3, 0x06,
			0x30, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO42 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts0"), /* SOP */
			BERLIN_PINCTRL_FUNCTION(0x2, "syspll"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts5"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x4, "uart3"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG1 */
	BERLIN_PINCTRLCONF_GROUP("STS0_SD", 0x4, 0x3, 0x09,
			0x34, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO41 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts0"), /* SD */
			BERLIN_PINCTRL_FUNCTION(0x2, "mempll"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x4, "uart3"), /* CTSn */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG2 */
	BERLIN_PINCTRLCONF_GROUP("STS0_VALD", 0x4, 0x3, 0x0c,
			0x38, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO40 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts0"), /* VALD */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts5"), /* SD */
			BERLIN_PINCTRL_FUNCTION(0x4, "uart3"), /* RTSn */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG3 */
	BERLIN_PINCTRLCONF_GROUP("STS1_CLK", 0x4, 0x3, 0x0f,
			0x3c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO39 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts1"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* pwm0 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG4 */
	BERLIN_PINCTRLCONF_GROUP("STS1_SOP", 0x4, 0x3, 0x12,
			0x40, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO38 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts1"), /* SOP */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts6"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG5 */
	BERLIN_PINCTRLCONF_GROUP("STS1_SD", 0x4, 0x3, 0x15,
			0x44, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO37 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts1"), /* SD */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM2 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG6 */
	BERLIN_PINCTRLCONF_GROUP("STS1_VALD", 0x4, 0x3, 0x18,
			0x48, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO36 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sts1"), /* VALD */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM3 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts6"), /* SD */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG7 */
	BERLIN_PINCTRLCONF_GROUP("USB2_DRV_VBUS", 0x4, 0x3, 0x1b,
			0x4c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "usb2"), /* VBUS */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO55 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_MDC", 0x8, 0x3, 0x00,
			0x50, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* MDC */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO29 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG8 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_MDIO", 0x8, 0x3, 0x03,
			0x54, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* MDIO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO28 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG9 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_TXC", 0x8, 0x3, 0x06,
			0x58, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* TXC */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO23 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_TXD0", 0x8, 0x3, 0x09,
			0x5c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* TXD0 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO27 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_TXD1", 0x8, 0x3, 0x0c,
			0x60, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* TXD1 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO26 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG15 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_TXD2", 0x8, 0x3, 0x0f,
			0x64, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* TXD2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO25 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_TXD3", 0x8, 0x3, 0x12,
			0x68, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* TXD3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO24 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_TXCTL", 0x8, 0x3, 0x15,
			0x6c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* TXCTL */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO22 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_RXC", 0x8, 0x3, 0x18,
			0x70, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* RXC */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO31 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_RXD0", 0x8, 0x3, 0x1b,
			0x74, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* RXD0 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO35 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_RXD1", 0xc, 0x3, 0x00,
			0x78, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* RXD1 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO34 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_RXD2", 0xc, 0x3, 0x03,
			0x7c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* RXD2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO33 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_RXD3", 0xc, 0x3, 0x06,
			0x80, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* RXD3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO32 */
	BERLIN_PINCTRLCONF_GROUP("RGMII_RXCTL", 0xc, 0x3, 0x09,
			0x84, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rgmii"), /* RXCTL */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO30 */
};

static const struct berlin_desc_group vs680_avio_pinctrl_groups[] = {
	BERLIN_PINCTRLCONF_GROUP("I2S1_DO0", 0x0, 0x3, 0x00,
			0x0, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO19 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* DO0 */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG4 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_DO1", 0x0, 0x3, 0x03,
			0x4, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO17 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* DO1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts2"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG5 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_DO2", 0x0, 0x3, 0x06,
			0x8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO16 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* DO2 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM2 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts2"), /* SD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pdm"), /* DI2 */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG6 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_DO3", 0x0, 0x3, 0x09,
			0xc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO15 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* DO3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM3 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts3"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x4, "pdm"), /* DI3 */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG7 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_LRCKIO", 0x0, 0x3, 0x0c,
			0x10, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO21 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* LRCKIO */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "arc_test"), /* OUT */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG0 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_BCLKIO", 0x0, 0x3, 0x0f,
			0x14, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO20 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* BCLKIO */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG1 */
	BERLIN_PINCTRLCONF_GROUP("SPDIFO", 0x0, 0x3, 0x12,
			0x18, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO14 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spdifo"),
			BERLIN_PINCTRL_FUNCTION(0x4, "avpll")), /* CLKO */
	BERLIN_PINCTRLCONF_GROUP("SPDIFI", 0x0, 0x3, 0x15,
			0x1c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO4 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spdifi"),
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm")), /* DI */
	BERLIN_PINCTRLCONF_GROUP("I2S2_LRCKIO", 0x0, 0x3, 0x18,
			0x20, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO13 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2")), /* LRCKIO */
	BERLIN_PINCTRLCONF_GROUP("I2S2_BCLKIO", 0x0, 0x3, 0x1b,
			0x24, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO12 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* BCLKIO */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm")), /* CLKIO */
	BERLIN_PINCTRLCONF_GROUP("I2S2_DI0", 0x4, 0x3, 0x00,
			0x28, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO11 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* DI0 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm")), /* DI3 */
	BERLIN_PINCTRLCONF_GROUP("I2S2_DI1", 0x4, 0x3, 0x03,
			0x2c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO10 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* DI2 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts3")), /* SD */
	BERLIN_PINCTRLCONF_GROUP("I2S2_DI2", 0x4, 0x3, 0x06,
			0x30, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO9 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* DI2 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts4")), /* CLK */
	BERLIN_PINCTRLCONF_GROUP("I2S2_DI3", 0x4, 0x3, 0x09,
			0x34, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO8 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* DI3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* DI0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts4")), /* SD */
	BERLIN_PINCTRLCONF_GROUP("I2S1_MCLK", 0x4, 0x3, 0x0c,
			0x38, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO18 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* MCLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts2"), /* SOP */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG3 */
	BERLIN_PINCTRLCONF_GROUP("I2S2_MCLK", 0x4, 0x3, 0x0f,
			0x3c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO7 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* MCLK */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* CLKIO */
			BERLIN_PINCTRL_FUNCTION(0x4, "hdmi")), /* FBCLK */
	BERLIN_PINCTRLCONF_GROUP("TX_EDDC_SCL", 0x4, 0x3, 0x12,
			0x40, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO6 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tx_eddc")), /* SCL */
	BERLIN_PINCTRLCONF_GROUP("TX_EDDC_SDA", 0x4, 0x3, 0x15,
			0x44, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO5 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tx_eddc")), /* SDA */
	BERLIN_PINCTRLCONF_GROUP("I2S3_DO", 0x4, 0x3, 0x18,
			0x48, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO1 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3"), /* DO */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts3"), /* SOP */
			BERLIN_PINCTRL_FUNCTION(0x7, "avio")), /* DBG2 */
	BERLIN_PINCTRLCONF_GROUP("I2S3_LRCKIO", 0x4, 0x3, 0x1b,
			0x4c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3"), /* LRCKIO */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts3")), /* CLK */
	BERLIN_PINCTRLCONF_GROUP("I2S3_BCLKIO", 0x8, 0x3, 0x00,
			0x50, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3"), /* BCLKIO */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts3")), /* SD */
	BERLIN_PINCTRLCONF_GROUP("I2S3_DI", 0x8, 0x3, 0x03,
			0x54, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO0 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3"), /* DI */
			BERLIN_PINCTRL_FUNCTION(0x3, "sts3")), /* VALD */
};

static const struct berlin_desc_group vs680_sysmgr_pinctrl_groups[] = {
	BERLIN_PINCTRLCONF_GROUP("SM_TW2_SCL", 0x0, 0x3, 0x00,
			0x0, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rx_edid"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw2a"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* SM GPIO0 */
	BERLIN_PINCTRLCONF_GROUP("SM_TW2_SDA", 0x0, 0x3, 0x03,
			0x4, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "rx_edid"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw2a"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* SM GPIO1 */
	BERLIN_PINCTRLCONF_GROUP("SM_URT1_TXD", 0x0, 0x3, 0x06,
			0x8, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "porb"), /* VOUT 1p05 */
			BERLIN_PINCTRL_FUNCTION(0x1, "uart1"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* SM GPIO4 */
			BERLIN_PINCTRL_FUNCTION(0x3, "pwm"), /* PWM2 */
			BERLIN_PINCTRL_FUNCTION(0x4, "timer"), /* TIMER0 */
			BERLIN_PINCTRL_FUNCTION(0x5, "porb"), /* AVDD LV */
			BERLIN_PINCTRL_FUNCTION(0x6, "tw2b")), /* SCL */
	BERLIN_PINCTRLCONF_GROUP("SM_URT1_RXD", 0x0, 0x3, 0x09,
			0xc, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* SM GPIO5 */
			BERLIN_PINCTRL_FUNCTION(0x1, "uart1"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x2, "clk_25m"),
			BERLIN_PINCTRL_FUNCTION(0x3, "pwm"), /* PWM3 */
			BERLIN_PINCTRL_FUNCTION(0x4, "timer"), /* TIMER1 */
			BERLIN_PINCTRL_FUNCTION(0x5, "por"), /* VDDSOC_RSTB */
			BERLIN_PINCTRL_FUNCTION(0x6, "tw2b")), /* SDA */
	BERLIN_PINCTRLCONF_GROUP("SM_HDMI_HPD", 0x0, 0x3, 0x0c,
			0x10, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* SM GPIO2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "hdmi")), /* HPD */
	BERLIN_PINCTRLCONF_GROUP("SM_HDMI_CEC", 0x0, 0x3, 0x0f,
			0x14, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* SM GPIO3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "hdmi")), /* CEC */
	BERLIN_PINCTRLCONF_GROUP("SM_TMS", 0x0, 0x3, 0x12,
			0x18, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "jtag"), /* TMS */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO6 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm")), /* PWM0 */
	BERLIN_PINCTRLCONF_GROUP("SM_TDI", 0x0, 0x3, 0x15,
			0x1c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "jtag"), /* TDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO7 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm")), /* PWM1 */
	BERLIN_PINCTRLCONF_GROUP("SM_TDO", 0x0, 0x3, 0x18,
			0x20, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "jtag"), /* TDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* SM GPIO8 */
	BERLIN_PINCTRLCONF_GROUP("SM_TW3_SCL", 0x0, 0x3, 0x1b,
			0x24, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* SM GPIO9 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw3"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm")), /* PWM2 */
	BERLIN_PINCTRLCONF_GROUP("SM_TW3_SDA", 0x4, 0x3, 0x00,
			0x28, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* SM GPIO10 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw3"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm")), /* PWM3 */
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SS0n", 0x4, 0x3, 0x03,
			0x2c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SS0 n*/
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO17 */
			BERLIN_PINCTRL_FUNCTION(0x7, "porb")), /* AVDD33_LV */
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SS1n", 0x4, 0x3, 0x06,
			0x30, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* SM GPIO16 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi2"), /* SS1n */
			BERLIN_PINCTRL_FUNCTION(0x6, "uart1"), /* RTSn */
			BERLIN_PINCTRL_FUNCTION(0x7, "vdd")), /* CPU PORTB */
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SS2n", 0x4, 0x3, 0x09,
			0x34, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "mon"), /* VDD 1P8 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi2"), /* SS2n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* SM GPIO15 */
			BERLIN_PINCTRL_FUNCTION(0x3, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x4, "timer"), /* TIMER0 */
			BERLIN_PINCTRL_FUNCTION(0x5, "uart2"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x7, "clk_25m")),
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SS3n", 0x4, 0x3, 0x0c,
			0x38, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "pwr_ok"),
			BERLIN_PINCTRL_FUNCTION(0x1, "spi2"), /* SS3n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* SM GPIO14 */
			BERLIN_PINCTRL_FUNCTION(0x3, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x4, "timer"), /* TIMER1 */
			BERLIN_PINCTRL_FUNCTION(0x5, "uart2"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x7, "uart1")), /* CTSn */
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SDO", 0x4, 0x3, 0x0f,
			0x3c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO13 */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart2")), /* RTSn */
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SDI", 0x4, 0x3, 0x12,
			0x40, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO12 */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart2")), /* CTSn */
	BERLIN_PINCTRLCONF_GROUP("SM_SPI2_SCLK", 0x4, 0x3, 0x15,
			0x44, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SCLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* SM GPIO11 */
	BERLIN_PINCTRLCONF_GROUP("SM_URT0_TXD", 0x4, 0x3, 0x18,
			0x48, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "uart0"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* SM GPIO19 */
	BERLIN_PINCTRLCONF_GROUP("SM_URT0_RXD", 0x4, 0x3, 0x1b,
			0x4c, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "uart0"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* SM GPIO18 */
	BERLIN_PINCTRLCONF_GROUP("SM_HDMIRX_HPD", 0x8, 0x3, 0x00,
			0x50, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmirx"), /* HPD */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* SM GPIO20 */
	BERLIN_PINCTRLCONF_GROUP("SM_HDMIRX_PWR5V", 0x8, 0x3, 0x03,
			0x54, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmirx"), /* PWR5V */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* SM GPIO21 */
};

static const struct berlin_pinctrl_desc vs680_soc_pinctrl_data = {
	.groups = vs680_soc_pinctrl_groups,
	.ngroups = ARRAY_SIZE(vs680_soc_pinctrl_groups),
};

static const struct berlin_pinctrl_desc vs680_avio_pinctrl_data = {
	.groups = vs680_avio_pinctrl_groups,
	.ngroups = ARRAY_SIZE(vs680_avio_pinctrl_groups),
};

static const struct berlin_pinctrl_desc vs680_sysmgr_pinctrl_data = {
	.groups = vs680_sysmgr_pinctrl_groups,
	.ngroups = ARRAY_SIZE(vs680_sysmgr_pinctrl_groups),
};

static const struct of_device_id vs680_pinctrl_match[] = {
	{
		.compatible = "syna,vs680-soc-pinctrl",
		.data = &vs680_soc_pinctrl_data,
	},
	{
		.compatible = "syna,vs680-avio-pinctrl",
		.data = &vs680_avio_pinctrl_data,
	},
	{
		.compatible = "syna,vs680-system-pinctrl",
		.data = &vs680_sysmgr_pinctrl_data,
	},
	{}
};

static int vs680_pinctrl_probe(struct platform_device *pdev)
{
	const struct of_device_id *match =
		of_match_device(vs680_pinctrl_match, &pdev->dev);
	struct regmap_config *rmconfig;
	struct regmap *regmap, *conf;
	struct resource *res;
	void __iomem *base;

	rmconfig = devm_kzalloc(&pdev->dev, sizeof(*rmconfig), GFP_KERNEL);
	if (!rmconfig)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	rmconfig->reg_bits = 32,
	rmconfig->val_bits = 32,
	rmconfig->reg_stride = 4,
	rmconfig->max_register = resource_size(res);

	regmap = devm_regmap_init_mmio(&pdev->dev, base, rmconfig);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	rmconfig->name = "conf";
	rmconfig->max_register = resource_size(res);

	conf = devm_regmap_init_mmio(&pdev->dev, base, rmconfig);
	if (IS_ERR(conf))
		return PTR_ERR(conf);

	return berlin_pinctrl_probe_regmap(pdev, match->data, regmap, conf);
}

static const struct dev_pm_ops vs680_pinctrl_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(berlin_pinctrl_suspend, berlin_pinctrl_resume)
};

static struct platform_driver vs680_pinctrl_driver = {
	.probe	= vs680_pinctrl_probe,
	.driver	= {
		.name = "vs680-pinctrl",
		.pm = &vs680_pinctrl_pm_ops,
		.of_match_table = vs680_pinctrl_match,
	},
};

static int __init vs680_pinctrl_init(void)
{
	return platform_driver_register(&vs680_pinctrl_driver);
}
arch_initcall(vs680_pinctrl_init);
