// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/printk.h>
#include <linux/err.h>
#include <linux/delay.h>

#include "aio_common.h"
#include "pdm_common.h"
#include "aio_dmic_hal.h"
#include "aio_priv.h"

static int dmic_id_to_ch_id(u32 mic_id)
{
	int ch_id;
	int dmic_module_num = MAX_DMIC_MODULES;

	/*
	 * if MAX_DMIC_MODULES == AIO_MIC_CH3+1, dmic and channel id are
	 * one-to-one mapping. just retrun mic_id. Otherwise, new mapping
	 * is required. The new mapping is hardware speific.
	 */
	if (dmic_module_num == (AIO_MIC_CH3+1))
		return mic_id;
	else {
		switch (mic_id) {
		case 0:
			ch_id = AIO_MIC_CH0;
			break;
		case 1:
			ch_id = AIO_MIC_CH3;
			break;
		case 2:
		case 3:
		default:
			printk(KERN_ERR "Invalid mic id %d\n", mic_id);
			ch_id = -EINVAL;
		}
	}

	return ch_id;
}

int dmic_pair_enable(void *hd, u32 mic_id, bool enable)
{
	u32 offset;
	T32DMIC_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	reg.u32 = aio_read(aio, offset);

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uCONTROL_Enable_A = enable;
		break;
	case AIO_MIC_CH1:
		reg.uCONTROL_Enable_B = enable;
		break;
	case AIO_MIC_CH2:
		reg.uCONTROL_Enable_C = enable;
		break;
	case AIO_MIC_CH3:
		reg.uCONTROL_Enable_D = enable;
		break;
	default:
		return -EINVAL;
	}

	aio_write(aio, offset, reg.u32);
	return 0;
}
EXPORT_SYMBOL(dmic_pair_enable);

void dmic_module_enable(void *hd, bool enable)
{
	u32 offset;
	T32DMIC_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	reg.u32 = aio_read(aio, offset);
	reg.uCONTROL_Enable = enable;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_module_enable);

int dmic_enable_run(void *hd, u32 mic_id, bool left, bool right)
{
	u32 offset;
	T32DMIC_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	reg.u32 = aio_read(aio, offset);

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uCONTROL_Run_A_L = left;
		reg.uCONTROL_Run_A_R = right;
		break;
	case AIO_MIC_CH1:
		reg.uCONTROL_Run_B_L = left;
		reg.uCONTROL_Run_B_R = right;
		break;
	case AIO_MIC_CH2:
		reg.uCONTROL_Run_C_L = left;
		reg.uCONTROL_Run_C_R = right;
		break;
	case AIO_MIC_CH3:
		reg.uCONTROL_Run_D_L = left;
		reg.uCONTROL_Run_D_R = right;
		break;
	default:
		return -EINVAL;
	}

	aio_write(aio, offset, reg.u32);
	return 0;
}
EXPORT_SYMBOL(dmic_enable_run);

int dmic_swap_left_right(void *hd, u32 mic_id, bool swap)
{
	u32 offset;
	T32DMIC_MICROPHONE_CONFIGURATION reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_MICROPHONE_CONFIGURATION;
	reg.u32 = aio_read(aio, offset);

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uMICROPHONE_CONFIGURATION_A_Left_Right_Swap = swap;
		break;
	case AIO_MIC_CH1:
		reg.uMICROPHONE_CONFIGURATION_B_Left_Right_Swap = swap;
		break;
	case AIO_MIC_CH2:
		reg.uMICROPHONE_CONFIGURATION_C_Left_Right_Swapz = swap;
		break;
	case AIO_MIC_CH3:
		reg.uMICROPHONE_CONFIGURATION_D_Left_Right_Swap = swap;
		break;
	default:
		return -EINVAL;
	}

	aio_write(aio, offset, reg.u32);
	return 0;
}
EXPORT_SYMBOL(dmic_swap_left_right);

int dmic_set_time_order(void *hd, u32 mic_id, u32 l_r_order)
{
	u32 offset;
	T32DMIC_MICROPHONE_CONFIGURATION reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_MICROPHONE_CONFIGURATION;
	reg.u32 = aio_read(aio, offset);

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uMICROPHONE_CONFIGURATION_A_Left_Right_Time_Order = l_r_order;
		break;
	case AIO_MIC_CH1:
		reg.uMICROPHONE_CONFIGURATION_B_Left_Right_Time_Order = l_r_order;
		break;
	case AIO_MIC_CH2:
		reg.uMICROPHONE_CONFIGURATION_C_Left_Right_Time_Order = l_r_order;
		break;
	case AIO_MIC_CH3:
		reg.uMICROPHONE_CONFIGURATION_D_Left_Right_Time_Order = l_r_order;
		break;
	default:
		return -EINVAL;
	}

	aio_write(aio, offset, reg.u32);
	return 0;
}
EXPORT_SYMBOL(dmic_set_time_order);

int dmic_set_gain_right(void *hd, u32 mic_id, u32 gain)
{
	u32 offset;
	T32DMIC_GAIN_MIC_PAIR_A reg_gain_pair_a;
	T32DMIC_GAIN_MIC_PAIR_B reg_gain_pair_b;
	T32DMIC_GAIN_MIC_PAIR_C reg_gain_pair_c;
	T32DMIC_GAIN_MIC_PAIR_D reg_gain_pair_d;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_A;
		reg_gain_pair_a.u32 = aio_read(aio, offset);
		reg_gain_pair_a.uGAIN_MIC_PAIR_A_Gain_L = gain;
		aio_write(aio, offset, reg_gain_pair_a.u32);
		break;
	case AIO_MIC_CH1:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_B;
		reg_gain_pair_b.u32 = aio_read(aio, offset);
		reg_gain_pair_b.uGAIN_MIC_PAIR_B_Gain_L = gain;
		aio_write(aio, offset, reg_gain_pair_b.u32);
		break;
	case AIO_MIC_CH2:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_C;
		reg_gain_pair_c.u32 = aio_read(aio, offset);
		reg_gain_pair_c.uGAIN_MIC_PAIR_C_Gain_L = gain;
		aio_write(aio, offset, reg_gain_pair_c.u32);
		break;
	case AIO_MIC_CH3:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_D;
		reg_gain_pair_d.u32 = aio_read(aio, offset);
		reg_gain_pair_d.uGAIN_MIC_PAIR_D_Gain_L = gain;
		aio_write(aio, offset, reg_gain_pair_d.u32);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(dmic_set_gain_right);

int dmic_set_gain_left(void *hd, u32 mic_id, u32 gain)
{
	u32 offset;
	T32DMIC_GAIN_MIC_PAIR_A reg_gain_pair_a;
	T32DMIC_GAIN_MIC_PAIR_B reg_gain_pair_b;
	T32DMIC_GAIN_MIC_PAIR_C reg_gain_pair_c;
	T32DMIC_GAIN_MIC_PAIR_D reg_gain_pair_d;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_A;
		reg_gain_pair_a.u32 = aio_read(aio, offset);
		reg_gain_pair_a.uGAIN_MIC_PAIR_A_Gain_R = gain;
		aio_write(aio, offset, reg_gain_pair_a.u32);
		break;
	case AIO_MIC_CH1:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_B;
		reg_gain_pair_b.u32 = aio_read(aio, offset);
		reg_gain_pair_b.uGAIN_MIC_PAIR_B_Gain_R = gain;
		aio_write(aio, offset, reg_gain_pair_b.u32);
		break;
	case AIO_MIC_CH2:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_C;
		reg_gain_pair_c.u32 = aio_read(aio, offset);
		reg_gain_pair_c.uGAIN_MIC_PAIR_C_Gain_R = gain;
		aio_write(aio, offset, reg_gain_pair_c.u32);
		break;
	case AIO_MIC_CH3:
		offset = RA_AIO_DMIC + RA_DMIC_GAIN_MIC_PAIR_D;
		reg_gain_pair_d.u32 = aio_read(aio, offset);
		reg_gain_pair_d.uGAIN_MIC_PAIR_D_Gain_R = gain;
		aio_write(aio, offset, reg_gain_pair_d.u32);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(dmic_set_gain_left);

void dmic_set_slot(void *hd, u32 bits_per_slot)
{
	u32 offset;
	T32DMIC_DECIMATION_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	reg.u32 = aio_read(aio, offset);
	reg.uDECIMATION_CONTROL_PDM_Bits_Per_Slot = bits_per_slot;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_slot);

void dmic_set_fir_filter_sel(void *hd, u32 sel)
{
	u32 offset;
	T32DMIC_DECIMATION_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	reg.u32 = aio_read(aio, offset);
	reg.uDECIMATION_CONTROL_FIR_Filter_Selection = sel;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_fir_filter_sel);

int dmic_enable_dc_filter(void *hd, u32 mic_id, bool enable)
{
	u32 offset;
	T32DMIC_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	reg.u32 = aio_read(aio, offset);

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uCONTROL_Enable_DC_A = enable;
		break;
	case AIO_MIC_CH1:
		reg.uCONTROL_Enable_DC_B = enable;
		break;
	case AIO_MIC_CH2:
		reg.uCONTROL_Enable_DC_C = enable;
		break;
	case AIO_MIC_CH3:
		reg.uCONTROL_Enable_DC_D = enable;
		break;
	default:
		return -EINVAL;
	}
	aio_write(aio, offset, reg.u32);

	return 0;
}
EXPORT_SYMBOL(dmic_enable_dc_filter);

void dmic_set_cic_ratio(void *hd, u32 ratio)
{
	u32 offset;
	T32DMIC_DECIMATION_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	reg.u32 = aio_read(aio, offset);
	reg.uDECIMATION_CONTROL_CIC_Ratio_PCM = ratio;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_cic_ratio);

void dmic_set_cic_ratio_D(void *hd, u32 ratio)
{
	u32 offset;
	T32DMIC_DECIMATION_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_DECIMATION_CONTROL;
	reg.u32 = aio_read(aio, offset);
	reg.uDECIMATION_CONTROL_CIC_Ratio_PCM_D = ratio;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_cic_ratio_D);

void dmic_set_pdm_adma_mode(void *hd, u32 mode)
{
	u32 offset;
	T32DMIC_MICROPHONE_CONFIGURATION reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_MICROPHONE_CONFIGURATION;
	reg.u32 = aio_read(aio, offset);
	reg.uMICROPHONE_CONFIGURATION_D_PDM_from_ADMA = mode;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_pdm_adma_mode);

void dmic_interface_clk_enable_all(void *hd, bool enable)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	reg.u32 = aio_read(aio, offset);
	reg.uDMIC_Core_Clock_Config_Enable_All = enable;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_interface_clk_enable_all);

void dmic_set_clk_source_enable(void *hd, bool enable)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	reg.u32 = aio_read(aio, offset);
	reg.uDMIC_Core_Clock_Config_Source_En = enable;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_clk_source_enable);

static void dmic_set_clk_disable(void *hd)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	dmic_interface_clk_enable_all(hd, 0);
	usleep_range(100, 200);

	reg.u32 = aio_read(aio, offset);
	reg.uDMIC_Core_Clock_Config_Enable_A = 0;
	reg.uDMIC_Core_Clock_Config_Enable_B = 0;
	reg.uDMIC_Core_Clock_Config_Enable_C = 0;
	reg.uDMIC_Core_Clock_Config_Enable_D = 0;
	aio_write(aio, offset, reg.u32);

	/* disable the source */
	reg.u32 = aio_read(aio, offset);
	reg.uDMIC_Core_Clock_Config_Source_En = 0;
	aio_write(aio, offset, reg.u32);
}

void dmic_set_interface_clk(void *hd, u32 clk_src, u32 div_m,
			u32 div_n, u32 div_p)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg_core_clk;
	T32DMIC_CLK_DMIC_EXTIFABC_Clock_Config reg_extabc_clk;
	T32DMIC_CLK_DMIC_EXTIFD_Clock_Config reg_extd_clk;
	struct aio_priv *aio = hd_to_aio(hd);

	dmic_set_clk_disable(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	reg_core_clk.u32 = aio_read(aio, offset);
	reg_core_clk.uDMIC_Core_Clock_Config_Source_Sel = clk_src;
	aio_write(aio, offset, reg_core_clk.u32);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
	reg_extabc_clk.u32 = aio_read(aio, offset);
	reg_extabc_clk.uDMIC_EXTIFABC_Clock_Config_DividerM = div_m;
	reg_extabc_clk.uDMIC_EXTIFABC_Clock_Config_DividerN = div_n;
	reg_extabc_clk.uDMIC_EXTIFABC_Clock_Config_DividerP = div_p;
	aio_write(aio, offset, reg_extabc_clk.u32);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
	reg_extd_clk.u32 = aio_read(aio, offset);
	reg_extd_clk.uDMIC_EXTIFD_Clock_Config_ABCClk_Select = 1;
	aio_write(aio, offset, reg_extd_clk.u32);
}
EXPORT_SYMBOL(dmic_set_interface_clk);

void dmic_set_interface_D_clk(void *hd, u32 clk_src, u32 div_m,
			u32 div_n, u32 div_p)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg_core_clk;
	T32DMIC_CLK_DMIC_EXTIFD_Clock_Config reg_extd_clk;
	struct aio_priv *aio = hd_to_aio(hd);

	dmic_set_clk_disable(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	reg_core_clk.u32 = aio_read(aio, offset);
	reg_core_clk.uDMIC_Core_Clock_Config_Source_Sel = clk_src;
	aio_write(aio, offset, reg_core_clk.u32);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_EXTIFD_Clock_Config;
	reg_extd_clk.u32 = aio_read(aio, offset);
	reg_extd_clk.uDMIC_EXTIFD_Clock_Config_DividerM = div_m;
	reg_extd_clk.uDMIC_EXTIFD_Clock_Config_DividerN = div_n;
	reg_extd_clk.uDMIC_EXTIFD_Clock_Config_DividerP = div_p;
	aio_write(aio, offset, reg_extd_clk.u32);
}
EXPORT_SYMBOL(dmic_set_interface_D_clk);

void dmic_set_dpath_clk(void *hd, u32 div)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg;
	struct aio_priv *aio = hd_to_aio(hd);

	dmic_set_clk_disable(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	reg.u32 = aio_read(aio, offset);
	reg.uDMIC_Core_Clock_Config_Divider = div;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_set_dpath_clk);

int dmic_interface_clk_enable(void *hd, u32 mic_id, bool enable)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Core_Clock_Config reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Core_Clock_Config;
	reg.u32 = aio_read(aio, offset);
	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uDMIC_Core_Clock_Config_Enable_A = enable;
		break;
	case AIO_MIC_CH1:
		reg.uDMIC_Core_Clock_Config_Enable_B = enable;
		break;
	case AIO_MIC_CH2:
		reg.uDMIC_Core_Clock_Config_Enable_C = enable;
		break;
	case AIO_MIC_CH3:
		reg.uDMIC_Core_Clock_Config_Enable_D = enable;
		break;
	default:
		return -EINVAL;
	}

	aio_write(aio, offset, reg.u32);
	return 0;
}
EXPORT_SYMBOL(dmic_interface_clk_enable);

void dmic_sw_reset(void *hd)
{
	u32 offset;
	T32DMIC_CLK_DMIC_Reset reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC_CLK + RA_DMIC_CLK_DMIC_Reset;
	reg.u32 = aio_read(aio, offset);
	reg.uDMIC_Reset_Reset = 1;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_sw_reset);

void dmic_rd_wr_flush(void *hd)
{
	u32 offset;
	T32DMIC_FLUSH reg;
	struct aio_priv *aio = hd_to_aio(hd);

	offset = RA_AIO_DMIC + RA_DMIC_FLUSH;
	reg.u32 = aio_read(aio, offset);
	reg.uFLUSH_PCMWR_FLUSH = 1;
	reg.uFLUSH_PDMRD_FLUSH = 1;
	aio_write(aio, offset, reg.u32);

	reg.u32 = aio_read(aio, offset);
	reg.uFLUSH_PCMWR_FLUSH = 0;
	reg.uFLUSH_PDMRD_FLUSH = 0;
	aio_write(aio, offset, reg.u32);
}
EXPORT_SYMBOL(dmic_rd_wr_flush);

int dmic_enable_mono_mode(void *hd, u32 mic_id, bool enable)
{
	u32 offset;
	T32DMIC_CONTROL reg;
	struct aio_priv *aio = hd_to_aio(hd);

	if (mic_id >= AIO_MIC_CH_NUM) {
		printk(KERN_ERR "mic id %d exceed max number\n", mic_id);
		return -EINVAL;
	}

	offset = RA_AIO_DMIC + RA_DMIC_CONTROL;
	reg.u32 = aio_read(aio, offset);

	switch (dmic_id_to_ch_id(mic_id)) {
	case AIO_MIC_CH0:
		reg.uCONTROL_Mono_A = enable;
		break;
	case AIO_MIC_CH1:
		reg.uCONTROL_Mono_B = enable;
		break;
	case AIO_MIC_CH2:
		reg.uCONTROL_Mono_C = enable;
		break;
	case AIO_MIC_CH3:
		reg.uCONTROL_Mono_D = enable;
		break;
	default:
		return -EINVAL;
	}
	aio_write(aio, offset, reg.u32);

	return 0;
}
EXPORT_SYMBOL(dmic_enable_mono_mode);
