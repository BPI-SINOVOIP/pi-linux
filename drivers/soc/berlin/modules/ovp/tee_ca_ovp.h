// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */
#ifndef _TEE_CA_OVP_H_
#define _TEE_CA_OVP_H_
/** VPP_API TA wrapper
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "tee_client_api.h"

/* enum for OVP commands */
typedef enum {
	OVP_CREATE,
	OVP_DESTROY,
	OVP_GETFRAMESWAIT,
	OVP_PASSSHM,
	OVP_GET_CLR_INTR_STS,
	OVP_SUSPEND,
	OVP_RESUME,
} OVP_CMD_ID;


int tz_ovp_invoke_cmd(OVP_CMD_ID cmd);
int tz_ovp_initialize(void);
void tz_ovp_finalize(void);

#endif //_TEE_CA_OVP_H_
