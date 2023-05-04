// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */
#include "aio_common.h"
#include "aio_hal.h"
#include "i2s_common.h"

int aio_enablerxport_extra(void *hd, u32 id, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;

	switch (id) {
	case AIO_ID_MIC4_RX:
		{
			T32MIC4_RXPORT reg;

			address = RA_AIO_MIC4 + RA_MIC4_RXPORT;
			reg.u32 = aio_read(aio, address);
			reg.uRXPORT_ENABLE = enable;
			aio_write(aio, address, reg.u32);
			pr_debug("MIC4 RXPORT_ENABL (%d)\n", enable);
		}
		break;
	case AIO_ID_MIC5_RX:
		{
			T32MIC5_RXPORT reg;

			address = RA_AIO_MIC5 + RA_MIC5_RXPORT;
			reg.u32 = aio_read(aio, address);
			reg.uRXPORT_ENABLE = enable;
			aio_write(aio, address, reg.u32);
			pr_debug("MIC5 RXPORT_ENABL (%d)\n", enable);
		}
		break;
	default:
		break;
	}
	return 0;
}


