// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#include "aio_common.h"
#include "aio_hal.h"

int aio_misc_enable_audio_timer(void *hd, bool en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_SAMP_CTRL reg;
	u32 address = RA_AIO_SAMP_CTRL;

	reg.u32[0] = aio_read(aio, address);
	reg.uSAMP_CTRL_EN_AUDTIMER = en;
	aio_write(aio, address, reg.u32[0]);
	return 0;
}

int aio_misc_get_audio_timer(void *hd, u32 *val)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_ATR reg;
	u32 address = RA_AIO_ATR;

	reg.u32[0] = aio_read(aio, address);
	*val = reg.u32[0];
	return 0;
}

int aio_misc_enable_sampinfo(void *hd, u32 idx, bool en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_SAMP_CTRL ctrl_reg;
	u32 address = RA_AIO_SAMP_CTRL;

	ctrl_reg.u32[0] = aio_read(aio, address);
	switch (idx) {
	case AIO_ATS_I2STX1:
		ctrl_reg.uSAMP_CTRL_EN_I2STX1 = en;
		break;
	case AIO_ATS_I2STX2:
		ctrl_reg.uSAMP_CTRL_EN_I2STX1 = en;
		break;
	case AIO_ATS_HDMITX:
		ctrl_reg.uSAMP_CTRL_EN_HDMI = en;
		break;
	case AIO_ATS_HDMIARCTX:
		ctrl_reg.uSAMP_CTRL_EN_HDMIARCTX = en;
		break;
	case AIO_ATS_SPDIFTX:
		ctrl_reg.uSAMP_CTRL_EN_SPDIFTX = en;
		break;
	case AIO_ATS_SPDIFTX1:
		ctrl_reg.uSAMP_CTRL_EN_SPDIFTX1 = en;
		break;
	case AIO_ATS_SPDIFRX:
		ctrl_reg.uSAMP_CTRL_EN_SPDIFRX = en;
		break;
	case AIO_ATS_I2SRX1:
		ctrl_reg.uSAMP_CTRL_EN_I2SRX1 = en;
		break;
	case AIO_ATS_I2SRX2:
		ctrl_reg.uSAMP_CTRL_EN_I2SRX2 = en;
		break;
	case AIO_ATS_I2SRX3:
		ctrl_reg.uSAMP_CTRL_EN_I2SRX3 = en;
		break;
	case AIO_ATS_I2SRX4:
		ctrl_reg.uSAMP_CTRL_EN_I2SRX4 = en;
		break;
	case AIO_ATS_I2SRX5:
		ctrl_reg.uSAMP_CTRL_EN_I2SRX5 = en;
		break;
	case AIO_ATS_PDMRX1:
		ctrl_reg.uSAMP_CTRL_EN_PDMRX1 = en;
		break;
	default:
		pr_err("%s, idx(%d) not supported\n", __func__, idx);
		return -EINVAL;
	}
	aio_write(aio, address, ctrl_reg.u32[0]);
	return 0;
}

int aio_misc_set_sampinfo_req(void *hd, u32 idx, bool en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_SAMPINFO_REQ req_reg;
	u32 address = RA_AIO_SAMPINFO_REQ;

	req_reg.u32[0] = aio_read(aio, address);
	switch (idx) {
	case AIO_ATS_I2STX1:
		req_reg.uSAMPINFO_REQ_I2STX1 = en;
		break;
	case AIO_ATS_I2STX2:
		req_reg.uSAMPINFO_REQ_I2STX2 = en;
		break;
	case AIO_ATS_HDMITX:
		req_reg.uSAMPINFO_REQ_HDMITX = en;
		break;
	case AIO_ATS_HDMIARCTX:
		req_reg.uSAMPINFO_REQ_HDMIARCTX = en;
		break;
	case AIO_ATS_SPDIFTX:
		req_reg.uSAMPINFO_REQ_SPDIFTX = en;
		break;
	case AIO_ATS_SPDIFTX1:
		req_reg.uSAMPINFO_REQ_SPDIFTX1 = en;
		break;
	case AIO_ATS_SPDIFRX:
		req_reg.uSAMPINFO_REQ_SPDIFRX = en;
		break;
	case AIO_ATS_I2SRX1:
		req_reg.uSAMPINFO_REQ_I2SRX1 = en;
		break;
	case AIO_ATS_I2SRX2:
		req_reg.uSAMPINFO_REQ_I2SRX2 = en;
		break;
	case AIO_ATS_I2SRX3:
		req_reg.uSAMPINFO_REQ_I2SRX3 = en;
		break;
	case AIO_ATS_I2SRX4:
		req_reg.uSAMPINFO_REQ_I2SRX4 = en;
		break;
	case AIO_ATS_I2SRX5:
		req_reg.uSAMPINFO_REQ_I2SRX5 = en;
		break;
	case AIO_ATS_PDMRX1:
		req_reg.uSAMPINFO_REQ_PDMRX1 = en;
		break;
	default:
		pr_err("%s, idx(%d) not supported\n", __func__, idx);
		return -EINVAL;
	}
	aio_write(aio, address, req_reg.u32[0]);
	return 0;
}

int aio_misc_get_audio_counter(void *hd, u32 idx, u32 *c)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;

	switch (idx) {
	case AIO_ATS_I2STX1:
	case AIO_ATS_I2STX2:
	case AIO_ATS_HDMITX:
	case AIO_ATS_HDMIARCTX:
	case AIO_ATS_SPDIFTX:
	case AIO_ATS_SPDIFTX1:
	case AIO_ATS_SPDIFRX:
	case AIO_ATS_I2SRX1:
	case AIO_ATS_I2SRX2:
	case AIO_ATS_I2SRX3:
	case AIO_ATS_I2SRX4:
	case AIO_ATS_I2SRX5:
	case AIO_ATS_PDMRX1:
		break;
	default:
		pr_err("%s, idx(%d) not supported\n", __func__, idx);
		return -EINVAL;
	}

	address = RA_AIO_SCR + 4 * idx;
	*c = aio_read(aio, address);
	return 0;
}

int aio_misc_get_audio_timestamp(void *hd, u32 idx, u32 *t)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address, map_idx;

	switch (idx) {
	case AIO_ATS_I2STX1:
		map_idx = 0;
		break;
	case AIO_ATS_I2STX2:
		map_idx = 1;
		break;
	case AIO_ATS_HDMITX:
		map_idx = 4;
		break;
	case AIO_ATS_HDMIARCTX:
		map_idx = 5;
		break;
	case AIO_ATS_SPDIFTX:
		map_idx = 2;
		break;
	case AIO_ATS_SPDIFTX1:
		map_idx = 3;
		break;
	case AIO_ATS_SPDIFRX:
		map_idx = 6;
		break;
	case AIO_ATS_I2SRX1:
		map_idx = 7;
		break;
	case AIO_ATS_I2SRX2:
		map_idx = 8;
		break;
	case AIO_ATS_I2SRX3:
		map_idx = 9;
		break;
	case AIO_ATS_I2SRX4:
		map_idx = 10;
		break;
	case AIO_ATS_I2SRX5:
		map_idx = 11;
		break;
	case AIO_ATS_PDMRX1:
		map_idx = 12;
		break;
		break;
	default:
		pr_err("%s, idx(%d) not supported\n", __func__, idx);
		return -EINVAL;
	}

	address = RA_AIO_STR + 4 * map_idx;
	*t = aio_read(aio, address);
	return 0;
}

int __weak aio_misc_sw_rst_extra(void *hd, u32 option, u32 val)
{
	return 0;
}

int aio_misc_sw_rst(void *hd, u32 option, u32 val)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_SW_RST reg;
	u32 address = RA_AIO_SW_RST;

	switch (option) {
	case AIO_SW_RST_SPDF:
		reg.u32[0] = aio_read(aio, address);
		reg.uSW_RST_SPFRX = val;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_SW_RST_REFCLK:
		reg.u32[0] = aio_read(aio, address);
		reg.uSW_RST_REFCLK = val;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_SW_RST_MIC3:
		reg.u32[0] = aio_read(aio, address);
		reg.uSW_RST_MIC3 = val;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_SW_RST_MIC4:
		reg.u32[0] = aio_read(aio, address);
		reg.uSW_RST_MIC4 = val;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_SW_RST_MIC5:
		reg.u32[0] = aio_read(aio, address);
		reg.uSW_RST_MIC5 = val;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_SW_RST_MIC6:
		aio_misc_sw_rst_extra(hd, option, val);
		break;
	default:
		pr_err("%s, option(%d) not supported\n", __func__, option);
		return -EINVAL;
	}

	return 0;
}

int __weak aio_misc_set_loopback_clk_gate_extra(void *hd, u32 idx, u32 en)
{
	return 0;
}

int aio_misc_set_loopback_clk_gate(void *hd, u32 idx, u32 en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_CLK_GATE_EN reg;
	u32 address = RA_AIO_CLK_GATE_EN;

	switch (idx) {
	case AIO_LOOPBACK_CLK_GATE_MIC3:
		reg.u32[0] = aio_read(aio, address);
		reg.uCLK_GATE_EN_MIC3 = en;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_LOOPBACK_CLK_GATE_MIC4:
		reg.u32[0] = aio_read(aio, address);
		reg.uCLK_GATE_EN_MIC4 = en;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_LOOPBACK_CLK_GATE_MIC5:
		reg.u32[0] = aio_read(aio, address);
		reg.uCLK_GATE_EN_MIC5 = en;
		aio_write(aio, address, reg.u32[0]);
		break;
	case AIO_LOOPBACK_CLK_GATE_MIC6:
		aio_misc_set_loopback_clk_gate_extra(hd, idx, en);
		break;
	default:
		pr_err("%s, idx(%d) not supported\n", __func__, idx);
		return -EINVAL;
	}

	return 0;
}

int aio_spdifi_enable_refclk(void *hd)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32SPDIFRX_CTRL_CTRL1 reg;
	u32 address = RA_AIO_SPDIFRX_CTRL + RA_SPDIFRX_CTRL_CTRL1;

	reg.u32 = aio_read(aio, address);
	reg.uCTRL1_REFCLK_GATE = 0x1;
	aio_write(aio, address, reg.u32);
	return 0;
}
