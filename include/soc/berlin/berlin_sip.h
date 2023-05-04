/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Synaptics SIP Function Identifier
 *
 * Copyright (C) 2020, Synaptics Incorporated
 */

#ifndef __BERLIN_SIP_H
#define __BERLIN_SIP_H

/*
 * Usage:
 * Read/write a protected register at EL3
 * Call parameters:
 * a0: SYNA_SIP_SMC32_SREGISTER_OP or SYNA_SIP_SMC64_SREGISTER_OP
 * a1: SYNA_SREGISTER_READ or SYNA_SREGISTER_WRITE
 * a2: Secure register ID
 * a3: Value program to the register
 * Return parameters:
 * a0: SYNA_SECURE_REGISTER_OK or SYNA_SECURE_REGISTER_REJECTED
 * a1: register value if SYNA_SREGISTER_READ and return value is OK
 */
#define SYNA_SIP_SMC32_SREGISTER_OP		0x82000001
#define SYNA_SIP_SMC64_SREGISTER_OP		0xC2000001
#define SYNA_SREGISTER_READ				0xF1
#define SYNA_SREGISTER_WRITE			0xF2
#define SYNA_SECURE_REGISTER_OK			0x0
#define SYNA_SECURE_REGISTER_REJECTED	0x1

/*
 * Secure register ID
 */
#define SEM_INTR_ENABLE_1	0
#define SEM_INTR_ENABLE_2	1
#define SEM_INTR_ENABLE_3	2
#define SEM_CHK_FULL		3
#define SEM_POP				4
#define OVP_INTSTATUS		5
#define TSB_INTR_ENABLE_1	6
#define TSB_INTR_ENABLE_2	7
#define TSB_INTR_ENABLE_3	8
#define TSB_SEM_POP			9
#define TSB_SEM_FULL		10
#define FWR_INTR_ENABLE_1	11
#define FWR_INTR_ENABLE_2	12
#define FWR_INTR_ENABLE_3	13
#define FWR_SEM_POP			14
#define FWR_SEM_FULL		15
#define ISP_INT_STATUS		16
#define ISP_CORE_TOP_CTRL	17
#define ISP_ISP_INTR_STS	18
#define ISP_ISP_INTR_CLR	19
#define ISP_MI_INTR_STS		20
#define ISP_MI_INTR_CLR		21
#define NSK_CORE_INTSTATUS	22
#define NSK_SOC_INTSTATUS	23
#define NSK_SOC_INTCLEAR	24
#define DSP0_INTR_STATUS	25
#define DSP1_INTR_STATUS	26
#define IFCP_I2H_CTRL_STATUS	27

#endif /* __BERLIN_SIP_H */
