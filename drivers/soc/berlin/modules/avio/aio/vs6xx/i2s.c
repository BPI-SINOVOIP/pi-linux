// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "aio_hal.h"
#include "aio_common.h"
#include "i2s_common.h"
#include "avioDhub.h"
#include <linux/delay.h>

enum aio_hdmi_lpcm_src_option {
	AIO_HDMI_LPCM_SRC_64b = 1,
	AIO_HDMI_LPCM_SRC_256b = 3
};

#define __SET32MIC4_RXPORT_ENABLE(r32, v)	_BFSET_(r32, 0, 0, v)
#define __SET32MIC5_RXPORT_ENABLE(r32, v)	_BFSET_(r32, 0, 0, v)

static void aio_trace_reg(struct aio_priv *aio,
	const char *name, u32 add, u32 val)
{
	pr_debug("%s: %p@%08x --> %08x\n", name, aio->pbase, add, val);
}

u32 __weak get_intl_base_addr_extra(u32 id)
{
	return INVALID_ADDRESS;
}

u32 get_intl_base_addr(u32 id)
{
	u32 address;

	switch (id) {
	case AIO_ID_MIC1_RX:
		address = RA_AIO_MIC1 + RA_MIC1_INTLMODE;
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_MIC2 + RA_MIC2_INTLMODE;
		break;
	case AIO_ID_MIC6_RX:
		address = get_intl_base_addr_extra(id);
		break;
	default:
		/* will return AIO PRI address*/
		pr_warn("%s: id(%d) not supported\n", __func__, id);
		return INVALID_ADDRESS;
	}

	return address;
}

u32 __weak aio_get_offset_extra_plus(u32 ID, u32 tsd)
{
	return 0;
}

u32 aio_get_offset_extra(u32 ID, u32 tsd)
{
	u32 offset = 0;

	switch (ID) {
	case AIO_ID_HDMI_TX:
		offset = RA_AIO_HDMI + RA_HDMI_HDTSD;
		break;
	case AIO_ID_MIC4_RX:
		if (tsd == AIO_TSD0)
			offset = RA_AIO_MIC4 + RA_MIC4_RSD0;
		else
			offset = RA_AIO_MIC4 + RA_MIC4_RSD1 +
					sizeof(SIE_AUDCH) * (tsd - 1);
		break;
	case AIO_ID_MIC5_RX:
		if (tsd == AIO_TSD0)
			offset = RA_AIO_MIC5 + RA_MIC5_RSD0;
		else
			offset = RA_AIO_MIC5 + RA_MIC5_RSD1 +
					sizeof(SIE_AUDCH) * (tsd - 1);
		break;
	case AIO_ID_SPDIF_TX:
		offset = I2S_SPDIF_OFFSET;
		break;
	case AIO_ID_MIC6_RX:
		offset = aio_get_offset_extra_plus(ID, tsd);
		break;
	default:
		offset = 0;
		break;
	}

	return offset;
}

u32 __weak aio_get_addr_extra_plus(u32 id)
{
	return INVALID_ADDRESS;
}

u32 aio_get_addr_extra(u32 id)
{
	u32 address;

	switch (id) {
	case AIO_ID_HDMI_TX:
		address = RA_AIO_HDMI + RA_HDMI_HDAUD;
		break;
	case AIO_ID_MIC4_RX:
		address = RA_AIO_MIC4 + RA_MIC4_MICCTRL;
		break;
	case AIO_ID_MIC5_RX:
		address = RA_AIO_MIC5 + RA_MIC5_MICCTRL;
		break;
	case AIO_ID_MIC6_RX:
		address = aio_get_addr_extra_plus(id);
		break;
	default:
		/* will return AIO PRI address*/
		pr_warn("%s: id(%d) not supported\n", __func__, id);
		return INVALID_ADDRESS;
	}

	return address;
}

int aio_setirq(void *hd,
		u32 pri, u32 sec,
		u32 mic, u32 spdif, u32 hdmi)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32AIO_IRQENABLE reg;

	address = RA_AIO_IRQENABLE;
	reg.u32 = aio_read(aio, address);
	reg.uIRQENABLE_PRIIRQ = pri;
	reg.uIRQENABLE_SECIRQ = sec;
	reg.uIRQENABLE_MIC1IRQ = mic;
	reg.uIRQENABLE_MIC2IRQ = mic;
	reg.uIRQENABLE_SPDIFIRQ = spdif;
	reg.uIRQENABLE_HDMIIRQ = hdmi;
	aio_write(aio, address, reg.u32);
	return 0;
}
EXPORT_SYMBOL(aio_setirq);

int aio_selhdport(void *hd, u32 sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32HDMI_HDPORT reg;
	u32 address;

	address = RA_AIO_HDMI + RA_HDMI_HDPORT;
	reg.u32 = aio_read(aio, address);
	reg.uHDPORT_TXSEL = sel;
	aio_write(aio, address, reg.u32);
	return 0;
}
EXPORT_SYMBOL(aio_selhdport);

int aio_selhdsource(void *hd, u32 sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_HDSRC reg;
	u32 address;

	address = RA_AIO_HDSRC;
	reg.u32 = aio_read(aio, address);
	reg.uHDSRC_SEL = sel;
	aio_write(aio, address, reg.u32);
	return 0;
}
EXPORT_SYMBOL(aio_selhdsource);

int aio_i2s_set_clock(void *hd, u32 id, u32 clkSwitch,
	 u32 clkD3Switch, u32 clkSel, u32 pllUsed, u32 en)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32ACLK_ACLK_CTRL reg;
	u32 address;

	switch (id) {
	case AIO_ID_PRI_TX:
		address = RA_AIO_MCLKPRI;
		break;
	case AIO_ID_SEC_TX:
		address = RA_AIO_MCLKSEC;
		break;
	case AIO_ID_MIC_RX:
		address = RA_AIO_MCLKMIC1;
		break;
	case AIO_ID_MIC2_RX:
		address = RA_AIO_MCLKMIC2;
		break;
	case AIO_ID_HDMI_TX:
		address = RA_AIO_MCLKHD;
		break;
	case AIO_ID_SPDIF_TX:
		address = RA_AIO_MCLKSPF;
		break;
	case AIO_ID_PDM_RX:
		address = RA_AIO_MCLKPDM;
		break;
	default:
		pr_err("%s, id(%d) not supported\n", __func__, id);
		return -EINVAL;
	}

	reg.u32 = aio_read(aio, address);
	reg.uACLK_CTRL_clkSwitch = clkSwitch;
	reg.uACLK_CTRL_clkD3Switch = clkD3Switch;
	reg.uACLK_CTRL_clkSel = clkSel;
	reg.uACLK_CTRL_src_sel = pllUsed;
	reg.uACLK_CTRL_clk_Enable = en ? 1 : 0;
	/* toggle sw_sync_rst bit to do reset */
	reg.uACLK_CTRL_sw_sync_rst = 0;
	aio_write(aio, address, reg.u32);
	usleep_range(1000, 2000);
	reg.uACLK_CTRL_sw_sync_rst = 1;
	aio_write(aio, address, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_i2s_set_clock);

u32 aio_get_tsd_from_chid(void *hd, u32 chid)
{
	u32 tsd = MAX_TSD;

	switch (chid) {
	case avioDhubChMap_aio64b_MIC1_CH_W:
		tsd = AIO_TSD0;
		break;

	default:
		tsd = MAX_TSD;
	}

	return tsd;
}
EXPORT_SYMBOL(aio_get_tsd_from_chid);

int aio_set_xfeed_mode(void *hd, s32 i2sid, u32 lrclk, u32 bclk)
{
	struct aio_priv *aio = hd_to_aio(hd);
	T32AIO_XFEED reg;
	u32 offset;

	offset = RA_AIO_XFEED;
	reg.u32 = aio_read(aio, offset);

	switch (i2sid) {
	case I2S1_ID:
		reg.uXFEED_I2S1_LRCKIO_MODE = lrclk;
		reg.uXFEED_I2S1_BCLKIO_MODE = bclk;
		break;
	case I2S2_ID:
		reg.uXFEED_I2S2_LRCKIO_MODE = lrclk;
		reg.uXFEED_I2S2_BCLKIO_MODE = bclk;
		break;
	case I2S3_ID:
		reg.uXFEED_I2S3_LRCKIO_MODE = lrclk;
		reg.uXFEED_I2S3_BCLKIO_MODE = bclk;
		break;
	default:
		pr_err("%s, id(%d) is not supported\n", __func__, i2sid);
		return -EINVAL;
	}
	aio_write(aio, offset, reg.u32);

	return 0;
}

static void aio_set_prisrc(void *hd, u32 src, u8 *map_val)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_PRISRC reg;
	u32 address = RA_AIO_PRISRC;

	reg.u32[0] = aio_read(aio, address);
	reg.uPRISRC_SEL = src;

	reg.uPRISRC_L0DATAMAP = map_val[0];

	if (src > 0)
		reg.uPRISRC_L1DATAMAP = map_val[1];

	if (src > 1)
		reg.uPRISRC_L2DATAMAP = map_val[2];

	if (src > 2)
		reg.uPRISRC_L3DATAMAP = map_val[3];

	aio_write(aio, address, reg.u32[0]);
}

static void aio_set_secsrc(void *hd, u32 src, u8 *map_val)
{
	struct aio_priv *aio = hd_to_aio(hd);
	TAIO_SECSRC reg;
	u32 address = RA_AIO_SECSRC;

	reg.u32[0] = aio_read(aio, address);
	reg.uSECSRC_SEL = src;

	reg.uSECSRC_L0DATAMAP = map_val[0];

	if (src > 0)
		reg.uSECSRC_L1DATAMAP = map_val[1];

	if (src > 1)
		reg.uSECSRC_L2DATAMAP = map_val[2];

	if (src > 2)
		reg.uSECSRC_L3DATAMAP = map_val[3];

	aio_write(aio, address, reg.u32[0]);
}

int aio_set_interleaved_mode(void *hd, u32 id, u32 src, u32 ch_map)
{
	u8 map_val[4] = {ch_map & 0x3, (ch_map >> 2) & 0x3,
		(ch_map >> 4) & 0x3, (ch_map >> 6) & 0x3};
	u8 i;

	if (src > 3) {
		pr_err("%s, src(%d) not supported\n", __func__, src);
		return -EINVAL;
	}

	for (i = 0; i <= src; i++)
		if (map_val[i] > src) {
			pr_err("%s, map(0x%x) not supported\n",
				    __func__, ch_map);
			return -EINVAL;
		}

	switch (id) {
	case AIO_ID_PRI_TX:
		aio_set_prisrc(hd, src, map_val);
		break;
	case AIO_ID_SEC_TX:
		aio_set_secsrc(hd, src, map_val);
		break;
	default:
		pr_err("%s, id(%d) not supported\n", __func__, id);
		return -EINVAL;
	}

	return 0;
}

static int aio_get_pri_channel_num(u32 tdm, u32 tdmchcnt, u32 src)
{
	if (tdm)
		return (src + 1) * tdmchcnt;
	else
		return (src + 1)*2;
}

static int aio_configure_pri_lpbk(struct aio_priv *aio,
				 u8 chan_num, u8 dummy_data)
{
	TAIO_PRISRC reg_prisrc;
	T32PRIAUD_CLKDIV reg_clkdiv;
	T32PRIAUD_CTRL reg_mic4ctrl;
	T32PRIAUD_CTRL reg_prictrl;
	T32MIC4_HBRDMAP reg_hbrmap;
	T32MIC4_INTLMODE reg_intlmode;
	TMIC4_RXDATA  reg_rxdata;
	u32 address;

	/* CLKDIV */
	address = RA_AIO_MIC4 + RA_MIC4_MICCTRL + RA_PRIAUD_CLKDIV;
	reg_clkdiv.uCLKDIV_SETTING = 0;
	aio_write(aio, address, reg_clkdiv.u32);
	aio_trace_reg(aio, "CLKDIV", address, reg_clkdiv.u32);

	/* read HDMI i2s configure */
	address = RA_AIO_PRI + RA_PRI_PRIAUD + RA_PRIAUD_CTRL;
	reg_prictrl.u32 = aio_read(aio, address);
	aio_trace_reg(aio, "PRI ctrl", address, reg_prictrl.u32);
	/* MICCTRL register*/
	address = RA_AIO_MIC5 + RA_MIC4_MICCTRL + RA_PRIAUD_CTRL;
	reg_mic4ctrl.u32 = aio_read(aio, address);
	aio_trace_reg(aio, "MICCTRL", address, reg_mic4ctrl.u32);
	reg_mic4ctrl.u32 = reg_prictrl.u32;
	reg_mic4ctrl.uCTRL_INVCLK = 0;
	reg_mic4ctrl.uCTRL_TDM = AIO_32DFM;
	reg_mic4ctrl.uCTRL_TCF = AIO_32CFM;
	reg_mic4ctrl.uCTRL_TDMMODE = 0;
	aio_write(aio, address, reg_mic4ctrl.u32);
	aio_trace_reg(aio, "MICCTRL", address, reg_mic4ctrl.u32);

	/* RxData register*/
	reg_rxdata.uRXDATA_HBR = 0;
	reg_rxdata.uRXDATA_TDM_HR = reg_mic4ctrl.uCTRL_TDMMODE;
	address = RA_AIO_MIC4 + RA_MIC4_RXDATA;
	aio_write(aio, address, reg_rxdata.u32[0]);
	aio_trace_reg(aio, "RxData", address, reg_rxdata.u32[0]);

	/* INTLMODE */
	reg_prisrc.u32[0] = aio_read(aio, RA_AIO_PRISRC);
	address = RA_AIO_MIC4 + RA_MIC4_INTLMODE;
	reg_intlmode.u32 = 0;
	if (dummy_data) {
		reg_intlmode.uINTLMODE_DUMMYDATA_EN = 1;
		pr_info(
		"capture %d chan,%d chan with dummy data\n",
				chan_num, 8 - chan_num);
		chan_num = 8;
	} else {
		int pri_chn_num;

		pri_chn_num = aio_get_pri_channel_num(
				reg_prictrl.uCTRL_TDMMODE,
				reg_prictrl.uCTRL_TDMCHCNT,
				reg_prisrc.uPRISRC_SEL);

		if (chan_num > pri_chn_num) {
			pr_err("%d/%d,no dummy data, fail\n",
				   pri_chn_num, chan_num);
			return -EINVAL;
		}
	}

	reg_intlmode.uINTLMODE_PORT0_EN = 1;
	if (reg_prisrc.uPRISRC_SEL > 0)
		reg_intlmode.uINTLMODE_PORT1_EN = 1;

	if (reg_prisrc.uPRISRC_SEL > 1)
		reg_intlmode.uINTLMODE_PORT2_EN = 1;

	if (reg_prisrc.uPRISRC_SEL > 2)
		reg_intlmode.uINTLMODE_PORT3_EN = 1;

	aio_write(aio, address, reg_intlmode.u32);
	aio_trace_reg(aio, "INTLMODE", address, reg_intlmode.u32);

	/* HBRDMAP */
	address = RA_AIO_MIC4 + RA_MIC4_HBRDMAP;
	reg_hbrmap.u32 = 0;
	reg_hbrmap.uHBRDMAP_PORT3 = reg_prisrc.uPRISRC_L3DATAMAP;
	reg_hbrmap.uHBRDMAP_PORT2 = reg_prisrc.uPRISRC_L2DATAMAP;
	reg_hbrmap.uHBRDMAP_PORT1 = reg_prisrc.uPRISRC_L1DATAMAP;
	reg_hbrmap.uHBRDMAP_PORT0 = reg_prisrc.uPRISRC_L0DATAMAP;
	aio_write(aio, address, reg_hbrmap.u32);
	aio_trace_reg(aio, "HBRDMAP", address, reg_hbrmap.u32);
	return 0;
}

static int aio_configure_hdmi_lpbk(struct aio_priv *aio,
				 u8 chan_num, u8 dummy_data)
{
	T32PRIAUD_CTRL reg_hdmictrl;
	T32PRIAUD_CLKDIV reg_clkdiv;
	T32PRIAUD_CTRL reg_mic5ctrl;
	T32MIC5_HBRDMAP reg_hbrmap;
	T32MIC5_INTLMODE reg_intlmode;
	TMIC5_RXDATA  reg_rxdata;
	u32 address;

	/* CLKDIV */
	address = RA_AIO_MIC5 + RA_MIC5_MICCTRL + RA_PRIAUD_CLKDIV;
	reg_clkdiv.uCLKDIV_SETTING = 0;
	aio_write(aio, address, reg_clkdiv.u32);
	aio_trace_reg(aio, "CLKDIV", address, reg_clkdiv.u32);

	/* read PRI i2s configure */
	address = RA_AIO_HDMI + RA_HDMI_HDAUD + RA_PRIAUD_CTRL;
	reg_hdmictrl.u32 = aio_read(aio, address);
	aio_trace_reg(aio, "HDMI ctrl", address, reg_hdmictrl.u32);
	/* MICCTRL register*/
	address = RA_AIO_MIC5 + RA_MIC5_MICCTRL + RA_PRIAUD_CTRL;
	reg_mic5ctrl.u32 = aio_read(aio, address);
	aio_trace_reg(aio, "MICCTRL", address, reg_mic5ctrl.u32);
	reg_mic5ctrl.u32 = reg_hdmictrl.u32;
	reg_mic5ctrl.uCTRL_INVCLK = 0;/*noise without it*/
	reg_mic5ctrl.uCTRL_TDM = AIO_32DFM;
	reg_mic5ctrl.uCTRL_TCF = AIO_32CFM;
	reg_mic5ctrl.uCTRL_TDMMODE = 0;
	/* make remain same as HDMI i2s configure*/
	aio_write(aio, address, reg_mic5ctrl.u32);
	aio_trace_reg(aio, "MICCTRL", address, reg_mic5ctrl.u32);

	/* RxData register*/
	reg_rxdata.uRXDATA_HBR = 0;
	reg_rxdata.uRXDATA_TDM_HR = 0;
	address = RA_AIO_MIC5 + RA_MIC5_RXDATA;
	aio_write(aio, address, reg_rxdata.u32[0]);
	aio_trace_reg(aio, "RxData", address, reg_rxdata.u32[0]);

	/* INTLMODE */
	address = RA_AIO_MIC5 + RA_MIC5_INTLMODE;
	reg_intlmode.u32 = 0;
	if (dummy_data) {
		reg_intlmode.uINTLMODE_DUMMYDATA_EN = 1;
		pr_info(
		"capture %d chan,%d chan with dummy data\n",
				chan_num, 8 - chan_num);
		chan_num = 8;
	} else {
		T32AIO_HDSRC reg;

		reg.u32 = aio_read(aio, RA_AIO_HDSRC);
		if (reg.uHDSRC_SEL == AIO_HDMI_LPCM_SRC_64b &&
			chan_num > 2) {
			pr_err("%d chan, no dummy data, fail\n",
				   chan_num);
			return -EINVAL;
		}
	}
	chan_num = (chan_num + 1) / 2;

	reg_intlmode.uINTLMODE_PORT0_EN = 1;

	if (chan_num > 1)
		reg_intlmode.uINTLMODE_PORT1_EN = 1;

	if (chan_num > 2)
		reg_intlmode.uINTLMODE_PORT2_EN = 1;

	if (chan_num > 3)
		reg_intlmode.uINTLMODE_PORT3_EN = 1;

	aio_write(aio, address, reg_intlmode.u32);
	aio_trace_reg(aio, "INTLMODE", address, reg_intlmode.u32);

	/* HBRDMAP */
	address = RA_AIO_MIC5 + RA_MIC5_HBRDMAP;
	reg_hbrmap.u32 = 0;

	reg_hbrmap.uHBRDMAP_PORT0 = 0;

	if (chan_num > 1)
		reg_hbrmap.uHBRDMAP_PORT1 = 1;

	if (chan_num > 2)
		reg_hbrmap.uHBRDMAP_PORT2 = 2;

	if (chan_num > 3)
		reg_hbrmap.uHBRDMAP_PORT3 = 3;

	aio_write(aio, address, reg_hbrmap.u32);
	aio_trace_reg(aio, "HBRDMAP", address, reg_hbrmap.u32);

	return 0;
}

int aio_configure_loopback(void *hd, u32 id, u8 chan_num, u8 dummy_data)
{
	struct aio_priv *aio = hd_to_aio(hd);
	int ret = 0;

	switch (id) {
	case AIO_ID_MIC4_RX:
		ret = aio_configure_pri_lpbk(aio, chan_num, dummy_data);
		break;
	case AIO_ID_MIC5_RX:
		ret = aio_configure_hdmi_lpbk(aio, chan_num, dummy_data);
		break;
	default:
		pr_err("%s, id(%d) not supported\n", __func__, id);
		return -EINVAL;
	}
	return ret;
}

int aio_setspdif_en(void *hd, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32AUDCH_CTRL reg;

	address = RA_AIO_SPDIF + RA_SPDIF_SPDIF + RA_AUDCH_CTRL;
	reg.u32 = aio_read(aio, address);
	reg.uCTRL_ENABLE = enable;

	aio_write(aio, address, reg.u32);

	return 0;
}

int aio_setspdifclk(void *hd, u32 div)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32SPDIF_CLKDIV reg;

	address = RA_AIO_SPDIF + RA_SPDIF_CLKDIV;
	reg.u32 = aio_read(aio, address);
	reg.uCLKDIV_SETTING = div;
	aio_write(aio, address, reg.u32);

	return 0;
}
