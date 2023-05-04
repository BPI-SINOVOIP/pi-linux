/*
 * Copyright (c) 2013-2016 Marvell International Ltd. and its affiliates.
 * All rights reserved.
 *
 * This software file (the "File") is owned and distributed by Marvell
 * International Ltd. and/or its affiliates ("Marvell") under the following
 * licensing terms.
 *
 * Marvell Commercial License Option
 * ------------------------------------
 * If you received this File from Marvell and you have entered into a
 * commercial license agreement (a "Commercial License") with Marvell, the
 * File is licensed to you under the terms of the applicable Commercial
 * License.
 *
 * Marvell GPL License Option
 * ------------------------------------
 * If you received this File from Marvell, you may opt to use, redistribute
 * and/or modify this File in accordance with the terms and conditions of the
 * General Public License Version 2, June 1991 (the "GPL License"), a copy of
 * which is available along with the File in the license.txt file or by writing
 * to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE
 * EXPRESSLY DISCLAIMED. The GPL License provides additional details about this
 * warranty disclaimer.
 */

#ifndef _TZ_BOOT_CMD_H_
#define _TZ_BOOT_CMD_H_

#include "smc.h"

enum tz_boot_func_id {
	/* param: none;
	 * return: version number */
	SMC_FUNC_TOS_BOOT_VERSION		= TOS_BOOT(0x00),
	/* param: {stage, mode};
	 * return: error code */
	SMC_FUNC_TOS_BOOT_STAGE			= TOS_BOOT(0x10),
	/* param: {attr_mask, attr_val};
	 * return: count */
	SMC_FUNC_TOS_MEM_REGION_COUNT		= TOS_BOOT(0x20),
	/* param: {attr_mask, attr_val, region, max_num};
	 * return: count */
	SMC_FUNC_TOS_MEM_REGION_LIST,
	/* param: none;
	 * return: error code */
	SMC_FUNC_TOS_OUTER_CACHE_ENABLE		= TOS_BOOT(0x30),
	/* param: none;
	 * return: error code */
	SMC_FUNC_TOS_OUTER_CACHE_DISABLE,
	/* param: none;
	 * return: error code */
	SMC_FUNC_TOS_OUTER_CACHE_RESUME,
	/* param: {src, src_len, dst, dst_len};
	 * return: {errcode,dec_len} */
	SMC_FUNC_TOS_CRYPTO_VERIFY_IMAGE	= TOS_BOOT(0x40),

	/* param: {start_phy_addr, size};
	 * return: error code */
	SMC_FUNC_TOS_SECURE_CACHE_CLEAN		= TOS_BOOT(0x50),
	/* param: {start_phy_addr, size};
	 * return: error code */
	SMC_FUNC_TOS_SECURE_CACHE_INVALIDATE,
	/* param: {start_phy_addr, size};
	 * return: error code */
	SMC_FUNC_TOS_SECURE_CACHE_CLEAN_INVALIDATE,

	/* param: {src, dst, len};
	 * return: {errcode} */
	SMC_FUNC_TOS_MEM_MOVE			= TOS_BOOT(0x60),

	/*
	 * param: {param_addr, param_len};
	 * return: {errcode}
	 */
	SMC_FUNC_TOS_FASTCALL_GENERIC		= TOS_BOOT(0x70),

	/* param: {addr, ops, value};
	 * return: {errcode} */
	SMC_FUNC_TOS_SECURE_REG			= TOS_BOOT(0x80),
};

enum tz_boot_stage {
	TZ_BOOT_STAGE_ROMCODE,
	TZ_BOOT_STAGE_SYSINIT,
	TZ_BOOT_STAGE_TRUSTZONE,
	TZ_BOOT_STAGE_BOOTLOADER,
	TZ_BOOT_STAGE_LINUX,
	TZ_BOOT_STAGE_ANDROID
};

enum tz_boot_mode {
	TZ_BOOT_MODE_NORMAL,
	TZ_BOOT_MODE_RECOVERY
};

#endif /* _TZ_BOOT_CMD_H_ */
