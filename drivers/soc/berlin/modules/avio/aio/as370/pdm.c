// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/printk.h>
#include <linux/err.h>

#include "aio_common.h"
#include "aio_hal.h"
#include "pdm_common.h"
#include "aio_priv.h"

int aio_get_pdm_offset(u32 chanid)
{
	int ret = 0;

	switch(chanid) {
	case AIO_MIC_CH0:
		ret = RA_AIO_PDM + RA_PDM_PDM0;
		break;
	case AIO_MIC_CH1:
		ret = RA_AIO_PDM  + RA_PDM_PDM1;
		break;
	case AIO_MIC_CH2:
		ret = RA_AIO_PDM + RA_PDM_PDM2;
		break;
	case AIO_MIC_CH3:
		ret = RA_AIO_PDM + RA_PDM_PDM3;
		break;
	default:
		printk(KERN_ERR "illegal pdm channd id %d\n", chanid);
		return -EINVAL;
	}

	return ret;
}
