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
 * @param func_id	function ID in x0
 * @param param0	parameters 0 in x1
 * @param param1	parameters 1 in x2
 * @param result	pointer to returned results (x1-x2) in x3 if not NULL
 *
 * @retval 0xffffffff   invalidate function ID
 * @retval others       status
 *
 */
	.globl __smc_cmd2
__smc_cmd2:
	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * Save in the stack, stack pointer MUST be quadword aligned
	 * that is it must be aligned to 16 bytes. Misalignment
	 * generates a Stack Alighment fault
	 * ----------------------------------------------------------
	 */
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp

	/* ----------------------------------------------------------
	 * x0 = function ID, x1 = param0, x2 = param1
	 *
	 * preserve x19 on the stack and x3 in x19
	 *   x3 contains the pointer to returned value
	 *   x19 as a temporary register
	 * ----------------------------------------------------------
	 */
	str	x19, [sp, #16]
	mov	x19, x3

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * x0-x3 contains the returned value
	 *
	 * If the pointer is not NULL saved x1-x2 to result[2]
	 * ---------------------------------------------------------
	 */
	cbz	x19, 1f
	stp	x1, x2, [x19]
1:
	/* ----------------------------------------------------------
	 * restore x19 from stack
	 * ----------------------------------------------------------
	 */
	ldr	x19, [sp, #16]

	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * restore link address register & frame pointer register
	 * ----------------------------------------------------------
	 */
	ldp	x29, x30, [sp], #32

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	ret

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
 * @param func_id	function ID in x0
 * @param param0	parameter 0 in x1
 * @param param1	parameter 1 in x2
 * @param param2	parameter 2 in x3
 * @param result	pointer to return results (x1-x3) in x4 if not NULL
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc_cmd3
__smc_cmd3:
	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * Save in the stack, stack pointer MUST be quadword aligned
	 * that is it must be aligned to 16 bytes. Misalignment
	 * generates a Stack Alighment fault
	 * ----------------------------------------------------------
	 */
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp

	/* ----------------------------------------------------------
	 * x0 = function ID, x1 = param0, x2 = param1, x3 = param2
	 *
	 * preserve x19 on the stack and x4 in x19
	 *   x4 contains the pointer to returned value
	 *   x19 as a temporary register
	 * ----------------------------------------------------------
	 */
	str	x19, [sp, #16]
	mov	x19, x4

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * x0-x3 contains the returned value
	 *
	 * If the pointer is not NULL saved x1-x3 to result[3]
	 * ---------------------------------------------------------
	 */
	cbz	x19, 1f
	stp	x1, x2, [x19]
	str	x3, [x19, #16]
1:
	/* ----------------------------------------------------------
	 * restore x19 from stack
	 * ----------------------------------------------------------
	 */
	ldr	x19, [sp, #16]

	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * restore link address register & frame pointer register
	 * ----------------------------------------------------------
	 */
	ldp	x29, x30, [sp], #32

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	ret

/** SMC call without results.
 *
 * unsigned long __smc(unsigned long func_id, ...);
 *
 * @note it can be used as bellow:
 *       status = smc_nores(func_id);
 *       status = smc_nores(func_id, param0);
 *       status = smc_nores(func_id, param0, param1);
 *       status = smc_nores(func_id, param0, param1, param2);
 *       status = smc_nores(func_id, param0, param1, param2, param3);
 *
 * @param func_id	function ID in x0
 * @param param0	parameter 0 in x1
 * @param param1	parameter 1 in x2
 * @param param2	parameter 2 in x3
 * @param param3	parameter 3 in x4
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc
__smc:
	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * Save in the stack, stack pointer MUST be quadword aligned
	 * that is it must be aligned to 16 bytes. Misalignment
	 * generates a Stack Alighment fault
	 * ----------------------------------------------------------
	 */
	stp	x29, x30, [sp, #-16]!
	mov	x29, sp

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * restore link address register & frame pointer register
	 * ----------------------------------------------------------
	 */
	ldp	x29, x30, [sp], #16

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	ret

/** SMC call with 4 parameters without results.
 *
 * unsigned long __smc4(unsigned long func_id,
 *                      unsigned long param0,
 *                      unsigned long param1,
 *                      unsigned long param2,
 *                      unsigned long param3)
 *
 * @param func_id	function ID in x0
 * @param param0	parameter 0 in x1
 * @param param1	parameter 1 in x2
 * @param param2	parameter 2 in x3
 * @param param3	parameter 3 in x4
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc4
__smc4:
	b	__smc

/** SMC call command with 3 parameters.
 *
 *  unsigned long __smc3(unsigned long func_id,
 *                       unsigned long param0,
 *                       unsigned long param1,
 *                       unsigned long param2,
 *                       unsigned long result[4]);
 *
 * @param func_id	function ID in x0
 * @param param0	parameter 0 in x1
 * @param param1	parameter 1 in x2
 * @param param2	parameter 2 in x3
 * @param result	pointer to return results (x0-x3) in x4 if not NULL
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc3
__smc3:
	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * Save in the stack, stack pointer MUST be quadword aligned
	 * that is it must be aligned to 16 bytes. Misalignment
	 * generates a Stack Alighment fault
	 * ----------------------------------------------------------
	 */
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp

	/* ----------------------------------------------------------
	 * x0 = function ID, x1 = param0, x2 = param1, x3 = param2
	 *
	 * preserve x4 and x19 on the stack and x4 in x19
	 *   x4 contains the pointer to returned value
	 *   x19 as a temporary register
	 * ----------------------------------------------------------
	 */
	str	x19, [sp, #16]
	mov	x19, x4

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * x0-x3 contains the returned value
	 *
	 * If the pointer is not NULL saved x0-x3 to result[4]
	 * ---------------------------------------------------------
	 */
	cbz	x19, 1f
	stp	x0, x1, [x19]
	stp	x2, x3, [x19, #16]
1:
	/* ----------------------------------------------------------
	 * restore x19 from stack
	 * ----------------------------------------------------------
	 */
	ldr	x19, [sp, #16]

	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * restore link address register & frame pointer register
	 * ----------------------------------------------------------
	 */
	ldp	x29, x30, [sp], #32

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	ret

/** SMC call with 6 parameters.
 *
 * unsigned long __smc6(unsigned long func_id,
 *                      const unsigned long param[6],
 *                      unsigned long result[4]);
 *
 * @param func_id	function ID in x0
 * @param param		buffer to store 6 parmeters in r1
 * @param result	buffer to return results (x0-x3) in r2 if not NULL
 *			result[0] is returned by function too
 *
 * @retval 0xffffffff	invalidate Function ID.
 * @retval others	status
 */
	.globl __smc6
__smc6:
	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * Save in the stack, stack pointer MUST be quadword aligned
	 * that is it must be aligned to 16 bytes. Misalignment
	 * generates a Stack Alighment fault
	 * ----------------------------------------------------------
	 */
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp

	/* ----------------------------------------------------------
	 * x0 = function ID
	 * x1 = pointer to 6 parameters (x1-x6)
	 * x2 = pointer to return results (x0-x3)
	 *
	 * preserve x19 on the stack and x2 in x19
	 *   x2 contains the pointer to the returned value
	 *   x19 as a temporary register
	 * ----------------------------------------------------------
	 */
	str	x19, [sp, #16]
	mov	x19, x2

	/* ----------------------------------------------------------
	 * Load 6 parameters into r1-r6, will panic if r1=NULL
	 *
	 * x9 = ip0, first intra-procedure-call scratch register
	 * ----------------------------------------------------------
	 */
	mov	x9, x1
	ldp	x1, x2, [x9]
	ldp	x3, x4, [x9, #16]
	ldp	x5, x6, [x9, #32]

	/* ----------------------------------------------------------
	 * make smc call now
	 * ----------------------------------------------------------
	 */
	smc	#0

	/* ---------------------------------------------------------
	 * x0-x3 contains the returned value
	 *
	 * If the pointer is not NULL saved x1-x3 to result[4]
	 * ---------------------------------------------------------
	 */
	cbz	x19, 1f
	stp	x0, x1, [x19]
	stp	x2, x3, [x19, #16]
1:
	/* ----------------------------------------------------------
	 * restore x19 from stack
	 * ----------------------------------------------------------
	 */
	ldr	x19, [sp, #16]

	/* ----------------------------------------------------------
	 * x30 = link address, x29 = frame pointer
	 *
	 * restore link address register & frame pointer register
	 * ----------------------------------------------------------
	 */
	ldp	x29, x30, [sp], #32

	/* ---------------------------------------------------------
	 * back to caller
	 * ---------------------------------------------------------
	 */
	ret
