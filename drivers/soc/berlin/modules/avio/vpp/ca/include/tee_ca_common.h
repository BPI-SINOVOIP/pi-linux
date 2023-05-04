// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _TEE_COMMON_CA_H_
#define _TEE_COMMON_CA_H_

#include "tee_client_type.h"
#include "tee_client_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define VPP_TEEC_LOGIFERROR(result) \
		do { \
			if (result != TEEC_SUCCESS) \
				pr_err("TEEC Error ret=0x%08x\n", result); \
		} while (0)


//#define TAVPP_USE_SEPARATE_ISR_INSTANCE
enum {
	TAVPP_API_INSTANCE = 0,
#ifdef TAVPP_USE_SEPARATE_ISR_INSTANCE
	TAVPP_ISR_INSTANCE,
#endif //TAVPP_USE_SEPARATE_ISR_INSTANCE
	MAX_TAVPP_INSTANCE_NUM,
};

#ifdef __cplusplus
}
#endif

#endif /* _TEE_COMMON_CA_H_ */
