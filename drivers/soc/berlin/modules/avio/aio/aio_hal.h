// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _AIO_HAL_H_
#define _AIO_HAL_H_
#include <linux/module.h>
#include <linux/kernel.h>

#include "pdm_common.h"
#include "aio_dmic_hal.h"
#include "spdifi_common.h"

enum aio_id {
	AIO_ID_PRI_TX = 0,
	AIO_ID_SEC_TX = 1,
	AIO_ID_MIC1_RX = 2,
	AIO_ID_SPDIF_TX = 3,
	AIO_ID_HDMI_TX = 4,
	AIO_ID_MIC2_RX = 5,
	AIO_ID_MIC4_RX = 6,
	AIO_ID_MIC5_RX = 7,
	AIO_ID_MIC6_RX = 8,
	AIO_ID_PDM_RX = 9,
};
#define AIO_ID_MIC_RX (AIO_ID_MIC1_RX)

enum aio_tsd {
	AIO_TSD0 = 0,
	AIO_TSD1,
	AIO_TSD2,
	AIO_TSD3,
	MAX_TSD
};

#define TSD_START	AIO_TSD0
#define TSD_END		AIO_TSD3

enum aio_i2s_mode {
	AIO_DSP_MODE = 0x00,
	AIO_LEFT_JISTIFIED_MODE = 0x01,
	AIO_RIGHT_JISTIFIED_MODE = 0x11,
	AIO_I2S_MODE = 0x02
};

enum aio_div {
	AIO_DIV1	= 0x0,
	AIO_DIV2	= 0x1,
	AIO_DIV4	= 0x2,
	AIO_DIV8	= 0x3,
	AIO_DIV16	= 0x4,
	AIO_DIV32	= 0x5,
	AIO_DIV64	= 0x6,
	AIO_DIV128	= 0x7
};

enum aio_dfm {
	AIO_16DFM = 0,
	AIO_18DFM,
	AIO_20DFM,
	AIO_24DFM,
	AIO_32DFM,
	AIO_8DFM,
};

enum aio_cfm {
	AIO_16CFM = 0,
	AIO_24CFM,
	AIO_32CFM,
	AIO_8CFM,
};

enum aio_apll {
	AIO_APLL_0 = 0,
	AIO_APLL_1,
};

enum aio_audio_time_stamp {
	AIO_ATS_I2STX1 = 0,
	AIO_ATS_I2STX2,
	AIO_ATS_HDMITX,
	AIO_ATS_HDMIARCTX,
	AIO_ATS_SPDIFTX,
	AIO_ATS_SPDIFTX1,
	AIO_ATS_SPDIFRX,
	AIO_ATS_I2SRX1,
	AIO_ATS_I2SRX2,
	AIO_ATS_I2SRX3,
	AIO_ATS_I2SRX4,
	AIO_ATS_I2SRX5,
	AIO_ATS_PDMRX1,
};

enum aio_loopback_clk_gate {
	AIO_LOOPBACK_CLK_GATE_MIC3 = 0,
	AIO_LOOPBACK_CLK_GATE_MIC4,
	AIO_LOOPBACK_CLK_GATE_MIC5,
	AIO_LOOPBACK_CLK_GATE_MIC6,
};

enum aio_sw_rst_option {
	AIO_SW_RST_SPDF = 0,
	AIO_SW_RST_REFCLK,
	AIO_SW_RST_MIC3,
	AIO_SW_RST_MIC4,
	AIO_SW_RST_MIC5,
	AIO_SW_RST_MIC6,
};

/* for I2S/TDM AUD_CTRL */
struct aud_ctrl {
	u32 chcnt;

	u32 width_word;
	u32 width_sample;

	/* decide the delay cycles in bit clock for
	 * data sent/received when fs is valid to transition.
	 */
	u32 data_fmt;

	bool isleftjfy;
	bool invbclk;
	bool invfs;
	bool msb;

	bool istdm;
	/* is long frame (fs high with 2 bclk) */
	bool islframe;
};

/* export fucntion for other module*/
void *open_aio(const char *name);
int close_aio(void *hd);
int aio_clk_enable(void *hd, u32 clk_idx, bool en);
int aio_set_clk_rate(void *hd, u32 clk_idx, unsigned long rate);
unsigned long aio_get_clk_rate(void *hd, u32 clk_idx);
bool aio_get_aud_ch_en(void *hd, u32 id, u32 tsd);
void aio_set_aud_ch_en(void *hd, u32 id, u32 tsd, bool enable);
void aio_set_aud_ch_mute(void *hd, u32 id, u32 tsd, u32 mute);
void aio_set_aud_ch_flush(void *hd, u32 id, u32 tsd, u32 flush);
void aio_set_ctl(void *hd, u32 id, u32 data_fmt,
		u32 width_word, u32 width_sample);
void aio_set_data_fmt(void *hd, u32 id, u32 data_fmt);
void aio_set_width_word(void *hd, u32 id, u32 width_word);
void aio_set_width_sample(void *hd, u32 id, u32 width_sample);
void aio_set_invclk(void *hd, u32 id, bool invclk);
void aio_set_invfs(void *hd, u32 id, bool invfs);
void aio_set_tdm(void *hd, u32 id, bool en, u32 chcnt, u32 wshigh);
int aio_enabletxport(void *hd, u32 id, bool enable);
int aio_setirq(void *hd, u32 pri, u32 sec,
					u32 mic, u32 spdif, u32 hdmi);
int aio_getirqsts(void *hd);
int aio_enablerxport(void *hd, u32 id, bool enable);
int aio_reset(void *hd);
int aio_selhdsource(void *hd, u32 sel);
int aio_setspdif_en(void *hd, bool enable);
int aio_setspdifclk(void *hd, u32 div);
int aio_selhdport(void *hd, u32 sel);
int aio_setclkdiv(void *hd, u32 id, u32 div);
int aio_i2s_set_clock(void *hd, u32 id, u32 clkSwitch,
	 u32 clkD3Switch, u32 clkSel, u32 pllUsed, u32 en);
int aio_set_pdmmicsel(void *hd, u32 sel);
int aio_set_ctl_ext(void *hd, u32 id, struct aud_ctrl *ctrl);
int aio_set_bclk_sel(void *hd, u32 id, u32 sel);
int aio_set_bclk_inv(void *hd, u32 id, u32 inv);
int aio_set_fsync(void *hd, u32 id, u32 sel, u32 inv);
int aio_set_mic1_mm_mode(void *hd, u32 en);
int aio_set_mic_intlmode(void *hd, u32 id, u32 tsd, u32 en);
int aio_set_slave_mode(void *hd, u32 id, u32 mod);
u32 aio_get_tsd_from_chid(void *hd, u32 chid);
int aio_set_xfeed_mode(void *hd, s32 i2sid, u32 lrclk, u32 bclk);
int aio_set_interleaved_mode(void *hd, u32 id, u32 src, u32 ch_map);
int aio_set_pcm_mono(void *hd, u32 id, bool en);
/* misc api*/
int aio_enable_audio_timer(void *hd, bool en);
int aio_get_audio_timer(void *hd, u32 *val);
int aio_enable_sampinfo(void *hd, u32 idx, bool en);
int aio_set_sampinfo_req(void *hd, u32 idx, bool en);
int aio_get_audio_counter(void *hd, u32 idx, u32 *c);
int aio_get_audio_timestamp(void *hd, u32 idx, u32 *t);
int aio_sw_rst(void *hd, u32 option, u32 val);
int aio_set_loopback_clk_gate(void *hd, u32 idx, u32 en);
int aio_earc_arc(void *hd, u32 sel);
int aio_configure_loopback(void *hd, u32 id, u8 chan_num, u8 dummy_data);
void aio_spdifi_src_sel(void *hd, u32 clk_sel, u32 data_sel);
void aio_earc_src_sel(void *hd, u32 sel);
void aio_earc_i2s_frord_sel(void *hd, u32 sel);
#endif
