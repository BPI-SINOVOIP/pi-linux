// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _REGMAP_BASE_H_
#define _REGMAP_BASE_H_

#include "ctypes.h"
#include "avio_type.h"

#define REGMAP_IS_VALID_NDX(NDX)                    \
	((NDX >= 0) && (NDX <= REGMAP_MAX_ENTRY))
#define REGMAP_IS_ADDR_IN_RANGE(ADDR, BASE, SIZE)   \
	((ADDR >= BASE) && (ADDR <= (BASE + SIZE)))

#define REGMAP_IS_ENTRY_IN_PHY_RANGE(ADDR, i)   \
	REGMAP_IS_ADDR_IN_RANGE(ADDR, regmap.regmap_list[i].phy_base, regmap.regmap_list[i].phy_size)
#define REGMAP_IS_ENTRY_IN_VIRT_RANGE(ADDR, i)  \
	REGMAP_IS_ADDR_IN_RANGE(ADDR, regmap.regmap_list[i].virt_base, regmap.regmap_list[i].phy_size)

void *regmap_add_entry(UINT32 phy_base, UINT32 phy_size, void *virt_base);
int regmap_remove_entry(UINT32 phy_base);

int regmap_get_handle_by_phy_addr(UINT32 phy_addr);
int regmap_get_handle_by_virt_addr(void *virt_addr);

void *regmap_phy_to_virt_by_handle(int handle, UINT32 phy_addr);
UINT32 regmap_virt_to_phy_by_handle(int handle, void *virt_addr);

void *regmap_phy_to_virt(UINT32 phy_addr);
UINT32 regmap_virt_to_phy(void *virt_addr);

#endif //_REGMAP_BASE_H_
