/*
 * TrustZone Driver
 *
 * Copyright (C) 2016 Marvell Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>
#include "tz_utils.h"
#include "smc.h"
#include "tz_boot_cmd.h"

/*
 * This function walks through the page tables to convert a userland
 * virtual address to a page table entry (PTE)
 */
unsigned long tz_user_virt_to_pte(struct mm_struct *mm, unsigned long address)
{
	uint32_t tmp;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;
#ifdef pte_offset_map_lock
	spinlock_t *lock;
#endif

	/* va in user space might not be mapped yet, so do a dummy read here
	 * to trigger a page fault and tell kernel to create the revalant
	 * pte entry in the page table for it. It is possible to fail if
	 * the given va is not a valid one
	 */
	if (get_user(tmp, (uint32_t __user *)address))
		return 0;

	pgd = pgd_offset(mm, address);
	if (!pgd_present(*pgd))
		return 0;

	pud = pud_offset(pgd, address);
	if (!pud_present(*pud))
		return 0;

	pmd = pmd_offset(pud, address);
	if (!pmd_present(*pmd))
		return 0;

#ifdef pte_offset_map_lock
	ptep = pte_offset_map_lock(mm, pmd, address, &lock);
#else
	ptep = pte_offset_map(pmd, address);
#endif
	if (!ptep) {
#ifdef pte_offset_map_lock
		pte_unmap_unlock(ptep, lock);
#endif
		return 0;
	}
	pte = *ptep;
#ifdef pte_offset_map_lock
	pte_unmap_unlock(ptep, lock);
#endif

	if (pte_present(pte))
		return pte_val(pte) & PHYS_MASK;

	return 0;
}

int tz_secure_reg_rw(tz_secure_reg reg, tz_secure_reg_ops ops, uint32_t *value)
{
	unsigned long param[6], result[4];
	param[0] = (unsigned long)reg;
	param[1] = (unsigned long)ops;
	param[2] = (unsigned long)*value;

	__smc6(SMC_FUNC_TOS_SECURE_REG, param, result);

	*value = result[1];

	return result[0];
}
EXPORT_SYMBOL(tz_secure_reg_rw);
