// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _I2S_H_
#define _I2S_H_

#include "aio.h"

enum aio_i2s_id {
	I2S1_ID = 0,
	I2S2_ID = 1,
	I2S3_ID = 2,
};

#define I2S_PRI_OFFSET (RA_AIO_PRI + RA_PRI_TSD0_PRI)
#define I2S_SEC_OFFSET (RA_AIO_SEC + RA_SEC_TSD0_SEC)
#define I2S_SPDIF_OFFSET (RA_AIO_SPDIF + RA_SPDIF_SPDIF)
#define I2S_MIC2_OFFSET (RA_AIO_MIC2 + RA_MIC2_RSD0)

#define INVALID_ADDRESS			(0xFFFFFFFF)
#define IS_INVALID_1BIT_VAL(val)		((val) & 0xFFFFFFFE)
#define IS_INVALID_2BITS_VAL(val)		((val) & 0xFFFFFFFC)
#define IS_INVALID_ID(id)			((id) > AIO_ID_MIC6_RX)
#define IS_INVALID_TSD(tsd)			((tsd) >= MAX_TSD)

#endif
