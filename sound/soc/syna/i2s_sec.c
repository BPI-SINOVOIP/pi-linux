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

#define I2S_SEC_RATES   (SNDRV_PCM_RATE_8000_192000)
#define I2S_SEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
					| SNDRV_PCM_FMTBIT_S24_LE \
					| SNDRV_PCM_FMTBIT_S24_3LE \
					| SNDRV_PCM_FMTBIT_S32_LE)

struct sec_priv {
	const char *dev_name;
	unsigned int irq;
	u32 chid;
	struct aud_ctrl ctrl;
	bool requested;
	bool is_master;
	bool continuous_clk;
	/*  duplex mode: playbck (I2S3_BCLK/I2S3_LRCK/I2S3_DO)
	 *  and capture (I2S3_BCLK/I2S3_LRCK/I2S3_DI) at the same time
	 */
	bool duplex;
	/*  sample_period: sample period in terms of bclk numbers.
	 *  Typically 32 is used. For some pcm mono format, 16 may be used
	 */
	int  sample_period;
	u32  apll_id;  /* APLL id being used. 0: AIO_APLL_0, 1: AIO_APLL_1 */
	void *aio_handle;
};

static struct snd_kcontrol_new i2s_sec_ctrls[] = {
	//TODO: add dai control here
};

/*
 * Apply output configuration of |berlin_pcm| to i2s.
 * Must be called with instance spinlock held.
 * Only one dai instance for playback, so no spin_lock needed
 */
static void sec_set_aio_fmt(struct sec_priv *sec, u32 fs, int width, int chnum)
{
	unsigned int div, dfm, cfm, bclk;
	struct aud_ctrl ctrl;

	/* Change AIO_24DFM to AIO_32DFM */
	dfm = berlin_get_sample_resolution((width == 24 ? 32 : width));

	/* Alghough h/w supports AIO_24CFM, but 24 is not multiples of 2.
	 * There could be some restriction on clock generation for certain
	 * frequency with AIO_24CFM. Change AIO_24CFM to AIO_32CFG instead
	 */
	cfm = berlin_get_sample_period_in_bclk(sec->sample_period == 24 ?
						32 : sec->sample_period);
	if (sec->ctrl.istdm) {
		/* TDM */
		bclk = fs * sec->sample_period * chnum;
	} else {
		/* i2s mode: each I2S_DO[0:3] supports 2 channels */
		bclk = fs * sec->sample_period * 2;
	}

	div = (24576000 * 8) / (8 * bclk);
	div = ilog2(div);

	ctrl.chcnt	= chnum;
	ctrl.sample_period_in_bclk = cfm;
	ctrl.sample_resolution	= dfm;
	ctrl.data_fmt	= sec->ctrl.data_fmt;
	ctrl.isleftjfy	= sec->ctrl.isleftjfy;
	ctrl.invbclk	= sec->ctrl.invbclk;
	ctrl.invfs	= sec->ctrl.invfs;
	ctrl.msb	= true;
	ctrl.istdm	= (chnum == 1) ? 0 : sec->ctrl.istdm;
	ctrl.islframe	= sec->ctrl.islframe;

	aio_setclkdiv(sec->aio_handle, AIO_ID_SEC_TX, div);
	aio_set_ctl_ext(sec->aio_handle, AIO_ID_SEC_TX, &ctrl);
	aio_set_pcm_mono(sec->aio_handle, AIO_ID_SEC_TX, (chnum == 1));
}

static int i2s_sec_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	snd_printd("%s\n", __func__);
	return 0;
}

static void i2s_sec_shutdown(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s\n", __func__);

	/* Reset to master mode when i2s sec hw is free as i2s3 input needs the clock */
	if (!sec->is_master) {
		aio_set_slave_mode(sec->aio_handle, AIO_ID_SEC_TX, 0);
		aio_set_bclk_sel(sec->aio_handle, AIO_ID_SEC_TX, 1);
	}

	if (sec->continuous_clk) {
		aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_BCLK, 1);
		aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_LRCK, 1);
	} else {
		if (!sec->duplex | !aio_get_i2s_ch_en(AIO_ID_MIC2_RX)) {
			aio_enabletxport(sec->aio_handle, AIO_ID_SEC_TX, 0);
			aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
			aio_set_aud_ch_flush(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
			aio_set_aud_ch_en(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);
			snd_printd("%s: aio_set_aud_ch_en(AIO_ID_SEC_TX): OFF\n", __func__);
			aio_set_aud_ch_en(sec->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 0);
			snd_printd("%s: aio_set_aud_ch_en(AIO_ID_MIC2_RX): OFF\n", __func__);
			aio_i2s_clk_sync_reset(sec->aio_handle, AIO_ID_MIC2_RX);
			aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_BCLK, 0);
			aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_LRCK, 0);
			aio_i2s_clk_sync_reset(sec->aio_handle, AIO_ID_SEC_TX);
		}
	}

	aio_set_i2s_ch_en(AIO_ID_SEC_TX, 0);
}

static int i2s_sec_setfmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sec_priv *outdai = snd_soc_dai_get_drvdata(dai);
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
		dev_err(dai->dev, "Unknown DAI invert mask 0x%x\n", fmt);
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

	if (outdai->continuous_clk) {
		aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		aio_set_i2s_clk_enable(outdai->aio_handle, AIO_I2S_I2S3_BCLK, 1);
		aio_enabletxport(outdai->aio_handle, AIO_ID_SEC_TX, 1);
	}

	return ret;
}

static int i2s_sec_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);
	int ret;
	u32 fs = params_rate(params);
	struct berlin_ss_params ssparams;

	snd_printd("%s\n", __func__);

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = I2SO_MODE;
	ssparams.irq = &sec->irq;
	ssparams.dev_name = sec->dev_name;
	ret = berlin_pcm_request_dma_irq(substream, &ssparams);
	if (ret == 0)
		sec->requested = true;
	else
		return ret;

	if (sec->is_master) {
		/* pll */
		berlin_set_pll(sec->aio_handle, sec->apll_id, fs);
		/* mclk */
		aio_i2s_set_clock(sec->aio_handle, AIO_ID_SEC_TX, 1, AIO_CLK_D3_SWITCH_NOR,
							AIO_CLK_SEL_D8, sec->apll_id, 1);
		/* Set bclk master mode */
		aio_set_slave_mode(sec->aio_handle, AIO_ID_SEC_TX, 0);
		aio_set_bclk_sel(sec->aio_handle, AIO_ID_SEC_TX, 1);
	} else {
		/* Set slave mode and bclk from external */
		aio_set_slave_mode(sec->aio_handle, AIO_ID_SEC_TX, 1);
		aio_set_bclk_sel(sec->aio_handle, AIO_ID_SEC_TX, 0);
	}

	aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
	aio_set_aud_ch_flush(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);

	sec_set_aio_fmt(sec, fs, params_width(params), params_channels(params));

	if (sec->is_master) {
		aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_BCLK, 1);
		aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_LRCK, 1);
	} else {
		aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_BCLK, 0);
		aio_set_i2s_clk_enable(sec->aio_handle, AIO_I2S_I2S3_LRCK, 0);
	}

	aio_set_aud_ch_en(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
	aio_enabletxport(sec->aio_handle, AIO_ID_SEC_TX, 1);
	aio_set_i2s_ch_en(AIO_ID_SEC_TX, 1);

	return ret;
}

static int i2s_sec_hw_free(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s\n", __func__);

	if (sec->requested && sec->irq >= 0) {
		berlin_pcm_free_dma_irq(substream, 1, &sec->irq);
		sec->requested = false;
	}

	return 0;
}

static int i2s_sec_trigger(struct snd_pcm_substream *substream,
			       int cmd, struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s (%d)\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);
		aio_set_aud_ch_flush(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		aio_set_aud_ch_flush(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		/* FIXME, this is for duplex case, don't disable tx port in case rx
		 * is still active as BCLK/FSYNC of RX relies on XFEED from tx
		 */
		//aio_enabletxport(sec->aio_handle, AIO_ID_SEC_TX, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2s_sec_dai_probe(struct snd_soc_dai *dai)
{
	snd_printd("%s\n", __func__);
	snd_soc_add_dai_controls(dai, i2s_sec_ctrls,
				 ARRAY_SIZE(i2s_sec_ctrls));
	return 0;
}

static struct snd_soc_dai_ops i2s_dai_sec_ops = {
	.startup = i2s_sec_startup,
	.set_fmt = i2s_sec_setfmt,
	.hw_params = i2s_sec_hw_params,
	.hw_free = i2s_sec_hw_free,
	.trigger = i2s_sec_trigger,
	.shutdown = i2s_sec_shutdown,
};

static struct snd_soc_dai_driver i2s_sec_dai = {
	.name = "i2s-sec-dai",
	.probe = i2s_sec_dai_probe,
	.playback = {
		.stream_name = "SEC-Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = I2S_SEC_RATES,
		.formats = I2S_SEC_FORMATS,
	},
	.ops = &i2s_dai_sec_ops,
};

static const struct snd_soc_component_driver i2s_sec_component = {
	.name = "i2s-sec",
};

static int i2s_sec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct sec_priv *sec;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	sec = devm_kzalloc(dev, sizeof(struct sec_priv), GFP_KERNEL);
	if (!sec)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		snd_printk("fail to get irq for node %s\n", pdev->name);
		return -EINVAL;
	}

	sec->irq = irq;
	sec->chid = irqd_to_hwirq(irq_get_irq_data(irq));
	sec->duplex = of_property_read_bool(np, "duplex");
	ret = of_property_read_u32(np, "sample-period", &sec->sample_period);
	if (ret)
		sec->sample_period = 32;
	ret = of_property_read_u32(np, "apll-id", &sec->apll_id);
	if (ret)
		sec->apll_id = 0;

	sec->dev_name = dev_name(dev);
	sec->aio_handle = open_aio(sec->dev_name);
	if (unlikely(sec->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", sec->aio_handle);
		return -EBUSY;
	}

	dev_set_drvdata(dev, sec);

	ret = devm_snd_soc_register_component(dev,
					      &i2s_sec_component,
					      &i2s_sec_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}
	snd_printd("%s: done irq %d chid %d\n", __func__, sec->irq, sec->chid);

	return ret;
}

static int i2s_sec_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sec_priv *sec;

	sec = (struct sec_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (sec && sec->aio_handle) {
		close_aio(sec->aio_handle);
		sec->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id i2s_sec_dt_ids[] = {
	{ .compatible = "syna,as370-sec",  },
	{ .compatible = "syna,as470-sec",  },
	{ .compatible = "syna,vs680-i2s-sec",  },
	{ .compatible = "syna,vs640-i2s-sec",  },
	{}
};
MODULE_DEVICE_TABLE(of, i2s_sec_dt_ids);

static struct platform_driver i2s_sec_driver = {
	.probe = i2s_sec_probe,
	.remove = i2s_sec_remove,
	.driver = {
		.name = "syna-i2s-sec",
		.of_match_table = i2s_sec_dt_ids,
	},
};
module_platform_driver(i2s_sec_driver);

MODULE_DESCRIPTION("Synaptics playback ALSA dai");
MODULE_ALIAS("platform:i2so-sec");
MODULE_LICENSE("GPL v2");
