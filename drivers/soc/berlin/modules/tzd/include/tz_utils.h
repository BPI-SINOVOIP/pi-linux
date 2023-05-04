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
#ifndef __TZ_UTILS_H__
#define __TZ_UTILS_H__

#include "types.h"

typedef enum _tz_secure_reg_ops_ {
	TZ_SECURE_REG_READ   =   0,
	TZ_SECURE_REG_WRITE  =   1,
}tz_secure_reg_ops;

typedef enum _tz_secure_reg_ {
	TZ_REG_SEM_INTR_ENABLE_1   =   0,
	TZ_REG_SEM_INTR_ENABLE_2   =   1,
	TZ_REG_SEM_INTR_ENABLE_3   =   2,
	TZ_REG_SEM_CHK_FULL		   =   3,
	TZ_REG_SEM_POP			   =   4,
	TZ_REG_OVP_INTR_STATUS     =   5,
	TZ_REG_MAX,
}tz_secure_reg;

struct mm_struct;

/*
 * This function walks through the page tables to convert a userland
 * virtual address to a page table entry (PTE)
 */
unsigned long tz_user_virt_to_pte(struct mm_struct *mm, unsigned long vaddr);

/*
 * Implement fastcall to acess(read/write) registers.
 */
int tz_secure_reg_rw(tz_secure_reg reg, tz_secure_reg_ops ops, uint32_t *value);

#endif /* __TZ_UTILS_H__ */
