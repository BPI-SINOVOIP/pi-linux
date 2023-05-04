// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */
#include "aio_common.h"
#include "aio_hal.h"
#include "i2s_common.h"

int aio_spdifi_enable_sysclk(void *hd, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32SPDIFRX_CTRL_CTRL1 reg;
	u32 address = RA_AIO_SPDIFRX_CTRL + RA_SPDIFRX_CTRL_CTRL1;

	reg.u32 = aio_read(aio, address);
	reg.uCTRL1_CLK_GATE = enable ? 1 : 0;
	aio_write(aio, address, reg.u32);

	return 0;
}

int aio_misc_sw_rst_extra(void *hd, u32 option, u32 val)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_SW_RST reg;
	u32 address = RA_AIO_SW_RST;

	reg.u32[0] = aio_read(aio, address);
	switch (option) {
	case AIO_SW_RST_MIC6:
		reg.uSW_RST_MIC6 = val;
		break;
	default:
		pr_err("%s, option(%d) not supported\n", __func__, option);
		return -EINVAL;
	}
	aio_write(aio, address, reg.u32[0]);

	return 0;
}


int aio_misc_set_loopback_clk_gate_extra(void *hd, u32 idx, u32 en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_CLK_GATE_EN reg;
	u32 address = RA_AIO_CLK_GATE_EN;

	reg.u32[0] = aio_read(aio, address);
	switch (idx) {
	case AIO_LOOPBACK_CLK_GATE_MIC6:
		reg.uCLK_GATE_EN_MIC6 = en;
		break;
	default:
		pr_err("%s, idx(%d) not supported\n", __func__, idx);
		return -EINVAL;
	}
	aio_write(aio, address, reg.u32[0]);

	return 0;
}

int get_intl_base_addr_extra(u32 id)
{
	u32 address;

	switch (id) {
	case AIO_ID_MIC6_RX:
		address = RA_AIO_MIC6 + RA_MIC6_INTLMODE;
		break;
	default:
		/* will return AIO PRI address*/
		pr_warn("%s: id(%d) not supported\n", __func__, id);
		return INVALID_ADDRESS;
	}

	return address;
}

u32 aio_get_offset_extra_plus(u32 ID, u32 tsd)
{
	u32 offset = 0;

	switch (ID) {
	case AIO_ID_MIC6_RX:
		if (tsd == AIO_TSD0)
			offset = RA_AIO_MIC6 + RA_MIC6_RSD0;
		else
			offset = RA_AIO_MIC6 + RA_MIC6_RSD1 +
					sizeof(SIE_AUDCH) * (tsd - 1);
		break;
	default:
		break;
	}

	return offset;
}

u32 aio_get_addr_extra_plus(u32 id)
{
	u32 address;

	switch (id) {
	case AIO_ID_MIC6_RX:
		address = RA_AIO_MIC6 + RA_MIC6_MICCTRL;
		break;
	default:
		/* will return AIO PRI address*/
		pr_warn("%s: id(%d) not supported\n", __func__, id);
		return INVALID_ADDRESS;
	}

	return address;
}

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
	case AIO_ID_MIC6_RX:
		{
			T32MIC6_RXPORT reg;

			address = RA_AIO_MIC6 + RA_MIC6_RXPORT;
			reg.u32 = aio_read(aio, address);
			reg.uRXPORT_ENABLE = enable;
			aio_write(aio, address, reg.u32);
			pr_debug("MIC6 RXPORT_ENABL (%d)\n", enable);
		}
		break;
	default:
		break;
	}
	return 0;
}

