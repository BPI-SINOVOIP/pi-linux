// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef __AIO_DMIC_HAL_H__
#define __AIO_DMIC_HAL_H__

#include "aio.h"

int dmic_pair_enable(void *hd, u32 mic_id, bool enable);
void dmic_module_enable(void *hd, bool enable);
int dmic_enable_run(void *hd, u32 mic_id, bool left, bool right);
int dmic_swap_left_right(void *hd, u32 mic_id, bool swap);
int dmic_set_time_order(void *hd, u32 mic_id, u32 l_r_order);
int dmic_set_gain_left(void *hd, u32 mic_id, u32 gain);
int dmic_set_gain_right(void *hd, u32 mic_id, u32 gain);
void dmic_set_slot(void *hd, u32 bits_per_slot);
void dmic_set_fir_filter_sel(void *hd, u32 sel);
int dmic_enable_dc_filter(void *hd, u32 mic_id, bool enable);
void dmic_set_cic_ratio(void *hd, u32 ratio);
void dmic_set_cic_ratio_D(void *hd, u32 ratio);
void dmic_set_pdm_adma_mode(void *hd, u32 mode);
void dmic_interface_clk_enable_all(void *hd, bool enable);
void dmic_set_clk_source_enable(void *hd, bool enable);
void dmic_set_interface_clk(void *hd, u32 clk_src, u32 div_m,
			u32 div_n, u32 div_p);
void dmic_set_interface_D_clk(void *hd, u32 clk_src, u32 div_m,
			u32 div_n, u32 div_p);
void dmic_set_dpath_clk(void *hd, u32 div);
int dmic_interface_clk_enable(void *hd, u32 mic_id, bool enable);
void dmic_sw_reset(void *hd);
void dmic_rd_wr_flush(void *hd);
int dmic_enable_mono_mode(void *hd, u32 mic_id, bool enable);

#endif
