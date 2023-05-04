// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef __AIO_PDM_COMMON_H__
#define __AIO_PDM_COMMON_H__

#include "aio.h"

#define AIO_MIC_CH0         (0)
#define AIO_MIC_CH1         (1)
#define AIO_MIC_CH2         (2)
#define AIO_MIC_CH3         (3)

int aio_pdm_ch_mute(void *hd, u32 chanid, bool mute);
int aio_pdm_ch_en(void *hd, u32 chanid, bool en);
void aio_pdm_global_en(void *hd, bool en);
int aio_pdm_rdfd_set(void *hd, u32 chanid, u16 rdlt, u16 fdlt);
int aio_pdm_interleaved_mode_en(void *hd, u32 chanid, bool enable);
void aio_pdm_set_ctrl1_div(void *hd, u32 fs);
int aio_pdm_get_div(u32 fs);
void aio_pdm_ctrl1_set(void *hd, u8 div, bool invclk_int,
			u8 rlsb, u8 rdm, u8 mode, u8 latch_mode);
void xfeed_set_pdm_sel(void *hd, u32 pdm_clk_sel, u32 pdmc_sel, u32 pdm_sel);
#endif
