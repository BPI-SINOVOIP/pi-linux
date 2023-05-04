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

	.text
	.arch_extension sec

/** SMC Calling Conversion - Argument passing
 *             Document number: ARM DEN 0028A
 *
 * http://infocenter.arm.com/help/topic/com.arm.doc.den0028a/index.html
 *
 * [SMC32 Argument passing]
 *
 * When the SMC32 convention is used, the SMC intructions take up to seven
 * 32-bit arguments in register and can return up to four 32-bit values in
 * registers.
 *
 * When an SMC32 call is made from AArch32:
 * o Arguments are passed in register R0-R6
 * o Results are returned in R0-R3
 * o Registers R4-R14 are callee-saved and must be preserved over the SMC call
 *
 * When an SMC32 call is made from AArch64:
 * o Arguments are passed in register X0-X6
 * o Results are returned in X0-X3
 * o Registers X18-X30 and stack pointers SP_EL0 and SP_ELx are callee-saved and
 *   must be preserved over the SMC call
 *
 * [SMC64 Argument passing]
 *
 * When the SMC64 convention is used, the SMC instructions take up to seven 64-bit
 * arguments in registers and can return up to four 64-bit values in register
 *
 * When an SMC64 call is made from AArch64:
 * o Arguments are passed in register X0-X6
 * o Results are returned in X0-X3
 * o Registers X18-X30 and stack pointers SP_EL0 and SP_ELx are callee-saved and
 *   must ber preserved over the SMC call
 *
 * Then calling convention cannot be used by code executing AArch32 state
 */

/** SMC call command with 2 parameters
 *
 * unsigned long __smc_cmd2(unsigned long func_id,
 *                          unsigned long param0,
 *                          unsigned long param1,
 *                          unsigned long result[2])
 *
 * @param func_id	function ID in r0
 * @param param0	parameters 0 in r1
 * @param param1	parameters 1 in r2
 * @param result	pointer to returned results (r1-r2) in r3 if not NULL
 *
 * @retval 0xffffffff   invalidate function ID
 * @retval others       status
 *
 */
	.globl __smc_cmd2
__smc_cmd2:
	/* ----------------------------------------------------------
	 * r0 = function ID, r1 = param0, r2 = param1
	 *
	 * r3 contains the pointer to returned value, preserve it
	 * on the stack
	 * ----------------------------------------------------------
	 */
	push	{r3}

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * retrieve the pointer of returned value previously saved
	 * on the stack
	 * ---------------------------------------------------------
	 */
	pop	{ip}
	/* ---------------------------------------------------------
	 * r0-r3 contains the returned value
	 *
	 * If the pointer is not NULL saved r1-r2 to result[2]
	 * ---------------------------------------------------------
	 */
	cmp	ip, #0
	stmne	ip!, {r1-r2}

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	bx	lr

/** SMC call command with 3 parameters.
 *
 *  unsigned long __smc_cmd3(unsigned long func_id,
 *                           unsigned long param0,
 *                           unsigned long param1,
 *                           unsigned long param2,
 *                           unsigned long result[3]);
 *
 *  unsigned long __smc3(unsigned long func_id,
 *                       unsigned long param0,
 *                       unsigned long param1,
 *                       unsigned long param2,
 *                       unsigned long result[3]);
 *
 * @param func_id	function ID in r0
 * @param param0	parameter 0 in r1
 * @param param1	parameter 1 in r2
 * @param param2	parameter 2 in r3
 * @param result	pointer to return results (r1-r3) in r4 if not NULL
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc_cmd3
__smc_cmd3:
	/* ----------------------------------------------------------
	 * r0 = function ID, r1 = param0, r2 = param1, r3 = param2
	 *
	 * r4 is preserved on the stack by caller
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * retrieve the pointer to returned value preserved on the
	 * stack by the caller
	 * ---------------------------------------------------------
	 */
	ldr	ip, [sp]
	/* ---------------------------------------------------------
	 * r0-r3 contains the returned value
	 *
	 * If the pointer is not NULL saved r1-r3 to result[3]
	 * ---------------------------------------------------------
	 */
	cmp	ip, #0
	stmne	ip!, {r1-r3}

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	bx	lr

/** SMC call without results.
 *
 * unsigned long __smc(unsigned long func_id, ...);
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
	.globl __smc
__smc:
	smc	#0
	bx	lr

/** SMC call command with 3 parameters.
 *
 *  unsigned long __smc3(unsigned long func_id,
 *                       unsigned long param0,
 *                       unsigned long param1,
 *                       unsigned long param2,
 *                       unsigned long result[4]);
 *
 * @param func_id	function ID in r0
 * @param param0	parameter 0 in r1
 * @param param1	parameter 1 in r2
 * @param param2	parameter 2 in r3
 * @param result	pointer to return results (r0-r3) in r4 if not NULL
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc3
__smc3:
	/* ----------------------------------------------------------
	 * r0 = function ID, r1 = param0, r2 = param1, r3 = param2
	 *
	 * r4 is preserved on the stack by caller
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * retrieve the pointer to returned value preserved on the
	 * stack by the caller
	 * ---------------------------------------------------------
	 */
	ldr	ip, [sp]
	/* ---------------------------------------------------------
	 * r0-r3 contains the returned value
	 *
	 * If the pointer is not NULL saved r1-r3 to result[3]
	 * ---------------------------------------------------------
	 */
	cmp	ip, #0
	stmne	ip!, {r0-r3}

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	bx	lr

/** SMC call command with 4 parameters without results
 *
 *  unsigned long __smc4(unsigned long func_id,
 *                       unsigned long param0,
 *                       unsigned long param1,
 *                       unsigned long param2,
 *                       unsigned long param3)
 *
 * @param func_id       function ID in r0
 * @param param         parameter 0 in r1
 * @param param         parameter 1 in r2
 * @param param         parameter 2 in r3
 * @param param         parameter 3 in r4
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc4
__smc4:
	/* ----------------------------------------------------------
	 * r0 = function ID
	 * r1 = parameter 0
	 * r2 = parameter 1
	 * r3 = parameter 2
	 * r4 = parameter 3, callee-saved register
	 * ----------------------------------------------------------
	 */
	ldr	ip, [sp]
	push	{r4}
	mov	r4, ip

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ----------------------------------------------------------
	 * restore r4 from stack
	 * ----------------------------------------------------------
	 */
	pop	{r4}

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	bx	lr

/** SMC call with 6 parameters.
 *
 * unsigned long __smc6(unsigned long func_id,
 *                      const unsigned long param[6],
 *                      unsigned long result[4]);
 *
 * @param func_id	function ID in r0
 * @param param		buffer to store 6 parmeters in r1
 * @param result	buffer to return results (r0-r3) in r2 if not NULL
 *			result[0] is returned by function too
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc6
__smc6:
	/* ----------------------------------------------------------
	 * r0 = function ID
	 * r1 = pointer to 6 parameters
	 * r2 = pointer to return results (r0-r3)
	 *
	 * preserve callee-saved r4-r6, and save r2 on the stack
	 * ----------------------------------------------------------
	 */
	mov	ip, r2
	push	{r4-r6, ip}

	/* ----------------------------------------------------------
	 * Load 6 parameters into r1-r6, will panic if r1=NULL
	 * ----------------------------------------------------------
	 */
	mov	ip, r1
	ldm	ip!, {r1-r6}

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ----------------------------------------------------------
	 * restore r4-r6 from stack and retrieve the pointer to return
	 * results in ip
	 * ----------------------------------------------------------
	 */
	pop	{r4-r6, ip}

	/* ---------------------------------------------------------
	 * r0-r3 contains the returned value
	 *
	 * If the pointer is not NULL saved r1-r3 to result[4]
	 * ---------------------------------------------------------
	 */
	cmp	ip, #0
	stmne	ip!, {r0-r3}

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	bx	lr
