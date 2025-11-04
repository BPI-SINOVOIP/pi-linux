/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
 *
 * Copyright (C) 2024 Renesas Electronics Corp.
 */
#ifndef __DT_BINDINGS_CLOCK_RENESAS_R9A09G056_CPG_H__
#define __DT_BINDINGS_CLOCK_RENESAS_R9A09G056_CPG_H__

#include <dt-bindings/clock/renesas-cpg-mssr.h>

/* Core Clock list */
#define R9A09G056_SYS_0_PCLK			0
#define R9A09G056_CA55_0_CORE_CLK0		1
#define R9A09G056_CA55_0_CORE_CLK1		2
#define R9A09G056_CA55_0_CORE_CLK2		3
#define R9A09G056_CA55_0_CORE_CLK3		4
#define R9A09G056_CA55_0_PERIPHCLK		5
#define R9A09G056_CM33_CLK0			6
#define R9A09G056_CST_0_SWCLKTCK		7
#define R9A09G056_IOTOP_0_SHCLK			8

/* XSPI Core Clocks */
#define CLK_SMUX2_XSPI_CLK1			9
#define CLK_SMUX2_XSPI_CLK0			10
#define CLK_PLLCM33_XSPI			11

#endif /* __DT_BINDINGS_CLOCK_RENESAS_R9A09G056_CPG_H__ */
