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

/** ARM TrustZone SMC Calling Convention API.
 *
 * It follows ARM DEN 0028A (0.9.0) SMC CALLING CONVENTION.
 */
#ifndef _SMC_H_
#define _SMC_H_

/* Function ID */
#define SMC_FUNC_ID_MASK_FASTCALL	(0x80000000)
#define SMC_FUNC_ID_MASK_SMC64		(0x40000000)
#define SMC_FUNC_ID_MASK_RANGE		(0x3f000000)
#define SMC_FUNC_ID_MASK_NUMBER		(0x0000ffff)

#define SMC_FUNC_ID_ARCH		(0x00000000)
#define SMC_FUNC_ID_CPU			(0x01000000)
#define SMC_FUNC_ID_SIP			(0x02000000)
#define SMC_FUNC_ID_OEM			(0x03000000)
#define SMC_FUNC_ID_STD			(0x04000000)
#define SMC_FUNC_ID_TA_START		(0x30000000)
#define SMC_FUNC_ID_TA_END		(0x31000000)
#define SMC_FUNC_ID_TOS_START		(0x32000000)
#define SMC_FUNC_ID_TOS_END		(0x3f000000)

#define SMC_FUNC_ID_UNKNOWN		(0xffffffff)

/* Hypervisor Client ID */
#define SMC_CLIENT_HYPERVISOR		(0x00000000)

/* Bellow is specified by Marvell */
#define SMC_FUNC_ID_TA			SMC_FUNC_ID_TA_START
#define SMC_FUNC_ID_TOS			SMC_FUNC_ID_TOS_START
#define SMC_FUNC_DONE			(0x00000000)

#define TOS_STDCALL(number)		\
	(SMC_FUNC_ID_TOS | ((number) & SMC_FUNC_ID_MASK_NUMBER))
#define TA_STDCALL(number)		\
	(SMC_FUNC_ID_TA | ((number) & SMC_FUNC_ID_MASK_NUMBER))
#define TOS_FASTCALL(number)		\
	(SMC_FUNC_ID_MASK_FASTCALL | TOS_STDCALL(number))
#define TA_FASTCALL(number)		\
	(SMC_FUNC_ID_MASK_FASTCALL | TA_STDCALL(number))

/*
 * Function ID Defined by ARM DEN 0028A
 */
/* 0x80000000-0x8000ffff: ARM Architecture Calls */
#define SMC_FUNC_ARCH_COUNT		(0x8000ff00)
#define SMC_FUNC_ARCH_UID		(0x8000ff01)
#define SMC_FUNC_ARCH_REVISION		(0x8000ff03)

/* 0x81000000-0x8100ffff: CPU Service Calls */
#define SMC_FUNC_CPU_COUNT		(0x8100ff00)
#define SMC_FUNC_CPU_UID		(0x8100ff01)
#define SMC_FUNC_CPU_REVISION		(0x8100ff03)

/* 0x82000000-0x8200ffff: SIP Service Calls */
#define SMC_FUNC_SIP_COUNT		(0x8200ff00)
#define SMC_FUNC_SIP_UID		(0x8200ff01)
#define SMC_FUNC_SIP_REVISION		(0x8200ff03)

/* 0x83000000-0x8300ffff: OEM Service Calls */
#define SMC_FUNC_OEM_COUNT		(0x8300ff00)
#define SMC_FUNC_OEM_UID		(0x8300ff01)
#define SMC_FUNC_OEM_REVISION		(0x8300ff03)

/* 0x84000000-0x8400ffff: STD Service Calls */
#define SMC_FUNC_STD_COUNT		(0x8400ff00)
#define SMC_FUNC_STD_UID		(0x8400ff01)
#define SMC_FUNC_STD_REVISION		(0x8400ff03)

/* 0x84000000-0x8400001f: PSCI */
#define SMC_FUNC_PSCI_VERSION		(0x84000000)
#define SMC_FUNC_PSCI_CPU_SUSPEND	(0x84000001)
#define SMC_FUNC_PSCI_CPU_OFF		(0x84000002)
#define SMC_FUNC_PSCI_CPU_ON		(0x84000003)
#define SMC_FUNC_PSCI_AFFINITY_INFO	(0x84000004)
#define SMC_FUNC_PSCI_MIGRATE		(0x84000005)
#define SMC_FUNC_PSCI_MIGRATE_INFO_TYPE	(0x84000006)
#define SMC_FUNC_PSCI_MIGRATE_INFO_UP_CPU	(0x84000007)
#define SMC_FUNC_PSCI_SYSTEM_OFF	(0x84000008)
#define SMC_FUNC_PSCI_SYSTEM_RESET	(0x84000009)

/* TOS Service Calls */
#define SMC_FUNC_TOS_COUNT		(0xbf00ff00)
#define SMC_FUNC_TOS_UID		(0xbf00ff01)
#define SMC_FUNC_TOS_REVISION		(0xbf00ff03)

#define SMC_FUNC_TOS_BOOT_START		(0xbf000000)
#define TOS_BOOT(number)		(SMC_FUNC_TOS_BOOT_START + (number))

#ifndef __ASSEMBLY__

/** SMC call command with 2 parameters.
 *
 * @param func_id	function ID.
 * @param param0	parameter 0.
 * @param param1	parameter 1.
 * @param result	buffer to return results (r1-r2). NULL for not return.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc_cmd2(unsigned long func_id,
		unsigned long param0,
		unsigned long param1,
		unsigned long result[2]);

/** SMC call command with 3 parameters.
 *
 * @param func_id	function ID.
 * @param param0	parameter 0.
 * @param param1	parameter 1.
 * @param param2	parameter 2.
 * @param result	buffer to return results (r1-r3). NULL for not return.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc_cmd3(unsigned long func_id,
		unsigned long param0,
		unsigned long param1,
		unsigned long param2,
		unsigned long result[3]);

/** SMC call with 3 parameters.
 *
 * @param func_id	function ID.
 * @param param0	parameter 0.
 * @param param1	parameter 1.
 * @param param2	parameter 2.
 * @param result	buffer to return results (r0-r3). NULL for not return.
 *			result[0] is returned by function too.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc3(unsigned long func_id,
		unsigned long param0,
		unsigned long param1,
		unsigned long param2,
		unsigned long result[4]);

/** SMC call with 3 parameters.
 *
 * @param func_id	function ID.
 * @param param		buffer to store 6 parmeters.
 * @param result	buffer to return results (r0-r3). NULL for not return.
 *			result[0] is returned by function too.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc6(unsigned long func_id,
		const unsigned long param[6],
		unsigned long result[4]);

/** SMC call without results.
 *
 * @note it can be used as bellow:
 *       status = smc_nores(func_id);
 *       status = smc_nores(func_id, param0);
 *       status = smc_nores(func_id, param0, param1);
 *       status = smc_nores(func_id, param0, param1, param2);
 *
 * @param func_id	function ID.
 * @param param0	parameter 0.
 * @param param1	parameter 1.
 * @param param2	parameter 2.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc(unsigned long func_id, ...);

/** SMC call with 4 parameters without results
 *
 * @param func_id	function ID.
 * @param param0	parameter 0.
 * @param param1	parameter 1.
 * @param param2	parameter 2.
 * @param param2	parameter 3.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc4(unsigned long func_id,
		unsigned long param0,
		unsigned long param1,
		unsigned long param2,
		unsigned long param3);

/** smc function call map.
 *
 * @param func_id	function ID.
 * @param call		handler of the function ID.
 */
struct smc_func {
	unsigned long func_id;
	void (*call)(unsigned long func_id,
		const unsigned long param[6],
		unsigned long result[4]);
};

/*
 * bellow is deprecated.
 *
 * it's just used to bring up new communication protocol.
 *
 * - for client (nw or client TA), call by:
 *     status = __smc_tee(task_id, cmd_id, param, call_id, &cb_cmd);
 * - for service, return as:
 *     __smc_tee(TZ_TASK_ID_CALLER, TZ_CMD_DONE, result, origin, NULL);
 */
#if defined(__TRUSTZONE__)
/** SMC call command with 3 parameters.
 *
 * @param func_id	function ID.
 * @param param0	parameter 0.
 * @param param1	parameter 1.
 * @param param2	parameter 2.
 * @param result	buffer to return results (r1-r3). NULL for not return.
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
unsigned long __smc_call(unsigned long func_id,
			unsigned long param0,
			unsigned long param1,
			unsigned long param2,
			unsigned long result[3]);
#define __smc_tee(task_id, func_id, param0, param1, ret)	\
	__smc_call(func_id, task_id, param0, param1, ret)
#else
#define __smc_tee(task_id, func_id, param0, param1, ret)	\
	__smc_cmd3(func_id, task_id, param0, param1, ret)
#endif

#endif /* __ASSEMBLY__ */

#endif /* _SMC_H_ */
