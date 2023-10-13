// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <sound/soc.h>

#include "berlin_pcm.h"
#include "berlin_util.h"
#include "aio_hal.h"
#include "avio_common.h"

#define I2S_PLAYBACK_RATES   (SNDRV_PCM_RATE_8000_192000)
#define I2S_PLAYBACK_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
				| SNDRV_PCM_FMTBIT_S24_LE \
				| SNDRV_PCM_FMTBIT_S24_3LE \
				| SNDRV_PCM_FMTBIT_S32_LE)

struct outdai_priv {
	struct device *dev;
	const char *dev_name;
	unsigned int i2s_irq;
	u32 i2s_chid;
	u32 mode;
	struct aud_ctrl ctrl;
	bool i2s_requested;
	bool is_master;
	bool continuous_clk;
	bool output_mclk;
	/*  sample_period: sample period in terms of bclk numbers.
	 *  Typically 32 is used. For some pcm mono format, 16 may be used
	 */
	int  sample_period;
	void *aio_handle;
};

static void outdai_ch_flush(struct outdai_priv *outdai, bool en)
{
	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, en);
	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD1, en);
	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD2, en);
	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD3, en);
}

static void outdai_ch_en(struct outdai_priv *outdai, bool en)
{
	aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, en);
	aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD1, en);
	aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD2, en);
	aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD3, en);
}

static void outdai_ch_mute(struct outdai_priv *outdai, bool en)
{
	aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, en);
	aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD1, en);
	aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD2, en);
	aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD3, en);
}

static void outdai_set_ctl(struct outdai_priv *out, struct aud_ctrl *ctrl)
{
	int ret;

	ret = aio_set_ctl_ext(out->aio_handle, AIO_ID_PRI_TX, ctrl);
	if (ret != 0)
		snd_printk("aio_set_ctl_ext() return error(ret=%d)\n", ret);
}

static void outdai_set_clk_div(struct outdai_priv *out, u32 div)
{
	int ret;

	ret = aio_setclkdiv(out->aio_handle, AIO_ID_PRI_TX, div);
	if (ret != 0)
		snd_printk("aio_setclkdiv() return error(ret=%d)\n", ret);
}

static void outdai_enable_primaryport(struct outdai_priv *out, bool en)
{
	int ret;

	ret = aio_enabletxport(out->aio_handle, AIO_ID_PRI_TX, en);
	if (ret != 0)
		snd_printk("aio_enabletxport() return error(ret=%d)\n", ret);
}

static struct snd_kcontrol_new berlin_outdai_ctrls[] = {
	//TODO: add dai control here
};


static void outdai_iosel_set_bclk(struct outdai_priv *out,
			   bool inv)
{
	int ret;

	/* BCLK generated from (MCLK) */
	ret = aio_set_bclk_sel(out->aio_handle, AIO_ID_PRI_TX, 1);
	if (ret != 0)
		snd_printk("aio_set_bclk_sel() return error(ret=%d)\n", ret);

	ret = aio_set_bclk_inv(out->aio_handle, AIO_ID_PRI_TX, inv);
	if (ret != 0)
		snd_printk("aio_set_bclk_inv() return error(ret=%d)\n", ret);
}

/*
 * Applies output configuration of |berlin_pcm| to i2s.
 * Must be called with instance spinlock held.
 * Only one dai instance for playback, so no spin_lock needed
 */
static void outdai_set_aio(struct outdai_priv *out,
			   u32 fs, int width, int chnum)
{
	unsigned int cfm, dfm;
	struct aud_ctrl ctrl;
	unsigned int div, bclk;

	/* Change AIO_24DFM to AIO_32DFM */
	dfm = berlin_get_sample_resolution((width == 24 ? 32 : width));

	/* Alghough h/w supports AIO_24CFM, but 24 is not multiples of 2.
	 * There could be some restriction on clock generation for certain
	 * frequency with AIO_24CFM. Change AIO_24CFM to AIO_32CFG instead
	 */
	cfm = berlin_get_sample_period_in_bclk(out->sample_period == 24 ?
						32 : out->sample_period);
	if (out->ctrl.istdm) {
		/* TDM */
		bclk = fs * out->sample_period * chnum;
	} else {
		/* i2s mode: each I2S_DO[0:3] supports 2 channels */
		bclk = fs * out->sample_period * 2;
	}

	ctrl.chcnt	= chnum;
	ctrl.sample_resolution	= dfm;
	ctrl.sample_period_in_bclk	= cfm;
	ctrl.data_fmt	= out->ctrl.data_fmt;
	ctrl.isleftjfy	= out->ctrl.isleftjfy;
	ctrl.invbclk	= out->ctrl.invbclk;
	ctrl.invfs	= out->ctrl.invfs;
	ctrl.msb	= true;
	ctrl.istdm	= out->ctrl.istdm;
	ctrl.islframe	= out->ctrl.islframe;

	snd_printd("%s: chnum: %d\n", __func__, chnum);

	if (out->ctrl.istdm) {
		/* TDM */
		aio_set_interleaved_mode(out->aio_handle, AIO_ID_PRI_TX, 0, 0);
	} else {
		/* i2s */
		if (chnum == 2)
			aio_set_interleaved_mode(out->aio_handle, AIO_ID_PRI_TX, 0, 0);
		else if (chnum == 4)
			aio_set_interleaved_mode(out->aio_handle, AIO_ID_PRI_TX, 1, (1<<2));
		else if (chnum == 6)
			aio_set_interleaved_mode(out->aio_handle, AIO_ID_PRI_TX, 2, (1<<2) | (2<<4));
		else if (chnum == 8)
			aio_set_interleaved_mode(out->aio_handle, AIO_ID_PRI_TX, 3, (1<<2) | (2<<4) | (3<<6));
		else
			snd_printk("not supported chnum: %d in I2S mode\n", chnum);
	}
	div = (24576000 * 8) / (8 * bclk);
	div = ilog2(div);

	outdai_set_clk_div(out, div);
	outdai_set_ctl(out, &ctrl);

	/* MIC1 will use the PRIAUD bclk and need to invert it */
	outdai_iosel_set_bclk(out, true);
}

static int berlin_outdai_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	//TODO
	snd_printd("i2s pri start...\n");
	return 0;
}

static void berlin_outdai_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);

	aio_i2s_clk_sync_reset(outdai->aio_handle, AIO_ID_PRI_TX);
	snd_printd("i2s pri shutdown...\n");
}

static int berlin_outdai_setfmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		outdai->ctrl.data_fmt  = 2;
		outdai->ctrl.istdm    = false;
		outdai->ctrl.isleftjfy = true;   /* don't care if data_fmt = 2 */
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		outdai->ctrl.data_fmt  = 1;
		outdai->ctrl.istdm    = false;
		outdai->ctrl.isleftjfy = true;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		outdai->ctrl.data_fmt  = 1;
		outdai->ctrl.istdm    = false;
		outdai->ctrl.isleftjfy = false;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		outdai->ctrl.data_fmt  = 1;
		outdai->ctrl.istdm    = true;
		outdai->ctrl.isleftjfy = true;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		outdai->ctrl.data_fmt  = 2;
		outdai->ctrl.istdm    = true;
		outdai->ctrl.isleftjfy = true;  /* don't care if data_fmt = 2 */
		break;
	default:
		dev_err(dai->dev, "Unknown DAI format mask %x\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		outdai->ctrl.invbclk = false;
		outdai->ctrl.invfs   = false;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		outdai->ctrl.invbclk = false;
		outdai->ctrl.invfs   = true;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		outdai->ctrl.invbclk = true;
		outdai->ctrl.invfs   = false;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		outdai->ctrl.invbclk = true;
		outdai->ctrl.invfs   = true;
		break;
	default:
		dev_err(outdai->dev, "Unknown DAI invert mask 0x%x\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		outdai->is_master = true;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		outdai->is_master = false;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
	case SND_SOC_DAIFMT_CBS_CFM:
	default:
		dev_err(dai->dev, "Do not support DAI master mask %x\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_CLOCK_MASK) {
	case SND_SOC_DAIFMT_CONT:
		outdai->continuous_clk = true;
		break;
	case SND_SOC_DAIFMT_GATED:
		outdai->continuous_clk = false;
		break;
	default:
		dev_err(dai->dev, "Do not support DAI clock mask 0x%x\n", fmt);
		return -EINVAL;
	}

	snd_printd("%s: data_fmt: %d isleftjfy: %d istdm: %d is_master: %d invbclk: %d invfs: %d continuous clk: %d\n",
				__func__,
				outdai->ctrl.data_fmt, outdai->ctrl.isleftjfy,
				outdai->ctrl.istdm, outdai->is_master,
				outdai->ctrl.invbclk, outdai->ctrl.invfs,
				outdai->continuous_clk);

	snd_printd("%s: sample_period: %d", __func__, outdai->sample_period);

	if (outdai->continuous_clk)
		outdai_ch_en(outdai, 1);
	if (outdai->output_mclk)
		aio_set_i2s_clk_enable(outdai->aio_handle, AIO_I2S_I2S1_MCLK, 1);

	return ret;
}

static int berlin_outdai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params), chnum = params_channels(params);
	int ret;
	struct berlin_ss_params ssparams;

	if (!(outdai->mode & I2SO_MODE)) {
		snd_printk("not in i2s mode, fatal error.\n");
		return -EINVAL;
	}
	berlin_set_pll(outdai->aio_handle, AIO_APLL_0, fs);
	/* mclk */
	aio_i2s_set_clock(outdai->aio_handle, AIO_ID_PRI_TX, 1, AIO_CLK_D3_SWITCH_NOR,
						AIO_CLK_SEL_D8, AIO_APLL_0, 1);

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = I2SO_MODE;
	ssparams.irq = &outdai->i2s_irq;
	ssparams.dev_name = outdai->dev_name;
	ret = berlin_pcm_request_dma_irq(substream, &ssparams);
	if (ret == 0)
		outdai->i2s_requested = true;
	else
		return ret;
	if (!outdai->continuous_clk)
		outdai_ch_en(outdai, 1);

	outdai_ch_mute(outdai, 1);
	outdai_ch_flush(outdai, 0);
	outdai_set_aio(outdai, fs, params_width(params), chnum);
	snd_printd("i2s pri hw ready\n");
	return ret;
}

static int berlin_outdai_hw_free(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s\n", __func__);

	outdai_ch_flush(outdai, 1);

	if (!outdai->continuous_clk)
		outdai_ch_en(outdai, 0);

	if (outdai->i2s_requested && outdai->i2s_irq >= 0) {
		berlin_pcm_free_dma_irq(substream, 1, &outdai->i2s_irq);
		outdai->i2s_requested = false;
	}

	return 0;
}

static int berlin_outdai_trigger(struct snd_pcm_substream *substream,
				 int cmd, struct snd_soc_dai *dai)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s (%d)\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		outdai_ch_mute(outdai, 0);
		outdai_enable_primaryport(outdai, 1);
		outdai_ch_flush(outdai, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (!outdai->continuous_clk) {
			outdai_enable_primaryport(outdai, 0);
			outdai_ch_mute(outdai, 1);
		}
		outdai_ch_flush(outdai, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int berlin_outdai_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, berlin_outdai_ctrls,
				 ARRAY_SIZE(berlin_outdai_ctrls));
	return 0;
}

static struct snd_soc_dai_ops berlin_dai_outdai_ops = {
	.startup   = berlin_outdai_startup,
	.set_fmt   = berlin_outdai_setfmt,
	.hw_params = berlin_outdai_hw_params,
	.hw_free   = berlin_outdai_hw_free,
	.trigger   = berlin_outdai_trigger,
	.shutdown  = berlin_outdai_shutdown,
};

static struct snd_soc_dai_driver berlin_outdai_dai = {
	.name = "i2s-outdai",
	.probe = berlin_outdai_dai_probe,
	.playback = {
		.stream_name = "Pri-I2S-Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = I2S_PLAYBACK_RATES,
		.formats = I2S_PLAYBACK_FORMATS,
	},
	.ops = &berlin_dai_outdai_ops,
};

static const struct snd_soc_component_driver berlin_outdai_component = {
	.name = "i2s-outdai",
};

static int i2s_outdai_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct outdai_priv *outdai;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	outdai = devm_kzalloc(dev, sizeof(struct outdai_priv),
			      GFP_KERNEL);
	if (!outdai)
		return -ENOMEM;
	outdai->dev_name = dev_name(dev);
	outdai->dev = dev;

	/*open aio handle for alsa*/
	outdai->aio_handle = open_aio(outdai->dev_name);
	if (unlikely(outdai->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", outdai->aio_handle);
		return -EBUSY;
	}
	dev_set_drvdata(dev, outdai);

	irq = platform_get_irq_byname(pdev, "pri");
	if (irq >= 0) {
		outdai->mode |= I2SO_MODE;
		outdai->i2s_irq = irq;
		outdai->i2s_chid = irqd_to_hwirq(irq_get_irq_data(irq));
		snd_printd("get pri irq %d for node %s\n",
			   irq, pdev->name);
	}

	if (outdai->mode == 0) {
		snd_printk("non valid irq found in dts\n");
		return -EINVAL;
	}

	outdai->output_mclk = of_property_read_bool(np, "output-mclk");
	outdai->ctrl.islframe = of_property_read_bool(np, "long-frame");
	ret = of_property_read_u32(np, "sample-period", &outdai->sample_period);
	if (ret)
		outdai->sample_period = 32;

	ret = devm_snd_soc_register_component(dev,
					      &berlin_outdai_component,
					      &berlin_outdai_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}
	snd_printd("%s: done i2s [%d %d]\n", __func__,
		   outdai->i2s_irq, outdai->i2s_chid);

	return ret;
}

static int i2s_outdai_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct outdai_priv *outdai;

	outdai = (struct outdai_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (outdai && outdai->aio_handle) {
		close_aio(outdai->aio_handle);
		outdai->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id i2s_outdai_dt_ids[] = {
	{ .compatible = "syna,as370-outdai",  },
	{ .compatible = "syna,vs680-i2s-pri",  },
	{ .compatible = "syna,as470-outdai",  },
	{ .compatible = "syna,vs640-i2s-pri",  },
	{}
};
MODULE_DEVICE_TABLE(of, i2s_outdai_dt_ids);

static struct platform_driver i2s_outdai_driver = {
	.probe = i2s_outdai_probe,
	.remove = i2s_outdai_remove,
	.driver = {
		.name = "syna-i2s-pri",
		.of_match_table = i2s_outdai_dt_ids,
	},
};
module_platform_driver(i2s_outdai_driver);

MODULE_DESCRIPTION("Synaptics I2S ALSA output dai");
MODULE_ALIAS("platform:i2s-outdai");
MODULE_LICENSE("GPL v2");
