/* SPDX-License-Identifier: (GPL-2.0 OR MIT) */
/*
 * Synaptics AS370 clock tree IDs
 *
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

/* common clks */
#define CLK_CPUFASTREF		0
#define CLK_MEMFASTREF		1
#define CLK_CFG			2
#define CLK_PERIFSYS		3
#define CLK_ATB			4
#define CLK_AVIOSYS		5
#define CLK_APBCORE		6
#define CLK_NNASYS		7
#define CLK_NNACORE		8
#define CLK_EMMC		9
#define CLK_SD0			10
#define CLK_PCIE_500M_TXTEST	11
#define CLK_PCIE_250M_PIPETEST1	12
#define CLK_PCIE_250M_PIPETEST2	13
#define CLK_PCIE_500M_RXTEST	14
#define CLK_PCIE_SERDESTEST	15
#define CLK_NFCECC		16
#define CLK_NFCCORE		17
#define CLK_USBOTG60MTEST	18
#define CLK_USBOTG50MTEST	19
#define CLK_USBOTG12MTEST	20
#define CLK_USBOTG480MTEST	21
#define CLK_BCM			22

/* gate clks */
#define CLK_TSPSYS		0
#define CLK_USB0CORE		1
#define CLK_SDIOSYS		2
#define CLK_PCIE0SYS		3
#define CLK_PCIE1SYS		4
#define CLK_NFCSYS		5
#define CLK_EMMCSYS		6
#define CLK_PBRIDGECORE		7
