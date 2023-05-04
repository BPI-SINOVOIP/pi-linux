// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/printk.h>

#include "avio_debug.h"
#include "regmap_base.h"

#define REGMAP_MAX_ENTRY	10
#define REGMAP_ENABLE_MORE_PARAM_CHECK

typedef struct _REGMAP_ENTRY_ {
	UINT32 phy_base;
	UINT32 phy_size;
	void *virt_base;
	UINT32 usedFlag;
} REGMAP_ENTRY;

typedef struct _REGMAP_CONTEXT_ {
	REGMAP_ENTRY regmap_list[REGMAP_MAX_ENTRY];
	int regmap_count;
} REGMAP_CONTEXT;

static REGMAP_CONTEXT regmap;
static DEFINE_MUTEX(dhub_mutex);

int regmap_get_handle_by_phy_addr(UINT32 phy_addr)
{
	int i, visit_cnt = 0;
	int found_ndx = 1;

	for (i = 0; i < REGMAP_MAX_ENTRY; i++) {
		if (regmap.regmap_list[i].usedFlag) {
			if (REGMAP_IS_ENTRY_IN_PHY_RANGE(phy_addr, i)) {
				found_ndx = i;
				break;
			}
			visit_cnt++;
			if (visit_cnt == regmap.regmap_count)
				break;
		}
	}

	return found_ndx;
}

int regmap_get_handle_by_virt_addr(void *virt_addr)
{
	int i, visit_cnt = 0;
	int found_ndx = 1;

	for (i = 0; i < REGMAP_MAX_ENTRY; i++) {
		if (regmap.regmap_list[i].usedFlag) {
			if (REGMAP_IS_ENTRY_IN_VIRT_RANGE(virt_addr, i)) {
				found_ndx = i;
				break;
			}
			visit_cnt++;
			if (visit_cnt == regmap.regmap_count)
				break;
		}
	}

	return found_ndx;
}



void *regmap_add_entry(UINT32 phy_base, UINT32 phy_size, void *virt_base)
{
	int i, freeNdx, mappedNdx;

	mappedNdx = freeNdx = -1;

	mutex_lock(&dhub_mutex);
	for (i = 0; i < REGMAP_MAX_ENTRY; i++) {
		if (!regmap.regmap_list[i].usedFlag && (freeNdx == -1)) {
			freeNdx = i;
		} else if (REGMAP_IS_ENTRY_IN_PHY_RANGE(phy_base, i)) {
			mappedNdx = i;
			break;
		}
	}

	if (mappedNdx == -1) {
		if (freeNdx != -1) {
			if (!virt_base)
				virt_base = ioremap(phy_base, phy_size);

			if (!virt_base) {
				avio_error("Fail to map address before it is used - system error!\n");
			} else {
				//Store the new entry at freeNdx
				regmap.regmap_list[freeNdx].usedFlag = 1;
				regmap.regmap_list[freeNdx].phy_base = phy_base;
				regmap.regmap_list[freeNdx].phy_size = phy_size;
				regmap.regmap_list[freeNdx].virt_base = virt_base;
				regmap.regmap_count++;
			}
		} else {
			avio_error("Fail to map address - no free entry\n");
		}
	} else {
		virt_base = regmap.regmap_list[mappedNdx].virt_base;
		avio_error("Fail to map address - Already mapped\n");
	}

	mutex_unlock(&dhub_mutex);

	return virt_base;
}

int regmap_remove_entry(UINT32 phy_base)
{
	int foundNdx;
	int isValidNdx;

	mutex_lock(&dhub_mutex);

	foundNdx = regmap_get_handle_by_phy_addr(phy_base);
	isValidNdx = REGMAP_IS_VALID_NDX(foundNdx);
	if (isValidNdx) {
		iounmap(regmap.regmap_list[foundNdx].virt_base);
		regmap.regmap_list[foundNdx].virt_base = NULL;
		regmap.regmap_list[foundNdx].usedFlag = 0;
		regmap.regmap_count--;
	}

	mutex_unlock(&dhub_mutex);

	return isValidNdx;
}

void *regmap_phy_to_virt_by_handle(int handle, UINT32 phy_addr)
{
	int foundNdx = handle;
	void *virt_addr = NULL;

	if (REGMAP_IS_VALID_NDX(foundNdx)) {
		REGMAP_ENTRY *entry = &regmap.regmap_list[foundNdx];
		#ifdef REGMAP_ENABLE_MORE_PARAM_CHECK
		if (entry->virt_base &&
			(phy_addr >= entry->phy_base) &&
			(phy_addr < (entry->phy_base + entry->phy_size)))
		#endif
		{
			virt_addr = entry->virt_base + (phy_addr - entry->phy_base);
		}
	} else {
		avio_error("Fail to map phyaddr: 0x%x!\n", phy_addr);
	}

	return virt_addr;
}

UINT32 regmap_virt_to_phy_by_handle(int handle, void *virt_addr)
{
	int foundNdx = handle;
	UINT32 phy_addr = 0;

	if (REGMAP_IS_VALID_NDX(foundNdx)) {
		REGMAP_ENTRY *entry = &regmap.regmap_list[foundNdx];
		#ifdef REGMAP_ENABLE_MORE_PARAM_CHECK
		if (entry->virt_base && (virt_addr >= entry->virt_base) &&
			(virt_addr < entry->virt_base + entry->phy_size))
		#endif
		{
			phy_addr = entry->phy_base + (virt_addr - entry->virt_base);
		}
	} else {
		avio_error("Fail to map virtaddr: 0x%pK!\n", virt_addr);
	}

	return phy_addr;
}

void *regmap_phy_to_virt(UINT32 phy_addr)
{
	int foundNdx;

	foundNdx = regmap_get_handle_by_phy_addr(phy_addr);

	return regmap_phy_to_virt_by_handle(foundNdx, phy_addr);
}

UINT32 regmap_virt_to_phy(void *virt_addr)
{
	int foundNdx;

	foundNdx = regmap_get_handle_by_virt_addr(virt_addr);

	return regmap_virt_to_phy_by_handle(foundNdx, virt_addr);
}


