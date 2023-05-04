// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/printk.h>
#include <linux/err.h>

#include "aio_common.h"
#include "pdm_common.h"
#include "aio_dmic_hal.h"
#include "aio_priv.h"

int dmic_pair_enable(void *hd, u32 mic_id, bool enable)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	val = aio_read(aio, offset);

	switch (mic_id) {
	case AIO_MIC_CH0:
		SET32DMIC_CONTROL_Enable_A(val, enable);
		break;
	case AIO_MIC_CH1:
		SET32DMIC_CONTROL_Enable_D(val, enable);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_pair_enable);

void dmic_module_enable(void *hd, bool enable)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	val = aio_read(aio, offset);
	SET32DMIC_CONTROL_Enable(val, enable);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_module_enable);

int dmic_enable_run(void *hd, u32 mic_id, bool left, bool right)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	val = aio_read(aio, offset);

	switch (mic_id) {
	case AIO_MIC_CH0:
		SET32DMIC_CONTROL_Run_A_L(val, left);
		SET32DMIC_CONTROL_Run_A_R(val, right);
		break;
	case AIO_MIC_CH1:
		SET32DMIC_CONTROL_Run_D_L(val, left);
		SET32DMIC_CONTROL_Run_D_R(val, right);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_enable_run);

int dmic_swap_left_right(void *hd, u32 mic_id, bool swap)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_MICROPHONE_CONFIGURATION;
	val = aio_read(aio, offset);

	switch (mic_id) {
	case AIO_MIC_CH0:
		SET32DMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Swap(val, swap);
		break;
	case AIO_MIC_CH1:
		SET32DMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Swap(val, swap);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_swap_left_right);

int dmic_set_time_order(void *hd, u32 mic_id, u32 l_r_order)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_MICROPHONE_CONFIGURATION;
	val = aio_read(aio, offset);

	switch (mic_id) {
	case AIO_MIC_CH0:
		SET32DMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Time_Order(val, l_r_order);
		break;
	case AIO_MIC_CH1:
		SET32DMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Time_Order(val, l_r_order);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_set_time_order);

int dmic_set_gain_left(void *hd, u32 mic_id, u32 gain)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	switch (mic_id) {
	case AIO_MIC_CH0:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_A;
		val = aio_read(aio, offset);
		SET32DMIC_GAIN_MIC_PAIR_A_Gain_L(val, gain);
		break;
	case AIO_MIC_CH1:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_B;
		val = aio_read(aio, offset);
		SET32DMIC_GAIN_MIC_PAIR_D_Gain_L(val, gain);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_set_gain_left);

int dmic_set_gain_right(void *hd, u32 mic_id, u32 gain)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	switch (mic_id) {
	case AIO_MIC_CH0:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_A;
		val = aio_read(aio, offset);
		SET32DMIC_GAIN_MIC_PAIR_A_Gain_R(val, gain);
		break;
	case AIO_MIC_CH1:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_D;
		val = aio_read(aio, offset);
		SET32DMIC_GAIN_MIC_PAIR_B_Gain_R(val, gain);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_set_gain_right);

void dmic_set_slot(void *hd, u32 bits_per_slot)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	val = aio_read(aio, offset);
	SET32DMIC_DECIMATION_CONTROL_PDM_Bits_Per_Slot(val, bits_per_slot);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_slot);

void dmic_set_fir_filter_sel(void *hd, u32 sel)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	val = aio_read(aio, offset);
	SET32DMIC_DECIMATION_CONTROL_FIR_Filter_Selection(val, sel);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_fir_filter_sel);

int dmic_enable_dc_filter(void *hd, u32 mic_id, bool enable)
{
	pr_warning("not implement for ASxxx");
	return 0;
}
EXPORT_SYMBOL(dmic_enable_dc_filter);

void dmic_set_cic_ratio(void *hd, u32 ratio)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	val = aio_read(aio, offset);
	SET32DMIC_DECIMATION_CONTROL_CIC_Ratio_PCM(val, ratio);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_cic_ratio);

void dmic_set_cic_ratio_D(void *hd, u32 ratio)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	val = aio_read(aio, offset);
	SET32DMIC_DECIMATION_CONTROL_CIC_Ratio_PCM_D(val, ratio);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_cic_ratio_D);

void dmic_set_pdm_adma_mode(void *hd, u32 mode)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_MICROPHONE_CONFIGURATION;
	val = aio_read(aio, offset);
	SET32DMIC_MICROPHONE_CONFIGURATION_D_PDM_from_ADMA(val, mode);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_pdm_adma_mode);

void dmic_interface_clk_enable_all(void *hd, bool enable)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_All(val, enable);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_interface_clk_enable_all);

void dmic_set_clk_source_enable(void *hd, bool enable)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Source_En(val, enable);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_clk_source_enable);

static void dmic_set_clk_disable(void *hd)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	dmic_interface_clk_enable_all(hd, 0);
	//may need delay
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_A(val, 0);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_D(val, 0);
	aio_write(aio, offset, val);

	//disable source
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Source_En(val, 0);
	aio_write(aio, offset, val);
}

void dmic_set_interface_clk(void *hd, u32 clk_src, u32 div_m,
			u32 div_n, u32 div_p)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	dmic_set_clk_disable(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Source_Sel(val, clk_src);
	aio_write(aio, offset, val);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerM(val, div_m);
	SET32DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerN(val, div_n);
	SET32DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerP(val, div_p);
	aio_write(aio, offset, val);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_EXTIFD_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_ABCClk_Select(val, 1);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_interface_clk);

void dmic_set_interface_D_clk(void *hd, u32 clk_src, u32 div_m,
			u32 div_n, u32 div_p)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	dmic_set_clk_disable(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Source_Sel(val, clk_src);
	aio_write(aio, offset, val);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_EXTIFD_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerM(val, div_m);
	SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerN(val, div_n);
	SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerP(val, div_p);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_interface_D_clk);

void dmic_set_dpath_clk(void *hd, u32 div)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	dmic_set_clk_disable(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Core_Clock_Config_Divider(val, div);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_set_dpath_clk);

int dmic_interface_clk_enable(void *hd, u32 mic_id, bool enable)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);
	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	val = aio_read(aio, offset);
	switch (mic_id) {
	case AIO_MIC_CH0:
		SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_A(val, enable);
		break;
	case AIO_MIC_CH1:
		SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_D(val, enable);
		break;
	}

	aio_write(aio, offset, val);
	return 0;
}
EXPORT_SYMBOL(dmic_interface_clk_enable);

void dmic_sw_reset(void *hd)
{
	u32 val, offset;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Reset;
	val = aio_read(aio, offset);
	SET32DMIC_CLK_DMIC_Reset_Reset(val, 1);
	aio_write(aio, offset, val);
}
EXPORT_SYMBOL(dmic_sw_reset);
