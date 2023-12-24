/*
 * es8326.c -- es8326 ALSA SoC audio driver
 *
 * Copyright (c) 2016 Rockchip Electronics Co. Ltd.
 *
 * Author: Mark Brown <will@everset-semi.com>
 * Author: Jianqun Xu <jay.xu@rock-chips.com>
 * Author: Nickey Yang <nickey.yang@rock-chips.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <sound/jack.h>
#include "es8326.h"
#define DEBUG

#define ES8326_CODEC_SET_SPK	1
#define ES8326_CODEC_SET_HP	2

#define es8326_DEF_VOL	0x1b

#define ES8326_LRCK 48000



static int es8326_set_bias_level(struct snd_soc_component*codec,
				 enum snd_soc_bias_level level);

/*
 * es8326 register cache
 * We can't read the es8326 register space when we
 * are using 2 wire for device control, so we cache them instead.
 */

static const struct reg_default es8326_reg_defaults[] = {
	{0x00, 0x03}, {0x01, 0x03}, {0x02, 0x00}, {0x03, 0x20},
	{0x04, 0x11}, {0x05, 0x00}, {0x06, 0x11}, {0x07, 0x00},
	{0x08, 0x00}, {0x09, 0x01}, {0x0a, 0x00}, {0x0b, 0x00},
	{0x0c, 0xf8}, {0x0d, 0x3f}, {0x0e, 0x00}, {0x0f, 0x00},
	{0x10, 0x01}, {0x11, 0xfc}, {0x12, 0x28}, {0x13, 0x00},
	{0x14, 0x00}, {0x15, 0x33}, {0x16, 0x00}, {0x17, 0x00},
	{0x18, 0x88}, {0x19, 0x06}, {0x1a, 0x22}, {0x1b, 0x03},
	{0x1c, 0x0f}, {0x1d, 0x00}, {0x1e, 0x80}, {0x1f, 0x80},
	{0x20, 0x00}, {0x21, 0x00}, {0x22, 0xc0}, {0x23, 0x00},
	{0x24, 0x01}, {0x25, 0x08}, {0x26, 0x10}, {0x27, 0xc0},
	{0x28, 0x00}, {0x29, 0x1c}, {0x2a, 0x00}, {0x2b, 0xb0},
	{0x2c, 0x32}, {0x2d, 0x03}, {0x2e, 0x00}, {0x2f, 0x11},
	{0x30, 0x10}, {0x31, 0x00}, {0x32, 0x00}, {0x33, 0xc0},
	{0x34, 0xc0}, {0x35, 0x1f}, {0x36, 0xf7}, {0x37, 0xfd},
	{0x38, 0xff}, {0x39, 0x1f}, {0x3a, 0xf7}, {0x3b, 0xfd},
	{0x3c, 0xff}, {0x3d, 0x1f}, {0x3e, 0xf7}, {0x3f, 0xfd},
	{0x40, 0xff}, {0x41, 0x1f}, {0x42, 0xf7}, {0x43, 0xfd},
	{0x44, 0xff}, {0x45, 0x1f}, {0x46, 0xf7}, {0x47, 0xfd},
	{0x48, 0xff}, {0x49, 0x1f}, {0x4a, 0xf7}, {0x4b, 0xfd},
	{0x4c, 0xff}, {0x4d, 0x00}, {0x4e, 0x00}, {0x4f, 0xff},
	{0x50, 0x00}, {0x51, 0x00}, {0x52, 0x00}, {0x53, 0x00},
	{0x54, 0x00}, {0x55, 0x00}, {0x56, 0x00}, {0x57, 0x1f},
	{0x58, 0x00}, {0x59, 0x00}, {0x5a, 0x00}, {0x5b, 0x00},
	{0x5c, 0x00}, {0xf9, 0x00}, {0xfa, 0x00}, {0xfb, 0x00},
	{0xfc, 0x00}, {0xfd, 0x00}, {0xfe, 0x00}, {0xff, 0x00},
};
/* codec private data */
struct es8326_priv {
	struct clk *mclk;
	struct snd_pcm_hw_constraint_list *sysclk_constraints;
	struct i2c_client *i2c;
	struct regmap *regmap;
	struct snd_soc_component *codec;
	bool start;
	bool muted;
	bool hp_inserted;
	bool spk_gpio_level;
	bool hp_det_level;
	struct snd_soc_jack *jack;
	int irq;
	struct mutex lock;
	
    bool mastermode;
    u8 mic1_src;
    u8 mic2_src;
    u8 jack_pol;
    u32 mclk_rate;
};

//static struct es8326_priv *es8326_private;
//static struct snd_soc_component *es8326_codec;
/*
static int es8326_set_gpio(int gpio, bool level)
{
	struct es8326_priv *es8326 = es8326_private;

	if (!es8326) {
		return 0;
	}

	if ((gpio & ES8326_CODEC_SET_SPK) && es8326
	    && es8326->spk_ctl_gpio != -1) {
		gpio_set_value(es8326->spk_ctl_gpio, level);
	}

	return 0;
}*/

static irqreturn_t es8326_irq(int irq, void *dev_id)
{
	struct es8326_priv *es8326 = dev_id;
	unsigned int iface;

	mutex_lock(&es8326->lock);
    
    mutex_unlock(&es8326->lock);
	return IRQ_HANDLED;
}

static const SNDRV_CTL_TLVD_DECLARE_DB_SCALE(dac_vol_tlv, -9550, 50, 0);
static const SNDRV_CTL_TLVD_DECLARE_DB_SCALE(adc_vol_tlv, -9550, 50, 0);
static const SNDRV_CTL_TLVD_DECLARE_DB_SCALE(adc_pga_tlv, 0, 600, 0);
static const SNDRV_CTL_TLVD_DECLARE_DB_SCALE(softramp_rate, 0, 100, 0);
static const SNDRV_CTL_TLVD_DECLARE_DB_SCALE(drc_target_tlv, -3200, 200, 0);
static const SNDRV_CTL_TLVD_DECLARE_DB_SCALE(drc_recovery_tlv, -125, 250, 0);


static const char *const winsize[] ={
    "0.25db/2  LRCK",
    "0.25db/4  LRCK",
    "0.25db/8  LRCK",
    "0.25db/16  LRCK",
    "0.25db/32  LRCK",
    "0.25db/64  LRCK",
    "0.25db/128  LRCK",
    "0.25db/256  LRCK",
    "0.25db/512  LRCK",
    "0.25db/1024  LRCK",
    "0.25db/2048  LRCK",
    "0.25db/4096  LRCK",
    "0.25db/8192  LRCK",
    "0.25db/16384  LRCK",
    "0.25db/32768  LRCK",
    "0.25db/65536  LRCK",
};

static const char *const dacpol_txt[] =
	{ "Normal", "R Invert", "L Invert", "L + R Invert" };
static const struct soc_enum dacpol =
	SOC_ENUM_SINGLE(0x4d, 4, 4, dacpol_txt);
static const struct soc_enum alc_winsize = 
    SOC_ENUM_SINGLE(0x2e, 4, 16, winsize);
static const struct soc_enum drc_winsize = 
    SOC_ENUM_SINGLE(0x54, 4, 16, winsize);
static const struct snd_kcontrol_new es8326_snd_controls[]={
    SOC_SINGLE_TLV("DAC Playback Volume", 0x50, 0, 0xBF, 0,
		       dac_vol_tlv),
    SOC_ENUM("Playback Polarity", dacpol),
    SOC_SINGLE_TLV("DAC Ramp Rate", 0x4e, 0, 0x0f, 0,
		       softramp_rate),
    SOC_SINGLE("DRC Switch", 0x53, 3, 1, 0),
	SOC_SINGLE_TLV("DRC Recovery Level", 0x53, 0, 4, 0, drc_recovery_tlv),
    SOC_ENUM("DRC Winsize", drc_winsize),
    SOC_SINGLE_TLV("DRC Target Level", 0x54, 0, 0x0f, 0,
		       drc_target_tlv),

    SOC_DOUBLE_R_TLV("ADC Capture Volume", 0x2C, 0x2D, 0, 0xff, 0,
		       adc_vol_tlv),
    SOC_DOUBLE_TLV("ADC PGA Gain", 0x29, 4, 0, 5, 0, adc_pga_tlv), 
    SOC_SINGLE_TLV("ADC Ramp Rate", 0x2e, 0, 0x0f, 0,
		       softramp_rate),
    SOC_SINGLE("ALC Switch", 0x32, 3, 1, 0),
	SOC_SINGLE_TLV("ALC Recovery Level", 0x33, 0, 4, 0, drc_recovery_tlv),
    SOC_ENUM("ALC Winsize", alc_winsize),
    SOC_SINGLE_TLV("ALC Target Level", 0x33, 0, 0x0f, 0,
		       drc_target_tlv),

};

static const struct snd_soc_dapm_widget es8326_dapm_widgets[] = {
    SND_SOC_DAPM_INPUT("MIC1"),
    SND_SOC_DAPM_INPUT("MIC2"),

    SND_SOC_DAPM_ADC("Right ADC", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("Left ADC", NULL, SND_SOC_NOPM, 0, 0),

    	/* Digital Interface */
	SND_SOC_DAPM_AIF_OUT("I2S OUT", "I2S1 Capture",   0,
			    SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("I2S IN", "I2S1 Playback", 0,
			    SND_SOC_NOPM, 0, 0),

    SND_SOC_DAPM_DAC("Right DAC", NULL, ES8326_ANA_PDN_16, 0, 1),
	SND_SOC_DAPM_DAC("Left DAC", NULL, ES8326_ANA_PDN_16, 1, 1),
    SND_SOC_DAPM_PGA("LHPMIX", 0x25, 7, 0, NULL, 0),
    SND_SOC_DAPM_PGA("RHPMIX", 0x25, 3, 0, NULL, 0),
    SND_SOC_DAPM_SUPPLY("HPOR Cal", 0x27, 7, 1, NULL, 0),
    SND_SOC_DAPM_SUPPLY("HPOL Cal", 0x27, 3, 1, NULL, 0),
    SND_SOC_DAPM_REG(snd_soc_dapm_supply, "HPOR Supply", 0x27,
			 4, (7<<4), (7<<4), 0),
    SND_SOC_DAPM_REG(snd_soc_dapm_supply, "HPOL Supply", 0x27,
			 0, 7, 7, 0),
    SND_SOC_DAPM_OUTPUT("HPOL"),
	SND_SOC_DAPM_OUTPUT("HPOR"),

};

static const struct snd_soc_dapm_route es8326_dapm_routes[] = {
    {"Left ADC",NULL,"MIC1"},
    {"Right ADC",NULL,"MIC2"},

    {"I2S OUT", NULL, "Left ADC"},
    {"I2S OUT", NULL, "Right ADC"},

    {"Right DAC", NULL, "I2S IN"},
    {"Left DAC", NULL, "I2S IN"},
    
    {"LHPMIX",NULL,"Left DAC"},
    {"RHPMIX",NULL,"Right DAC"},

    {"HPOR", NULL ,"HPOR Cal"},
    {"HPOL", NULL ,"HPOL Cal"},
    {"HPOR", NULL ,"HPOR Supply"},
    {"HPOL", NULL ,"HPOL Supply"},

	{"HPOL", NULL, "LHPMIX"},
	{"HPOR", NULL, "RHPMIX"},

};

static const struct regmap_range es8316_volatile_ranges[] = {
	regmap_reg_range(0xfb, 0xfb),
};

static const struct regmap_access_table es8316_volatile_table = {
	.yes_ranges	= es8316_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(es8316_volatile_ranges),
};

const struct regmap_config es8326_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
	.volatile_table	= &es8316_volatile_table,
	.cache_type = REGCACHE_RBTREE,
};

struct _coeff_div {
	u16 fs;
	u32 rate;
	u32 mclk;
	u8 reg4;
	u8 reg5;
	u8 reg6;
	u8 reg7;
	u8 reg8;
	u8 reg9;
	u8 rega;
	u8 regb; 
};

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {

//ratio,LRCK,MCLK,REG04,REG05,REG06,REG07,REG08,REG09,REG10,REG11,,
{32,8000,256000,0x60,0x00,0x0F,0x75,0x0A,0x1B,0x1F,0x7F}, 
{32,16000,512000,0x20,0x00,0x0D,0x75,0x0A,0x1B,0x1F,0x3F}, 
{32,44100,1411200,0x00,0x00,0x13,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{32,48000,1536000,0x00,0x00,0x13,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{36,8000,288000,0x20,0x00,0x0D,0x75,0x0A,0x1B,0x23,0x47}, 
{36,16000,576000,0x20,0x00,0x0D,0x75,0x0A,0x1B,0x23,0x47}, 
{48,8000,384000,0x60,0x02,0x1F,0x75,0x0A,0x1B,0x1F,0x7F}, 
{48,16000,768000,0x20,0x02,0x0F,0x75,0x0A,0x1B,0x1F,0x3F}, 
{48,48000,2304000,0x00,0x02,0x0D,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{64,8000,512000,0x60,0x00,0x0D,0x75,0x0A,0x1B,0x1F,0x7F}, 
{64,16000,1024000,0x20,0x00,0x05,0x75,0x0A,0x1B,0x1F,0x3F}, 

{64,44100,2822400,0x00,0x00,0x11,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{64,48000,3072000,0x00,0x00,0x11,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{72,8000,576000,0x20,0x00,0x13,0x35,0x0A,0x1B,0x23,0x47}, 
{72,16000,1152000,0x20,0x00,0x05,0x75,0x0A,0x1B,0x23,0x47}, 
{96,8000,768000,0x60,0x02,0x1D,0x75,0x0A,0x1B,0x1F,0x7F}, 
{96,16000,1536000,0x20,0x02,0x0D,0x75,0x0A,0x1B,0x1F,0x3F}, 
{100,48000,4800000,0x04,0x04,0x1F,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{125,48000,6000000,0x04,0x04,0x1F,0x2D,0x0A,0x0A,0x27,0x27}, 
{128,8000,1024000,0x60,0x00,0x13,0x35,0x0A,0x1B,0x1F,0x7F}, 
{128,16000,2048000,0x20,0x00,0x11,0x35,0x0A,0x1B,0x1F,0x3F},

{128,44100,5644800,0x00,0x00,0x01,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{128,48000,6144000,0x00,0x00,0x01,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{144,8000,1152000,0x20,0x00,0x03,0x35,0x0A,0x1B,0x23,0x47}, 
{144,16000,2304000,0x20,0x00,0x11,0x35,0x0A,0x1B,0x23,0x47}, 
{192,8000,1536000,0x60,0x02,0x0D,0x75,0x0A,0x1B,0x1F,0x7F}, 
{192,16000,3072000,0x20,0x02,0x05,0x75,0x0A,0x1B,0x1F,0x3F}, 
{200,48000,9600000,0x04,0x04,0x0F,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{250,48000,12000000,0x04,0x04,0x0F,0x2D,0x0A,0x0A,0x27,0x27}, 
{256,8000,2048000,0x60,0x00,0x11,0x35,0x0A,0x1B,0x1F,0x7F}, 
{256,16000,4096000,0x20,0x00,0x01,0x35,0x0A,0x1B,0x1F,0x3F}, 

{256,44100,11289600,0x00,0x00,0x30,0x2B,0x1A,0x0A,0x4F,0x1F}, //works
{256,48000,12288000,0x00,0x00,0x30,0x2B,0x1A,0x0A,0x4F,0x1F}, //works
{288,8000,2304000,0x20,0x00,0x01,0x35,0x0A,0x1B,0x23,0x47}, 
{384,8000,3072000,0x60,0x02,0x05,0x75,0x0A,0x1B,0x1F,0x7F}, 
{384,16000,6144000,0x20,0x02,0x03,0x35,0x0A,0x1B,0x1F,0x3F}, 
{384,48000,18432000,0x00,0x02,0x01,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{400,48000,19200000,0x09,0x04,0x0f,0x6d,0x3a,0x0A,0x4F,0x1F},//works 
{500,48000,24000000,0x18,0x04,0x1F,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{512,8000,4096000,0x60,0x00,0x01,0x35,0x0A,0x1B,0x1F,0x7F}, 
{512,16000,8192000,0x20,0x00,0x10,0x35,0x0A,0x1B,0x1F,0x3F}, 

{512,44100,22579200,0x00,0x00,0x00,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{512,48000,24576000,0x00,0x00,0x00,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{768,8000,6144000,0x60,0x02,0x11,0x35,0x0A,0x1B,0x1F,0x7F}, 
{768,16000,12288000,0x20,0x02,0x01,0x35,0x0A,0x1B,0x1F,0x3F}, 
{800,48000,38400000,0x00,0x18,0x13,0x2D,0x0A,0x0A,0x1F,0x1F}, 
//{812.5,16000,13000000,0x0C,0x04,0x0F,0x2D,0x0A,0x0A,0x31,0x31}, 
{812,16000,13000000,0x0C,0x04,0x0F,0x2D,0x0A,0x0A,0x31,0x31}, 
{1024,8000,8192000,0x60,0x00,0x10,0x35,0x0A,0x1B,0x1F,0x7F}, 
{1024,16000,16384000,0x20,0x00,0x00,0x35,0x0A,0x1B,0x1F,0x3F}, 
{1152,16000,18432000,0x20,0x08,0x11,0x35,0x0A,0x1B,0x1F,0x3F}, 
{1536,8000,12288000,0x60,0x02,0x01,0x35,0x0A,0x1B,0x1F,0x7F}, 

{1536,16000,24576000,0x20,0x02,0x10,0x35,0x0A,0x1B,0x1F,0x3F}, 
{1625,8000,13000000,0x0C,0x18,0x1F,0x2D,0x0A,0x0A,0x27,0x27}, 
{1625,16000,26000000,0x0C,0x18,0x1F,0x2D,0x0A,0x0A,0x27,0x27}, 
{2048,8000,16384000,0x60,0x00,0x00,0x35,0x0A,0x1B,0x1F,0x7F}, 
{2304,8000,18432000,0x40,0x02,0x10,0x35,0x0A,0x1B,0x1F,0x5F}, 
{3072,8000,24576000,0x60,0x02,0x10,0x35,0x0A,0x1B,0x1F,0x7F}, 
{3250,8000,26000000,0x0C,0x18,0x0F,0x2D,0x0A,0x0A,0x27,0x27}, 
//{21.34,48000,1024320,0x00,0x00,0x09,0x2D,0x0A,0x0A,0x1F,0x1F}, 
{21,48000,1024320,0x00,0x00,0x09,0x2D,0x0A,0x0A,0x1F,0x1F}, 
//{541.67,48000,26000000,0x00,0x00,0x00,0x35,0x0A,0x1B,0x20,0x20}, 
{541,48000,26000000,0x00,0x00,0x00,0x35,0x0A,0x1B,0x20,0x20}, 
};

static inline int get_coeff(int mclk, int rate)
{
	int i;
	//mclk=ES8326_MCLK;
	rate=ES8326_LRCK;
	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
			return i;
	}

	return -EINVAL;
}

/* The set of rates we can generate from the above for each SYSCLK */

static unsigned int rates_12288[] = {
	8000, 12000, 16000, 24000, 24000, 32000, 48000, 96000,
};

static struct snd_pcm_hw_constraint_list constraints_12288 = {
	.count = ARRAY_SIZE(rates_12288),
	.list = rates_12288,
};

static unsigned int rates_112896[] = {
	8000, 11025, 22050, 44100,
};

static struct snd_pcm_hw_constraint_list constraints_112896 = {
	.count = ARRAY_SIZE(rates_112896),
	.list = rates_112896,
};

static unsigned int rates_12[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
	48000, 88235, 96000,
};

static struct snd_pcm_hw_constraint_list constraints_12 = {
	.count = ARRAY_SIZE(rates_12),
	.list = rates_12,
};

/*
 * Note that this should be called from init rather than from hw_params.
 */
static int es8326_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				 int clk_id, unsigned int freq, int dir)
{
	
	struct snd_soc_component*codec = codec_dai->component;
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);
	unsigned int freq2=es8326->mclk_rate;
	if (freq == 0) {
		es8326->sysclk_constraints->list = NULL;
		es8326->sysclk_constraints->count = 0;

		return 0;
	}
	printk(KERN_ERR"Entering %s\n",__func__);
	switch (freq2) {
	case 11289600:
	case 18432000:
	case 22579200:
	case 36864000:
		es8326->sysclk_constraints = &constraints_112896;
		return 0;

	case 12288000:
	case 16934400:
	case 24576000:
	case 33868800:
		es8326->sysclk_constraints = &constraints_12288;
		return 0;

	case 12000000:
	case 19200000:
	case 24000000:
		es8326->sysclk_constraints = &constraints_12;
		return 0;
	}
	
	return -EINVAL;
	
	
}

static int es8326_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	
	struct snd_soc_component*codec = codec_dai->component;
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);
	unsigned int iface = 0;

	printk(KERN_ERR"Entering %s\n",__func__);

#if 0
	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:	/* MASTER MODE */
		iface |= 0x80;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:	/* SLAVE MODE */
		iface &= 0x7F;
		break;
	default:
		return -EINVAL;
	}
#endif
	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
			iface &= 0xFC;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	case SND_SOC_DAIFMT_DSP_A:
		break;
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		return -EINVAL;
	}
#if 0
	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iface &= 0xDF;
		adciface &= 0xDF;
		daciface &= 0xBF;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x20;
		adciface |= 0x20;
		daciface |= 0x40;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x20;
		adciface &= 0xDF;
		daciface &= 0xBF;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface &= 0xDF;
		adciface |= 0x20;
		daciface |= 0x40;
		break;
	default:
		return -EINVAL;
	}
#endif
	snd_soc_component_write(codec, ES8326_FMT_13, iface);


	return 0;
}

static int es8326_pcm_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_component*codec = dai->component;
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);

	/* The set of sample rates that can be supported depends on the
	 * MCLK supplied to the CODEC - enforce this.
	 */
	printk(KERN_ERR"Entering %s\n",__func__);

    if (es8326->sysclk_constraints->list)
	snd_pcm_hw_constraint_list(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_RATE,
				   es8326->sysclk_constraints);

	return 0;
}

static int es8326_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component*codec = dai->component;
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);
	unsigned int srate = 0;
//	u8 srate = regmap_read(es8326->regmap, ES8326_FMT_13) & 0xe3;

	int coeff;

	printk(KERN_ERR"Entering %s\n",__func__);

	regmap_read(es8326->regmap,ES8326_FMT_13, &srate);
	srate = srate & 0xe3;
	coeff = get_coeff(es8326->mclk_rate, params_rate(params));
	printk(KERN_ERR"%s %d",__func__,coeff);
/*
	if (coeff < 0) {
		coeff = get_coeff(es8326->sysclk / 2, params_rate(params));
		//srate |= 0x40;
	}
	if (coeff < 0) {
		dev_err(codec->dev,
			"Unable to configure sample rate %dHz with %dHz MCLK\n",
			params_rate(params), es8326->sysclk);
		return coeff;
	}
	*/
	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		srate |= 0x0C;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		srate |= 0x04;
		break;
	case SNDRV_PCM_FORMAT_S18_3LE:
		srate |= 0x08;
	case SNDRV_PCM_FORMAT_S24_LE:
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		srate |= 0x10;
		break;
	}

	/* set iface & srate */
	snd_soc_component_write(codec, ES8326_FMT_13, srate);
	
	if (coeff >= 0) {
		printk(KERN_ERR"%s %d",__func__,coeff);
		snd_soc_component_write(codec, ES8326_CLK_DIV1_04,
				coeff_div[coeff].reg4);
		snd_soc_component_write(codec, ES8326_CLK_DIV2_05,
				coeff_div[coeff].reg5 );
		snd_soc_component_write(codec, ES8326_CLK_DLL_06,
				coeff_div[coeff].reg6 );
		snd_soc_component_write(codec, ES8326_CLK_MUX_07,
				coeff_div[coeff].reg7 );
		snd_soc_component_write(codec, ES8326_CLK_ADC_SEL_08,
				coeff_div[coeff].reg8 );
		snd_soc_component_write(codec, ES8326_CLK_DAC_SEL_09,
				coeff_div[coeff].reg9 );
		snd_soc_component_write(codec, ES8326_CLK_ADC_OSR_0A,
				coeff_div[coeff].rega );
		snd_soc_component_write(codec, ES8326_CLK_DAC_OSR_0B,
				coeff_div[coeff].regb );		  
	}
	
	return 0;
}



static int es8326_set_bias_level(struct snd_soc_component*codec,
				 enum snd_soc_bias_level level)
{
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);
	int ret;
	printk(KERN_ERR"Entering %s\n",__func__);

    switch (level) {
	case SND_SOC_BIAS_ON:
		dev_dbg(codec->dev, "%s on\n", __func__);
		break;
	case SND_SOC_BIAS_PREPARE:
		dev_dbg(codec->dev, "%s prepare\n", __func__);
		if (!IS_ERR(es8326->mclk))
		{
			if (snd_soc_component_get_bias_level(codec) == SND_SOC_BIAS_ON) {
				clk_disable_unprepare(es8326->mclk);
			} else {
				ret = clk_prepare_enable(es8326->mclk);
				if (ret)
					return ret;
			}
		}
		snd_soc_component_write(codec,0x01,0x7F);
		snd_soc_component_write(codec,0x00,0x00);
		snd_soc_component_write(codec,0x59,0x45);
		snd_soc_component_write(codec,0x5A,0x90);
		snd_soc_component_write(codec,0x5B,0x00);
		snd_soc_component_write(codec,0x03,0x05);
		snd_soc_component_write(codec,0x24,0x00);
		snd_soc_component_write(codec,0x18,0x02);
		snd_soc_component_write(codec,0x16,0x00);
		snd_soc_component_write(codec,0x17,0x40);
		snd_soc_component_write(codec,0x25,0xAA);
		snd_soc_component_write(codec,0x15,0x00);
		snd_soc_component_write(codec,0x00,0x80);

		break;
	case SND_SOC_BIAS_STANDBY:
#if 0
		dev_dbg(codec->dev, "%s standby xx \n", __func__);
		snd_soc_component_write(codec,0x15,0x1F);
		snd_soc_component_write(codec,0x25,0x11);
		snd_soc_component_write(codec,0x00,0x20);
		snd_soc_component_write(codec,0x17,0xF8);
		snd_soc_component_write(codec,0x16,0xFB);
		snd_soc_component_write(codec,0x18,0x00);
		snd_soc_component_write(codec,0x24,0x0F);
		snd_soc_component_write(codec,0x58,0x08);
		snd_soc_component_write(codec,0x5A,0x00);
		snd_soc_component_write(codec,0x5B,0x00);
		snd_soc_component_write(codec,0x00,0x2F);
		snd_soc_component_write(codec,0x01,0x00);
#endif
		break;
	case SND_SOC_BIAS_OFF:
#if 0
		if (es8326->mclk)
			clk_disable_unprepare(es8326->mclk);
		dev_dbg(codec->dev, "%s off\n", __func__);
		//snd_soc_component_write(codec, ES8326_ADCPOWER, 0xFF);
		snd_soc_component_write(codec,0x15,0x1F);
		snd_soc_component_write(codec,0x25,0x11);
		snd_soc_component_write(codec,0x00,0x20);
		snd_soc_component_write(codec,0x17,0xF8);
		snd_soc_component_write(codec,0x16,0xFB);
		snd_soc_component_write(codec,0x18,0x00);
		snd_soc_component_write(codec,0x24,0x0F);
		snd_soc_component_write(codec,0x58,0x08);
		snd_soc_component_write(codec,0x5A,0x00);
		snd_soc_component_write(codec,0x5B,0x00);
		snd_soc_component_write(codec,0x00,0x2F);
		snd_soc_component_write(codec,0x01,0x00);
#endif
		break;
	}
	return 0;
}

#define es8326_RATES (SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_44100)

#define es8326_FORMATS (SNDRV_PCM_FMTBIT_S32_LE|SNDRV_PCM_FMTBIT_S16_LE)

static struct snd_soc_dai_ops es8326_ops = {
	//.startup = es8326_pcm_startup,
	.hw_params = es8326_pcm_hw_params,
	.set_fmt = es8326_set_dai_fmt,
	.set_sysclk = es8326_set_dai_sysclk,
	//.digital_mute = es8326_mute,
};

static struct snd_soc_dai_driver es8326_dai = {
	.name = "es8326-amplifier",
	.playback = {
		     .stream_name = "Playback",
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = es8326_RATES,
		     .formats = es8326_FORMATS,
		     },
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 2,
		    .rates = es8326_RATES,
		    .formats = es8326_FORMATS,
		    },
	.ops = &es8326_ops,
	//.symmetric_rate = 1,
};

static int es8326_suspend(struct snd_soc_component*codec)
{
	//snd_soc_component_write(codec, 0x19, 0x06);
	printk(KERN_ERR"%s",__func__);
	usleep_range(18000, 20000);
	return 0;
}

static int es8326_resume(struct snd_soc_component*codec)
{
	//snd_soc_component_write(codec, 0x2b, 0x80);
	printk(KERN_ERR"%s",__func__);
	return 0;
}


static int es8326_probe(struct snd_soc_component*codec)
{
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);
	int ret = 0;
	unsigned int srate = 0;
	es8326->codec=codec;

	if (codec == NULL) {
		dev_err(codec->dev, "Codec device not registered\n");
		return -ENODEV;
	}
/*
    es8326->mclk = devm_clk_get_optional(codec->dev, "mclk");
	if (IS_ERR(es8326->mclk)) {
		dev_err(codec->dev,"%s,unable to get mclk\n", __func__);
		return PTR_ERR(es8326->mclk);
	}
	if (!es8326->mclk)
		dev_err(codec->dev,"%s, assuming static mclk\n", __func__);

	ret = clk_prepare_enable(es8326->mclk);
	if (ret) {
		dev_err(codec->dev,"%s, unable to enable mclk\n", __func__);
		return ret;
	}
*/

	int esid = 0;
	esid = snd_soc_component_read32(codec, 0xFD);
	dev_err(codec->dev, "es8326 id = %d\n",esid);
	
	snd_soc_component_write(codec,0x04,0x3C);
	snd_soc_component_write(codec,0x01,0x7F);
	snd_soc_component_write(codec,0xF9,0x02);//2CH ADC
	snd_soc_component_write(codec,0x02,0x00);
	snd_soc_component_write(codec,0x03,0x05);
	snd_soc_component_write(codec,0x04,0x01);
	snd_soc_component_write(codec,0x05,0x00);
	snd_soc_component_write(codec,0x06,0x30);
	snd_soc_component_write(codec,0x07,0x2D);
	snd_soc_component_write(codec,0x08,0x26);
	snd_soc_component_write(codec,0x09,0x26);
	snd_soc_component_write(codec,0x0A,0x1F);
	snd_soc_component_write(codec,0x0B,0x1F);
//	snd_soc_component_write(codec,0x0C,0x1F);
	snd_soc_component_write(codec,0x0C,0x04);

	snd_soc_component_write(codec,0x10,0xC8);
	snd_soc_component_write(codec,0x11,0x88);
	snd_soc_component_write(codec,0x12,0x20);
	snd_soc_component_write(codec,0x13,0x00);
	snd_soc_component_write(codec,0x14,0x00);
	snd_soc_component_write(codec,0x19,0xF0);
	snd_soc_component_write(codec,0x1B,0x7c);
	snd_soc_component_write(codec,0x1D,0x08);
	snd_soc_component_write(codec,0x22,0x3f); //add
	snd_soc_component_write(codec,0x23,0x96);
	dev_err(codec->dev, "Codec device 0x23 = 0x96\n");
	snd_soc_component_write(codec,0x25,0x22);
//	snd_soc_component_write(codec,0x29,0x00);
	snd_soc_component_write(codec,0x29,0x00);

	//snd_soc_component_write(codec,0x2A,0x00);
	//snd_soc_component_write(codec,0x2B,0x44);
    snd_soc_component_write(codec,0x2A,es8326->mic1_src);
//    snd_soc_component_write(codec,0x2A,es8326->mic2_src);

    snd_soc_component_write(codec,0x2B,es8326->mic2_src);
	snd_soc_component_write(codec,0x2C,0xDF);
	snd_soc_component_write(codec,0x2D,0xDF);
	dev_err(codec->dev, "Codec device 0x2C = 0xDF\n");
	snd_soc_component_write(codec,0x2E,0x00);
	snd_soc_component_write(codec,0x4A,0x00);
	snd_soc_component_write(codec,0x4D,0x08);
	snd_soc_component_write(codec,0x4E,0x20);
	snd_soc_component_write(codec,0x4F,0x15);
	snd_soc_component_write(codec,0x50,0xBF);
	snd_soc_component_write(codec,0x56,0x88);
    snd_soc_component_write(codec,0x57,0x10|es8326->jack_pol);
	//snd_soc_component_write(codec,0x57,0x1f);
	snd_soc_component_write(codec,0x58,0x08);
	snd_soc_component_write(codec,0x59,0x45);
	snd_soc_component_write(codec,0x5A,0x98);
	snd_soc_component_write(codec,0x5B,0x00);
	snd_soc_component_write(codec,0x15,0x00);
	snd_soc_component_write(codec,0x00,0x80);
	snd_soc_component_write(codec,0x27,0x77);

//  **** test
//{200,48000,9600000,0x04,0x04,0x0F,0x2D,0x0A,0x0A,0x1F,0x1F}, 
//{500,48000,24000000,0x18,0x04,0x1F,0x2D,0x0A,0x0A,0x1F,0x1F}, 
//{256,48000,12288000,0x00,0x00,0x30,0x2B,0x1A,0x0A,0x4F,0x1F},

//regmap_read(es8326->regmap,ES8326_FMT_13, &srate);
//srate = srate & 0xe3;
//srate |= 0x0C;
/*
snd_soc_component_write(codec, ES8326_FMT_13, srate);
snd_soc_component_write(codec, ES8326_CLK_DIV1_04,0x18);
snd_soc_component_write(codec, ES8326_CLK_DIV2_05,0x04);
snd_soc_component_write(codec, ES8326_CLK_DLL_06,0x1F);
snd_soc_component_write(codec, ES8326_CLK_MUX_07,0x2D);
snd_soc_component_write(codec, ES8326_CLK_ADC_SEL_08,0x0A);
snd_soc_component_write(codec, ES8326_CLK_DAC_SEL_09,0x0A);
snd_soc_component_write(codec, ES8326_CLK_ADC_OSR_0A,0x1F);
snd_soc_component_write(codec, ES8326_CLK_DAC_OSR_0B,0x1F);
*/
snd_soc_component_write(codec, ES8326_FMT_13, srate);
snd_soc_component_write(codec, ES8326_CLK_DIV1_04,0x00);
snd_soc_component_write(codec, ES8326_CLK_DIV2_05,0x00);
snd_soc_component_write(codec, ES8326_CLK_DLL_06,0x30);
snd_soc_component_write(codec, ES8326_CLK_MUX_07,0x2B);
snd_soc_component_write(codec, ES8326_CLK_ADC_SEL_08,0x1A);
snd_soc_component_write(codec, ES8326_CLK_DAC_SEL_09,0x0A);
snd_soc_component_write(codec, ES8326_CLK_ADC_OSR_0A,0x4F);
snd_soc_component_write(codec, ES8326_CLK_DAC_OSR_0B,0x1F);

//  **** test end


	
	//es8326_pcm_add_widgets(codec);
	//es8326_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	es8326_set_bias_level(codec, SND_SOC_BIAS_PREPARE);
	return 0;
}

static void es8326_remove(struct snd_soc_component*codec)
{
	es8326_set_bias_level(codec, SND_SOC_BIAS_OFF);	
}

static void es8326_enable_jack_detect(struct snd_soc_component *codec,
				      struct snd_soc_jack *jack)
{
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);

	dev_dbg(codec->dev,"Enter into %s\n", __func__);
	
	mutex_lock(&es8326->lock);

	es8326->jack = jack;

	mutex_unlock(&es8326->lock);
	/* Enable irq and sync initial jack state */
	enable_irq(es8326->irq);
	es8326_irq(es8326->irq, es8326);
	
}



static void es8326_disable_jack_detect(struct snd_soc_component *codec)
{
	struct es8326_priv *es8326 = snd_soc_component_get_drvdata(codec);

	dev_dbg(codec->dev,"Enter into %s\n", __func__);
	if (!es8326->jack)
		return; /* Already disabled (or never enabled) */

	disable_irq(es8326->irq);

	mutex_lock(&es8326->lock);

	if (es8326->jack->status & SND_JACK_MICROPHONE) {
		snd_soc_jack_report(es8326->jack, 0, SND_JACK_BTN_0);
	}

	es8326->jack = NULL;

	mutex_unlock(&es8326->lock);
}

static int es8326_set_jack(struct snd_soc_component *codec,
			   struct snd_soc_jack *jack, void *data)
{
	dev_dbg(codec->dev,"Enter into %s\n", __func__);
	if (jack)
		es8326_enable_jack_detect(codec, jack);
	else
		es8326_disable_jack_detect(codec);
	return 0;
}
static struct snd_soc_component_driver soc_codec_dev_es8326 = {
	.probe = es8326_probe,
	.remove = es8326_remove,
	.suspend = es8326_suspend,
	.resume = es8326_resume,
	.set_bias_level = es8326_set_bias_level,
	.set_jack = es8326_set_jack,
	.dapm_widgets		= es8326_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(es8326_dapm_widgets),
	.dapm_routes		= es8326_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(es8326_dapm_routes),
	.controls		= es8326_snd_controls,
	.num_controls		= ARRAY_SIZE(es8326_snd_controls),

};

static int es8326_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct es8326_priv *es8326;
	u8 reg;
	int ret = -1;
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);


	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_warn(&adapter->dev,
			 "I2C-Adapter doesn't support I2C_FUNC_I2C\n");
		return -EIO;
	}

	dev_err(&adapter->dev, "es8326_i2c_probe enter .....\n");

	es8326 = devm_kzalloc(&i2c->dev, sizeof(struct es8326_priv), GFP_KERNEL);
	if (es8326 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, es8326);
	es8326->i2c = i2c;
	mutex_init(&es8326->lock);
	es8326->regmap = devm_regmap_init_i2c(i2c, &es8326_regmap_config);
	if (IS_ERR(es8326->regmap)) {
		ret = PTR_ERR(es8326->regmap);
		dev_err(&i2c->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}
	
	reg = 0x00;
	ret = i2c_master_recv(i2c, &reg, 1);
	if (ret < 0) {
		dev_err(&i2c->dev, "i2c recv Failed\n");
		return ret;
	}
    es8326->irq = i2c->irq;
	ret = devm_request_threaded_irq(&i2c->dev, es8326->irq, NULL, es8326_irq,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"es8326", es8326);
	if(ret==0)	
	{   
        disable_irq(es8326->irq);
    }
    else{
        dev_warn(&i2c->dev,"Getting irq failed.");
    }
	
    es8326->mastermode=device_property_read_bool(&i2c->dev,"mastermode");
	dev_dbg(&i2c->dev,"master mode %d",es8326->mastermode);

    ret = of_property_read_u8(i2c->dev.of_node, "mic1-src", &es8326->mic1_src);
	if(ret!=0){
        dev_dbg(&i2c->dev,"mic1-src return %d",ret);
        es8326->mic1_src = 0x22;
    }
    dev_dbg(&i2c->dev,"mic1-src %x",es8326->mic1_src);

    ret = of_property_read_u8(i2c->dev.of_node, "mic2-src", &es8326->mic2_src);
	if(ret!=0){
        dev_dbg(&i2c->dev,"mic2-src return %d",ret);
        es8326->mic2_src = 0x44;
    }
    dev_dbg(&i2c->dev,"mic2-src %x",es8326->mic2_src);
       
    ret = of_property_read_u8(i2c->dev.of_node, "jack-pol", &es8326->jack_pol);
	if(ret!=0){
        dev_dbg(&i2c->dev,"jack-pol return %d",ret);
        es8326->jack_pol = 0x0f;
    }
    dev_dbg(&i2c->dev,"jack-pol %x",es8326->jack_pol);

    ret = of_property_read_u32(i2c->dev.of_node, "mclk-rate", &es8326->mclk_rate);
	if(ret!=0){
        dev_dbg(&i2c->dev,"mclk-rate return %d",ret);
        es8326->mclk_rate = 24576000;
    }
    dev_dbg(&i2c->dev,"mclk-rate %u",es8326->mclk_rate);

	ret = snd_soc_register_component(&i2c->dev,
				     &soc_codec_dev_es8326,
				     &es8326_dai, 1);
	dev_err(&adapter->dev, "snd_soc_register_component ret %d .....\n",ret);				 
	return ret;
}

static int es8326_i2c_remove(struct i2c_client *client)
{
	//snd_soc_unregister_codec(&client->dev);
	return 0;
}

void es8326_i2c_shutdown(struct i2c_client *client)
{
	//struct es8326_priv *es8326 = es8326_private;

	//es8326_set_gpio(ES8326_CODEC_SET_SPK, !es8326->spk_gpio_level);
	mdelay(20);
	//snd_soc_component_write(es8326_codec, ES8326_CONTROL2, 0x58);

}

static const struct i2c_device_id es8326_i2c_id[] = {
	{"es8326", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, es8326_i2c_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id es8326_acpi_match[] = {
	{"ESSX8326", 0},
	{},
};
MODULE_DEVICE_TABLE(acpi, es8326_acpi_match);
#endif

static const struct of_device_id es8326_of_match[] = {
	{ .compatible = "everest,es8326", },
	{ }
};
MODULE_DEVICE_TABLE(of, es8326_of_match);

static struct i2c_driver es8326_i2c_driver = {
	.driver = {
		.name = "es8326",
#ifdef CONFIG_ACPI		
		.acpi_match_table	= ACPI_PTR(es8326_acpi_match),
#endif	
		.of_match_table = of_match_ptr(es8326_of_match),
		},
	.shutdown = es8326_i2c_shutdown,
	.probe = es8326_i2c_probe,
	.remove = es8326_i2c_remove,
	.id_table = es8326_i2c_id,
};
static int __init es8326_modinit(void)
{
	int ret;
	printk(KERN_ERR"%s enter\n", __func__);
	ret = i2c_add_driver(&es8326_i2c_driver);
	if(ret != 0)
		printk(KERN_ERR"Failed to register es8326 i2c driver\n");
	return ret;
}
late_initcall(es8326_modinit);

MODULE_DESCRIPTION("ASoC es8326 driver");
MODULE_AUTHOR("David <zhuning@everset-semi.com>");
MODULE_LICENSE("GPL");

