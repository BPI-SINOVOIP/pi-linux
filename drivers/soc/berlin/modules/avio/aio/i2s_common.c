// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "i2s_common.h"
#include "aio_hal.h"
#include "aio_common.h"
#include "avioDhub.h"


bool aio_ch_en[AIO_ID_MAX_NUM];

enum aio_ch_operation {
	AIO_CH_EN,
	AIO_CH_MUTE,
	AIO_CH_FLUSH
};

u32 __weak aio_get_addr_extra(u32 id)
{
	return INVALID_ADDRESS;
}

u32 __weak aio_get_offset_extra(u32 ID, u32 tsd)
{
	return 0;
}

static u32 aio_get_offset_from_ch(
	struct aio_priv *aio, u32 ID, u32 tsd)
{
	u32 offset = 0;

	switch (ID) {
	case AIO_ID_PRI_TX:
		offset = I2S_PRI_OFFSET + sizeof(SIE_AUDCH) * tsd;
		break;
	case AIO_ID_SEC_TX:
		offset =  I2S_SEC_OFFSET;
		break;
	case AIO_ID_MIC_RX:
		if (tsd == AIO_TSD0)
			offset = RA_AIO_MIC1 + RA_MIC1_RSD0;
		else
			offset = RA_AIO_MIC1 + RA_MIC1_RSD1 +
					sizeof(SIE_AUDCH) * (tsd - 1);
		break;
	case AIO_ID_MIC2_RX:
		offset = I2S_MIC2_OFFSET;
		break;
	case AIO_ID_SPDIF_TX:
	case AIO_ID_HDMI_TX:
	case AIO_ID_MIC4_RX:
	case AIO_ID_MIC5_RX:
	case AIO_ID_MIC6_RX:
		offset = aio_get_offset_extra(ID, tsd);
		break;
	default:
		offset = 0;
		break;
	}

	return offset;
}

static u32 aio_get_audch_ctrl(void *hd, u32 id, u32 tsd, u32 op)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset, setting = 0;
	T32AUDCH_CTRL reg;

	offset = aio_get_offset_from_ch(aio, id, tsd);

	if (offset == 0)
		return 0;
	offset += RA_AUDCH_CTRL;
	reg.u32 = aio_read(aio, offset);
	switch (op) {
	case AIO_CH_EN:
		setting = reg.uCTRL_ENABLE;
		break;
	case AIO_CH_MUTE:
		setting = reg.uCTRL_MUTE;
		break;
	case AIO_CH_FLUSH:
		setting = reg.uCTRL_FLUSH;
		break;
	default:
		break;
	}

	return setting;
}

static void aio_set_audch_ctrl(void *hd, u32 id, u32 tsd, u32 op, u32 setting)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	T32AUDCH_CTRL reg;

	offset = aio_get_offset_from_ch(aio, id, tsd);

	if (offset == 0)
		return;
	offset += RA_AUDCH_CTRL;
	reg.u32 = aio_read(aio, offset);
	switch (op) {
	case AIO_CH_EN:
		reg.uCTRL_ENABLE = setting;
		break;
	case AIO_CH_MUTE:
		reg.uCTRL_MUTE = setting;
		break;
	case AIO_CH_FLUSH:
		reg.uCTRL_FLUSH = setting;
		break;
	default:
		break;
	}
	aio_write(aio, offset, reg.u32);
}

bool aio_get_aud_ch_en(void *hd, u32 id, u32 tsd)
{
	return aio_get_audch_ctrl(hd, id, tsd, AIO_CH_EN);
}
EXPORT_SYMBOL(aio_get_aud_ch_en);

void aio_set_aud_ch_en(void *hd, u32 id, u32 tsd, bool enable)
{
	/* This enable FSYNC/LRCK on the audio port */
	aio_set_audch_ctrl(hd, id, tsd, AIO_CH_EN, enable);
	pr_debug("%s: id:%d, tsd:%d, enable:%d\n", __func__, id, tsd, enable);
}
EXPORT_SYMBOL(aio_set_aud_ch_en);

void aio_set_i2s_ch_en(u32 id, bool enable)
{
	aio_ch_en[id] = enable;
}
EXPORT_SYMBOL(aio_set_i2s_ch_en);

bool aio_get_i2s_ch_en(u32 id)
{
	return aio_ch_en[id];
}
EXPORT_SYMBOL(aio_get_i2s_ch_en);

void aio_set_aud_ch_mute(void *hd, u32 id, u32 tsd, u32 mute)
{
	aio_set_audch_ctrl(hd, id, tsd, AIO_CH_MUTE, mute);
}
EXPORT_SYMBOL(aio_set_aud_ch_mute);

void aio_set_aud_ch_flush(void *hd, u32 id, u32 tsd, u32 flush)
{
	aio_set_audch_ctrl(hd, id, tsd, AIO_CH_FLUSH, flush);
}
EXPORT_SYMBOL(aio_set_aud_ch_flush);

static u32 get_ctrl_base_addr(u32 id)
{
	u32 address;

	switch (id) {
	case AIO_ID_MIC_RX:
		address = RA_AIO_MIC1 + RA_MIC1_MICCTRL;
		break;
	case AIO_ID_PRI_TX:
		address = RA_AIO_PRI + RA_PRI_PRIAUD;
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_SEC + RA_SEC_SECAUD;
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_MIC2 + RA_MIC2_MICCTRL;
		break;
	case AIO_ID_HDMI_TX:
	case AIO_ID_MIC4_RX:
	case AIO_ID_MIC5_RX:
	case AIO_ID_MIC6_RX:
		address = aio_get_addr_extra(id);
		break;
	default:
		/* will return AIO PRI address*/
		pr_warn("%s: id(%d) not supported\n", __func__, id);
		return INVALID_ADDRESS;
	}

	return address;
}

static u32 get_ctrl_address(u32 id)
{
	u32 address;

	address = get_ctrl_base_addr(id);
	if (address == INVALID_ADDRESS)
		return address;

	address += RA_PRIAUD_CTRL;
	return address;
}

u32 __weak get_intl_base_addr(u32 id)
{
	return INVALID_ADDRESS;
}

static u32 get_intl_address(u32 id)
{
	u32 address;

	address = get_intl_base_addr(id);

	return address;
}

void aio_set_data_fmt(void *hd, u32 id, u32 data_fmt)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = get_ctrl_address(id);
	T32PRIAUD_CTRL reg;

	if (address == INVALID_ADDRESS)
		return;

	reg.u32 = aio_read(aio, address);
	switch (data_fmt) {
	case AIO_I2S_MODE:
		reg.uCTRL_TFM = PRIAUD_CTRL_TFM_I2S;
		reg.uCTRL_LEFTJFY = data_fmt >> 4;
		break;
	case AIO_LEFT_JISTIFIED_MODE:
		reg.uCTRL_TFM = PRIAUD_CTRL_TFM_JUSTIFIED;
		reg.uCTRL_LEFTJFY = data_fmt >> 4;
		break;
	case AIO_RIGHT_JISTIFIED_MODE:
		reg.uCTRL_TFM = PRIAUD_CTRL_TFM_JUSTIFIED;
		reg.uCTRL_LEFTJFY = data_fmt >> 4;
		break;
	default:
		return;
	}

	aio_write(aio, address, reg.u32);
}
EXPORT_SYMBOL(aio_set_data_fmt);

/* TCF: half period of FSYNC (sampling rate) in terms of number of bit-clocks */
void aio_set_width_word(void *hd, u32 id, u32 width_word)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = get_ctrl_address(id);
	T32PRIAUD_CTRL reg;

	if (address == INVALID_ADDRESS)
		return;
	reg.u32 = aio_read(aio, address);
	reg.uCTRL_TCF = width_word;
	aio_write(aio, address, reg.u32);
}
EXPORT_SYMBOL(aio_set_width_word);

/* TDM: channel resolution (number of valid bits in a half period of FSYNC) */
void aio_set_width_sample(void *hd, u32 id, u32 width_sample)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = get_ctrl_address(id);
	T32PRIAUD_CTRL reg;

	if (address == INVALID_ADDRESS)
		return;
	reg.u32 = aio_read(aio, address);
	reg.uCTRL_TDM = width_sample;
	aio_write(aio, address, reg.u32);
}
EXPORT_SYMBOL(aio_set_width_sample);

void aio_set_invclk(void *hd, u32 id, bool invclk)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = get_ctrl_address(id);
	T32PRIAUD_CTRL reg;

	if (address == INVALID_ADDRESS)
		return;
	reg.u32 = aio_read(aio, address);
	reg.uCTRL_INVCLK = invclk;
	aio_write(aio, address, reg.u32);
}
EXPORT_SYMBOL(aio_set_invclk);

void aio_set_invfs(void *hd, u32 id, bool invfs)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = get_ctrl_address(id);
	T32PRIAUD_CTRL reg;

	if (address == INVALID_ADDRESS)
		return;
	reg.u32 = aio_read(aio, address);
	reg.uCTRL_INVFS = invfs;
	aio_write(aio, address, reg.u32);
}
EXPORT_SYMBOL(aio_set_invfs);

void aio_set_tdm(void *hd, u32 id, bool en, u32 chcnt, u32 wshigh)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = get_ctrl_address(id);
	T32PRIAUD_CTRL reg;

	switch (id) {
	case AIO_ID_PRI_TX:
	case AIO_ID_SEC_TX:
	case AIO_ID_MIC6_RX:
		break;
	default:
		pr_info("%s: id(%d) not supported\n", __func__, id);
		return;
	}

	if (address == INVALID_ADDRESS)
		return;

	reg.u32 = aio_read(aio, address);
	reg.uCTRL_TDMMODE = en;
	reg.uCTRL_TDMCHCNT = chcnt;
	reg.uCTRL_TDMWSHIGH = wshigh;
	reg.uCTRL_TCF_MANUAL = 0;
	aio_write(aio, address, reg.u32);
}
EXPORT_SYMBOL(aio_set_tdm);

void aio_set_ctl(void *hd,
		u32 id, u32 data_fmt,
		u32 width_word, u32 width_sample)
{
	switch (id) {

	case AIO_ID_MIC_RX:
		aio_set_invfs(hd, id, PRIAUD_CTRL_INVFS_NORMAL);
		aio_set_invclk(hd, id, PRIAUD_CTRL_INVCLK_NORMAL);
		break;
	case AIO_ID_PRI_TX:
		aio_set_invfs(hd, id, PRIAUD_CTRL_INVFS_INVERTED);
		aio_set_invclk(hd, id, PRIAUD_CTRL_INVCLK_INVERTED);
		break;
	case AIO_ID_SEC_TX:
		aio_set_invfs(hd, id, PRIAUD_CTRL_INVFS_INVERTED);
		aio_set_invclk(hd, id, PRIAUD_CTRL_INVCLK_INVERTED);
		break;
	case AIO_ID_HDMI_TX:
		aio_set_invfs(hd, id, PRIAUD_CTRL_INVFS_INVERTED);
		aio_set_invclk(hd, id, PRIAUD_CTRL_INVCLK_INVERTED);
		break;
	case AIO_ID_MIC2_RX:
		aio_set_invfs(hd, id, PRIAUD_CTRL_INVFS_NORMAL);
		aio_set_invclk(hd, id, PRIAUD_CTRL_INVCLK_INVERTED);
		break;
	case AIO_ID_MIC6_RX:
		aio_set_invfs(hd, id, PRIAUD_CTRL_INVFS_NORMAL);
		aio_set_invclk(hd, id, PRIAUD_CTRL_INVCLK_INVERTED);
		break;
	default:
		pr_err("%s , %d not support\n", __func__, id);
		return;
	}

	aio_set_data_fmt(hd, id, data_fmt);
	aio_set_width_sample(hd, id, width_sample);
	aio_set_width_word(hd, id, width_word);
}
EXPORT_SYMBOL(aio_set_ctl);

int aio_enabletxport(void *hd, u32 id, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32PRI_PRIPORT reg_pri;
	T32SEC_SECPORT reg_sec;

	switch (id) {
	case AIO_ID_PRI_TX:
		address = RA_AIO_PRI + RA_PRI_PRIPORT;
		reg_pri.u32 = aio_read(aio, address);
		reg_pri.uPRIPORT_ENABLE = enable;
		aio_write(aio, address, reg_pri.u32);
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_SEC + RA_SEC_SECPORT;
		reg_sec.u32 = aio_read(aio, address);
		reg_sec.uSECPORT_ENABLE = enable;
		aio_write(aio, address, reg_sec.u32);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(aio_enabletxport);

int __weak aio_enablerxport_extra(void *hd, u32 id, bool enable)
{
	return 0;
}

int aio_enablerxport(void *hd, u32 id, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32MIC1_RXPORT reg_mic1;
	T32MIC2_RXPORT reg_mic2;

	switch (id) {
	case AIO_ID_MIC1_RX:
		address = RA_AIO_MIC1 + RA_MIC1_RXPORT;
		reg_mic1.u32 = aio_read(aio, address);
		reg_mic1.uRXPORT_ENABLE = enable;
		aio_write(aio, address, reg_mic1.u32);
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_MIC2 + RA_MIC2_RXPORT;
		reg_mic2.u32 = aio_read(aio, address);
		reg_mic2.uRXPORT_ENABLE = enable;
		aio_write(aio, address, reg_mic2.u32);
		break;
	case AIO_ID_MIC4_RX:
	case AIO_ID_MIC5_RX:
	case AIO_ID_MIC6_RX:
		aio_enablerxport_extra(hd, id, enable);
		break;
	default:
		pr_err("%s , %d not support\n", __func__, id);
		break;
	}
	return 0;
}
EXPORT_SYMBOL(aio_enablerxport);

static void mute_aio(struct aio_priv *aio, u32 id)
{
	u32 tsd;
	T32AUDCH_CTRL reg;
	u32 address;
	u32 tsd_end = AIO_TSD3;

	switch (id) {
	case AIO_ID_PRI_TX:
	case AIO_ID_MIC_RX:
	case AIO_ID_MIC4_RX:
	case AIO_ID_MIC5_RX:
	case AIO_ID_MIC6_RX:
		tsd_end = AIO_TSD3;
		break;

	case AIO_ID_SEC_TX:
	case AIO_ID_HDMI_TX:
	case AIO_ID_SPDIF_TX:
	case AIO_ID_MIC2_RX:
		tsd_end = AIO_TSD1;
		break;
	default:
		break;
	}

	for (tsd = AIO_TSD0; tsd < tsd_end; tsd++) {
		address = (aio_get_offset_from_ch(aio, id, tsd) +
				   RA_AUDCH_CTRL);
		reg.u32 = aio_read(aio, address);
		reg.uCTRL_MUTE = AUDCH_CTRL_MUTE_MUTE_ON;
		reg.uCTRL_ENABLE = AUDCH_CTRL_ENABLE_DISABLE;
		aio_write(aio, address, reg.u32);
	}
}

int aio_reset(void *hd)
{
	struct aio_priv *aio = hd_to_aio(hd);

	aio_enabletxport(hd, AIO_ID_PRI_TX, 0);
	aio_enablerxport(hd, AIO_ID_MIC1_RX, 0);
	aio_enablerxport(hd, AIO_ID_MIC2_RX, 0);
	aio_enablerxport(hd, AIO_ID_MIC6_RX, 0);

	mute_aio(aio, AIO_ID_PRI_TX);
	mute_aio(aio, AIO_ID_SEC_TX);
	mute_aio(aio, AIO_ID_MIC_RX);
	mute_aio(aio, AIO_ID_MIC2_RX);
	mute_aio(aio, AIO_ID_MIC6_RX);
	mute_aio(aio, AIO_ID_HDMI_TX);
	return 0;
}
EXPORT_SYMBOL(aio_reset);

int aio_setclkdiv(void *hd, u32 id, u32 div)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32PRIAUD_CLKDIV reg;

	address = get_ctrl_base_addr(id);
	if (address == INVALID_ADDRESS)
		return -EFAULT;

	address += RA_PRIAUD_CLKDIV;
	reg.u32 = aio_read(aio, address);
	/*
	 * Audio Master Clock (MCLK) Divider register
	 * decides the ratio between MCLK and Audio Bit Clock (BCLK).
	 */
	reg.uCLKDIV_SETTING = div;
	aio_write(aio, address, reg.u32);
	return 0;
}
EXPORT_SYMBOL(aio_setclkdiv);

int __weak aio_setspdifclk(void *hd, u32 div)
{
	return 0;
}
EXPORT_SYMBOL(aio_setspdifclk);

int __weak aio_setspdif_en(void *hd, bool enable)
{
	return 0;
}
EXPORT_SYMBOL(aio_setspdif_en);

int aio_getirqsts(void *hd)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_IRQSTS reg;

	reg.u32 = aio_read(aio, RA_AIO_IRQSTS);
	return reg.u32;
}
EXPORT_SYMBOL(aio_getirqsts);

int aio_set_pdmmicsel(void *hd, u32 sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address = RA_AIO_PDM_MIC_SEL;
	T32AIO_PDM_MIC_SEL reg;

	if ((aio == NULL) || IS_INVALID_1BIT_VAL(sel))
		return -EINVAL;

	/*
	 * ctrl.b0: 1 if MIC1 1-2 Channel is active,
	 *			0 if PDM 1-2 Channel is active
	 * ctrl.b1: 1 if MIC1 1-2 Channel is active,
	 *			0 if PDM 1-2 Channel is active
	 * ctrl.b2: 1 if MIC1 1-2 Channel is active,
	 *			0 if PDM 1-2 Channel is active
	 * ctrl.b3: 1 if MIC1 1-2 Channel is active,
	 *			0 if PDM 1-2 Channel is active
	 */
	reg.u32 = aio_read(aio, address);
	reg.uPDM_MIC_SEL_CTRL = sel;
	aio_write(aio, address, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_set_pdmmicsel);

static void aio_set_aud_ctrl(struct aio_priv *aio, u32 address, struct aud_ctrl *ctrl)
{
	T32PRIAUD_CTRL reg;

	reg.u32 = aio_read(aio, address);

	reg.uCTRL_LEFTJFY = !ctrl->isleftjfy;
	reg.uCTRL_INVCLK = ctrl->invbclk;
	reg.uCTRL_INVFS = ctrl->invfs;
	reg.uCTRL_TLSB = !ctrl->msb;
	reg.uCTRL_TDM = ctrl->sample_resolution;
	reg.uCTRL_TCF = ctrl->sample_period_in_bclk;
	reg.uCTRL_TFM = ctrl->data_fmt;
	reg.uCTRL_TDMMODE = ctrl->istdm;
	if (ctrl->istdm) {
		/* 0: short frame, 1: long frame */
		reg.uCTRL_TDMWSHIGH = ctrl->islframe;
		/* 2/4/6/8 channels */
		reg.uCTRL_TDMCHCNT = (ctrl->chcnt > 1) ? (ctrl->chcnt >> 1) - 1 : 0;
	}

	aio_write(aio, address, reg.u32);
}

int aio_set_ctl_ext(void *hd,
		u32 id, struct aud_ctrl *ctrl)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;

	if ((!aio) || (!ctrl) || IS_INVALID_ID(id))
		return -EINVAL;

	address = get_ctrl_address(id);
	aio_set_aud_ctrl(aio, address, ctrl);

	return 0;
}
EXPORT_SYMBOL(aio_set_ctl_ext);

int aio_set_bclk_sel(void *hd, u32 id, u32 sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32IOSEL_PRIBCLK reg_pri;
	T32IOSEL_SECBCLK reg_sec;
	T32IOSEL_MIC1BCLK reg_mic1;
	T32IOSEL_MIC2BCLK reg_mic2;

	/*
	 * sel is 2 bits number,
	 * 1 Bit Clock can be generated from Master Clock;
	 * 0 can be taken directly from the input.
	 */
	if ((aio == NULL) || IS_INVALID_ID(id) || IS_INVALID_2BITS_VAL(sel))
		return -EINVAL;


	switch (id) {
	case AIO_ID_PRI_TX:
		address = RA_AIO_IOSEL + RA_IOSEL_PRIBCLK;
		reg_pri.u32 = aio_read(aio, address);
		reg_pri.uPRIBCLK_SEL = sel;
		aio_write(aio, address, reg_pri.u32);
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_IOSEL + RA_IOSEL_SECBCLK;
		reg_sec.u32 = aio_read(aio, address);
		reg_sec.uSECBCLK_SEL = sel;
		aio_write(aio, address, reg_sec.u32);
		break;
	case AIO_ID_MIC1_RX:
		address = RA_AIO_IOSEL + RA_IOSEL_MIC1BCLK;
		reg_mic1.u32 = aio_read(aio, address);
		reg_mic1.uMIC1BCLK_SEL = sel;
		aio_write(aio, address, reg_mic1.u32);
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_IOSEL + RA_IOSEL_MIC2BCLK;
		reg_mic2.u32 = aio_read(aio, address);
		reg_mic2.uMIC2BCLK_SEL = sel;
		aio_write(aio, address, reg_mic2.u32);
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(aio_set_bclk_sel);

int aio_set_bclk_inv(void *hd, u32 id, u32 inv)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32IOSEL_PRIBCLK reg_pri;
	T32IOSEL_SECBCLK reg_sec;
	T32IOSEL_MIC1BCLK reg_mic1;
	T32IOSEL_MIC2BCLK reg_mic2;

	/*
	 * inv is 1 bit number,
	 */
	if ((aio == NULL) || IS_INVALID_ID(id) || IS_INVALID_1BIT_VAL(inv))
		return -EINVAL;

	switch (id) {
	case AIO_ID_PRI_TX:
		address = RA_AIO_IOSEL + RA_IOSEL_PRIBCLK;
		reg_pri.u32 = aio_read(aio, address);
		reg_pri.uPRIBCLK_SEL = inv;
		aio_write(aio, address, reg_pri.u32);
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_IOSEL + RA_IOSEL_SECBCLK;
		reg_sec.u32 = aio_read(aio, address);
		reg_sec.uSECBCLK_SEL = inv;
		aio_write(aio, address, reg_sec.u32);
		break;
	case AIO_ID_MIC1_RX:
		address = RA_AIO_IOSEL + RA_IOSEL_MIC1BCLK;
		reg_mic1.u32 = aio_read(aio, address);
		reg_mic1.uMIC1BCLK_SEL = inv;
		aio_write(aio, address, reg_mic1.u32);
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_IOSEL + RA_IOSEL_MIC2BCLK;
		reg_mic2.u32 = aio_read(aio, address);
		reg_mic2.uMIC2BCLK_SEL = inv;
		aio_write(aio, address, reg_mic2.u32);
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(aio_set_bclk_inv);

int aio_set_fsync(void *hd, u32 id, u32 sel, u32 inv)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32IOSEL_PRIFSYNC reg_pri;
	T32IOSEL_SECFSYNC reg_sec;
	T32IOSEL_MIC1FSYNC reg_mic1;
	T32IOSEL_MIC2FSYNC reg_mic2;

	if ((aio == NULL) || IS_INVALID_ID(id)
		|| IS_INVALID_1BIT_VAL(sel) || IS_INVALID_1BIT_VAL(inv)) {
		return -EINVAL;
	}

	switch (id) {
	case AIO_ID_PRI_TX:
		address = RA_AIO_IOSEL + RA_IOSEL_PRIFSYNC;
		reg_pri.u32 = aio_read(aio, address);
		reg_pri.uPRIFSYNC_SEL = sel;
		reg_pri.uPRIFSYNC_INV = inv;
		aio_write(aio, address, reg_pri.u32);
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_IOSEL + RA_IOSEL_SECFSYNC;
		reg_sec.u32 = aio_read(aio, address);
		reg_sec.uSECFSYNC_SEL = sel;
		reg_sec.uSECFSYNC_INV = inv;
		aio_write(aio, address, reg_sec.u32);
		break;
	case AIO_ID_MIC1_RX:
		address = RA_AIO_IOSEL + RA_IOSEL_MIC1FSYNC;
		reg_mic1.u32 = aio_read(aio, address);
		reg_mic1.uMIC1FSYNC_SEL = sel;
		reg_mic1.uMIC1FSYNC_INV = inv;
		aio_write(aio, address, reg_mic1.u32);
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_IOSEL + RA_IOSEL_MIC2FSYNC;
		reg_mic2.u32 = aio_read(aio, address);
		reg_mic2.uMIC2FSYNC_SEL = sel;
		reg_mic2.uMIC2FSYNC_INV = inv;
		aio_write(aio, address, reg_mic2.u32);
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(aio_set_fsync);

int aio_set_mic1_mm_mode(void *hd, u32 en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32MIC1_MM_MODE reg;

	/*
	 * en is 1 bit number,
	 * bydeafult is 0;
	 * 1 Receiver in Master Mode to generate internal generated WS/FSYNC.
	 */
	if ((aio == NULL) || IS_INVALID_1BIT_VAL(en))
		return -EINVAL;

	address = RA_AIO_MIC1 + RA_MIC1_MM_MODE;
	reg.u32 = aio_read(aio, address);
	reg.uMM_MODE_RCV_MASTER = en;
	aio_write(aio, address, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_set_mic1_mm_mode);


/* WS (Fsync/LRCK) period setting when receiver is set to Master Mode and WS generator
 * acts as a standalone generator.
 *
 *  highP: High Cycle value for the FSYNC generation. writing 31 to this means the FSYNC
 *         is high for 32 BCLK
 *
 *  totalP: Total FSYNC Period. Writing 63 to this means the total FSYNC Period is 64 BCLK
 *
 *  wsInv:  To invert the FSYNC
 *      0 : Left Channel is High, Right Channel Low (LJ/RJ/TDM)
 *      1 : Right Channel is High, Left Channel Low (I2S)
 *
 */
int aio_set_mic1_ws_prd(void *hd, u32 highP, u32 totalP, u32 wsInv)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32MIC1_MM_MODE reg;

	if (aio == NULL)
		return -EINVAL;

	address = RA_AIO_MIC1 + RA_MIC1_MM_MODE;
	reg.u32 = aio_read(aio, address);
	reg.uMM_MODE_WS_HIGH_PRD = highP;
	reg.uMM_MODE_WS_TOTAL_PRD = totalP;
	reg.uMM_MODE_WS_INV = wsInv;
	aio_write(aio, address, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_set_mic1_ws_prd);


int aio_set_mic_intlmode(void *hd, u32 id, u32 tsd, u32 en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32MIC1_INTLMODE reg;

	/*
	 * inv is 1 bit number
	 * 0: default value
	 * 1: run in Interleaved Mode
	 */
	if ((aio == NULL) || IS_INVALID_ID(id)
		|| IS_INVALID_ID(tsd) || IS_INVALID_1BIT_VAL(en)) {
		return -EINVAL;
	}

	address = get_intl_address(id);

	if (address == INVALID_ADDRESS)
		return -EINVAL;

	reg.u32 = aio_read(aio, address);
	switch (tsd) {
	case AIO_TSD0:
		reg.uINTLMODE_PORT0_EN = en;
		break;
	case AIO_TSD1:
		reg.uINTLMODE_PORT1_EN = en;
		break;
	case AIO_TSD2:
		reg.uINTLMODE_PORT2_EN = en;
		break;
	case AIO_TSD3:
		reg.uINTLMODE_PORT3_EN = en;
		break;
	default:
		break;
	}
	aio_write(aio, address, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_set_mic_intlmode);

int aio_set_slave_mode(void *hd, u32 id, u32 mod)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32AIO_PRISRC reg_pri;
	T32AIO_SECSRC reg_sec;

	if ((aio == NULL) || IS_INVALID_ID(id) || IS_INVALID_ID(mod))
		return -EINVAL;

	if (id == AIO_ID_PRI_TX) {
		address = RA_AIO_PRISRC;
		reg_pri.u32 = aio_read(aio, address);
		reg_pri.uPRISRC_SLAVEMODE = mod;
		aio_write(aio, address, reg_pri.u32);
	} else if (id == AIO_ID_SEC_TX) {
		address = RA_AIO_SECSRC;
		reg_sec.u32 = aio_read(aio, address);
		reg_sec.uSECSRC_SLAVEMODE = mod;
		aio_write(aio, address, reg_sec.u32);
	}

	return 0;
}
EXPORT_SYMBOL(aio_set_slave_mode);

int aio_set_pcm_mono(void *hd,
		u32 id, bool en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	TPRIAUD_CTRL reg;

	if (aio == NULL)
		return -EINVAL;

	switch (id) {
	case AIO_ID_PRI_TX:
		address = RA_AIO_PRI + RA_PRI_PRIAUD + RA_PRIAUD_CTRL;
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_SEC + RA_SEC_SECAUD + RA_PRIAUD_CTRL1;
		break;
	case AIO_ID_MIC_RX:
		address = RA_AIO_MIC1 + RA_MIC1_MICCTRL + RA_PRIAUD_CTRL1;
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_MIC2 + RA_MIC2_MICCTRL + RA_PRIAUD_CTRL1;
		break;
	default:
		return -EINVAL;
	}

	reg.u32[1] = aio_read(aio, address);
	reg.uCTRL_PCM_MONO_CH = en;
	aio_write(aio, address, reg.u32[1]);
	return 0;
}
EXPORT_SYMBOL(aio_set_pcm_mono);

int __weak aio_set_xfeed_mode(void *hd, s32 i2sid, u32 lrclk, u32 bclk)
{
	return 0;
}
EXPORT_SYMBOL(aio_set_xfeed_mode);

int __weak aio_set_interleaved_mode(void *hd, u32 id, u32 src, u32 ch_map)
{
	return 0;
}
EXPORT_SYMBOL(aio_set_interleaved_mode);

int __weak aio_configure_loopback(void *hd, u32 id, u8 chan_num, u8 dummy_data)
{
	return 0;
}
EXPORT_SYMBOL(aio_configure_loopback);
