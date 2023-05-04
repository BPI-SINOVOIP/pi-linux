/* SPDX-License-Identifier: (GPL-2.0 OR MIT) */
/*
 * Synaptics VS640 clock tree IDs
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Benson Gui <begu@synaptics.com>
 */

/* common clks */
#define CLK_CPUFASTREF		0
#define CLK_MEMFASTREF		1
#define CLK_CFG			2
#define CLK_PERIFSYS		3
#define CLK_ATB			4
#define CLK_DECODER		5
#define CLK_ENCODER		6
#define CLK_OVPCORE		7
#define CLK_GFX3DCORE		8
#define CLK_APBCORE		9
#define CLK_EMMC		10
#define CLK_SD0			11
#define CLK_PERIFTEST125M	12
#define CLK_USB2TEST		13
#define CLK_PERIFTEST250M	14
#define CLK_USB3CORE		15
#define CLK_NPU			16
#define CLK_HDMIRXREF		17
#define CLK_USB2TEST480MG0	18
#define CLK_USB2TEST480MG1	19
#define CLK_USB2TEST480MG2	20
#define CLK_USB2TEST100MG0	21
#define CLK_USB2TEST100MG1	22
#define CLK_USB2TEST100MG2	23
#define CLK_USB2TEST100MG3	24
#define CLK_USB2TEST100MG4	25
#define CLK_PERIFTEST200MG0	26
#define CLK_PERIFTEST200MG1	27
#define CLK_PERIFTEST500MG0	28
#define CLK_AIOSYS		29
#define CLK_USIM		30
#define CLK_PERIFTEST50MG0	31

/* gate clks */
#define CLK_USB0CORE		0
#define CLK_SDIOSYS		1
#define CLK_PCIE0SYS		2
#define CLK_EMMCSYS		3
#define CLK_PBRIDGECORE		4
#define CLK_NPUAXI		5
#define CLK_GETHRGMIISYS	6
#define CLK_GFXAXI		7
#define CLK_USBOTG		8
