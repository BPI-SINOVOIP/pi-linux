// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/err.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <linux/gpio/consumer.h>

#include "dmic.h"
#include "berlin_pcm.h"
#include "aio_hal.h"
#include "drv_aio.h"
#include "avio_common.h"
#include "aio_priv.h"

#define DMIC_CLK_TABLE_MAX       (10)

static struct dmic_clk_cfg dmic_clk_table[DMIC_CLK_TABLE_MAX] = {
	{8000,  196608000, 95, 64, 0, 1, 48},
	{11025, 180633600, 63, 256, 0, 1, 8},
	{16000, 196608000, 47, 192, 0, 1, 8},
	{22050, 180633600, 31, 128, 0, 1, 8},
	{32000, 196608000, 23, 96, 0, 1, 8},
	{44100, 180633600, 15, 64, 0, 1, 8},
	{48000, 196608000, 15, 64, 0, 1, 8},
	{64000, 196608000, 11, 48, 0, 1, 8},
	{88200, 180633600, 7, 32, 0, 1, 8},
	{96000, 196608000, 7, 32, 0, 1, 8},
};

enum pdm_type {
	PDM_A = 0,
	PDM_B,
	PDM_C,
};

enum pdm_clk {
	CLK_MST_A = 0,
	CLK_SLV_A,
	CLK_MST_B,
	CLK_SLV_B,
	CLK_I2S2_BCLK_XFEED,

};

static const char * const pdm_type_text[] = { "PDM_A", "PDM_B", "PDM_C" };
static const char * const pdm_clk_text[] = { "CLK_MST_A", "CLK_SLV_A",
						"CLK_MST_B", "CLK_SLV_B",
							"CLK_I2S2_BCLK_XFEED"};
static const char * const dc_state_text[] = {"OFF", "ON"};
static SOC_ENUM_SINGLE_EXT_DECL(dmic_pdm_type, pdm_type_text);
static SOC_ENUM_SINGLE_EXT_DECL(dmic_pdm_clk, pdm_clk_text);
static SOC_ENUM_SINGLE_EXT_DECL(dmic_dc_state, dc_state_text);
static const DECLARE_TLV_DB_MINMAX(dmic_gain_tlv, -7500, 1200);

static int pdm_type_control_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);

	ucontrol->value.enumerated.item[0] = dmic->pdm_type;

	return 0;
}

static int pdm_type_control_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);

	dmic->pdm_type = ucontrol->value.enumerated.item[0];

	return 0;
}

static int pdm_clk_control_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);

	ucontrol->value.enumerated.item[0] = dmic->pdm_clk_sel;

	return 0;
}

static int pdm_clk_control_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);

	dmic->pdm_clk_sel = ucontrol->value.enumerated.item[0];

	return 0;
}

static int dmic_dc_control_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);

	ucontrol->value.enumerated.item[0] = dmic->enable_dc_filter;

	return 0;
}

static int dmic_dc_control_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);

	dmic->enable_dc_filter = ucontrol->value.enumerated.item[0];

	return 0;
}

static int dmic_gain_control_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int ch = mc->reg;

	ucontrol->value.enumerated.item[0] = dmic->gain[ch];

	return 0;
}

static int dmic_gain_control_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *cpu_dai = snd_kcontrol_chip(kcontrol);
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(cpu_dai);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int ch = mc->reg;

	dmic->gain[ch] = ucontrol->value.enumerated.item[0];

	if ((ch % 2) == 0)
		dmic_set_gain_left(dmic->aio_handle, ch / 2, dmic->gain[ch]);
	else
		dmic_set_gain_right(dmic->aio_handle, ch / 2, dmic->gain[ch]);

	return 0;
}

static struct snd_kcontrol_new dmic_pdm_ctrls[] = {
	SOC_ENUM_EXT("DMIC PDM Type", dmic_pdm_type,
			pdm_type_control_get, pdm_type_control_put),
	SOC_ENUM_EXT("DMIC PDM CLK", dmic_pdm_clk,
			pdm_clk_control_get, pdm_clk_control_put),
	SOC_ENUM_EXT("DMIC DC FILTER SWITCH", dmic_dc_state,
			dmic_dc_control_get, dmic_dc_control_put),
	SOC_SINGLE_EXT_TLV("DMIC CH0 LEFT GAIN", 0, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH0 RIGHT GAIN", 1, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH1 LEFT GAIN", 2, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH1 RIGHT GAIN", 3, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH2 LEFT GAIN", 4, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH2 RIGHT GAIN", 5, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH3 LEFT GAIN", 6, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
	SOC_SINGLE_EXT_TLV("DMIC CH3 RIGHT GAIN", 7, 0, 0x2B8, 0,
			dmic_gain_control_get, dmic_gain_control_put,
			dmic_gain_tlv),
};

static int dmic_get_clk_config_index(u32 fs, int *apllclk, int *config_index)
{
	switch (fs) {
	case 8000:
		*apllclk = AIO_CLK_0;
		*config_index = 0;
		break;
	case 11025:
		*apllclk = AIO_CLK_1;
		*config_index = 1;
		break;
	case 16000:
		*apllclk = AIO_CLK_0;
		*config_index = 2;
		break;
	case 22050:
		*apllclk = AIO_CLK_1;
		*config_index = 3;
		break;
	case 32000:
		*apllclk = AIO_CLK_0;
		*config_index = 4;
		break;
	case 44100:
		*apllclk = AIO_CLK_1;
		*config_index = 5;
		break;
	case 48000:
		*apllclk = AIO_CLK_0;
		*config_index = 6;
		break;
	case 64000:
		*apllclk = AIO_CLK_0;
		*config_index = 7;
		break;
	case 88200:
		*apllclk = AIO_CLK_1;
		*config_index = 8;
		break;
	case 96000:
		*apllclk = AIO_CLK_0;
		*config_index = 9;
		break;
	default:
		snd_printk("fs %u not support\n", fs);
		return -EINVAL;
	}

	return 0;
}

static bool dmic_is_left_enable(u32 channel_map, int module)
{
	if (module < MAX_DMIC_MODULES)
		return channel_map & (0x2 << (module * 2));
	else
		return false;
}

static bool dmic_is_right_enable(u32 channel_map, int module)
{
	if (module < MAX_DMIC_MODULES)
		return channel_map & (0x1 << (module * 2));
	else
		return false;
}

static int dmic_pdm_startup(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	snd_printd("%s: start 0x%p 0x%p\n", __func__, ss, dai);

	return 0;
}

static void dmic_pdm_shutdown(struct snd_pcm_substream *ss,
				struct snd_soc_dai *dai)
{
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(dai);
	aio_i2s_clk_sync_reset(dmic->aio_handle, AIO_ID_PDM_RX);
	snd_printd("%s: end 0x%p 0x%p\n", __func__, ss, dai);
}

static int dmic_pdm_hw_params(struct snd_pcm_substream *ss,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params);
	int ret = 0, i, apllclk, config_index;
	struct berlin_ss_params ssparams;
	bool left, right;

	snd_printd("%s: ss(0x%p) dai(0x%p)\n", __func__, ss, dai);
	if (params_channels(params) > dmic->max_ch_inuse) {
		snd_printk("Not support channels number %d. Max %d\n",
		params_channels(params), dmic->max_ch_inuse);
		return -EINVAL;
	}

	dmic->ch_num = params_channels(params);

	/* dmic clock source initilization */
	aio_i2s_set_clock(dmic->aio_handle, AIO_ID_PDM_RX, 1, AIO_CLK_D3_SWITCH_NOR,
						AIO_CLK_SEL_D8, AIO_APLL_0, 1);

	/* dmic register setting */
	ret = dmic_get_clk_config_index(fs, &apllclk, &config_index);
	if (ret != 0)
		return ret;

	dmic_set_interface_clk(dmic->aio_handle, apllclk, dmic_clk_table[config_index].div_m,
					dmic_clk_table[config_index].div_n, dmic_clk_table[config_index].div_p);
	dmic_set_dpath_clk(dmic->aio_handle, dmic_clk_table[config_index].div_core);

	dmic_set_clk_source_enable(dmic->aio_handle, 0);
	usleep_range(20, 30);
	dmic_set_clk_source_enable(dmic->aio_handle, 1);

	/* set dmic module */
	dmic_module_enable(dmic->aio_handle, 0);
	for (i = 0; i < MAX_DMIC_MODULES; i++) {
		ret = dmic_pair_enable(dmic->aio_handle, i, 0);
		if (ret != 0)
			return ret;
	}
	dmic_set_slot(dmic->aio_handle, 23);

	dmic_set_cic_ratio(dmic->aio_handle, dmic_clk_table[config_index].cic_ratio);
	dmic_set_cic_ratio_D(dmic->aio_handle, dmic_clk_table[config_index].cic_ratio);

	dmic_set_fir_filter_sel(dmic->aio_handle, 0);
	for (i = 0; i < MAX_DMIC_MODULES; i++) {
		ret = dmic_enable_mono_mode(dmic->aio_handle, i, 0);
		if (ret != 0)
			return ret;
		ret = dmic_swap_left_right(dmic->aio_handle, i, 0);
		if (ret != 0)
			return ret;
		ret = dmic_set_time_order(dmic->aio_handle, i, 0);
		if (ret != 0)
			return ret;
	}
	dmic_rd_wr_flush(dmic->aio_handle);

	for (i = 0; i < MAX_DMIC_MODULES; i++) {
		if (dmic->enable_dc_filter) {
			ret = dmic_enable_dc_filter(dmic->aio_handle,
								i, 1);
			if (ret != 0)
				return ret;
		}
		left = dmic_is_left_enable(dmic->channel_map, i);
		right = dmic_is_right_enable(dmic->channel_map, i);
		ret = dmic_enable_run(dmic->aio_handle, i, left, right);
		if (ret != 0)
			return ret;
		ret = dmic_interface_clk_enable(dmic->aio_handle, i, 1);
		if (ret != 0)
			return ret;
		usleep_range(100, 200);
		dmic_interface_clk_enable_all(dmic->aio_handle, 1);
		ret = dmic_pair_enable(dmic->aio_handle, i, 1);
		if (ret != 0)
			return ret;
	}

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = DMICI_MODE;
	ssparams.interleaved = true;
	ssparams.dummy_data = true;
	ssparams.enable_mic_mute = !dmic->disable_mic_mute;
	ssparams.irq = dmic->irq;
	ssparams.dev_name = dmic->dev_name;
	ssparams.channel_map = dmic->channel_map;
	ssparams.ch_shift_check = dmic->ch_shift_check;
	ret = berlin_pcm_request_dma_irq(ss, &ssparams);

	if (ret == 0)
		dmic->irq_requested = true;

	berlin_pcm_max_ch_inuse(ss, dmic->max_ch_inuse);

	if (dmic->pdm_type == PDM_C)
		xfeed_set_pdm_sel(dmic->aio_handle, 0, 1, 0);
	else
		xfeed_set_pdm_sel(dmic->aio_handle,
			dmic->pdm_clk_sel, 0,
			dmic->pdm_type == PDM_A ? 0xF : 0);

	return ret;
}

static int dmic_pdm_hw_free(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(dai);
	int i, ret = 0;

	snd_printd("%s: ss(0x%p) dai(0x%p)\n", __func__, ss, dai);

	dmic_interface_clk_enable_all(dmic->aio_handle, 0);

	for (i = 0; i < MAX_DMIC_MODULES; i++) {
		ret = dmic_enable_run(dmic->aio_handle, i, 0, 0);
		if (ret != 0)
			return ret;
		ret = dmic_interface_clk_enable(dmic->aio_handle, i, 0);
		if (ret != 0)
			return ret;
		ret = dmic_pair_enable(dmic->aio_handle, i, 0);
		if (ret != 0)
			return ret;
		if (dmic->enable_dc_filter) {
			ret = dmic_enable_dc_filter(dmic->aio_handle,
								i, 0);
			if (ret != 0)
				return ret;
		}
	}

	if (dmic->irq_requested) {
		berlin_pcm_free_dma_irq(ss, 1, dmic->irq);
		dmic->irq_requested = false;
	}

	dmic_sw_reset(dmic->aio_handle);

	return ret;
}

static int dmic_pdm_trigger_start(struct snd_pcm_substream *ss,
				     struct snd_soc_dai *dai)
{
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(dai);
	int ret = 0, i;

	snd_printd("%s: ss(0x%p) dai(0x%p)\n", __func__, ss, dai);

	dmic_module_enable(dmic->aio_handle, 1);

	for (i = 0; i < MAX_DMIC_MODULES; i++) {
		ret = dmic_set_gain_left(dmic->aio_handle, i,
						dmic->gain[2 * i]);
		if (ret != 0)
			return ret;
		ret = dmic_set_gain_right(dmic->aio_handle, i,
						dmic->gain[2 * i + 1]);
		if (ret != 0)
			return ret;
	}

	return ret;
}

static int dmic_pdm_trigger_stop(struct snd_pcm_substream *ss,
				    struct snd_soc_dai *dai)
{
	struct dmic_data *dmic = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	snd_printd("%s: ss(0x%p) dai(0x%p)\n", __func__, ss, dai);

	dmic_module_enable(dmic->aio_handle, 0);

	return ret;
}

static int dmic_pdm_trigger(struct snd_pcm_substream *ss,
			      int cmd,
			      struct snd_soc_dai *dai)
{
	int ret;

	snd_printd("%s: ss(0x%p) dai(0x%p)\n", __func__, ss, dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = dmic_pdm_trigger_start(ss, dai);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		ret = dmic_pdm_trigger_stop(ss, dai);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int dmic_pdm_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, dmic_pdm_ctrls,
				 ARRAY_SIZE(dmic_pdm_ctrls));

	return 0;
}

static struct snd_soc_dai_ops dmic_pdm_dai_ops = {
	.startup = dmic_pdm_startup,
	.hw_params = dmic_pdm_hw_params,
	.hw_free = dmic_pdm_hw_free,
	.trigger = dmic_pdm_trigger,
	.shutdown = dmic_pdm_shutdown,
};

static struct snd_soc_dai_driver dmic_pdm_dai = {
	.name = "dmic_pdm",
	.probe = dmic_pdm_dai_probe,
	.capture = {
		.stream_name = "DMIC-PDM-Capture",
		.channels_min = MIN_CHANNELS,
		.channels_max = MAX_CHANNELS,
		.rates = PDM_PDMI_RATES,
		.formats = PDM_PDMI_FORMATS,
	},
	.ops = &dmic_pdm_dai_ops,
};

static const struct snd_soc_component_driver dmic_pdm_component = {
	.name = "dmic_pdm",
};

static int dmic_pdm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct dmic_data *dmic;
	int ret;
	u32 i = 0;
	int channel_setup;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	dmic = devm_kzalloc(dev, sizeof(struct dmic_data), GFP_KERNEL);
	if (!dmic)
		return -ENOMEM;

	dmic->irqc = platform_irq_count(pdev);

	if (dmic->irqc == 0) {
		snd_printk("no pdm channel defined in dts, do nothing\n");
		return 0;
	}
	for (i = 0; i < dmic->irqc; i++) {
		dmic->irq[i] = platform_get_irq(pdev, i);
		if (dmic->irq[i] <= 0) {
			snd_printk("%s: fail to get irq %d for %s\n",
				   __func__, dmic->irq[i], pdev->name);
			return -EINVAL;
		}
		dmic->chid[i] = irqd_to_hwirq(irq_get_irq_data(dmic->irq[i]));
		if (dmic->chid[i] < 0) {
			snd_printk("%s: got invalid dhub chid %d\n", __func__, dmic->chid[i]);
			return -EINVAL;
		}
	}

	dmic->dev_name = dev_name(dev);
	dmic->aio_handle = open_aio(dmic->dev_name);

	if (unlikely(dmic->aio_handle == NULL)) {
		snd_printk("%s: aio_handle:%p  get failed..\n", __func__, dmic->aio_handle);
		return -ENODEV;
	}

	for (i = 0; i < MAX_CHANNELS; i++)
		dmic->gain[i] = 0x2B8;

	dmic->channel_map = PCM_DEF_CHL_MAP;
	dmic->max_ch_inuse = MAX_CHANNELS;
	for (i = 0; i < MAX_CHANNELS; i++) {
		channel_setup = 1;
		ret = of_property_read_u32_index(np, "channels-map", i,
							 &channel_setup);
		if (!channel_setup) {
			dmic->channel_map &= ~(1 << i);
			dmic->max_ch_inuse -= 1;
		}
	}

	/*       pdmc_sel  pdm_clk_sel  pdm_sel
	 * PDMC    1         N/A         N/A
	 * PDMA    0         xxx         0xF
	 * PDMB    0         xxx         0x0
	 *
	 *			clk-pin						data-pin
	 * PDMC  I2S2_MCLK / ISS2_BCLKIO		SPDIFI
	 * PDMA  I2S2_MCLK / I2S2_BCLKIO  I2S2_DI3, I2S2_DI2, I2S2_DI1, I2S2_DI0
	 * PDMB  I2S2_MCLK / I2S2_BCLKIO  I2S2_DI3, I2S2_DI2, I2S1_DO3, I2S1_DO2
	 * 000: Clock A Master Mode and PDM_CLK_SEL(clock)
	 * 001: Clock A Slave Mode and PDMA_CLKIO_DI_2mux (from PinMux) (clock)
	 * 010: Clock B Master Mode and PDMB_CLKIO_DO_FB
	 * 011: Clock B Slave Mode and PDMB_CLKIO_DI_2mux (from PinMux) (clock)
	 * 1xx: I2S2 BCLK Cross feed and I2S2_BCLKIO_DI_2module
	 *		(After I2S2BCLK Final clock mux) (clock)
	 */
	ret = of_property_read_u32(np, "pdm-type", &dmic->pdm_type);
	if (ret != 0)
		dmic->pdm_type = PDM_C;

	ret = of_property_read_u32(np, "pdm-clk-sel", &dmic->pdm_clk_sel);
	if (ret != 0)
		dmic->pdm_clk_sel = 0;

	dmic->pdm_data_sel_gpio
		= devm_gpiod_get_optional(dev, "pdm-data-sel", GPIOD_OUT_LOW);
	/*
	 *	when return -EBUSY, it means this gpio is already
	 *	opened by another devcie, so set desc to NULL,
	 *	dont return error, keep probe going on;
	 */
	if (IS_ERR(dmic->pdm_data_sel_gpio)) {
		if (PTR_ERR(dmic->pdm_data_sel_gpio) == -EBUSY)
			dmic->pdm_data_sel_gpio = NULL;
		else {
			ret = PTR_ERR(dmic->pdm_data_sel_gpio);
			goto error;
		}
	}

	gpiod_set_value_cansleep(dmic->pdm_data_sel_gpio,
				dmic->pdm_type == PDM_C ? 0 : 1);

	dmic->disable_mic_mute = of_property_read_bool(np, "disablemicmute");
	dmic->enable_dc_filter = of_property_read_bool(np, "enable-dc-filter");
	dmic->ch_shift_check = of_property_read_bool(np, "ch-shift-check");

	snd_printk("%s: disable_mic_mute %d, enable_dc_filter %d, ch_shift_check %d\n",
				__func__,
				dmic->disable_mic_mute,
				dmic->enable_dc_filter,
				dmic->ch_shift_check);

	dev_set_drvdata(dev, dmic);

	ret = devm_snd_soc_register_component(dev,
					      &dmic_pdm_component,
					      &dmic_pdm_dai, 1);
	if (ret) {
		snd_printk("%s: failed to register DAI: %d\n", __func__, ret);
		goto error;
	}

	dmic_module_enable(dmic->aio_handle, 1);

	for (i = 0; i < MAX_DMIC_MODULES; i++) {
		ret = dmic_set_gain_left(dmic->aio_handle, i, 0x2B8);
		if (ret != 0)
			return ret;
		ret = dmic_set_gain_right(dmic->aio_handle, i, 0x2B8);
		if (ret != 0)
			return ret;
	}

	snd_printk("%s done irqc %d,  max ch inuse %d\n", __func__,
		   dmic->irqc, dmic->max_ch_inuse);
	return ret;

error:
	close_aio(dmic->aio_handle);
	dmic->aio_handle = NULL;

	return ret;
}

static int dmic_pdm_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dmic_data *dmic;

	dmic = (struct dmic_data *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (dmic && dmic->aio_handle) {
		close_aio(dmic->aio_handle);
		dmic->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id dmic_pdm_dt_ids[] = {
	{.compatible = "syna,vs680-dmic-pdm",},
	{.compatible = "syna,vs640-dmic-pdm",},
	{.compatible = "syna,as470-dmic-pdm",},
	{}
};
MODULE_DEVICE_TABLE(of, dmic_pdm_dt_ids);

static struct platform_driver dmic_pdm_driver = {
	.probe = dmic_pdm_probe,
	.remove = dmic_pdm_remove,
	.driver = {
		.name = "syna-dmic-pdm-dai",
		.of_match_table = dmic_pdm_dt_ids,
	},
};
module_platform_driver(dmic_pdm_driver);

MODULE_DESCRIPTION("Synaptics DMIC Capture ALSA driver");
MODULE_ALIAS("platform:dmic-pdm");
MODULE_LICENSE("GPL v2");
