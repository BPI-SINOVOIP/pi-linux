// SPDX-License-Identifier: GPL-2.0
/*
 * Synaptics AS470 pinctrl driver
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "berlin.h"

static const struct berlin_desc_group as470_soc_pinctrl_groups[] = {
	BERLIN_PINCTRLCONF_GROUP("TW2_SCL", 0x0, 0x3, 0x00,
			0x0, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO43 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw2")), /* SCL */
	BERLIN_PINCTRLCONF_GROUP("TW2_SDA", 0x0, 0x3, 0x03,
			0x4, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO42 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw2"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG15 */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SS0n", 0x0, 0x3, 0x06,
			0x8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SS0n */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO50 */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SS1n", 0x0, 0x3, 0x09,
			0xc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO49 */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi2"), /* SS1n */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG14 */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SS2n", 0x0, 0x3, 0x0c,
			0x10, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "por_n"), /* IO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO48 */
			BERLIN_PINCTRL_FUNCTION(0x2, "spi2"), /* SS2n */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG12 */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SS3n", 0x0, 0x3, 0x0f,
			0x14, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "pwr_ok"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO47 */
			BERLIN_PINCTRL_FUNCTION(0x2, "spi2"), /* SS3n */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG13 */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SDO", 0x0, 0x3, 0x12,
			0x18, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO46 */
			BERLIN_PINCTRL_FUNCTION(0x7, "mempll")), /* CLKO */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SCLK", 0x0, 0x3, 0x15,
			0x1c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SCLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO45 */
			BERLIN_PINCTRL_FUNCTION(0x7, "dbg")), /* CLK */
	BERLIN_PINCTRLCONF_GROUP("SPI2_SDI", 0x0, 0x3, 0x18,
			0x20, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi2"), /* SDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO44 */
			BERLIN_PINCTRL_FUNCTION(0x7, "cpupll")), /* CLKO */
	BERLIN_PINCTRLCONF_GROUP("TW3_SCL", 0x0, 0x3, 0x1b,
			0x24, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO39 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw3"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart1a"), /* RTSn */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG7 */
	BERLIN_PINCTRLCONF_GROUP("TW3_SDA", 0x4, 0x3, 0x00,
			0x28, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO38 */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw3"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart1a"), /* CTSn */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG6 */
	BERLIN_PINCTRLCONF_GROUP("URT1_RXD", 0x4, 0x3, 0x03,
			0x2c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO41 */
			BERLIN_PINCTRL_FUNCTION(0x1, "uart1"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG5 */
	BERLIN_PINCTRLCONF_GROUP("URT1_TXD", 0x4, 0x3, 0x06,
			0x30, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO40 */
			BERLIN_PINCTRL_FUNCTION(0x1, "uart1"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG4 */
	BERLIN_PINCTRLCONF_GROUP("PWM0", 0x4, 0x3, 0x09,
			0x34, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO34 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG3 */
	BERLIN_PINCTRLCONF_GROUP("PWM1", 0x4, 0x3, 0x0c,
			0x38, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO35 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG2 */
	BERLIN_PINCTRLCONF_GROUP("PWM2", 0x4, 0x3, 0x0f,
			0x3c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO36 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pwm"), /* PWM2 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG1 */
	BERLIN_PINCTRLCONF_GROUP("PWM3", 0x4, 0x3, 0x12,
			0x40, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO37 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pwm"), /* PWM3 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG0 */
	BERLIN_PINCTRLCONF_GROUP("GPIO_A0", 0x4, 0x3, 0x15,
			0x44, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO30 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG8 */
	BERLIN_PINCTRLCONF_GROUP("GPIO_A1", 0x4, 0x3, 0x18,
			0x48, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO31 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG9 */
	BERLIN_PINCTRLCONF_GROUP("GPIO_A2", 0x4, 0x3, 0x1b,
			0x4c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO32 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG10 */
	BERLIN_PINCTRLCONF_GROUP("GPIO_A3", 0x8, 0x3, 0x00,
			0x50, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO33 */
			BERLIN_PINCTRL_FUNCTION(0x7, "phy")), /* DBG11 */
	BERLIN_PINCTRLCONF_GROUP("SDIO_CDn", 0x8, 0x3, 0x03,
			0x58, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* CDn */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO64 */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart1b")), /* RTSn */
	BERLIN_PINCTRLCONF_GROUP("SDIO_WP", 0x8, 0x3, 0x06,
			0x5c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* WP */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO63 */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart1b")), /* CTSn */
	BERLIN_PINCTRLCONF_GROUP("SDIO_DATA0", 0x8, 0x3, 0x09,
			0x60, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* DATA0 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO65 */
	BERLIN_PINCTRLCONF_GROUP("SDIO_DATA1", 0x8, 0x3, 0x0c,
			0x64, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* DATA1 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO66 */
	BERLIN_PINCTRLCONF_GROUP("SDIO_DATA2", 0x8, 0x3, 0x0f,
			0x68, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* DATA2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO67 */
	BERLIN_PINCTRLCONF_GROUP("SDIO_DATA3", 0x8, 0x3, 0x12,
			0x6c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* DATA3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO68 */
	BERLIN_PINCTRLCONF_GROUP("SDIO_CLK", 0x8, 0x3, 0x15,
			0x70, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO70 */
	BERLIN_PINCTRLCONF_GROUP("SDIO_CMD", 0x8, 0x3, 0x18,
			0x74, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "sdio"), /* CMD */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO69 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_DO0", 0x8, 0x3, 0x1b,
			0x78, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO27 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* DO0 */
			BERLIN_PINCTRL_FUNCTION(0x7, "aio")), /* DBG4 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_DO1", 0xc, 0x3, 0x00,
			0x7c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO25 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* DO1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "apll0"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x7, "aio")), /* DBG5 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_LRCK", 0xc, 0x3, 0x03,
			0x80, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO29 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* LRCK */
			BERLIN_PINCTRL_FUNCTION(0x7, "aio")), /* DBG0 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_BCLK", 0xc, 0x3, 0x06,
			0x84, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO28 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* BCLK */
			BERLIN_PINCTRL_FUNCTION(0x7, "aio")), /* DBG1 */
	BERLIN_PINCTRLCONF_GROUP("I2S1_MCLK", 0xc, 0x3, 0x09,
			0x88, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO26 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s1"), /* MCLK */
			BERLIN_PINCTRL_FUNCTION(0x7, "aio")), /* DBG3 */
	BERLIN_PINCTRLCONF_GROUP("I2S2_LRCK", 0xc, 0x3, 0x0c,
			0x8c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO24 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* LRCK */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart0b")), /* CTSn */
	BERLIN_PINCTRLCONF_GROUP("I2S2_BCLK", 0xc, 0x3, 0x0f,
			0x90, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO23 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* BCLK */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart0b")), /* RTSn */
	BERLIN_PINCTRLCONF_GROUP("I2S2_DI0", 0xc, 0x3, 0x12,
			0x94, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO22 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2")), /* DI0 */
	BERLIN_PINCTRLCONF_GROUP("I2S2_DI1", 0xc, 0x3, 0x15,
			0x98, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO21 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s2"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s1")), /* DO2 */
	BERLIN_PINCTRLCONF_GROUP("PDM_CLKIO", 0xc, 0x3, 0x18,
			0x9c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO18 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pdm"), /* CLKIO */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s2")), /* MCLK */
	BERLIN_PINCTRLCONF_GROUP("PDM_DI0", 0xc, 0x3, 0x1b,
			0xa0, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO19 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pdm")), /* DI0 */
	BERLIN_PINCTRLCONF_GROUP("PDM_DI1", 0x10, 0x3, 0x00,
			0xa4, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO20 */
			BERLIN_PINCTRL_FUNCTION(0x1, "pdm"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s1")), /* DO3 */
	BERLIN_PINCTRLCONF_GROUP("I2S3_DO", 0x10, 0x3, 0x03,
			0xa8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO15 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3"), /* DO */
			BERLIN_PINCTRL_FUNCTION(0x3, "apll1"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x7, "aio")), /* DBG2 */
	BERLIN_PINCTRLCONF_GROUP("I2S3_LRCK", 0x10, 0x3, 0x06,
			0xac, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO17 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3")), /* LRCK */
	BERLIN_PINCTRLCONF_GROUP("I2S3_BCLK", 0x10, 0x3, 0x09,
			0xb0, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO16 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3")), /* BCLK */
	BERLIN_PINCTRLCONF_GROUP("I2S3_DI", 0x10, 0x3, 0x0c,
			0xb4, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO14 */
			BERLIN_PINCTRL_FUNCTION(0x1, "i2s3")), /* DI */
	BERLIN_PINCTRLCONF_GROUP("USB2_DRV_VBUS", 0x10, 0x3, 0x0f,
			0xb8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "usb2"), /* DRV VBUS */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO51 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_RSTn", 0x10, 0x3, 0x12,
			0xbc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* RSTn */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO53 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA0", 0x10, 0x3, 0x15,
			0xc0, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA0 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO62 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA1", 0x10, 0x3, 0x18,
			0xc4, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA1 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO61 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA2", 0x10, 0x3, 0x1b,
			0xc8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO60 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA3", 0x14, 0x3, 0x00,
			0xcc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO59 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA4", 0x14, 0x3, 0x03,
			0xd0, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA3 */
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA4 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO58 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA5", 0x14, 0x3, 0x06,
			0xd4, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA5 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO57 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA6", 0x14, 0x3, 0x09,
			0xd8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA6 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO56 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_DATA7", 0x14, 0x3, 0x0c,
			0xdc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA7 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO55 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_CLK", 0x14, 0x3, 0x0f,
			0xe0, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO54 */
	BERLIN_PINCTRLCONF_GROUP("EMMC_CMD", 0x14, 0x3, 0x12,
			0xe4, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* CMD */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO52 */
	BERLIN_PINCTRLCONF_GROUP("TW0_SCL", 0x14, 0x3, 0x15,
			0xe8, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "tw0"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO6 */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart0a")), /* CTSn */
	BERLIN_PINCTRLCONF_GROUP("TW0_SDA", 0x14, 0x3, 0x18,
			0xec, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "tw0"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO5 */
			BERLIN_PINCTRL_FUNCTION(0x2, "uart0a")), /* RTSn */
	BERLIN_PINCTRLCONF_GROUP("TW1_SCL", 0x14, 0x3, 0x1b,
			0xf0, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "tw1"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO4 */
			BERLIN_PINCTRL_FUNCTION(0x5, "por_n")), /* IO3P3 */
	BERLIN_PINCTRLCONF_GROUP("TW1_SDA", 0x18, 0x3, 0x00,
			0xf4, 0x4, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "tw1"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "clk_25m"),
			BERLIN_PINCTRL_FUNCTION(0x5, "por")), /* VDDSOC_RSTB */
	BERLIN_PINCTRLCONF_GROUP("TMS", 0x18, 0x3, 0x03,
			0xf8, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "jtag"), /* TMS */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO2 */
	BERLIN_PINCTRLCONF_GROUP("TDI", 0x18, 0x3, 0x06,
			0xfc, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "jtag"), /* TDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO1 */
	BERLIN_PINCTRLCONF_GROUP("TDO", 0x18, 0x3, 0x09,
			0x100, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "jtag"), /* TDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO0 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS0n", 0x18, 0x3, 0x0c,
			0x104, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SS0n */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO13 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS1n", 0x18, 0x3, 0x0f,
			0x108, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "por_n"), /* CORE */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS1n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* GPIO12 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS2n", 0x18, 0x3, 0x12,
			0x10c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "uart0"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS2n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* GPIO11 */
			BERLIN_PINCTRL_FUNCTION(0x7, "clk_25m")),
	BERLIN_PINCTRLCONF_GROUP("SPI1_SS3n", 0x18, 0x3, 0x15,
			0x110, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "uart0"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS3n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* GPIO10 */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SDO", 0x18, 0x3, 0x18,
			0x114, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO9 */
			BERLIN_PINCTRL_FUNCTION(0x7, "syspll0")), /* CLKO */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SDI", 0x18, 0x3, 0x1b,
			0x118, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO8 */
			BERLIN_PINCTRL_FUNCTION(0x7, "syspll1")), /* CLKO */
	BERLIN_PINCTRLCONF_GROUP("SPI1_SCLK", 0x1c, 0x3, 0x00,
			0x11c, 0x3, 0x0,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SCLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO7 */
};

static const struct berlin_pinctrl_desc as470_soc_pinctrl_data = {
	.groups = as470_soc_pinctrl_groups,
	.ngroups = ARRAY_SIZE(as470_soc_pinctrl_groups),
};

static const struct of_device_id as470_pinctrl_match[] = {
	{
		.compatible = "syna,as470-soc-pinctrl",
		.data = &as470_soc_pinctrl_data,
	},
	{}
};

static int as470_pinctrl_probe(struct platform_device *pdev)
{
	const struct of_device_id *match =
		of_match_device(as470_pinctrl_match, &pdev->dev);
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

static struct platform_driver as470_pinctrl_driver = {
	.probe	= as470_pinctrl_probe,
	.driver	= {
		.name = "as470-pinctrl",
		.of_match_table = as470_pinctrl_match,
	},
};

static int __init as470_pinctrl_init(void)
{
	return platform_driver_register(&as470_pinctrl_driver);
}
arch_initcall(as470_pinctrl_init);
