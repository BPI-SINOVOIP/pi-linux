// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include "aio_hal.h"
#include "aio_common.h"

void aio_earc_src_sel(void *hd, u32 sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32AIO_EARC_SRC earc_src;

	address = RA_AIO_EARC_SRC;
	earc_src.u32 = aio_read(aio, address);
	/* source selection
	 * 0: Selects SPDIF RX input or eARC RX SPDIF input
	 * 1 : Selects eARC RX DSD input
	 * 2 or 3: Selects eARC RX I2S input
	 */
	earc_src.uEARC_SRC_SEL = sel;
	aio_write(aio, address, earc_src.u32);
}
EXPORT_SYMBOL(aio_earc_src_sel);

void aio_earc_i2s_frord_sel(void *hd, u32 sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32MIC6_EARC_I2S earc_i2s;

	address = RA_AIO_MIC6 + RA_MIC6_EARC_I2S;
	earc_i2s.u32 = aio_read(aio, address);

	/* Reordering of the I2S channel data received from eARC Rx.
	 * Value
	 * I2S Frame received from eARC Rx
	 * I2S Frame to be stored in DRAM
	 * 000
	 * {B, Payload, V, U, C, P}
	 * {Payload, B, V, U, C, P}
	 * 001
	 * {V, U, C, P, Payload, B}
	 * {Payload, B, V, U, C, P}
	 * 010
	 * {Payload, B, V, U, C, P}
	 * {Payload, B, V, U, C, P}
	 * 011
	 * {B, V, U, C, P, Payload}
	 * {Payload, B, V, U, C, P}
	 * 100
	 * {B, Payload, V, U, C, P}
	 * {B, V, U, C, P, Payload}
	 * 101
	 * {V, U, C, P, Payload, B}
	 * {B, V, U, C, P, Payload}
	 * 110
	 * {Payload, B, V, U, C, P}
	 * {B, V, U, C, P, Payload}
	 * 111
	 * {B, V, U, C, P, Payload}
	 * {B, V, U, C, P, Payload}
	 */

	earc_i2s.uEARC_I2S_FRORD = sel;
	aio_write(aio, address, earc_i2s.u32);
}
EXPORT_SYMBOL(aio_earc_i2s_frord_sel);
