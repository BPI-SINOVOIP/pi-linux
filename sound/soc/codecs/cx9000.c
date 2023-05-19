// SPDX-License-Identifier: GPL-2.0
/*
 * ALSA SoC CX9000 Smart Amplifier
 *
 * Copyright:	(C) 2017 Synaptics Incorporated.
 * Author:	Simon Ho, <simon.ho@synaptics.com>
 *
 ************************************************************************
 *  Synaptics Audio Team
 *  Modified Date:  21/04/2022
 ************************************************************************
 */

#define DEBUG

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "cx9000.h"

/* #define CXDBG_REG_DUMP */

#define CX9000_PLBK_EQ_BAND_NUM 7
#define CX9000_PLBK_DRC_BAND_NUM 5
#define CX9000_PLBK_SA2_BAND_NUM 4
#define CX9000_PLBK_EQ_COEF_LEN 11
#define CX9000_PLBK_DRC_COEF_LEN 15
#define CX9000_PLBK_SA2_COEF_LEN 16
#define CX9000_PLBK_EQ_SAMPLE_RATE_NUM 3
#define CX9000_PLBK_SA2_POWER_THRESHOLD_LEN 4

#define CX9000_RATES_DSP	SNDRV_PCM_RATE_48000

struct CX9000_EQ_CTRL {
	u8 sample_rate;
	u8 ch;
	u8 band;
};

struct CX9000_DRC_CTRL {
	u8 ch;
	u8 band;
};

struct CX9000_SA2_CTRL {
	u8 ch;
	u8 band;
};

static const struct reg_default cx9000_reg_defs[] = {
	{ CX9000_CLOCK_CONTROL0,		0x25},
	{ CX9000_CLOCK_CONTROL1,		0x45},
	{ CX9000_CLOCK_CONTROL5,		0x30},
	{ CX9000_CLOCK_CONTROL2,		0x1F},
	{ CX9000_CLOCK_CONTROL3,		0x30},
	{ CX9000_CLOCK_CONTROL4,		0x80},
	{ CX9000_CLOCK_CONTROL6,		0x80},
	{ CX9000_PLL_CONTROL0,			0x01},
	{ CX9000_PLL_CONTROL1,			0x50},
	{ CX9000_PLL_CONTROL2,			0x2F},
	{ CX9000_PLL_CONTROL3,			0x81},
	{ CX9000_PLL_CONTROL4,			0x7D},
	{ CX9000_PLL_CONTROL5,			 0x9},
	{ CX9000_PLL_CONTROL6,			 0x0},
	{ CX9000_PLL_CONTROL7,			 0x0},
	{ CX9000_PLL_CONTROL8,			 0x0},
	{ CX9000_PLL_TRACK_CONTROL,		0x00},
	{ CX9000_PLL_TRACK_THRESHOLD0,		0x80},
	{ CX9000_PLL_TRACK_THRESHOLD1,		0xC0},
	{ CX9000_PLL_TRACK_ADJUST0,		 0x1},
	{ CX9000_PLL_TRACK_ADJUST1,		 0x2},
	{ CX9000_SOFT_RESET,			0x00},
	{ CX9000_OSS_CSR,		       0x100},
	{ CX9000_STATE_TRIGGERS_LP1_R1,		0x03},
	{ CX9000_STATE_TRIGGERS_LP1_R2,		0x00},
	{ CX9000_STATE_TRIGGERS_LP2_R1,		0x0C},
	{ CX9000_STATE_TRIGGERS_LP2_R2,		0x00},
	{ CX9000_STATE_TRIGGERS_LP3_R1,		0x70},
	{ CX9000_STATE_TRIGGERS_LP3_R2,		0x03},
	{ CX9000_STATE_ACTIONS_LP1_R1,		0x03},
	{ CX9000_STATE_ACTIONS_LP1_R2,		0x00},
	{ CX9000_STATE_ACTIONS_LP1_R3,		0x00},
	{ CX9000_STATE_ACTIONS_LP2_R1,		0x03},
	{ CX9000_STATE_ACTIONS_LP2_R2,		0x0F},
	{ CX9000_STATE_ACTIONS_LP2_R3,		0x00},
	{ CX9000_STATE_ACTIONS_LP3_R1,		0x03},
	{ CX9000_STATE_ACTIONS_LP3_R2,		0x3F},
	{ CX9000_STATE_ACTIONS_LP3_R3,		0x02},
	{ CX9000_DEBUG_STATE_OVERRIDE,		0x00},
	{ CX9000_DEBUG_R1,		       0x100},
	{ CX9000_DEBUG_R2,		       0x100},
	{ CX9000_DEBUG_R3,		       0x100},
	{ CX9000_DEBUG_R4,		       0x104},
	{ CX9000_DEBUG_R5,		       0x100},
	{ CX9000_DEBUG_R6,		       0x100},
	{ CX9000_DEBUG_R7,		       0x100},
	{ CX9000_SHORT_WAIT,		       0x119},
	{ CX9000_LONG_WAIT,		       0x17D},
	{ CX9000_WAIT_ASSIGN_FORWARD_R1,	0x00},
	{ CX9000_WAIT_ASSIGN_FORWARD_R2,	0x10},
	{ CX9000_WAIT_ASSIGN_REVERSE_R1,	0x00},
	{ CX9000_WAIT_ASSIGN_REVERSE_R2,	0x02},
	{ CX9000_SYSTEM_CFG_TEST0,		0x1F},
	{ CX9000_SYSTEM_CFG_TEST1,		0x00},
	{ CX9000_I2S_PCM_REG1,			0x3F},
	{ CX9000_I2S_PCM_REG2,			0x1F},
	{ CX9000_I2S_PCM_REG3,			0x00},
	{ CX9000_I2S_PCM_REG4,			0x3F},
	{ CX9000_I2S_PCM_REG5,			0x1F},
	{ CX9000_I2S_PCM_REG6,			0x00},
	{ CX9000_I2S_PCM_REG7,			0x17},
	{ CX9000_I2S_PCM_REG8,			0x00},
	{ CX9000_I2S_PCM_REG9,			0x17},
	{ CX9000_I2S_PCM_REG10,			0x00},
	{ CX9000_I2S_PCM_REG11,			0x17},
	{ CX9000_I2S_PCM_REG12,			0x00},
	{ CX9000_I2S_PCM_REG13,			0x17},
	{ CX9000_I2S_PCM_REG14,			0x20},
	{ CX9000_I2S_PCM_REG15,			0x17},
	{ CX9000_I2S_PCM_REG16,			0x00},
	{ CX9000_I2S_PCM_REG17,			0x17},
	{ CX9000_I2S_PCM_REG18,			0x20},
	{ CX9000_I2S_PCM_REG19,			0x17},
	{ CX9000_I2S_PCM_REG20,			0x00},
	{ CX9000_I2S_PCM_REG21,			0x17},
	{ CX9000_I2S_PCM_REG22,			0x20},
	{ CX9000_I2S_PCM_REG23,			0x3F},
	{ CX9000_I2S_PCM_REG24,			0x3F},
	{ CX9000_I2S_PCM_REG25,			0x04},
	{ CX9000_I2S_PCM_REG26,			0x00},
	{ CX9000_I2S_PCM_REG27,			0x05},
	{ CX9000_I2S_PCM_REG28,			0x03},
	{ CX9000_I2S_PCM_REG29,			0x0C},
	{ CX9000_I2S_PCM_REG30,			0x30},
	{ CX9000_I2S_PCM_REG31,			0x03},
	{ CX9000_I2S_PCM_REG32,			0x00},
	{ CX9000_I2S_PCM_REG33,			0x17},
	{ CX9000_I2S_PCM_REG34,			0x00},
	{ CX9000_I2S_PCM_REG35,			0x00},
	{ CX9000_I2S_PCM_REG36,			0x00},
	{ CX9000_I2S_PCM_REG37,			0x00},
	{ CX9000_GPIO_SELECT,			 0x3},
	{ CX9000_GPIO_OUTPUT_ENABLE,		 0x0},
	{ CX9000_GPIO_OUTPUT_LEVEL,		 0x0},
	{ CX9000_GPIO_INPUT_ENABLE,		 0x0},
	{ CX9000_GPIO_INPUT_STATUS,		 0x0},
	{ CX9000_GPIO_IRQ_EN,			 0x0},
	{ CX9000_GPIO_IRQ_EDGE,			 0x0},
	{ CX9000_GPIO_IRQ_POLARITY,		 0x0},
	{ CX9000_GPIO_IRQ_STATUS,		 0x0},
	{ CX9000_GPIO_ELECTRICAL_CONTROL_1_0,	 0x0},
	{ CX9000_GPIO_ELECTRICAL_CONTROL_3_2,	 0x0},
	{ CX9000_GPIO_ELECTRICAL_CONTROL_5_4,	 0x0},
	{ CX9000_GPIO_ELECTRICAL_CONTROL_7_6,	 0x0},
	{ CX9000_ELECTRICAL_CONTROL_BCLK_WS,	 0x0},
	{ CX9000_ELECTRICAL_CONTROL_SDIN,	 0x0},
	{ CX9000_INTRPT_ENABLE_R1,		 0x0},
	{ CX9000_INTRPT_ENABLE_R2,		 0x0},
	{ CX9000_INTRPT_MASK_R1,		0x85},
	{ CX9000_INTRPT_MASK_R2,		 0x0},
	{ CX9000_INTRPT_STATUS_R1,		 0x0},
	{ CX9000_INTRPT_STATUS_R2,		 0x0},
	{ CX9000_INTRPT_STATUS_MASKED_R1,	 0x0},
	{ CX9000_INTRPT_STATUS_MASKED_R2,	 0x0},
	{ CX9000_INTRPT_SET_R1,			 0x0},
	{ CX9000_COEF_CTRL0,			 0x0},
	{ CX9000_COEF_CTRL1,			 0x0},
	{ CX9000_COEF_WRITE_0_L,		 0x0},
	{ CX9000_COEF_WRITE_0_H,		 0x0},
	{ CX9000_COEF_WRITE_1_L,		 0x0},
	{ CX9000_COEF_WRITE_1_H,		 0x0},
	{ CX9000_COEF_WRITE_2_L,		 0x0},
	{ CX9000_COEF_WRITE_2_H,		 0x0},
	{ CX9000_COEF_WRITE_3_L,		 0x0},
	{ CX9000_COEF_WRITE_3_H,		 0x0},
	{ CX9000_COEF_WRITE_4_L,		 0x0},
	{ CX9000_COEF_WRITE_4_H,		 0x0},
	{ CX9000_COEF_WRITE_5_L,		 0x0},
	{ CX9000_COEF_WRITE_5_H,		 0x0},
	{ CX9000_COEF_WRITE_6_L,		 0x0},
	{ CX9000_COEF_WRITE_6_H,		 0x0},
	{ CX9000_COEF_WRITE_7_L,		 0x0},
	{ CX9000_COEF_WRITE_7_H,		 0x0},
	{ CX9000_COEF_READ_0_L,			 0x0},
	{ CX9000_COEF_READ_0_H,			 0x0},
	{ CX9000_COEF_READ_1_L,			 0x0},
	{ CX9000_COEF_READ_1_H,			 0x0},
	{ CX9000_COEF_READ_2_L,			 0x0},
	{ CX9000_COEF_READ_2_H,			 0x0},
	{ CX9000_COEF_READ_3_L,			 0x0},
	{ CX9000_COEF_READ_3_H,			 0x0},
	{ CX9000_COEF_READ_4_L,			 0x0},
	{ CX9000_COEF_READ_4_H,			 0x0},
	{ CX9000_COEF_READ_5_L,			 0x0},
	{ CX9000_COEF_READ_5_H,			 0x0},
	{ CX9000_COEF_READ_6_L,			 0x0},
	{ CX9000_COEF_READ_6_H,			 0x0},
	{ CX9000_COEF_READ_7_L,			 0x0},
	{ CX9000_COEF_READ_7_H,			 0x0},
	{ CX9000_DEV_ID_0,			 0x0},
	{ CX9000_DEV_ID_1,			 0x0},
	{ CX9000_DEV_ID_2,			 0x0},
	{ CX9000_DEV_ID_3,			 0x0},
	{ CX9000_BOND_OPT,			0x00},
	{ CX9000_STEPPING_NUMBER,		 0x0},
	{ CX9000_DAC_CONFIGURATION,		 0x4},
	{ CX9000_HPF_LEFT_CONTROL,		0x80},
	{ CX9000_HPF_RIGHT_CONTROL,		0x80},
	{ CX9000_ZERODETECT_CSR,		0x01},
	{ CX9000_NOISEGATE_CSR,			0x11},
	{ CX9000_NOISE_GATE_THRESHOLD_H,	0x00},
	{ CX9000_NOISE_GATE_THRESHOLD_M,	0x00},
	{ CX9000_NOISE_GATE_THRESHOLD_L,	0x03},
	{ CX9000_FIFO_STATUS_OVERFLOW,		 0x0},
	{ CX9000_FIFO_STATUS_UNDERFLOW,		 0x0},
	{ CX9000_ANALOG_GAIN,			0xC5},
	{ CX9000_ANALOG_TEST0_H,		 0x0},
	{ CX9000_ANALOG_TEST0_L,		 0x0},
	{ CX9000_ANALOG_TEST1_H,		 0x0},
	{ CX9000_ANALOG_TEST1_L,		 0x0},
	{ CX9000_ANALOG_TEST2_H,		 0x0},
	{ CX9000_ANALOG_TEST2_L,		 0x0},
	{ CX9000_ANALOG_TEST3_H,		 0x0},
	{ CX9000_ANALOG_TEST3_L,		 0x0},
	{ CX9000_RESERVED,			 0x0},
	{ CX9000_ANALOG_TEST4_L,		 0x0},
	{ CX9000_ANALOG_TEST5_H,		 0x0},
	{ CX9000_ANALOG_TEST5_L,		 0x0},
	{ CX9000_ANALOG_TEST7_H,		 0x0},
	{ CX9000_ANALOG_TEST7_L,		 0x0},
	{ CX9000_ANALOG_TEST8_H,		 0x0},
	{ CX9000_ANALOG_TEST8_L,		0x17},
	{ CX9000_ANALOG_TEST9_H,		 0x0},
	{ CX9000_ANALOG_TEST9_L,		 0x0},
	{ CX9000_ANALOG_TEST18_H,		 0x0},
	{ CX9000_ANALOG_TEST18_L,		 0x0},
	{ CX9000_ANALOG_TEST19_H,		 0x0},
	{ CX9000_ANALOG_TEST19_L,		 0x0},
	{ CX9000_ANALOG_TEST20_H,		 0x0},
	{ CX9000_ANALOG_TEST20_L,		 0x0},
	{ CX9000_ANALOG_TEST21_H,		 0x0},
	{ CX9000_ANALOG_TEST21_L,		 0x0},
	{ CX9000_ANALOG_TEST22_H,		 0x0},
	{ CX9000_ANALOG_TEST22_L,		0x11},
	{ CX9000_ANALOG_TEST23_H,		 0x0},
	{ CX9000_ANALOG_TEST23_L,		 0x0},
	{ CX9000_ANALOG_TEST25_H,		 0x0},
	{ CX9000_ANALOG_TEST25_L,		 0x0},
	{ CX9000_ANALOG_TEST26_H,		 0x0},
	{ CX9000_ANALOG_TEST26_L,		 0x0},
	{ CX9000_ANALOG_TEST27_H,		 0x0},
	{ CX9000_ANALOG_TEST27_L,		 0x0},
	{ CX9000_ANALOG_TEST28_H,		 0x0},
	{ CX9000_ANALOG_TEST28_L,		 0x0},
	{ CX9000_ANALOG_TEST29_H,		 0x0},
	{ CX9000_ANALOG_TEST29_L,		 0x0},
	{ CX9000_ANALOG_TEST30_H,		 0x0},
	{ CX9000_ANALOG_TEST30_L,		 0x0},
	{ CX9000_ANALOG_RELATED_CONTROL,	 0x0},
	{ CX9000_ANALOG_STATE_STATUS,		 0x0},
	{ CX9000_ANALOG_ERROR_STATUS,		 0x0},
	{ CX9000_ANALOG_READOUT_1,		 0x0},
	{ CX9000_ANALOG_READOUT_2,		 0x0},
	{ CX9000_DAC_TEST_CTRL_REG0_L,		0x00},
	{ CX9000_DAC_TEST_CTRL_REG0_H,		0x00},
	{ CX9000_DAC_TEST_CTRL_REG1,		0x00},
	{ CX9000_DAC_TEST_CTRL_REG2,		0x00},
	{ CX9000_FE1_GAIN_L_B0,			0x00},
	{ CX9000_FE1_GAIN_L_B1,			0X00},
	{ CX9000_FE1_GAIN_L_B2,			0x08},
	{ CX9000_FE1_GAIN_R_B0,			0x00},
	{ CX9000_FE1_GAIN_R_B1,			0x00},
	{ CX9000_CLASSD_CALIBRATION,		0x00},
	{ CX9000_VOLUME_LEFT,			0x94},
	{ CX9000_VOLUME_RIGHT,			0x94},
	{ CX9000_VOLUME_BOTH,			0x94},
	{ CX9000_TEST_CTRL,			 0x0},
	{ CX9000_TEST_OVERRIDE,			 0x0},
	{ CX9000_IRQ_ALT_7_0,			 0x0},
	{ CX9000_IRQ_ALT_8,			 0x1},
	{ CX9000_CALIB_OVERRIDE,		 0x0},
	{ CX9000_SPKR_OFFCAL_L,			 0x0},
	{ CX9000_SPKR_OFFCAL_R,			 0x0},
	{ CX9000_FIFO_OVERFLOW_MASK1,		 0x0},
	{ CX9000_FIFO_OVERFLOW_MASK2,		 0x0},
	{ CX9000_TBDRC_VOLUME_LEFT,		0x00},
	{ CX9000_TBDRC_VOLUME_RIGHT,		0x00},
	{ CX9000_TBDRC_LP_HIGH_INPUT_THRD,	0x00},
	{ CX9000_TBDRC_LP_LOW_INPUT_THRD,	0x22},
	{ CX9000_TBDRC_LP_HIGH_OUTPUT_THRD,	0x00},
	{ CX9000_TBDRC_LP_LOW_OUTPUT_THRD,	0x10},
	{ CX9000_TBDRC_LP_SLOOP,		0x46},
	{ CX9000_TBDRC_LP_ATTACH_RATE_GAIN_SHIFT, 0x32},
	{ CX9000_TBDRC_LP_RELEASE,		0xBA},
	{ CX9000_TBDRC_LP_FAST_RELEASE,		0x65},
	{ CX9000_TBDRC_LP_RATE_RELEASE,		0x00},
	{ CX9000_TBDRC_LP_BALANCE_RAMP_STEP,	0x40},
	{ CX9000_TBDRC_LP_VOLUME_RAMP_STEP_SEL,	0x02},
	{ CX9000_TBDRC_HP_HIGH_INPUT_THRD,	0x00},
	{ CX9000_TBDRC_HP_LOW_INPUT_THRD,	0x22},
	{ CX9000_TBDRC_HP_HIGH_OUTPUT_THRD,	0x00},
	{ CX9000_TBDRC_HP_LOW_OUTPUT_THRD,	0x10},
	{ CX9000_TBDRC_HP_SLOOP,		0x46},
	{ CX9000_TBDRC_HP_ATTACH_RATE_GAIN_SHIFT, 0x32},
	{ CX9000_TBDRC_HP_RELEASE,		0xBA},
	{ CX9000_TBDRC_HP_FAST_RELEASE,		0x65},
	{ CX9000_TBDRC_HP_RATE_RELEASE,		0x00},
	{ CX9000_TBDRC_HP_BALANCE_RAMP_STEP,	0x40},
	{ CX9000_TBDRC_HP_VOLUME_RAMP_STEP_SEL,	0x02},
	{ CX9000_TBDRC_MP_HIGH_INPUT_THRD,	0x00},
	{ CX9000_TBDRC_MP_LOW_INPUT_THRD,	0x22},
	{ CX9000_TBDRC_MP_HIGH_OUTPUT_THRD,	0x00},
	{ CX9000_TBDRC_MP_LOW_OUTPUT_THRD,	0x10},
	{ CX9000_TBDRC_MP_SLOOP,		0x46},
	{ CX9000_TBDRC_MP_ATTACH_RATE_GAIN_SHIFT, 0x32},
	{ CX9000_TBDRC_MP_RELEASE,		0xBA},
	{ CX9000_TBDRC_MP_FAST_RELEASE,		0x65},
	{ CX9000_TBDRC_MP_RATE_RELEASE,		0x00},
	{ CX9000_TBDRC_MP_BALANCE_RAMP_STEP,	0x40},
	{ CX9000_TBDRC_MP_VOLUME_RAMP_STEP_SEL,	0x02},
	{ CX9000_TBDRC_PD_HIGH_INPUT_THRD,	0x00},
	{ CX9000_TBDRC_PD_LOW_INPUT_THRD,	0x22},
	{ CX9000_TBDRC_PD_HIGH_OUTPUT_THRD,	0x00},
	{ CX9000_TBDRC_PD_LOW_OUTPUT_THRD,	0x10},
	{ CX9000_TBDRC_PD_SLOOP,		0x46},
	{ CX9000_TBDRC_PD_ATTACH_RATE_GAIN_SHIFT, 0x32},
	{ CX9000_TBDRC_PD_RELEASE,		0xBA},
	{ CX9000_TBDRC_PD_FAST_RELEASE,		0x65},
	{ CX9000_TBDRC_PD_RATE_RELEASE,		0x00},
	{ CX9000_TBDRC_PD_BALANCE_RAMP_STEP,	0x40},
	{ CX9000_TBDRC_PD_VOLUME_RAMP_STEP_SEL,	0x02},
};

static const struct reg_sequence cx9000_patch[] = {
	{ 0x02A0, 0x02, 0x0}, /* 4ohm */
	{ 0x0270, 0x02, 0x0}, /* Max speaker output around 3.15W */
	{ 0x0712, 0x02, 0x0}, /* Post-DRC enable with single-band */
	/*
	 * Belows are DRC configurations for
	 * Dynamic-Range Compression Gain-Function
	 */
	{ 0x0716, 0x22, 0x0},
	{ 0x0718, 0x10, 0x0},
	{ 0x0721, 0x22, 0x0},
	{ 0x0723, 0x10, 0x0},
	{ 0x072C, 0x22, 0x0},
	{ 0x072E, 0x10, 0x0},
	{ 0x0737, 0x22, 0x0},
	{ 0x0739, 0x10, 0x0},
	{ 0x0719, 0x46, 0x0},
	{ 0x0724, 0x46, 0x0},
	{ 0x072F, 0x46, 0x0},
	{ 0x073A, 0x46, 0x0},
	{ 0x071A, 0x32, 0x0},
	{ 0x0725, 0x32, 0x0},
	{ 0x0730, 0x32, 0x0},
	{ 0x073B, 0x32, 0x0},
};

#define CX9000_NUM_OSS_STATE	5
static const char *cx9000_oss_state[CX9000_NUM_OSS_STATE] = {
	"OSS_IDLE",
	"OSS_FULL",
	"OSS_LP1",
	"OSS_LP2",
	"OSS_LP3"
};

struct cx9000_data {
	struct snd_soc_component *component;
	struct regmap *regmap;
	struct i2c_client *cx9000_client;
	struct gpio_desc *enable_gpio;
	/* OSS status */
	unsigned int lp_mode;
	/* PLL and I2S format */
	unsigned int pll_clkin;
	int pll_clk_id;
	unsigned int bclk_ratio;
	unsigned int frame_size;
	unsigned int sample_size;
	unsigned int sample_rate;
	unsigned int dai_fmt;
	bool i2spcm_changed;
	bool pll_changed;
	int iter_PLL;
	int iter_I2S;
	int pll_src; /* 0: mclk; 1: bclk */
	/* Stereo, Mono Left, Mono Right, Bi-Mono(default) */
	unsigned int dac_config;
	/* 48k(default), 96k, 192k */
	unsigned int dac_sample_rate;

	/* EQ buffer */
	bool plbk_eq_en[2];
	bool plbk_eq_en_changed;
	bool plbk_eq_changed;
	u8 plbk_eq[3][2][CX9000_PLBK_EQ_BAND_NUM][CX9000_PLBK_EQ_COEF_LEN];
	struct mutex eq_coeff_lock; /* EQ DSP lock */
	/* DRC buffer */
	bool plbk_drc_en;
	bool plbk_drc_en_changed;
	bool plbk_drc_changed;
	u8 plbk_drc[2][CX9000_PLBK_DRC_BAND_NUM][CX9000_PLBK_DRC_COEF_LEN];
	struct mutex drc_coeff_lock; /* DRC DSP lock */
	/* SA2 buffer */
	bool plbk_sa2_changed;
	u8 plbk_sa2[2][CX9000_PLBK_SA2_BAND_NUM][CX9000_PLBK_SA2_COEF_LEN];
	struct mutex sa2_coeff_lock; /* SA2 DSP lock */
};

static void cx9000_update_dsp(struct snd_soc_component *component);

static int cx9000_reg_write(void *context, unsigned int reg,
			    unsigned int value)
{
	struct i2c_client *client = context;
	u8 buf[3];
	int ret;
	struct device *dev = &client->dev;

#ifdef CXDBG_REG_DUMP
	dev_dbg(dev, "I2C write address 0x%04x <= %02x\n",
		reg, value);
#endif

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = value;

	ret = i2c_master_send(client, buf, 3);
	if (ret == 3) {
		ret =  0;
	} else if (ret < 0) {
		dev_err(dev, "I2C write address failed, error = %d\n", ret);
	} else {
		dev_err(dev, "I2C write failed\n");
		ret =  -EIO;
	}
	return ret;
}

static int cx9000_reg_read(void *context, unsigned int reg,
			   unsigned int *value)
{
	int ret;
	u8 send_buf[2];
	unsigned int recv_buf = 0;
	struct i2c_client *client = context;
	struct i2c_msg msgs[2];
	struct device *dev = &client->dev;

	send_buf[0] = reg >> 8;
	send_buf[1] = reg & 0xff;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = 1;
	msgs[1].buf = (u8 *)&recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(dev,
			"Failed to register component: %d\n", ret);
		return ret;
	} else if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev,
			"Failed to register component: %d\n", ret);
		return -EIO;
	}

	*value = recv_buf;

#ifdef CXDBG_REG_DUMP
	dev_dbg(dev,
		"I2C read address 0x%04x => %02x\n",
		reg, *value);
#endif
	return 0;
}

static bool cx9000_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CX9000_OSS_CSR:
	case CX9000_COEF_CTRL0:
		return true;
	default:
		return false;

	}
}

static int cx9000_enable_device(struct snd_soc_component *component, unsigned int en)
{

	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	int val, pin;

	gpiod_set_value(cx9000->enable_gpio, en);

	val = gpiod_get_value(cx9000->enable_gpio);
	pin = desc_to_gpio(cx9000->enable_gpio);
	dev_dbg(component->dev, "%s, enable_pin: %d(%d)\n", __func__, pin, val);
	msleep(200);

	return 0;
}

static void cx9000_check_oss_power_status(struct snd_soc_component *component,
					 unsigned int reg, unsigned int exp_val)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	unsigned long time_out;
	unsigned int val;

	time_out = msecs_to_jiffies(200);
	time_out += jiffies;
	do {
		val = CX9000_OSS_STATE_GET(snd_soc_component_read32(component, reg));
		dev_dbg(component->dev,
			"cur_state: %s, exp_status: %s\n",
			cx9000_oss_state[val], cx9000_oss_state[exp_val]);
		if (val == exp_val)
			break;
		msleep(CX9000_CHECK_DELAY);
	} while (!time_after(jiffies, time_out));
	cx9000->lp_mode = val;
}

static int cx9000_init(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	snd_soc_component_write(component, CX9000_SOFT_RESET, 1);
	msleep(CX9000_CHECK_DELAY);
	snd_soc_component_write(component, CX9000_SOFT_RESET, 0);
	msleep(CX9000_CHECK_DELAY);
	/* The PLL set to 147.456MHz (when DAC is 48/96/192Ksps) or
	 * 135.4752MHz (when DAC is 44.1Ksps).
	 * Input From BCLK Integer_Mode, Pre_Divide = 2, Loop_Div(N) = 96,
	 * PLL Output = 3.07/2 * 96 = 147.456MHz
	 * CLOCK_CONTROL0
	 * bit[5:4]=2b'10: Clocked by divided PLL output, unless
	 * a loss of PLL reference is detected. Then clocked by RC Oscillator.
	 * bit[0]=1: BCLK  as reference
	 */
	if (cx9000->pll_src) {
		/* reference clock is bclk */
		snd_soc_component_write(component, CX9000_CLOCK_CONTROL0, 0x25);
		/* PLL_CONTROL1, Input_Pre_Divide_Ratio = 2 */
		snd_soc_component_write(component, CX9000_PLL_CONTROL1, 0x40);
		/* PLL_CONTROL2, Multiply_Ratio_Integer = 96 */
		snd_soc_component_write(component, CX9000_PLL_CONTROL2, 0x2F);
	} else {
		/* reference clock is mclk */
		snd_soc_component_write(component, CX9000_CLOCK_CONTROL0, 0x24);
		snd_soc_component_write(component, CX9000_PLL_CONTROL1, 0x50);
		snd_soc_component_write(component, CX9000_PLL_CONTROL2, 0x17);
	}
	/* PLL_CONTROL0, Integer_Mode */
	snd_soc_component_write(component, CX9000_PLL_CONTROL0, 0x01);
	/* CLOCK_CONTROL1, Clock for FIR2 = 6.144MHz (FIR engine 2 clock may
	 * be selected as either 6.144MHz for sample rates of 48Ksps
	 * or less, or 12.288MHz for sample rates above 48Ksps)
	 */
	snd_soc_component_write(component, CX9000_CLOCK_CONTROL1, 0xC5);
	/* I2S Setting - Master, 4ch x 32bit/channel, MSB first,
	 * 4-lines mode (Share LRCK and BCLK)
	 * CLOCK_CONTROL3: AIF_BIT_CLK_INTEGER, BCLK=147.456/12=12.288MHz
	 */
	snd_soc_component_write(component, CX9000_CLOCK_CONTROL3, 0x30);
	snd_soc_component_write(component, CX9000_CLOCK_CONTROL4, 0x80);
	snd_soc_component_write(component, CX9000_CLOCK_CONTROL5, 0x30);
	snd_soc_component_write(component, CX9000_CLOCK_CONTROL6, 0x80);
	snd_soc_component_write(component, CX9000_PLL_TRACK_CONTROL, 0x01);
	/* DAC_Configuration,DAC Stereo - 48KFs */
	snd_soc_component_write(component, CX9000_DAC_CONFIGURATION, 0x04);
	/* ClassD CSDAC_Gain=-8db, around 2.65W */
	snd_soc_component_write(component, CX9000_ANALOG_GAIN, 0x02);
	/* 90Hz HPF filter on L/R channels */
	snd_soc_component_write(component, CX9000_HPF_LEFT_CONTROL, 0xa3);
	snd_soc_component_write(component, CX9000_HPF_RIGHT_CONTROL, 0xa3);
	/* Analog_Test30_L Set classD slew rate control */
	snd_soc_component_write(component, CX9000_ANALOG_TEST30_L, 0x64);

	/* Clear soft start OSS */
	snd_soc_component_write(component, CX9000_OSS_CSR, 0x00);
	/* 20 ms delay */
	msleep(CX9000_CHECK_DELAY);
	/* soft start OSS */
	snd_soc_component_write(component, CX9000_OSS_CSR, 0x01);

	/* Others configurations */
	/* Enable interrupt indicator */
	snd_soc_component_write(component, CX9000_INTRPT_ENABLE_R1, 0x85);
	/* use 4 ohm - if using 8 ohms, set bit [1] = 0 */
	snd_soc_component_write(component, CX9000_ANALOG_TEST26_H, 0x02);
	/* will prevent I2C getting stuck */
	snd_soc_component_write(component, 0x0303, 0x00);
	snd_soc_component_write(component, CX9000_STATE_ACTIONS_LP3_R3, 0x02);

	/* DC offset manual calibration */
	/* Enable manual csdac offset and Imeas offset calibration */
	snd_soc_component_write(component, CX9000_ANALOG_TEST18_L, 0x08);
	snd_soc_component_write(component, CX9000_CLASSD_CALIBRATION, 0x0F);
	/* add 20ms delay */
	msleep(CX9000_CHECK_DELAY);
	/* End manual csdac offset calibration */
	snd_soc_component_write(component, CX9000_CLASSD_CALIBRATION, 0x0E);
	/* add 20ms delay */
	msleep(CX9000_CHECK_DELAY);
	/* End Imeas offset calibration, End the manual calibration */
	snd_soc_component_write(component, CX9000_ANALOG_TEST18_L, 0x00);

	/* 20ms delay */
	msleep(CX9000_CHECK_DELAY);
	/* Send clock pulse for A0 clock detect blocks */
	snd_soc_component_write(component, CX9000_ANALOG_TEST3_H, 0x01);
	snd_soc_component_write(component, CX9000_ANALOG_TEST2_L, 0x04);
	/* 20ms delay */
	msleep(CX9000_CHECK_DELAY);
	snd_soc_component_write(component, CX9000_ANALOG_TEST3_H, 0x00);
	snd_soc_component_write(component, CX9000_ANALOG_TEST2_L, 0x00);

	/* calibration. Analog test18_L */
	snd_soc_component_write(component, CX9000_ANALOG_TEST18_L, 0x00);
	snd_soc_component_write(component, CX9000_ANALOG_TEST18_L, 0x08);
	snd_soc_component_write(component, CX9000_ANALOG_TEST18_L, 0x00);

	/* Enabling interrupts */
	snd_soc_component_write(component, CX9000_INTRPT_ENABLE_R1, 0xff);

	/* exit LP1,unmute */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, 0x00);
	msleep(CX9000_CHECK_DELAY);

	return 0;
}

static int cx9000_post_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	unsigned int lp_mode, old_lp_mode;

	old_lp_mode = cx9000->lp_mode;
	lp_mode = snd_soc_component_read32(component, CX9000_OSS_CSR);
	lp_mode = cx9000->lp_mode = CX9000_OSS_STATE_GET(lp_mode);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		dev_dbg(component->dev,
			"DAPM_PMU: OSS Power Mode - %s.\n",
			cx9000_oss_state[lp_mode]);

		/* Check if SA2/DRC/EQ have updated before playback */
		cx9000_update_dsp(component);

		/* Re-trigger PID */
		if ((old_lp_mode == CX9000_OSS_LP2) &&
		    (lp_mode < CX9000_OSS_LP2)) {
			snd_soc_component_write(component,
				      CX9000_CSR_CTRL, CX9000_PID_DISABLE);
			snd_soc_component_write(component,
				      CX9000_CSR_CTRL, CX9000_PID_HARDWARE);
		} else if ((lp_mode == CX9000_OSS_FULL) ||
			   (lp_mode == CX9000_OSS_LP1)) {
			snd_soc_component_write(component,
				      CX9000_CSR_CTRL, CX9000_PID_HARDWARE);
		}

		break;
	case SND_SOC_DAPM_POST_PMD:
		dev_dbg(component->dev,
			"DAPM_PMD: OSS Power Mode - %s.\n",
			cx9000_oss_state[lp_mode]);
		break;
	}

	return 0;
}

/* Input mux controls */
static const struct snd_soc_dapm_widget cx9000_dapm_widgets[] = {
	/* MUX Controls */
	SND_SOC_DAPM_AIF_IN("DAC IN", "DAC Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_OUT_DRV("ClassD", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("PLL", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_POST("Post Event", cx9000_post_event),
	SND_SOC_DAPM_OUTPUT("OUT")
};

static const struct snd_soc_dapm_route cx9000_audio_map[] = {
	{"DAC", NULL, "DAC IN"},
	{"ClassD", NULL, "DAC"},
	{"OUT", NULL, "ClassD"},
	{"ClassD", NULL, "PLL"},
};

/* get suggested pre_div valuce from clock source frequence */
static unsigned int get_div_from_input_clock(unsigned int clkin)
{
	unsigned int div, ref_clk;
	unsigned int max_div_ratio = CX9000_PLL_PRE_DIV_RATIO *
			CX9000_PLL_INT_DIV_RATIO;

	for (div = 1; div <= max_div_ratio; div <<= 1) {
		ref_clk = clkin / div;
		if ((ref_clk >= CX9000_REF_FREQ_MIN) &&
		    (ref_clk <= CX9000_REF_FREQ_MAX))
			break;
	}
	return div;
}

static int cx9000_setup_pll(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	bool bypass_pll = false;
	unsigned int pll_clkin = cx9000->pll_clkin;
	unsigned int clk_id = cx9000->pll_clk_id;
	unsigned int sample_rate = cx9000->sample_rate;
	unsigned int frame_len = cx9000->frame_size;
	unsigned int vco_freq_ratio;
	unsigned int pll_clkout;

	/* if pll_clkin == 0, MCLK isn't source */
	if (!pll_clkin) {
		if (cx9000->pll_clk_id != CX9000_PLL_CLKIN_BCLK)
			return -EINVAL;
		pll_clkin = sample_rate * frame_len;
	}

	switch (sample_rate) {
	case 44100:
	case 48000:
		vco_freq_ratio = 3072;
		break;
	case 96000:
		vco_freq_ratio = 1536;
		break;
	case 192000:
		vco_freq_ratio = 768;
		break;
	default:
		dev_err(component->dev,
			"Unsupported sample rate %d\n", sample_rate);
		return -EINVAL;
	}

	pll_clkout = sample_rate * vco_freq_ratio;
	if (pll_clkin == pll_clkout)
		bypass_pll = true;

	/* set mute before configuring the PLL settings */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, CX9000_MUTE);
	cx9000_check_oss_power_status(component, CX9000_OSS_CSR, CX9000_OSS_LP1);

	if (bypass_pll) {
		/* By pass the PLL configuration */
		snd_soc_component_update_bits(component, CX9000_CLOCK_CONTROL0,
				    CX9000_PLL_BYPASS, CX9000_PLL_BYPASS);
	} else {
		/* Fill in the PLL control registers for N-int & N-frac
		 * pll_clk = N-int + [7 * (N-frac/(2^20 -4)) -4]
		 * Need to fill in N-int and N-frac here based on incoming freq
		 */
		unsigned int pre_div = 1;
		unsigned int pre_div_int = 1;
		unsigned int pre_div_val;
		unsigned int pll_input;
		unsigned int pll_output;
		unsigned int int_div;
		unsigned int  frac_div;
		u64 frac_num;
		unsigned int frac = 0;

		/* Configure divider settings */
		pre_div = get_div_from_input_clock(pll_clkin);
		if (pre_div > CX9000_PLL_PRE_DIV_RATIO) {
			dev_dbg(component->dev,
				"Out of external pre-divider ratio.\n");
			/* use internal pre-divider */
			pre_div_int = pre_div / CX9000_PLL_PRE_DIV_RATIO;
			pre_div = CX9000_PLL_PRE_DIV_RATIO;
			if (pre_div_int > CX9000_PLL_INT_DIV_RATIO) {
				dev_err(component->dev,
				"Out of internal pre-divider ratio, (%d)\n",
				pre_div_int);
				return -EINVAL;
			}
		}
		/* calculate pll setting */
		pll_input = pll_clkin / pre_div / pre_div_int;
		pll_output = pll_clkout;
		int_div = pll_output / pll_input;
		frac_div = pll_output - (int_div * pll_input);

		if (frac_div) {
			frac_div *= 100;
			frac_div /= pll_input;
			frac_num = ((400 + frac_div) * ((1 << 20) - 4));

			dev_dbg(component->dev,
				"frac_div: %d, frac_num: %lld\n",
				frac_div, frac_num);

			do_div(frac_num, 7);

			dev_dbg(component->dev,
				"Nfrac: %lld\n", frac_num);

			frac = ((u32)frac_num + 49) / 100;
		}
		pre_div_val = (1 << 6) | ((pre_div >> 1) << 4) |
			      (pre_div_int - 1);

		dev_dbg(component->dev,
			"pre_div: %d, pre_div_int: %d, int_div: %d, frac: %d\n",
			pre_div, pre_div_int, int_div, frac);

		snd_soc_component_write(component, CX9000_PLL_CONTROL1, pre_div_val);
		if (frac_div == 0) {
			/*Int mode*/
			snd_soc_component_write(component, CX9000_PLL_CONTROL0, 1);
			snd_soc_component_write(component, CX9000_PLL_CONTROL2,
				      (int_div - 1));
		} else {
			/*frac mode*/
			snd_soc_component_write(component, CX9000_PLL_CONTROL0, 0);
			snd_soc_component_write(component, CX9000_PLL_CONTROL2,
				      (int_div - 1));
			snd_soc_component_write(component, CX9000_PLL_CONTROL3,
				      CX9000_PLL_FRAC_LOWER(frac));
			snd_soc_component_write(component, CX9000_PLL_CONTROL4,
				      CX9000_PLL_FRAC_MIDDLE(frac));
			snd_soc_component_write(component, CX9000_PLL_CONTROL5,
				      CX9000_PLL_FRAC_UPPER(frac));
		}

	}
	snd_soc_component_update_bits(component, CX9000_CLOCK_CONTROL0,
			    CX9000_PLL_SRC_MASK, clk_id);
	/* set unmute after configuring PLL settings */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, CX9000_UNMUTE);

	return 0;
}

static int cx9000_setup_i2spcm(struct snd_soc_component *component,
			      int dai_fmt)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	unsigned int data_delay;
	unsigned int slot_width, odd_slot_offset;
	unsigned int slots = 2;
	bool is_i2s = 0;
	bool is_frame_inv = 0;
	bool is_bclk_inv = 0;
	bool is_big_endian = 1;
	bool frame_start_falling_edge = false;
	union REG_I2SPCM_CTRL_REG1_REG2 reg1_reg2 = { { 0 } };
	union REG_I2SPCM_CTRL_REG3 reg3 = { { 0 } };
	union REG_I2SPCM_CTRL_REG4_REG5 reg4_reg5 = { { 0 } };
	union REG_I2SPCM_CTRL_REG6 reg6 = { { 0 } };
	union REG_I2SPCM_CTRL_REG7_REG10 reg7_reg10 = { { 0 } };
	union REG_I2SPCM_CTRL_REG11_REG14 reg11_reg14 = { { 0 } };
	union REG_I2SPCM_CTRL_REG15_REG18 reg15_reg18 = { { 0 } };
	union REG_I2SPCM_CTRL_REG23_REG26 reg23_reg26 = { { 0 } };
	union REG_I2SPCM_CTRL_REG27_REG30 reg27_reg30 = { { 0 } };
	union REG_I2SPCM_CTRL_REG31_REG32 reg31_reg32 = { { 0 } };
	int fmt = dai_fmt;
	unsigned int frame_length = cx9000->frame_size;
	unsigned int sample_width = cx9000->sample_size;

	odd_slot_offset = 0;

	/*
	 * only 2 channels are needed for 24bit I/V data (each frame/slot)
	 */
	if (cx9000->sample_size <= 24)
		slot_width = 24;
	else {
		/* check if 16, 24, 32, 64 frame size (2 channel) */
		if (frame_length % 8)
			slot_width = frame_length / 2;
		else
			slot_width = cx9000->sample_size;
	}

	if (frame_length <= 0) {
		dev_err(component->dev, "Incorrect frame len %d\n", frame_length);
		return -EINVAL;
	}

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	/* TODO: Master Mode (not support now)
	 * 1. need MCLK source
	 * 2. need to config registers 0x0003~0x0006 for
	 * generating BCLK rate of RX and TX.
	 * 3. need to config registers 0x0502'b1, 0x0505'b1,
	 * for generating LRCK of RX and TX
	 * 4. switch 0x0519'b0 and 0x0519'b3 to 1
	 */
	case SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		dev_err(component->dev, "Unsupported DAI mode\n");
		return -EINVAL;
	}

	/* set format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		/* I2S mode needs an even number of slots */
		if (slots & 1)
			return -EINVAL;

		is_i2s = 1;
		/*
		 * Use I2S-style logical slot numbering: even slots
		 * are in first half of frame, odd slots in second half.
		 */
		odd_slot_offset = 0;

		/* MSB starts one cycle after frame start */
		data_delay = 1;

		/* Setup frame sync signal for a duty cycle */
		frame_start_falling_edge = true;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		if (slots & 1)
			return -EINVAL;

		is_i2s = 1;
		odd_slot_offset = 0;
		data_delay = 0;
		frame_start_falling_edge = false;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		if (slots & 1)
			return -EINVAL;

		is_i2s = 1;
		odd_slot_offset = (frame_length / 2) - sample_width;
		data_delay = 0;
		frame_start_falling_edge = false;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		data_delay = 1;
		odd_slot_offset = (frame_length / slots);
		frame_start_falling_edge = false;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		data_delay = 0;
		odd_slot_offset = (frame_length / slots);
		frame_start_falling_edge = false;
		break;
	default:
		dev_err(component->dev, "Unsupported DAI format\n");
		return -EINVAL;
	}

	/* normal clocking mode, sampling on rising edge */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_NB_IF:
		is_bclk_inv = 0;
		break;
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_IB_IF:
		is_bclk_inv = 1;
		break;
	default:
		return -EINVAL;
	}

	/* normal fsync mode, frame start on rising edge */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_IB_NF:
		is_frame_inv = frame_start_falling_edge;
		break;
	case SND_SOC_DAIFMT_NB_IF:
	case SND_SOC_DAIFMT_IB_IF:
		is_frame_inv = !frame_start_falling_edge;
		break;
	default:
		return -EINVAL;
	}

	/* set data width of ch1 and ch2 (tx and rx) */
	reg7_reg10.r.rx1_sa_wdt = slot_width - 1;
	reg7_reg10.r.tx1_sa_wdt = slot_width - 1;
	reg11_reg14.r.rx2_sa_wdt = slot_width - 1;
	reg11_reg14.r.tx2_sa_wdt = slot_width - 1;
	/* set data width of ch3 and ch4 (I/V data) */
	reg15_reg18.r.tx3_sa_wdt = slot_width - 1;
	reg15_reg18.r.tx4_sa_wdt = slot_width - 1;

	if (is_i2s) {
		/* set data slot offset */
		reg7_reg10.r.rx1_slot = odd_slot_offset;
		reg7_reg10.r.tx1_slot = odd_slot_offset;
		reg11_reg14.r.rx2_slot = odd_slot_offset;
		reg11_reg14.r.tx2_slot = odd_slot_offset;
		/* set I/V slot offset */
		reg15_reg18.r.tx3_slot = odd_slot_offset;
		reg15_reg18.r.tx4_slot = odd_slot_offset;
		/* set i2s mode (tx and rx) */
		reg23_reg26.r.rx_mode = 0;
		reg23_reg26.r.tx_mode = 0;
		/* set delay bit for i2s mode */
		reg23_reg26.r.rx_dstart_dly = data_delay;
		reg23_reg26.r.tx_dstart_dly = data_delay;
		/* set clock edge for transmitting/receiver data */
		reg23_reg26.r.rx_on_posedge = !is_bclk_inv;
		reg23_reg26.r.tx_on_posedge = !is_bclk_inv;
		/* set lrck polarity */
		reg23_reg26.r.rx_lrck_pol = is_frame_inv;
		reg23_reg26.r.tx_lrck_pol = is_frame_inv;
		/* set high phase width of ws on master mode */
		reg1_reg2.r.rx_sync_lng = (frame_length / 2) - 1;
		reg4_reg5.r.tx_sync_lng = (frame_length / 2) - 1;
	} else {
		/* set data slot offset */
		reg7_reg10.r.rx1_slot = data_delay;
		reg7_reg10.r.tx1_slot = data_delay;
		reg11_reg14.r.rx2_slot = odd_slot_offset + data_delay;
		reg11_reg14.r.tx2_slot = odd_slot_offset + data_delay;
		/* set I/V slot offset */
		reg15_reg18.r.tx3_slot = data_delay;
		reg15_reg18.r.tx4_slot = odd_slot_offset + data_delay;
		/* set pcm mode (tx and rx) */
		reg23_reg26.r.rx_mode = 1;
		reg23_reg26.r.tx_mode = 1;
		/* set clock edge for transmitting/receiver data */
		reg23_reg26.r.rx_on_posedge = !is_bclk_inv;
		reg23_reg26.r.tx_on_posedge = !is_bclk_inv;
		/* set lrck polarity */
		reg23_reg26.r.rx_lrck_pol = is_frame_inv;
		reg23_reg26.r.tx_lrck_pol = is_frame_inv;
		/* set high phase width of ws on master mode */
		reg1_reg2.r.rx_sync_lng = 0;
		reg4_reg5.r.tx_sync_lng = 0;
	}

	/* set endian (tx and rx) */
	reg23_reg26.r.rx_dshift_sel = !is_big_endian;
	reg23_reg26.r.tx_dshift_sel = !is_big_endian;
	/* set frame size (tx and rx) */
	reg23_reg26.r.rxck_frame = frame_length - 1;
	reg23_reg26.r.txck_frame = frame_length - 1;
	/* set frame length on master mode */
	reg1_reg2.r.rx_lrck_frame = frame_length - 1;
	reg4_reg5.r.tx_lrck_frame = frame_length - 1;
	/* set polarity of lrck first pulse on master mode*/
	reg3.r.rx_lrck_low_first = is_frame_inv;
	reg6.r.tx_lrck_low_first = is_frame_inv;
	//test
	reg3.r.rx_lrck_en = true;
	/*
	 * 1nd byte: set bclk/lrck sharing
	 * 2nd byte: SDO_0 for ch1 and ch2
	 * 3nd byte: SDO_1 for ch3 and ch4
	 * 4nd byte: SDO_2 for ch5 and ch6
	 */
	reg27_reg30.ulval = 0x300c0305;
	/* set rx/tx ch 1&2&3&4 enable */
	reg31_reg32.ulval = 0x0F03;

	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG1,
			  &reg1_reg2.ulval,
			  sizeof(reg1_reg2.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG3,
			  &reg3.ulval,
			  sizeof(reg3.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG4,
			  &reg4_reg5.ulval,
			  sizeof(reg4_reg5.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG6,
			  &reg6.ulval,
			  sizeof(reg6.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG7,
			  &reg7_reg10.ulval,
			  sizeof(reg7_reg10.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG11,
			  &reg11_reg14.ulval,
			  sizeof(reg11_reg14.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG15,
			  &reg15_reg18.ulval,
			  sizeof(reg15_reg18.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG23,
			  &reg23_reg26.ulval,
			  sizeof(reg23_reg26.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG27,
			  &reg27_reg30.ulval,
			  sizeof(reg27_reg30.ulval));
	regmap_bulk_write(cx9000->regmap, CX9000_I2S_PCM_REG31,
			  &reg31_reg32.ulval,
			  sizeof(reg31_reg32.ulval));

	return 0;
}

static int cx9000_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	const unsigned int sample_rate = params_rate(params);
	int sample_size, frame_size;
	int dai_fmt = cx9000->dai_fmt;

	/* Data sizes if not using TDM */
	sample_size = snd_pcm_format_width(params_format(params));

	if (sample_size < 0)
		return sample_size;

	frame_size = snd_soc_params_to_frame_size(params);
	if (frame_size < 0)
		return frame_size;

	if (cx9000->bclk_ratio)
		frame_size = cx9000->bclk_ratio;

	switch (sample_rate) {
	case 48000:
	case 44100:
		cx9000->dac_sample_rate = CX9000_48K;
		break;
	default:
		dev_err(component->dev,
			"Unsupported sample rate %d\n", sample_rate);
		return -EINVAL;
	}

	cx9000->frame_size = frame_size;
	cx9000->sample_size = sample_size;
	cx9000->sample_rate = sample_rate;

	cx9000->i2spcm_changed = true;
	cx9000->pll_changed = true;

	dev_dbg(component->dev,
		"Sample size %d bits, frame = %d bits, rate = %d Hz\n",
		sample_size, frame_size, sample_rate);

	pr_info("%s, Sample size %d bits, frame = %d bits, rate = %d Hz\n",
		__func__,
		sample_size, frame_size, sample_rate);

	/*
	 * If Host(platform) is bitclk master,
	 * the pll_changed and i2spcm_changed can be removed to
	 * support variable sample rate.
	 */
	if (cx9000->pll_changed && cx9000->iter_PLL < 2) {
		cx9000_setup_pll(component);
		cx9000->iter_PLL++;
	} else {
		cx9000->pll_changed = false;
	}

	if (cx9000->i2spcm_changed && cx9000->iter_I2S < 2) {
		cx9000_setup_i2spcm(component, dai_fmt);
		cx9000->iter_I2S++;
	} else {
		cx9000->i2spcm_changed = false;
	}

	dev_dbg(component->dev, "Iter PLL %d, Iter I2S %d\n",
		cx9000->iter_PLL, cx9000->iter_I2S);

	return 0;
}

static int cx9000_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	dev_dbg(component->dev, "set_dai_fmt- %08x\n", fmt);
	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		dev_err(component->dev, "Unsupported DAI master mode\n");
		return -EINVAL;
	}

	/* set format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		dev_err(component->dev, "Unsupported DAI format\n");
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		dev_err(component->dev, "Unsupported DAI clock inversion\n");
		return -EINVAL;
	}

	cx9000->dai_fmt = fmt;
	return 0;
}

static int cx9000_set_dai_bclk_ratio(struct snd_soc_dai *dai,
				     unsigned int ratio)
{
	struct snd_soc_component *component = dai->component;
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	cx9000->bclk_ratio = ratio;
	return 0;
}

static int cx9000_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				  unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	dev_dbg(component->dev, "%s, clk_id: %d, freq: %d, dir: %d\n",
		__func__, clk_id, freq, dir);

	switch (clk_id) {
	case CX9000_PLL_CLKIN_MCLK:
		if (freq < CX9000_REF_FREQ_MIN || freq > CX9000_CLOCK_SRC_MAX) {
			/* out of range PLL_CLKIN, fall back to use BCLK */
			dev_warn(component->dev, "Out of range PLL_CLKIN: %u\n",
				 freq);
			clk_id = CX9000_PLL_CLKIN_BCLK;
			freq = 0;
		}
	/* fall through */
	case CX9000_PLL_CLKIN_BCLK:
		cx9000->pll_clk_id = clk_id;
		cx9000->pll_clkin = freq;
		break;
	default:
		dev_err(component->dev, "Invalid clk id: %d\n", clk_id);
		return -EINVAL;
	}

	cx9000->i2spcm_changed = true;
	cx9000->pll_changed = true;

	return 0;
}

#ifdef CONFIG_PM
static int cx9000_runtime_suspend(struct device *dev)
{
	struct cx9000_data *cx9000 = dev_get_drvdata(dev);

	regcache_cache_only(cx9000->regmap, true);
	regcache_mark_dirty(cx9000->regmap);

	gpiod_set_value(cx9000->enable_gpio, 0);

	return 0;
}

static int cx9000_runtime_resume(struct device *dev)
{
	struct cx9000_data *cx9000 = dev_get_drvdata(dev);

	gpiod_set_value(cx9000->enable_gpio, 1);

	regcache_cache_only(cx9000->regmap, false);
	regcache_sync(cx9000->regmap);

	return 0;
}
#endif

static const struct dev_pm_ops cx9000_pm = {
	SET_RUNTIME_PM_OPS(cx9000_runtime_suspend, cx9000_runtime_resume,
			   NULL)
};

static const struct snd_soc_dai_ops cx9000_speaker_dai_ops = {
	.hw_params	= cx9000_hw_params,
	.set_sysclk	= cx9000_set_dai_sysclk,
	.set_bclk_ratio = cx9000_set_dai_bclk_ratio,
	.set_fmt	= cx9000_set_dai_fmt,
};

/* Formats supported by CX9000 driver. */
#define CX9000_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

/* CX9000 dai structure. */
static struct snd_soc_dai_driver cx9000_dai[] = {
	{
		.name = "cx9000-amplifier",
		.id = 2,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = CX9000_RATES_DSP,
			.formats = CX9000_FORMATS,
		},
		.ops = &cx9000_speaker_dai_ops,
	},

	{ /* plabayck only, return echo reference through I2S TX*/
		.name = "cx9000-aec",
		.id = 3,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = CX9000_RATES_DSP,
			.formats = CX9000_FORMATS,
		},
	},
};

static void cx9000_update_function_en(struct snd_soc_component *component,
				      unsigned int mode)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	/* disable auto mute */
	snd_soc_component_write(component, CX9000_STATE_TRIGGERS_LP1_R1, 0x01);
	cx9000_check_oss_power_status(component, CX9000_OSS_CSR, CX9000_OSS_FULL);
	/* disable digital func*/
	snd_soc_component_write(component, CX9000_STATE_ACTIONS_LP1_R2, 0x08);
	/* reset all digital func */
	snd_soc_component_write(component, CX9000_DEBUG_R1, 0x00);
	snd_soc_component_write(component, CX9000_DEBUG_R3, 0x00);
	snd_soc_component_write(component, CX9000_DEBUG_R5, 0x00);
	/* digital mute */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, CX9000_MUTE);
	cx9000_check_oss_power_status(component, CX9000_OSS_CSR, CX9000_OSS_LP1);

	/* set EQ or DRC to enable/disable */
	if (mode == CX9000_DRC_EN) {
		snd_soc_component_update_bits(component, CX9000_TBDRC_CTRL_REG0,
				    CX9000_DRC_EN_MASK,
				    cx9000->plbk_drc_en ? CX9000_DRC_EN : 0x00);
	} else if (mode == CX9000_EQ_EN) {
		snd_soc_component_update_bits(component, CX9000_EQ_ENABLE_BYPASS,
				    CX9000_EQ_ENABLE_L_MASK,
				    cx9000->plbk_eq_en[0] ?
				    CX9000_EQ_ENABLE_L : CX9000_EQ_BYPASS_L);
		snd_soc_component_update_bits(component, CX9000_EQ_ENABLE_BYPASS,
				    CX9000_EQ_ENABLE_R_MASK,
				    cx9000->plbk_eq_en[1] ?
				    CX9000_EQ_ENABLE_R : CX9000_EQ_BYPASS_R);
	}

	/* set unmute after setting */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, CX9000_UNMUTE);
	cx9000_check_oss_power_status(component, CX9000_OSS_CSR, CX9000_OSS_FULL);
	/* recall previous status */
	snd_soc_component_write(component, CX9000_STATE_ACTIONS_LP1_R2, 0x00);
	snd_soc_component_write(component, CX9000_STATE_TRIGGERS_LP1_R1, 0x03);
	snd_soc_component_write(component, CX9000_CSR_CTRL, CX9000_PID_HARDWARE);
}

static void cx9000_update_drc_en(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	if (!cx9000->plbk_drc_en_changed)
		return;

	cx9000_update_function_en(component, CX9000_DRC_EN);
	cx9000->plbk_drc_en_changed = false;
}

static void cx9000_update_eq_en(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	if (!cx9000->plbk_eq_en_changed)
		return;

	cx9000_update_function_en(component, CX9000_EQ_EN);
	cx9000->plbk_eq_en_changed = false;
}

static void cx9000_wait_for_coeff_writing(struct snd_soc_component *component)
{
	unsigned long time_out;
	u8 done;

	time_out = msecs_to_jiffies(200);
	time_out += jiffies;
	do {
		done = snd_soc_component_read32(component, CX9000_COEF_CTRL0);
		dev_dbg(component->dev, "CTRL_0 is : %d\n", done);
		/* check if coefficients writing complete */
		if (CX9000_COEFF_CHECK_DONE(done) == 1)
			break;
		msleep(CX9000_COEFF_CHECK_DELAY);
	} while (!time_after(jiffies, time_out));
}

static void cx9000_update_coeff(struct snd_soc_component *component, unsigned int coeff,
				unsigned int rate_index, unsigned int ch_index,
				unsigned int band_num, unsigned int coeff_len)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	int rate, band, ch, val;

	/* set mute before configuring the EQ settings */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, CX9000_MUTE);
	cx9000_check_oss_power_status(component, CX9000_OSS_CSR, CX9000_OSS_LP1);

	for (band = 0; band < band_num; band++) {
		for (ch = 0; ch < ch_index; ch++) {
			for (rate = 0; rate < rate_index; rate++) {
				/* coeffieient memory target selection */
				if (coeff == CX9000_EQ_COEFF_L)
					val = band +
				    CX9000_FUNCTION_CHANNEL_SEL((ch + coeff)) +
				    CX9000_FUNCTION_RATE_SEL(rate);
				else
					val = band +
				    CX9000_FUNCTION_CHANNEL_SEL((ch + coeff));
				snd_soc_component_write(component, CX9000_COEF_CTRL1, val);

				/* write coefficients to memory */
				if (coeff == CX9000_EQ_COEFF_L)
					regmap_bulk_write(cx9000->regmap,
					    CX9000_COEF_WRITE_0_L,
					    &cx9000->plbk_eq[rate][ch][band][0],
					    coeff_len);
				else if (coeff == CX9000_DRC_COEFF_L)
					regmap_bulk_write(cx9000->regmap,
					    CX9000_COEF_WRITE_0_L,
					    &cx9000->plbk_drc[ch][band][0],
					    coeff_len);
				else if (coeff == CX9000_SA2_COEFF_L)
					regmap_bulk_write(cx9000->regmap,
					    CX9000_COEF_WRITE_0_L,
					    &cx9000->plbk_sa2[ch][band][0],
					    coeff_len);

				/* write to dsp from memory */
				snd_soc_component_write(component, CX9000_COEF_CTRL1, val);
				snd_soc_component_write(component, CX9000_COEF_CTRL0, 0x01);
				cx9000_wait_for_coeff_writing(component);
			}
		}
	}

	snd_soc_component_write(component, CX9000_CSR_CTRL, CX9000_PID_HARDWARE);
	/* set unmute after configuring the EQ settings */
	snd_soc_component_write(component, CX9000_MUTE_PRE_DRC_VOLUME, CX9000_UNMUTE);
}

static void cx9000_update_eq_coeff(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	if (!cx9000->plbk_eq_changed)
		return;
	if (!cx9000->plbk_eq_en[0] && !cx9000->plbk_eq_en[1])
		return;

	cx9000_update_coeff(component, CX9000_EQ_COEFF_L,
			    CX9000_PLBK_EQ_SAMPLE_RATE_NUM, 2,
			    CX9000_PLBK_EQ_BAND_NUM,
			    CX9000_PLBK_EQ_COEF_LEN);

	cx9000->plbk_eq_changed = false;
	cx9000->plbk_eq_en_changed = true;
}

static void cx9000_update_drc_coeff(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	if (!cx9000->plbk_drc_changed)
		return;
	if (!cx9000->plbk_drc_en)
		return;

	cx9000_update_coeff(component, CX9000_DRC_COEFF_L,
			    1, 2,
			    CX9000_PLBK_DRC_BAND_NUM,
			    CX9000_PLBK_DRC_COEF_LEN);

	cx9000->plbk_drc_changed = false;
	cx9000->plbk_drc_en_changed = true;
}

static void cx9000_update_sa2_coeff(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	if (!cx9000->plbk_sa2_changed)
		return;

	cx9000_update_coeff(component, CX9000_SA2_COEFF_L,
			    1, 2,
			    CX9000_PLBK_SA2_BAND_NUM,
			    CX9000_PLBK_SA2_COEF_LEN);

	cx9000->plbk_sa2_changed = false;
}

static void cx9000_update_dsp(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	unsigned int lp_mode = cx9000->lp_mode;

	if ((lp_mode < CX9000_OSS_FULL) || (lp_mode > CX9000_OSS_LP1)) {
		dev_dbg(component->dev,
			"Can't update SA2/EQ/DRC on %s mode.\n",
			cx9000_oss_state[lp_mode]);
		return;
	}

	cx9000_update_sa2_coeff(component);
	cx9000_update_eq_coeff(component);
	cx9000_update_eq_en(component);
	cx9000_update_drc_coeff(component);
	cx9000_update_drc_en(component);
}

static int cx9000_init_drc(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	int ch, band;

	for (ch = 0; ch < 2; ch++) {
		for (band = 0; band < CX9000_PLBK_DRC_BAND_NUM; band++) {
			if (band == 0) {
				cx9000->plbk_drc[ch][band][0] = 0x85;
				cx9000->plbk_drc[ch][band][1] = 0xEB;
				cx9000->plbk_drc[ch][band][2] = 0x01;
				cx9000->plbk_drc[ch][band][3] = 0x67;
				cx9000->plbk_drc[ch][band][4] = 0xD5;
				cx9000->plbk_drc[ch][band][5] = 0x03;
				cx9000->plbk_drc[ch][band][6] = 0x85;
				cx9000->plbk_drc[ch][band][7] = 0xEB;
				cx9000->plbk_drc[ch][band][8] = 0x01;
				cx9000->plbk_drc[ch][band][9] = 0x76;
				cx9000->plbk_drc[ch][band][10] = 0xE0;
				cx9000->plbk_drc[ch][band][11] = 0x5C;
				cx9000->plbk_drc[ch][band][12] = 0x96;
				cx9000->plbk_drc[ch][band][13] = 0x43;
				cx9000->plbk_drc[ch][band][14] = 0xDB;
			} else if (band == 1) {
				cx9000->plbk_drc[ch][band][0] = 0x93;
				cx9000->plbk_drc[ch][band][1] = 0x18;
				cx9000->plbk_drc[ch][band][2] = 0x00;
				cx9000->plbk_drc[ch][band][3] = 0x83;
				cx9000->plbk_drc[ch][band][4] = 0x2F;
				cx9000->plbk_drc[ch][band][5] = 0x00;
				cx9000->plbk_drc[ch][band][6] = 0x93;
				cx9000->plbk_drc[ch][band][7] = 0x18;
				cx9000->plbk_drc[ch][band][8] = 0x00;
				cx9000->plbk_drc[ch][band][9] = 0x60;
				cx9000->plbk_drc[ch][band][10] = 0xE5;
				cx9000->plbk_drc[ch][band][11] = 0x78;
				cx9000->plbk_drc[ch][band][12] = 0xF5;
				cx9000->plbk_drc[ch][band][13] = 0xB9;
				cx9000->plbk_drc[ch][band][14] = 0xC6;
			} else if (band == 2) {
				cx9000->plbk_drc[ch][band][0] = 0x44;
				cx9000->plbk_drc[ch][band][1] = 0x8B;
				cx9000->plbk_drc[ch][band][2] = 0x3C;
				cx9000->plbk_drc[ch][band][3] = 0x1C;
				cx9000->plbk_drc[ch][band][4] = 0xEB;
				cx9000->plbk_drc[ch][band][5] = 0x86;
				cx9000->plbk_drc[ch][band][6] = 0x44;
				cx9000->plbk_drc[ch][band][7] = 0x8B;
				cx9000->plbk_drc[ch][band][8] = 0x3C;
				cx9000->plbk_drc[ch][band][9] = 0x60;
				cx9000->plbk_drc[ch][band][10] = 0xE5;
				cx9000->plbk_drc[ch][band][11] = 0x78;
				cx9000->plbk_drc[ch][band][12] = 0xF5;
				cx9000->plbk_drc[ch][band][13] = 0xB9;
				cx9000->plbk_drc[ch][band][14] = 0xC6;
			} else if (band == 3) {
				cx9000->plbk_drc[ch][band][0] = 0x54;
				cx9000->plbk_drc[ch][band][1] = 0x74;
				cx9000->plbk_drc[ch][band][2] = 0x30;
				cx9000->plbk_drc[ch][band][3] = 0xFC;
				cx9000->plbk_drc[ch][band][4] = 0x18;
				cx9000->plbk_drc[ch][band][5] = 0x9F;
				cx9000->plbk_drc[ch][band][6] = 0x54;
				cx9000->plbk_drc[ch][band][7] = 0x74;
				cx9000->plbk_drc[ch][band][8] = 0x30;
				cx9000->plbk_drc[ch][band][9] = 0x76;
				cx9000->plbk_drc[ch][band][10] = 0xE0;
				cx9000->plbk_drc[ch][band][11] = 0x5C;
				cx9000->plbk_drc[ch][band][12] = 0x96;
				cx9000->plbk_drc[ch][band][13] = 0x43;
				cx9000->plbk_drc[ch][band][14] = 0xDB;
			} else if (band == 4) {
				cx9000->plbk_drc[ch][band][0] = 0x0B;
				cx9000->plbk_drc[ch][band][1] = 0x46;
				cx9000->plbk_drc[ch][band][2] = 0x39;
				cx9000->plbk_drc[ch][band][3] = 0xA0;
				cx9000->plbk_drc[ch][band][4] = 0x1A;
				cx9000->plbk_drc[ch][band][5] = 0x87;
				cx9000->plbk_drc[ch][band][6] = 0x00;
				cx9000->plbk_drc[ch][band][7] = 0x00;
				cx9000->plbk_drc[ch][band][8] = 0x40;
				cx9000->plbk_drc[ch][band][9] = 0x60;
				cx9000->plbk_drc[ch][band][10] = 0xE5;
				cx9000->plbk_drc[ch][band][11] = 0x78;
				cx9000->plbk_drc[ch][band][12] = 0xF5;
				cx9000->plbk_drc[ch][band][13] = 0xB9;
				cx9000->plbk_drc[ch][band][14] = 0xC6;
			}
		}
	}
	cx9000->plbk_drc_changed = true;

	return 0;
}

static int cx9000_init_eq(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	int rate, ch, band;

	for (rate = 0; rate < CX9000_PLBK_EQ_SAMPLE_RATE_NUM; rate++) {
		for (ch = 0; ch < 2; ch++) {
			for (band = 0; band < CX9000_PLBK_EQ_BAND_NUM; band++) {
				cx9000->plbk_eq[rate][ch][band][1] = 0x40;
				cx9000->plbk_eq[rate][ch][band][10] = 0x03;
			}
		}
	}
	cx9000->plbk_eq_changed = true;

	return 0;
}

static int cx9000_init_sa2(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	int ch, band;

	for (ch = 0; ch < 2; ch++) {
		for (band = 0; band < CX9000_PLBK_SA2_BAND_NUM; band++) {
			if (band == 0) {
				/* long-term avg power 2.0W */
				cx9000->plbk_sa2[ch][band][0] = 0x03;
				cx9000->plbk_sa2[ch][band][6] = 0xFD;
				cx9000->plbk_sa2[ch][band][7] = 0x7F;
				cx9000->plbk_sa2[ch][band][10] = 0x02;
				cx9000->plbk_sa2[ch][band][12] = 0xC3;
				cx9000->plbk_sa2[ch][band][13] = 0x00;
				cx9000->plbk_sa2[ch][band][14] = 0x38;
				cx9000->plbk_sa2[ch][band][15] = 0x1F;
			} else if (band == 1) {
				cx9000->plbk_sa2[ch][band][0] = 0xFF;
				cx9000->plbk_sa2[ch][band][1] = 0x7F;
				cx9000->plbk_sa2[ch][band][10] = 0x02;
				cx9000->plbk_sa2[ch][band][12] = 0x00;
				cx9000->plbk_sa2[ch][band][13] = 0x20;
				cx9000->plbk_sa2[ch][band][14] = 0x00;
				cx9000->plbk_sa2[ch][band][15] = 0x20;
			} else if (band == 2) {
				cx9000->plbk_sa2[ch][band][0] = 0xFF;
				cx9000->plbk_sa2[ch][band][1] = 0x7F;
				cx9000->plbk_sa2[ch][band][10] = 0x02;
				cx9000->plbk_sa2[ch][band][12] = 0x00;
				cx9000->plbk_sa2[ch][band][13] = 0x40;
				cx9000->plbk_sa2[ch][band][14] = 0x00;
				cx9000->plbk_sa2[ch][band][15] = 0x08;
			} else if (band == 3) {
				cx9000->plbk_sa2[ch][band][0] = 0x00;
				cx9000->plbk_sa2[ch][band][1] = 0x40;
				cx9000->plbk_sa2[ch][band][2] = 0x10;
				cx9000->plbk_sa2[ch][band][9] = 0x0A;
				cx9000->plbk_sa2[ch][band][10] = 0x00;
				cx9000->plbk_sa2[ch][band][12] = 0x00;
				cx9000->plbk_sa2[ch][band][13] = 0x40;
				cx9000->plbk_sa2[ch][band][14] = 0x00;
				cx9000->plbk_sa2[ch][band][15] = 0x00;
			}
		}
	}
	cx9000->plbk_sa2_changed = true;

	return 0;
}

static int cx9000_plbk_eq_en_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int cx9000_plbk_eq_en_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = cx9000->plbk_eq_en[0];
	ucontrol->value.integer.value[1] = cx9000->plbk_eq_en[1];

	return 0;
}

static int cx9000_plbk_eq_en_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	const bool enable_left = ucontrol->value.integer.value[0];
	const bool enable_right = ucontrol->value.integer.value[1];

	if (ucontrol->value.integer.value[0] > 1 ||
	    ucontrol->value.integer.value[1] > 1)
		return -EINVAL;

	if ((cx9000->plbk_eq_en[0] != enable_left) ||
	    (cx9000->plbk_eq_en[1] != enable_right)) {
		cx9000->plbk_eq_en[0] = enable_left;
		cx9000->plbk_eq_en[1] = enable_right;
		cx9000->plbk_eq_en_changed = true;
		cx9000_update_dsp(component);
	}
	return 0;
}

static int cx9000_plbk_eq_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = CX9000_PLBK_EQ_COEF_LEN;

	return 0;
}

static int cx9000_plbk_eq_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	struct CX9000_EQ_CTRL *eq =
		(struct CX9000_EQ_CTRL *)kcontrol->private_value;
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx9000->plbk_eq[eq->sample_rate][eq->ch][eq->band];

	memcpy(param, cache, CX9000_PLBK_EQ_COEF_LEN);

	return 0;
}

static int cx9000_plbk_eq_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	struct CX9000_EQ_CTRL *eq =
	(struct CX9000_EQ_CTRL *)kcontrol->private_value;
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx9000->plbk_eq[eq->sample_rate][eq->ch][eq->band];

	mutex_lock(&cx9000->eq_coeff_lock);

	/*do nothing if the value is the same*/
	if (!memcmp(cache, param, CX9000_PLBK_EQ_COEF_LEN))
		goto EXIT;

	memcpy(cache, param, CX9000_PLBK_EQ_COEF_LEN);

	cx9000->plbk_eq_changed = true;
	cx9000_update_dsp(component);
EXIT:
	mutex_unlock(&cx9000->eq_coeff_lock);
	return 0;
}

static int cx9000_plbk_drc_en_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int cx9000_plbk_drc_en_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = cx9000->plbk_drc_en;

	return 0;
}

static int cx9000_plbk_drc_en_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	const bool enable = ucontrol->value.integer.value[0];

	if (ucontrol->value.integer.value[0] > 1)
		return -EINVAL;

	if (cx9000->plbk_drc_en != enable) {
		cx9000->plbk_drc_en = enable;
		cx9000->plbk_drc_en_changed = true;
		cx9000_update_dsp(component);
	}
	return 0;
}

static int cx9000_plbk_drc_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = CX9000_PLBK_DRC_COEF_LEN;

	return 0;
}

static int cx9000_plbk_drc_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	struct CX9000_DRC_CTRL *drc =
		(struct CX9000_DRC_CTRL *)kcontrol->private_value;
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx9000->plbk_drc[drc->ch][drc->band];

	memcpy(param, cache, CX9000_PLBK_DRC_COEF_LEN);

	return 0;
}

static int cx9000_plbk_drc_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	struct CX9000_DRC_CTRL *drc =
	(struct CX9000_DRC_CTRL *)kcontrol->private_value;
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx9000->plbk_drc[drc->ch][drc->band];

	mutex_lock(&cx9000->drc_coeff_lock);

	/*do nothing if the value is the same*/
	if (!memcmp(cache, param, CX9000_PLBK_DRC_COEF_LEN))
		goto EXIT;

	memcpy(cache, param, CX9000_PLBK_DRC_COEF_LEN);

	cx9000->plbk_drc_changed = true;
	cx9000_update_dsp(component);
EXIT:
	mutex_unlock(&cx9000->drc_coeff_lock);
	return 0;
}

static int cx9000_plbk_sa2_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 106903601;
	uinfo->count = 1;

	return 0;
}

static int cx9000_plbk_sa2_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	struct CX9000_SA2_CTRL *sa2 =
		(struct CX9000_SA2_CTRL *)kcontrol->private_value;
	u32 val = 0;
	u8 *cache = cx9000->plbk_sa2[sa2->ch][sa2->band];
	int startbytes = (CX9000_PLBK_SA2_COEF_LEN -
			 CX9000_PLBK_SA2_POWER_THRESHOLD_LEN);
	cache += startbytes;

	val = (*(cache + 1) << 24) | (*(cache) << 16) |
	      (*(cache + 3) << 8) | (*(cache + 2) << 0);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int cx9000_plbk_sa2_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	struct CX9000_SA2_CTRL *sa2 =
		(struct CX9000_SA2_CTRL *)kcontrol->private_value;
	u32 param = ucontrol->value.integer.value[0];
	u8 *cache = cx9000->plbk_sa2[sa2->ch][sa2->band];
	int startbytes = (CX9000_PLBK_SA2_COEF_LEN -
			 CX9000_PLBK_SA2_POWER_THRESHOLD_LEN);
	cache += startbytes;

	mutex_lock(&cx9000->sa2_coeff_lock);
	param = cpu_to_le16((param >> 16) & 0xFFFF) |
		(cpu_to_le16(param & 0xFFFF) << 16);
	/*do nothing if the value is the same*/
	if (!memcmp(cache, &param, CX9000_PLBK_SA2_POWER_THRESHOLD_LEN))
		goto EXIT;

	memcpy(cache, &param, CX9000_PLBK_SA2_POWER_THRESHOLD_LEN);
	cx9000->plbk_sa2_changed = true;
	cx9000_update_dsp(component);
EXIT:
	mutex_unlock(&cx9000->sa2_coeff_lock);
	return 0;
}

static int cx9000_plbk_hpf_config(struct snd_soc_component *component,
				  unsigned int mask, unsigned int val_l,
				  unsigned int val_r)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	unsigned int lp_mode = cx9000->lp_mode;

	if (lp_mode == CX9000_OSS_LP2) {
		dev_dbg(component->dev,
			"Can't update HPF on the state (%s).\n",
			cx9000_oss_state[lp_mode]);
		return -EINVAL;
	}

	/*
	 * Configuration should occur after power-on-reset, but before enabling
	 * the device for operation in the OSS, or by placing the device into
	 * the Soft Disable state, also in the OSS.
	 */
	if ((lp_mode != CX9000_OSS_IDLE) && (lp_mode != CX9000_OSS_LP3)) {
		snd_soc_component_update_bits(component, CX9000_OSS_CSR, CX9000_OSS_CSR_MASK,
				    CX9000_OSS_CSR_SOFT_DISABLE);
		cx9000_check_oss_power_status(component, CX9000_OSS_CSR,
					      CX9000_OSS_LP3);
	}
	snd_soc_component_update_bits(component, CX9000_HPF_LEFT_CONTROL,
			    mask, val_l);
	snd_soc_component_update_bits(component, CX9000_HPF_RIGHT_CONTROL,
			    mask, val_r);

	if ((lp_mode != CX9000_OSS_IDLE) && (lp_mode != CX9000_OSS_LP3)) {
		snd_soc_component_update_bits(component, CX9000_OSS_CSR, CX9000_OSS_CSR_MASK,
				    CX9000_OSS_CSR_SOFT_CLEAR);
		cx9000_check_oss_power_status(component, CX9000_OSS_CSR, lp_mode);
	}

	snd_soc_component_write(component, CX9000_CSR_CTRL, CX9000_PID_HARDWARE);

	return 0;
}

static int cx9000_plbk_hpf_en_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 2;

	return 0;
}

static int cx9000_plbk_hpf_en_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);

	ucontrol->value.integer.value[0] =
		!((snd_soc_component_read32(component, CX9000_HPF_LEFT_CONTROL) >> 6) & 0x01);
	ucontrol->value.integer.value[1] =
		!((snd_soc_component_read32(component, CX9000_HPF_RIGHT_CONTROL) >> 6) & 0x01);

	return 0;
}

static int cx9000_plbk_hpf_en_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	const bool en_left = ucontrol->value.integer.value[0];
	const bool en_right = ucontrol->value.integer.value[1];

	if (ucontrol->value.integer.value[0] > 1 ||
	    ucontrol->value.integer.value[1] > 1)
		return -EINVAL;

	cx9000_plbk_hpf_config(component, CX9000_HPF_ENABLE_MASK,
			       en_left ? (en_left << 7) : CX9000_HPF_BYPASS,
			       en_right ? (en_right << 7) : CX9000_HPF_BYPASS
			      );

	return 0;
}

static int cx9000_plbk_hpf_2nder_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int cx9000_plbk_hpf_2nder_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);

	ucontrol->value.integer.value[0] =
		(snd_soc_component_read32(component, CX9000_HPF_LEFT_CONTROL) >> 5) & 0x01;
	ucontrol->value.integer.value[1] =
		(snd_soc_component_read32(component, CX9000_HPF_RIGHT_CONTROL) >> 5) & 0x01;

	return 0;
}

static int cx9000_plbk_hpf_2nder_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	u8 order_left = ucontrol->value.integer.value[0];
	u8 order_right = ucontrol->value.integer.value[1];

	if (ucontrol->value.integer.value[0] > 1 ||
	    ucontrol->value.integer.value[1] > 1)
		return -EINVAL;

	cx9000_plbk_hpf_config(component, CX9000_HPF_ORDER_MASK,
			       (order_left << 5), (order_right << 5));

	return 0;
}

static int cx9000_plbk_hpf_freq_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = CX9000_HPF_CUTOFF_MAX;

	return 0;
}

static int cx9000_plbk_hpf_freq_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);

	ucontrol->value.integer.value[0] =
		snd_soc_component_read32(component, CX9000_HPF_LEFT_CONTROL) &
			CX9000_HPF_CUTOFF_MASK;
	ucontrol->value.integer.value[1] =
		snd_soc_component_read32(component, CX9000_HPF_RIGHT_CONTROL) &
			CX9000_HPF_CUTOFF_MASK;

	return 0;
}

static int cx9000_plbk_hpf_freq_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	unsigned int freq_left = ucontrol->value.integer.value[0];
	unsigned int freq_right = ucontrol->value.integer.value[1];

	if ((ucontrol->value.integer.value[0] > CX9000_HPF_CUTOFF_MAX) ||
	    (ucontrol->value.integer.value[1] > CX9000_HPF_CUTOFF_MAX))
		return -EINVAL;

	cx9000_plbk_hpf_config(component, CX9000_HPF_CUTOFF_MASK,
			       freq_left, freq_right);
	return 0;
}

static int cx9000_get_dac_config(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = cx9000->dac_config;

	return 0;
}

static int cx9000_set_dac_config(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct cx9000_data *cx9000 =
		snd_soc_component_get_drvdata(component);
	unsigned int lp_mode = cx9000->lp_mode;
	unsigned int dac_config = ucontrol->value.integer.value[0];
	unsigned int val = CX9000_DAC_STEREO;

	if (lp_mode == CX9000_OSS_LP2) {
		dev_dbg(component->dev,
			"Sorry, Can't update dac config on the state (%s).\n",
			cx9000_oss_state[lp_mode]);
		return -EINVAL;
	}

	switch (dac_config) {
	case CX9000_STEREO:
		val = CX9000_DAC_STEREO;
		break;
	case CX9000_MONO_LEFT:
		val = CX9000_DAC_MONO;
		break;
	case CX9000_MONO_RIGHT:
		val = CX9000_DAC_MONO_RIGHT;
		break;
	case CX9000_BI_MONO:
		val = CX9000_DAC_MONO | CX9000_DAC_DUPLICATE;
		break;
	case CX9000_MONO_MIXED:
		val = CX9000_DAC_MONO_MIXED;
		break;
	case CX9000_BI_MIXED:
		val = CX9000_DAC_MONO_MIXED | CX9000_DAC_DUPLICATE;
		break;
	}

	/*
	 * Configuration should occur after power-on-reset, but before enabling
	 * the device for operation in the OSS, or by placing the device into
	 * the Soft Disable state, also in the OSS.
	 */
	if ((lp_mode != CX9000_OSS_IDLE) && (lp_mode != CX9000_OSS_LP3)) {
		snd_soc_component_update_bits(component, CX9000_OSS_CSR, CX9000_OSS_CSR_MASK,
				    CX9000_OSS_CSR_SOFT_DISABLE);
		cx9000_check_oss_power_status(component, CX9000_OSS_CSR,
					      CX9000_OSS_LP3);
	}

	snd_soc_component_update_bits(component, CX9000_DAC_CONFIGURATION,
			    CX9000_DAC_CONFIGURATION_MASK, val);

	if ((lp_mode != CX9000_OSS_IDLE) && (lp_mode != CX9000_OSS_LP3)) {
		snd_soc_component_update_bits(component, CX9000_OSS_CSR, CX9000_OSS_CSR_MASK,
				    CX9000_OSS_CSR_SOFT_CLEAR);
		cx9000_check_oss_power_status(component, CX9000_OSS_CSR, lp_mode);
	}

	snd_soc_component_write(component, CX9000_CSR_CTRL, CX9000_PID_HARDWARE);
	cx9000->dac_config = dac_config;

	return 0;
}

#define CX9000_PLBK_EQ_COEF(xname, xrate, xch, xband) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = cx9000_plbk_eq_info, \
	.get = cx9000_plbk_eq_get, .put = cx9000_plbk_eq_put, \
	.private_value = \
		(unsigned long)&(struct CX9000_EQ_CTRL) \
		 {.sample_rate = xrate, .ch = xch, .band = xband } }

#define CX9000_PLBK_DSP_EQ_SWITCH(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx9000_plbk_eq_en_info, \
	.get = cx9000_plbk_eq_en_get, .put = cx9000_plbk_eq_en_put}


#define CX9000_PLBK_DRC_COEF(xname, xch, xband) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = cx9000_plbk_drc_info, \
	.get = cx9000_plbk_drc_get, .put = cx9000_plbk_drc_put, \
	.private_value = \
		(unsigned long)&(struct CX9000_DRC_CTRL) \
		 {.ch = xch, .band = xband } }

#define CX9000_PLBK_DSP_DRC_SWITCH(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx9000_plbk_drc_en_info, \
	.get = cx9000_plbk_drc_en_get, .put = cx9000_plbk_drc_en_put}

#define CX9000_PLBK_SA2_COEF(xname, xch, xband) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = cx9000_plbk_sa2_info, \
	.get = cx9000_plbk_sa2_get, .put = cx9000_plbk_sa2_put, \
	.private_value = \
		(unsigned long)&(struct CX9000_SA2_CTRL) \
		 {.ch = xch, .band = xband } }

#define CX9000_PLBK_HPF_ENABLE(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx9000_plbk_hpf_en_info, \
	.get = cx9000_plbk_hpf_en_get, .put = cx9000_plbk_hpf_en_put}

#define CX9000_PLBK_HPF_2NDER_SEL(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx9000_plbk_hpf_2nder_info, \
	.get = cx9000_plbk_hpf_2nder_get, .put = cx9000_plbk_hpf_2nder_put}

#define CX9000_PLBK_HPF_FREQ(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx9000_plbk_hpf_freq_info, \
	.get = cx9000_plbk_hpf_freq_get, .put = cx9000_plbk_hpf_freq_put}

/*
 * DAC digital volumes. From -74 to 0 dB in 0.5 dB steps
 */
static DECLARE_TLV_DB_SCALE(dac_tlv, -7400, 50, 0);

static DECLARE_TLV_DB_RANGE(drc_high_in_out_tlv,
		0, 15, TLV_DB_SCALE_ITEM(0, 100, 0),
		16, 63, TLV_DB_SCALE_ITEM(-4800, 100, 0)
);
static DECLARE_TLV_DB_RANGE(drc_low_in_out_tlv,
		0, 15, TLV_DB_SCALE_ITEM(0, 0, 0),
		16, 63, TLV_DB_SCALE_ITEM(-9600, 200, 0)
);
static DECLARE_TLV_DB_RANGE(drc_rate_rel_tlv,
		0, 31, TLV_DB_SCALE_ITEM(0, 50, 0),
		40, 63, TLV_DB_SCALE_ITEM(-1200, 50, 0)
);

/* 0dB, +0.48dB, +0.32dB, +0.16dB, 0dB, -0.17dB, -0.33dB, -0.5dB */
static DECLARE_TLV_DB_RANGE(classd_gain_fine_tlv,
		0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
		1, 1, TLV_DB_SCALE_ITEM(48, 0, 0),
		2, 2, TLV_DB_SCALE_ITEM(32, 0, 0),
		3, 3, TLV_DB_SCALE_ITEM(16, 0, 0),
		4, 4, TLV_DB_SCALE_ITEM(0, 0, 0),
		5, 5, TLV_DB_SCALE_ITEM(-17, 0, 0),
		6, 6, TLV_DB_SCALE_ITEM(-33, 0, 0),
		7, 7, TLV_DB_SCALE_ITEM(-50, 0, 0)
);
static DECLARE_TLV_DB_SCALE(csdac_tlv, -1000, 100, 0);
static DECLARE_TLV_DB_RANGE(gain_shift_tlv,
		0, 7, TLV_DB_SCALE_ITEM(0, 602, 0)
);

static char const * const band_mode_text[] = {
	"Triple Band",
	"Double Band",
	"Single Band",
};
static char const * const vol_setting_sel_text[] = {
	"using TBDRC Volume Level",
	"using digital gain module DRC volume setting",
};
static char const * const pre_post_sel_text[] = {
	"Post DRC",
	"Pre DRC",
};
static char const * const sa2_mode_sel_text[] = {
	"Disable",
	"Hardware Driven",
	"Software Driven",
	"Fail-Safe",
};
static const char * const dac_config_text[] = {
	"Stereo", "Mono Left", "Mono Right",
	"Bi-Mono", "Mono Mixed", "Bi-Mixed"};

static const struct soc_enum band_mode_selection_enum =
	SOC_ENUM_SINGLE(CX9000_TBDRC_CTRL_REG1, 0,
			ARRAY_SIZE(band_mode_text),
			band_mode_text);
static const struct soc_enum drc_vol_setting_sel_enum =
	SOC_ENUM_SINGLE(CX9000_TBDRC_CTRL_REG1, 0,
			ARRAY_SIZE(vol_setting_sel_text),
			vol_setting_sel_text);
static const struct soc_enum pre_post_drc_sel_enum =
	SOC_ENUM_SINGLE(CX9000_MUTE_PRE_DRC_VOLUME, 0,
			ARRAY_SIZE(pre_post_sel_text),
			pre_post_sel_text);
static const struct soc_enum sa2_mode_sel_enum =
	SOC_ENUM_SINGLE(CX9000_CSR_CTRL, 0,
			ARRAY_SIZE(sa2_mode_sel_text),
			sa2_mode_sel_text);

static const struct soc_enum cx9000_dac_config_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(dac_config_text),
			    dac_config_text),
};

static char const * const expansion_sloop_text[] = {
	"slope 1",
	"slope 2",
	"slope 4",
	"slope 8",
};
static char const * const compr_sloop_text[] = {
	"slope 1",
	"slope 1/2",
	"slope 1/4",
	"slope 1/8",
	"slope 1/16",
	"slope 1/32",
	"slope 1/64",
	"slope 0",
};
static char const * const attach_update_rate_text[] = {
	"2 ms",
	"4 ms",
	"8 ms",
	"16 ms",
	"32 ms",
	"64 ms",
	"128 ms",
	"256 ms",
};
static char const * const release_delay_text[] = {
	"0.5 ms",
	"1 ms",
	"2 ms",
	"4 ms",
	"8 ms",
	"16 ms",
	"32 ms",
	"64 ms",
	"128 ms",
	"256 ms",
	"512 ms",
	"1024 ms",
	"2048 ms",
	"4096 ms",
	"8192 ms",
	"16384 ms",
};
static char const * const release_update_rate_text[] = {
	"2 ms",
	"4 ms",
	"8 ms",
	"16 ms",
	"32 ms",
	"64 ms",
	"128 ms",
	"256 ms",
	"512 ms",
	"1024 ms",
	"2048 ms",
	"4096 ms",
	"8192 ms",
	"16384 ms",
	"32768 ms",
	"65536 ms",
};

static char const * const classd_gain_txt[] = {
	"18dB", "15dB", "12dB"};
static char const * const speaker_impedance_txt[] = {
	"8ohm", "4ohm"};

/* Two Knee DRC w/ Noise Gate */
static SOC_ENUM_SINGLE_DECL(classd_gain,
		CX9000_ANALOG_GAIN, 4, classd_gain_txt);
static SOC_ENUM_SINGLE_DECL(speaker_impedance,
		CX9000_ANALOG_TEST26_H, 1, speaker_impedance_txt);

static SOC_ENUM_SINGLE_DECL(lp_expansion_sloop_enum,
		CX9000_TBDRC_LP_SLOOP, 6, expansion_sloop_text);
static SOC_ENUM_SINGLE_DECL(hp_expansion_sloop_enum,
		CX9000_TBDRC_HP_SLOOP, 6, expansion_sloop_text);
static SOC_ENUM_SINGLE_DECL(mp_expansion_sloop_enum,
		CX9000_TBDRC_MP_SLOOP, 6, expansion_sloop_text);
static SOC_ENUM_SINGLE_DECL(pd_expansion_sloop_enum,
		CX9000_TBDRC_PD_SLOOP, 6, expansion_sloop_text);

static SOC_ENUM_SINGLE_DECL(lp_mid_compr_sloop_enum,
		CX9000_TBDRC_LP_SLOOP, 3, compr_sloop_text);
static SOC_ENUM_SINGLE_DECL(hp_mid_compr_sloop_enum,
		CX9000_TBDRC_HP_SLOOP, 3, compr_sloop_text);
static SOC_ENUM_SINGLE_DECL(mp_mid_compr_sloop_enum,
		CX9000_TBDRC_MP_SLOOP, 3, compr_sloop_text);
static SOC_ENUM_SINGLE_DECL(pd_mid_compr_sloop_enum,
		CX9000_TBDRC_PD_SLOOP, 3, compr_sloop_text);

static SOC_ENUM_SINGLE_DECL(lp_high_compr_sloop_enum,
		CX9000_TBDRC_LP_SLOOP, 0, compr_sloop_text);
static SOC_ENUM_SINGLE_DECL(hp_high_compr_sloop_enum,
		CX9000_TBDRC_HP_SLOOP, 0, compr_sloop_text);
static SOC_ENUM_SINGLE_DECL(mp_high_compr_sloop_enum,
		CX9000_TBDRC_MP_SLOOP, 0, compr_sloop_text);
static SOC_ENUM_SINGLE_DECL(pd_high_compr_sloop_enum,
		CX9000_TBDRC_PD_SLOOP, 0, compr_sloop_text);

static SOC_ENUM_SINGLE_DECL(lp_attach_update_rate_enum,
		CX9000_TBDRC_LP_ATTACH_RATE_GAIN_SHIFT,
		4, attach_update_rate_text);
static SOC_ENUM_SINGLE_DECL(hp_attach_update_rate_enum,
		CX9000_TBDRC_HP_ATTACH_RATE_GAIN_SHIFT,
		4, attach_update_rate_text);
static SOC_ENUM_SINGLE_DECL(mp_attach_update_rate_enum,
		CX9000_TBDRC_MP_ATTACH_RATE_GAIN_SHIFT,
		4, attach_update_rate_text);
static SOC_ENUM_SINGLE_DECL(pd_attach_update_rate_enum,
		CX9000_TBDRC_PD_ATTACH_RATE_GAIN_SHIFT,
		4, attach_update_rate_text);

static SOC_ENUM_SINGLE_DECL(lp_release_delay_enum,
		CX9000_TBDRC_LP_RELEASE, 4, release_delay_text);
static SOC_ENUM_SINGLE_DECL(hp_release_delay_enum,
		CX9000_TBDRC_HP_RELEASE, 4, release_delay_text);
static SOC_ENUM_SINGLE_DECL(mp_release_delay_enum,
		CX9000_TBDRC_MP_RELEASE, 4, release_delay_text);
static SOC_ENUM_SINGLE_DECL(pd_release_delay_enum,
		CX9000_TBDRC_PD_RELEASE, 4, release_delay_text);

static SOC_ENUM_SINGLE_DECL(lp_release_update_rate_enum,
		CX9000_TBDRC_LP_RELEASE, 0, release_update_rate_text);
static SOC_ENUM_SINGLE_DECL(hp_release_update_rate_enum,
		CX9000_TBDRC_HP_RELEASE, 0, release_update_rate_text);
static SOC_ENUM_SINGLE_DECL(mp_release_update_rate_enum,
		CX9000_TBDRC_MP_RELEASE, 0, release_update_rate_text);
static SOC_ENUM_SINGLE_DECL(pd_release_update_rate_enum,
		CX9000_TBDRC_PD_RELEASE, 0, release_update_rate_text);

static SOC_ENUM_SINGLE_DECL(lp_fast_release_delay_enum,
		CX9000_TBDRC_LP_FAST_RELEASE, 4, release_delay_text);
static SOC_ENUM_SINGLE_DECL(hp_fast_release_delay_enum,
		CX9000_TBDRC_HP_FAST_RELEASE, 4, release_delay_text);
static SOC_ENUM_SINGLE_DECL(mp_fast_release_delay_enum,
		CX9000_TBDRC_MP_FAST_RELEASE, 4, release_delay_text);
static SOC_ENUM_SINGLE_DECL(pd_fast_release_delay_enum,
		CX9000_TBDRC_PD_FAST_RELEASE, 4, release_delay_text);

static SOC_ENUM_SINGLE_DECL(lp_fast_release_update_rate_enum,
		CX9000_TBDRC_LP_FAST_RELEASE, 0, release_update_rate_text);
static SOC_ENUM_SINGLE_DECL(hp_fast_release_update_rate_enum,
		CX9000_TBDRC_HP_FAST_RELEASE, 0, release_update_rate_text);
static SOC_ENUM_SINGLE_DECL(mp_fast_release_update_rate_enum,
		CX9000_TBDRC_MP_FAST_RELEASE, 0, release_update_rate_text);
static SOC_ENUM_SINGLE_DECL(pd_fast_release_update_rate_enum,
		CX9000_TBDRC_PD_FAST_RELEASE, 0, release_update_rate_text);

/* Export support features */
static const struct snd_kcontrol_new cx9000_snd_controls[] = {
	/* Configure Pre DRC Volume or Post-DRC volume */
	SOC_DOUBLE_R_RANGE_TLV("Playback Volume",
			       CX9000_VOLUME_LEFT, CX9000_VOLUME_RIGHT,
			       0, 0, 148, 0, dac_tlv),
	/* Configure Mute */
	SOC_SINGLE("Mute Left Channel", CX9000_MUTE_PRE_DRC_VOLUME, 5, 1, 0),
	SOC_SINGLE("Mute Right Channel", CX9000_MUTE_PRE_DRC_VOLUME, 4, 1, 0),

	/* Configuration EQ Switch (left and right separate) and coefficients */
	CX9000_PLBK_DSP_EQ_SWITCH("EQ Switch"),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 0", 0, 0, 0),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 1", 0, 0, 1),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 2", 0, 0, 2),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 3", 0, 0, 3),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 4", 0, 0, 4),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 5", 0, 0, 5),
	CX9000_PLBK_EQ_COEF("EQ 48K L Band 6", 0, 0, 6),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 0", 0, 1, 0),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 1", 0, 1, 1),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 2", 0, 1, 2),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 3", 0, 1, 3),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 4", 0, 1, 4),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 5", 0, 1, 5),
	CX9000_PLBK_EQ_COEF("EQ 48K R Band 6", 0, 1, 6),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 0", 1, 0, 0),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 1", 1, 0, 1),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 2", 1, 0, 2),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 3", 1, 0, 3),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 4", 1, 0, 4),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 5", 1, 0, 5),
	CX9000_PLBK_EQ_COEF("EQ 96K L Band 6", 1, 0, 6),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 0", 1, 1, 0),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 1", 1, 1, 1),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 2", 1, 1, 2),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 3", 1, 1, 3),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 4", 1, 1, 4),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 5", 1, 1, 5),
	CX9000_PLBK_EQ_COEF("EQ 96K R Band 6", 1, 1, 6),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 0", 2, 0, 0),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 1", 2, 0, 1),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 2", 2, 0, 2),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 3", 2, 0, 3),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 4", 2, 0, 4),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 5", 2, 0, 5),
	CX9000_PLBK_EQ_COEF("EQ 192K L Band 6", 2, 0, 6),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 0", 2, 1, 0),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 1", 2, 1, 1),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 2", 2, 1, 2),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 3", 2, 1, 3),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 4", 2, 1, 4),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 5", 2, 1, 5),
	CX9000_PLBK_EQ_COEF("EQ 192K R Band 6", 2, 1, 6),

	/* Configuration DRC crosspoint frequency coefficients */
	CX9000_PLBK_DSP_DRC_SWITCH("TBDRC Switch"),
	CX9000_PLBK_DRC_COEF("TBDRCL COEF LP1", 0, 0),
	CX9000_PLBK_DRC_COEF("TBDRCL COEF LP2", 0, 1),
	CX9000_PLBK_DRC_COEF("TBDRCL COEF HP2", 0, 2),
	CX9000_PLBK_DRC_COEF("TBDRCL COEF HP1", 0, 3),
	CX9000_PLBK_DRC_COEF("TBDRCL COEF AP",  0, 4),
	CX9000_PLBK_DRC_COEF("TBDRCR COEF LP1", 1, 0),
	CX9000_PLBK_DRC_COEF("TBDRCR COEF LP2", 1, 1),
	CX9000_PLBK_DRC_COEF("TBDRCR COEF HP2", 1, 2),
	CX9000_PLBK_DRC_COEF("TBDRCR COEF HP1", 1, 3),
	CX9000_PLBK_DRC_COEF("TBDRCR COEF AP",  1, 4),

	/* Configuration SA2 Coef */
	SOC_ENUM("SA2 Switch", sa2_mode_sel_enum),
	CX9000_PLBK_SA2_COEF("SA2 L PID Band 0", 0, 0),
	CX9000_PLBK_SA2_COEF("SA2 R PID Band 0", 1, 0),

	/* Configuration DRC REG0 */
	SOC_DOUBLE("TBDRC Mute",
		   CX9000_TBDRC_CTRL_REG0, 3, 4, 1, 0),
	/* Configuration DRC REG1 */
	SOC_ENUM("TBDRC Band Mode Select", band_mode_selection_enum),
	SOC_SINGLE("TBDRC Post-DRC Enable", CX9000_TBDRC_CTRL_REG1, 2, 1, 1),
	SOC_ENUM("TBDRC Volume Setting Select", drc_vol_setting_sel_enum),
	SOC_DOUBLE_R("TBDRC Volume Level",
		     CX9000_TBDRC_VOLUME_LEFT,
		     CX9000_TBDRC_VOLUME_RIGHT,
		     0, 160, 0),
	SOC_ENUM("TBDRC Volume Config", pre_post_drc_sel_enum),

	/*
	 * Basic Two Knee DRC w/ Noise Gate
	 */
	/* Configuration TBDRC High/Low Input/Output Threshold */
	SOC_SINGLE_TLV("TBDRC LP High Input Threshold",
		       CX9000_TBDRC_LP_HIGH_INPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC HP High Input Threshold",
		       CX9000_TBDRC_HP_HIGH_INPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC MP High Input Threshold",
		       CX9000_TBDRC_MP_HIGH_INPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC PD High Input Threshold",
		       CX9000_TBDRC_PD_HIGH_INPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC LP Low Input Threshold",
		       CX9000_TBDRC_LP_LOW_INPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC HP Low Input Threshold",
		       CX9000_TBDRC_HP_LOW_INPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC MP Low Input Threshold",
		       CX9000_TBDRC_MP_LOW_INPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC PD Low Input Threshold",
		       CX9000_TBDRC_PD_LOW_INPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC LP High Output Threshold",
		       CX9000_TBDRC_LP_HIGH_OUTPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC HP High Output Threshold",
		       CX9000_TBDRC_HP_HIGH_OUTPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC MP High Output Threshold",
		       CX9000_TBDRC_MP_HIGH_OUTPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC PD High Output Threshold",
		       CX9000_TBDRC_PD_HIGH_OUTPUT_THRD,
		       0, 63, 0, drc_high_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC LP Low Output Threshold",
		       CX9000_TBDRC_LP_LOW_OUTPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC HP Low Output Threshold",
		       CX9000_TBDRC_HP_LOW_OUTPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC MP Low Output Threshold",
		       CX9000_TBDRC_MP_LOW_OUTPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	SOC_SINGLE_TLV("TBDRC PD Low Output Threshold",
		       CX9000_TBDRC_PD_LOW_OUTPUT_THRD,
		       0, 63, 0, drc_low_in_out_tlv),
	/* Configuration DRC Slope */
	SOC_ENUM("TBDRC LP Expansion Slope", lp_expansion_sloop_enum),
	SOC_ENUM("TBDRC HP Expansion Slope", hp_expansion_sloop_enum),
	SOC_ENUM("TBDRC MP Expansion Slope", mp_expansion_sloop_enum),
	SOC_ENUM("TBDRC PD Expansion Slope", pd_expansion_sloop_enum),
	SOC_ENUM("TBDRC LP Middle Compression Slope", lp_mid_compr_sloop_enum),
	SOC_ENUM("TBDRC HP Middle Compression Slope", hp_mid_compr_sloop_enum),
	SOC_ENUM("TBDRC MP Middle Compression Slope", mp_mid_compr_sloop_enum),
	SOC_ENUM("TBDRC PD Middle Compression Slope", pd_mid_compr_sloop_enum),
	SOC_ENUM("TBDRC LP High Compression Slope", lp_high_compr_sloop_enum),
	SOC_ENUM("TBDRC HP High Compression Slope", hp_high_compr_sloop_enum),
	SOC_ENUM("TBDRC MP High Compression Slope", mp_high_compr_sloop_enum),
	SOC_ENUM("TBDRC PD High Compression Slope", pd_high_compr_sloop_enum),
	/* Configuration DRC Gain Shift and Attack Rate */
	SOC_ENUM("TBDRC LP Attack Rate", lp_attach_update_rate_enum),
	SOC_ENUM("TBDRC HP Attack Rate", hp_attach_update_rate_enum),
	SOC_ENUM("TBDRC MP Attack Rate", mp_attach_update_rate_enum),
	SOC_ENUM("TBDRC PD Attack Rate", pd_attach_update_rate_enum),
	SOC_SINGLE_TLV("TBDRC LP Gain Shift",
		       CX9000_TBDRC_LP_ATTACH_RATE_GAIN_SHIFT,
		       0, 7, 0, gain_shift_tlv),
	SOC_SINGLE_TLV("TBDRC HP Gain Shift",
		       CX9000_TBDRC_HP_ATTACH_RATE_GAIN_SHIFT,
		       0, 7, 0, gain_shift_tlv),
	SOC_SINGLE_TLV("TBDRC MP Gain Shift",
		       CX9000_TBDRC_MP_ATTACH_RATE_GAIN_SHIFT,
		       0, 7, 0, gain_shift_tlv),
	SOC_SINGLE_TLV("TBDRC PD Gain Shift",
		       CX9000_TBDRC_PD_ATTACH_RATE_GAIN_SHIFT,
		       0, 7, 0, gain_shift_tlv),
	/* Configuration DRC Release */
	SOC_ENUM("TBDRC LP Release Delay", lp_release_delay_enum),
	SOC_ENUM("TBDRC HP Release Delay", hp_release_delay_enum),
	SOC_ENUM("TBDRC MP Release Delay", mp_release_delay_enum),
	SOC_ENUM("TBDRC PD Release Delay", pd_release_delay_enum),
	SOC_ENUM("TBDRC LP Release Update Rate", lp_release_update_rate_enum),
	SOC_ENUM("TBDRC HP Release Update Rate", hp_release_update_rate_enum),
	SOC_ENUM("TBDRC MP Release Update Rate", mp_release_update_rate_enum),
	SOC_ENUM("TBDRC PD Release Update Rate", pd_release_update_rate_enum),
	/* Configuration DRC Fast Release */
	SOC_ENUM("TBDRC LP Fast Release Delay", lp_fast_release_delay_enum),
	SOC_ENUM("TBDRC HP Fast Release Delay", hp_fast_release_delay_enum),
	SOC_ENUM("TBDRC MP Fast Release Delay", mp_fast_release_delay_enum),
	SOC_ENUM("TBDRC PD Fast Release Delay",	pd_fast_release_delay_enum),
	SOC_ENUM("TBDRC LP Fast Release Update Rate",
		 lp_fast_release_update_rate_enum),
	SOC_ENUM("TBDRC HP Fast Release Update Rate",
		 hp_fast_release_update_rate_enum),
	SOC_ENUM("TBDRC MP Fast Release Update Rate",
		 mp_fast_release_update_rate_enum),
	SOC_ENUM("TBDRC PD Fast Release Update Rate",
		 pd_fast_release_update_rate_enum),
	/* Configuration DRC Release Rate Input Threshold */
	SOC_SINGLE_TLV("TBDRC LP Release Rate Input Threshold",
		       CX9000_TBDRC_LP_RATE_RELEASE,
		       0, 63, 0, drc_rate_rel_tlv),
	SOC_SINGLE_TLV("TBDRC HP Release Rate Input Threshold",
		       CX9000_TBDRC_HP_RATE_RELEASE,
		       0, 63, 0, drc_rate_rel_tlv),
	SOC_SINGLE_TLV("TBDRC MP Release Rate Input Threshold",
		       CX9000_TBDRC_MP_RATE_RELEASE,
		       0, 63, 0, drc_rate_rel_tlv),
	SOC_SINGLE_TLV("TBDRC PD Release Rate Input Threshold",
		       CX9000_TBDRC_PD_RATE_RELEASE,
		       0, 63, 0, drc_rate_rel_tlv),
	/* Configuration Balance Ramp Step */
	SOC_SINGLE("TBDRC LP Balance Ramp Step - MANTISSA",
		   CX9000_TBDRC_LP_BALANCE_RAMP_STEP,
		   4, 15, 0),
	SOC_SINGLE("TBDRC HP Balance Ramp Step - MANTISSA",
		   CX9000_TBDRC_HP_BALANCE_RAMP_STEP,
		   4, 15, 0),
	SOC_SINGLE("TBDRC MP Balance Ramp Step - MANTISSA",
		   CX9000_TBDRC_MP_BALANCE_RAMP_STEP,
		   4, 15, 0),
	SOC_SINGLE("TBDRC PD Balance Ramp Step - MANTISSA",
		   CX9000_TBDRC_PD_BALANCE_RAMP_STEP,
		   4, 15, 0),
	SOC_SINGLE("TBDRC LP Balance Ramp Step - EXPONENT",
		   CX9000_TBDRC_LP_BALANCE_RAMP_STEP,
		   0, 7, 0),
	SOC_SINGLE("TBDRC HP Balance Ramp Step - EXPONENT",
		   CX9000_TBDRC_HP_BALANCE_RAMP_STEP,
		   0, 7, 0),
	SOC_SINGLE("TBDRC MP Balance Ramp Step - EXPONENT",
		   CX9000_TBDRC_MP_BALANCE_RAMP_STEP,
		   0, 7, 0),
	SOC_SINGLE("TBDRC PD Balance Ramp Step - EXPONENT",
		   CX9000_TBDRC_PD_BALANCE_RAMP_STEP,
		   0, 7, 0),
	/* Configuration Volume Ramp Step Select */
	SOC_SINGLE("TBDRC LP Volume Ramp Step Select",
		   CX9000_TBDRC_LP_VOLUME_RAMP_STEP_SEL,
		   0, 7, 0),
	SOC_SINGLE("TBDRC HP Volume Ramp Step Select",
		   CX9000_TBDRC_HP_VOLUME_RAMP_STEP_SEL,
		   0, 7, 0),
	SOC_SINGLE("TBDRC MP Volume Ramp Step Select",
		   CX9000_TBDRC_MP_VOLUME_RAMP_STEP_SEL,
		   0, 7, 0),
	SOC_SINGLE("TBDRC PD Volume Ramp Step Select",
		   CX9000_TBDRC_PD_VOLUME_RAMP_STEP_SEL,
		   0, 7, 0),

	/* Configuration HPF Left/Right Control */
	CX9000_PLBK_HPF_ENABLE("HPF Enable"),
	CX9000_PLBK_HPF_2NDER_SEL("HPF 2nder"),
	CX9000_PLBK_HPF_FREQ("HPF Freq"),

	/* Configuration Class-D and DAC Gain */
	SOC_ENUM_EXT("DAC Config", cx9000_dac_config_enum[0],
		     cx9000_get_dac_config, cx9000_set_dac_config),
	SOC_SINGLE_TLV("CSDAC Gain", CX9000_ANALOG_GAIN, 0,
		       12, 0, csdac_tlv),
	SOC_ENUM("ClassD Gain", classd_gain),
	SOC_SINGLE_TLV("ClassD Gain Fine Control",
		       CX9000_ANALOG_TEST9_H,
		       1, 7, 0, classd_gain_fine_tlv),
	SOC_ENUM("Speaker Impedance", speaker_impedance),
};

static int cx9000_codec_probe(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);
	int ret, val;

	cx9000->component = component;

	cx9000_enable_device(component, 0);
	cx9000_enable_device(component, 1);

	cx9000_init(component);
	ret = regmap_register_patch(cx9000->regmap, cx9000_patch,
				    ARRAY_SIZE(cx9000_patch));

	val = snd_soc_component_read32(component, CX9000_OSS_CSR);
	cx9000->lp_mode = CX9000_OSS_STATE_GET(val);

	cx9000_init_eq(component);
	cx9000_init_drc(component);
	cx9000_init_sa2(component);

	cx9000->dac_config = CX9000_STEREO;
	cx9000->pll_clk_id = CX9000_PLL_CLKIN_BCLK;
	cx9000->i2spcm_changed = true;
	cx9000->pll_changed = true;

	cx9000->iter_PLL = 1;
	cx9000->iter_I2S = 1;

	/* Set GPIO5 as output and to let CX9000 enable itself */
	snd_soc_component_write(component, CX9000_GPIO_SELECT, 0x23);
	snd_soc_component_write(component, CX9000_GPIO_OUTPUT_ENABLE, 0x20);
	snd_soc_component_write(component, CX9000_GPIO_OUTPUT_LEVEL, 0x20);
	snd_soc_component_write(component, CX9000_GPIO_ELECTRICAL_CONTROL_5_4, 0x20);

	return 0;
}

static void cx9000_codec_remove(struct snd_soc_component *component)
{
	struct cx9000_data *cx9000 = snd_soc_component_get_drvdata(component);

	pm_runtime_put(component->dev);

	gpiod_set_value(cx9000->enable_gpio, 0);
};

const static struct snd_soc_component_driver soc_codec_dev_cx9000 = {
	.probe = cx9000_codec_probe,
	.remove = cx9000_codec_remove,
	.controls = cx9000_snd_controls,
	.num_controls = ARRAY_SIZE(cx9000_snd_controls),
	.dapm_widgets = cx9000_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cx9000_dapm_widgets),
	.dapm_routes = cx9000_audio_map,
	.num_dapm_routes = ARRAY_SIZE(cx9000_audio_map),
};

static const struct regmap_config cx9000_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,

	.max_register = CX9000_MAX_REG,
	.reg_defaults = cx9000_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(cx9000_reg_defs),
	.cache_type = REGCACHE_RBTREE,
	.volatile_reg = cx9000_volatile_register,
	.reg_read = cx9000_reg_read,
	.reg_write = cx9000_reg_write,
};

static int cx9000_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct device *dev;
	struct cx9000_data *data;
	int ret;

	dev = &client->dev;
	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data->enable_gpio = devm_gpiod_get_optional(dev, "enable",
						    GPIOD_OUT_LOW);
	if (IS_ERR(data->enable_gpio))
		return PTR_ERR(data->enable_gpio);

	data->cx9000_client = client;

	data->regmap = devm_regmap_init(&client->dev, NULL, client,
					&cx9000_regmap_config);
	if (IS_ERR(&data->regmap)) {
		ret = PTR_ERR(data->regmap);
		dev_err(&client->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	mutex_init(&data->eq_coeff_lock);
	mutex_init(&data->drc_coeff_lock);
	mutex_init(&data->sa2_coeff_lock);
	dev_set_drvdata(&client->dev, data);

	ret = devm_snd_soc_register_component(&client->dev,
				      &soc_codec_dev_cx9000,
				      cx9000_dai, ARRAY_SIZE(cx9000_dai));
	if (ret < 0)
		dev_err(&client->dev, "Failed to register component: %d\n", ret);

	dev_dbg(&client->dev, "%s, registered component success!!", __func__);

	pr_info("%s, cx9000 done\n", __func__);

	return ret;
}

static int cx9000_i2c_remove(struct i2c_client *client)
{
	pm_runtime_disable(&client->dev);
	return 0;
}

static const struct i2c_device_id cx9000_id[] = {
	{ "cx9000", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cx9000_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id cx9000_of_match[] = {
	{ .compatible = "cnxt,cx9000", },
	{},
};
MODULE_DEVICE_TABLE(of, cx9000_of_match);
#endif

static struct i2c_driver cx9000_i2c_driver = {
	.driver = {
		.name = "cx9000",
		.of_match_table = of_match_ptr(cx9000_of_match),
		.pm = &cx9000_pm,
	},
	.probe = cx9000_probe,
	.remove = cx9000_i2c_remove,
	.id_table = cx9000_id,
};

module_i2c_driver(cx9000_i2c_driver);

MODULE_AUTHOR("Simon Ho <simon.ho@synaptics.com>");
MODULE_DESCRIPTION("CX9000 Audio amplifier driver");
MODULE_LICENSE("GPL");
