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
#include "i2s_common.h"
#include "mic.h"
#include "avio_common.h"

#define MIC2_RATES   (SNDRV_PCM_RATE_8000_48000)
#define MIC2_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
		      | SNDRV_PCM_FMTBIT_S24_LE \
		      | SNDRV_PCM_FMTBIT_S24_3LE \
		      | SNDRV_PCM_FMTBIT_S32_LE)

static atomic_t mic2_cnt = ATOMIC_INIT(0);

struct mic2_priv {
	struct platform_device *pdev;
	const char *dev_name;
	unsigned int irq;
	u32 chid;
	struct aud_ctrl ctrl;
	struct mic_common_cfg cfg;
	void *aio_handle;
	bool irq_requested;
	bool continuous_clk;
	/*  duplex mode: playbck (I2S3_BCLK/I2S3_LRCK/I2S3_DO)
	 *  and capture (I2S3_BCLK/I2S3_LRCK/I2S3_DI) at the same time
	 */
	bool duplex;
	/*  sample_period: sample period in terms of bclk numbers.
	 *  Typically 32 is used. For some pcm mono format, 16 may be used
	 */
	int  sample_period;
	bool rx_force_tx;
	u32  apll_id;  /* APLL id being used. 0: AIO_APLL_0, 1: AIO_APLL_1 */
};

static void mic2_enable(struct mic2_priv *mic2, u32 ch, bool en)
{
	if (en && atomic_inc_return(&mic2_cnt) > 1)
		return;
	else if (!en && atomic_dec_if_positive(&mic2_cnt) != 0)
		return;

	aio_enablerxport(mic2->aio_handle, AIO_ID_MIC2_RX, en);
}

static void mic2_set_ctl(struct mic2_priv *mic2, struct aud_ctrl *ctrl)
{
	aio_set_ctl_ext(mic2->aio_handle, AIO_ID_MIC2_RX, ctrl);
	aio_set_pcm_mono(mic2->aio_handle, AIO_ID_MIC2_RX, (ctrl->chcnt == 1));
}

static struct snd_kcontrol_new i2s_mic2_ctrls[] = {
	//TODO: add dai controls here
};

static int i2s_mic2_startup(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	snd_printd("%s: start %p %p\n", __func__, ss, dai);
	return 0;
}

static void i2s_mic2_shutdown(struct snd_pcm_substream *ss,
				struct snd_soc_dai *dai)
{
	struct mic2_priv *mic2 = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s: start %p %p\n", __func__, ss, dai);

	aio_set_aud_ch_en(mic2->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 0);
	aio_i2s_clk_sync_reset(mic2->aio_handle, AIO_ID_MIC2_RX);

	/*  Check if AIO_ID_SEC_TX is in stop state, turn off AIO_ID_SEC_TX as well.
	 *  This is a workaround for duplex mode that I2S3 share the FSNC and BCLK, and
	 *  AIO_ID_SEC_TX should be controlled by i2s_sec.c, but here as workaround
	 */
	if (!aio_get_i2s_ch_en(AIO_ID_SEC_TX)) {
		aio_set_aud_ch_mute(mic2->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		aio_set_aud_ch_flush(mic2->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		aio_set_aud_ch_en(mic2->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);
		aio_set_i2s_clk_enable(mic2->aio_handle, AIO_I2S_I2S3_BCLK, 0);
		aio_set_i2s_clk_enable(mic2->aio_handle, AIO_I2S_I2S3_LRCK, 0);
		snd_printd("%s: aio_set_aud_ch_en: OFF\n", __func__);
		aio_i2s_clk_sync_reset(mic2->aio_handle, AIO_ID_SEC_TX);
		aio_enabletxport(mic2->aio_handle, AIO_ID_SEC_TX, 0);
	}

	aio_set_i2s_ch_en(AIO_ID_MIC2_RX, 0);
}

static int i2s_mic2_hw_params(struct snd_pcm_substream *ss,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct mic2_priv *mic2 = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params);
	u32 width = params_width(params);
	u32 chnum = params_channels(params);
	unsigned int dfm, cfm, bclk;
	struct berlin_ss_params ssparams;
	struct aud_ctrl ctrl;
	/* 00: Master mode and I2S1_LRCKIO_DO_FB (clock)
	 * 01: Slave mode and I2S1_LRCKIO_DI_2mux (from PinMux) (clock)
	 */
	u32 xfeed, blk_sel;
	int ret;

	snd_printd("%s\n", __func__);

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = I2SI_MODE;
	ssparams.enable_mic_mute = !mic2->cfg.disable_mic_mute;
	ssparams.irq = &mic2->irq;
	ssparams.interleaved = false;
	ssparams.dummy_data = false;
	ssparams.dev_name = mic2->dev_name;
	ret = berlin_pcm_request_dma_irq(ss, &ssparams);

	if (ret == 0)
		mic2->irq_requested = true;

	/* Change AIO_24DFM to AIO_32DFM */
	dfm = berlin_get_sample_resolution((width == 24 ? 32 : width));

	/* Alghough h/w supports AIO_24CFM, but 24 is not multiples of 2.
	 * There could be some restriction on clock generation for certain
	 * frequency with AIO_24CFM. Change AIO_24CFM to AIO_32CFG instead
	 */
	cfm = berlin_get_sample_period_in_bclk(mic2->sample_period == 24 ?
						32 : mic2->sample_period);
	if (mic2->cfg.is_tdm) {
		/* TDM */
		bclk = fs * mic2->sample_period * chnum;
	} else {
		/* i2s mode: each I2S_DI[0:3] supports 2 channels */
		bclk = fs * mic2->sample_period * 2;
	}

	ctrl.chcnt	= chnum;
	ctrl.sample_period_in_bclk	= cfm;
	ctrl.sample_resolution	= dfm;
	ctrl.data_fmt	= mic2->cfg.data_fmt;
	ctrl.isleftjfy	= mic2->cfg.isleftjfy;
	ctrl.invbclk	= mic2->cfg.invbclk;
	ctrl.invfs	= mic2->cfg.invfsync;
	ctrl.msb	= true;
	ctrl.istdm	= (chnum == 1) ? 0 : mic2->cfg.is_tdm;
	ctrl.islframe	= false;

	if (mic2->cfg.is_master) {
		u32 div;

		/* mic2 does not support master mode, using xfeed from tx */
		xfeed = 0;
		blk_sel = 0;

		/* pll */
		berlin_set_pll(mic2->aio_handle, mic2->apll_id, fs);

		/* Check whether tx is enabled by rx or tx */
		if (!aio_get_i2s_ch_en(AIO_ID_SEC_TX)) {
			mic2->rx_force_tx = true;

			div = (24576000 * 8) / (8 * bclk);
			div = ilog2(div);

			aio_i2s_set_clock(mic2->aio_handle, AIO_ID_SEC_TX, 1, AIO_CLK_D3_SWITCH_NOR,
								AIO_CLK_SEL_D8, mic2->apll_id, 1);
			aio_setclkdiv(mic2->aio_handle, AIO_ID_SEC_TX, div);
			aio_set_ctl_ext(mic2->aio_handle, AIO_ID_SEC_TX, &ctrl);
			aio_enabletxport(mic2->aio_handle, AIO_ID_SEC_TX, true);
			aio_set_aud_ch_mute(mic2->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
			aio_set_i2s_clk_enable(mic2->aio_handle, AIO_I2S_I2S3_BCLK, 1);
			aio_set_i2s_clk_enable(mic2->aio_handle, AIO_I2S_I2S3_LRCK, 1);
			aio_set_aud_ch_en(mic2->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		}
	} else {
		xfeed = 1;
		blk_sel = 0;
		aio_set_slave_mode(mic2->aio_handle, AIO_ID_MIC2_RX, blk_sel);
	}

	ret = aio_set_xfeed_mode(mic2->aio_handle, I2S3_ID, xfeed, xfeed);
	if (ret != 0) {
		snd_printk("fail to set xfeed mode for i2s, ret %d\n", ret);
		return ret;
	}

	ret = aio_set_bclk_sel(mic2->aio_handle, AIO_ID_MIC2_RX, blk_sel);
	if (ret != 0) {
		snd_printk("fail to set bclk, ret %d\n", ret);
		return ret;
	}

	mic2_set_ctl(mic2, &ctrl);
	aio_set_aud_ch_en(mic2->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 1);
	aio_set_i2s_ch_en(AIO_ID_MIC2_RX, 1);

	return ret;
}

static int i2s_mic2_set_dai_fmt(struct snd_soc_dai *dai,
				  unsigned int fmt)
{
	struct mic2_priv *outdai = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		outdai->cfg.data_fmt  = 2;
		outdai->cfg.is_tdm    = false;
		outdai->cfg.isleftjfy = true;   /* don't care if data_fmt = 2 */
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		outdai->cfg.data_fmt  = 1;
		outdai->cfg.is_tdm    = false;
		outdai->cfg.isleftjfy = true;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		outdai->cfg.data_fmt  = 1;
		outdai->cfg.is_tdm    = false;
		outdai->cfg.isleftjfy = false;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		outdai->cfg.data_fmt  = 1;
		outdai->cfg.is_tdm    = true;
		outdai->cfg.isleftjfy = true;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		outdai->cfg.data_fmt  = 2;
		outdai->cfg.is_tdm    = true;
		outdai->cfg.isleftjfy = true;  /* don't care if data_fmt = 2 */
		break;
	default:
		dev_err(dai->dev, "Unknown DAI format mask %x\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		outdai->cfg.invbclk  = false;
		outdai->cfg.invfsync = false;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		outdai->cfg.invbclk  = false;
		outdai->cfg.invfsync = true;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		outdai->cfg.invbclk  = true;
		outdai->cfg.invfsync = false;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		outdai->cfg.invbclk  = true;
		outdai->cfg.invfsync = true;
		break;
	default:
		dev_err(dai->dev, "Unknown DAI invert mask %x\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		outdai->cfg.is_master = true;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		outdai->cfg.is_master = false;
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
				outdai->cfg.data_fmt, outdai->cfg.isleftjfy,
				outdai->cfg.is_tdm, outdai->cfg.is_master,
				outdai->cfg.invbclk, outdai->cfg.invfsync,
				outdai->continuous_clk);

	snd_printd("%s: sample_period: %d", __func__, outdai->sample_period);

	if (outdai->continuous_clk) {
		aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 1);
		aio_set_i2s_clk_enable(outdai->aio_handle, AIO_I2S_I2S3_BCLK, 1);
		aio_enablerxport(outdai->aio_handle, AIO_ID_MIC2_RX, 1);
	}

	return ret;
}

static int i2s_mic2_hw_free(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct mic2_priv *mic2 = snd_soc_dai_get_drvdata(dai);

	if (mic2->irq_requested && mic2->irq >= 0) {
		berlin_pcm_free_dma_irq(ss, 1, &mic2->irq);
		mic2->irq_requested = false;
	}

	return 0;
}

static int i2s_mic2_trigger(struct snd_pcm_substream *ss,
			      int cmd, struct snd_soc_dai *dai)
{
	struct mic2_priv *mic2 = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		aio_set_aud_ch_mute(mic2->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 0);
		aio_set_aud_ch_flush(mic2->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 0);
		mic2_enable(mic2, mic2->chid, 1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		aio_set_aud_ch_mute(mic2->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 1);
		aio_set_aud_ch_flush(mic2->aio_handle, AIO_ID_MIC2_RX, AIO_TSD0, 1);
		mic2_enable(mic2, mic2->chid, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2s_mic2_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, i2s_mic2_ctrls,
				 ARRAY_SIZE(i2s_mic2_ctrls));

	return 0;
}

static struct snd_soc_dai_ops i2s_dai_mic2_ops = {
	.startup = i2s_mic2_startup,
	.hw_params = i2s_mic2_hw_params,
	.set_fmt = i2s_mic2_set_dai_fmt,
	.hw_free = i2s_mic2_hw_free,
	.trigger = i2s_mic2_trigger,
	.shutdown = i2s_mic2_shutdown,
};

static struct snd_soc_dai_driver i2s_mic2_dai = {
	.name = "i2s-mic2",
	.probe = i2s_mic2_dai_probe,
	.capture = {
		.stream_name = "MIC2-Capture",
		.channels_min = 1,
		.channels_max = 8,
		.rates = MIC2_RATES,
		.formats = MIC2_FORMATS,
	},
	.ops = &i2s_dai_mic2_ops,
};

static const struct snd_soc_component_driver i2s_mic2_component = {
	.name = "i2s-mic2",
};

static int i2s_mic2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mic2_priv *mic2;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	mic2 = devm_kzalloc(dev, sizeof(struct mic2_priv), GFP_KERNEL);
	if (!mic2)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		snd_printk("fail to get irq for node %s\n", pdev->name);
		return irq;
	}
	mic2->irq = irq;
	mic2->chid = irqd_to_hwirq(irq_get_irq_data(irq));
	if (mic2->chid < 0) {
		snd_printk("got invalid dhub chid %d\n", mic2->chid);
		return -EINVAL;
	}
	snd_printd("got irq %d chid %d\n", mic2->irq, mic2->chid);

	mic2->cfg.disable_mic_mute = of_property_read_bool(np, "disablemicmute");
	mic2->duplex = of_property_read_bool(np, "duplex");
	ret = of_property_read_u32(np, "sample-period", &mic2->sample_period);
	if (ret)
		mic2->sample_period = 32;
	ret = of_property_read_u32(np, "apll-id", &mic2->apll_id);
	if (ret)
		mic2->apll_id = 0;
	mic2->dev_name = dev_name(dev);
	mic2->pdev = pdev;
	mic2->aio_handle = open_aio(mic2->dev_name);
	if (unlikely(mic2->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", mic2->aio_handle);
		return -EBUSY;
	}
	dev_set_drvdata(dev, mic2);

	ret = devm_snd_soc_register_component(dev,
					      &i2s_mic2_component,
					      &i2s_mic2_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}
	snd_printd("%s: done irq %d chid %d\n", __func__, mic2->irq, mic2->chid);

	return ret;
}

static int i2s_mic2_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mic2_priv *mic2;

	mic2 = (struct mic2_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (mic2 && mic2->aio_handle) {
		close_aio(mic2->aio_handle);
		mic2->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id i2s_mic2_dt_ids[] = {
	{ .compatible = "syna,as370-mic2",  },
	{ .compatible = "syna,vs680-i2s-mic2",  },
	{ .compatible = "syna,vs640-i2s-mic2",  },
	{ .compatible = "syna,as470-mic2",  },
	{}
};
MODULE_DEVICE_TABLE(of, i2s_mic2_dt_ids);

static struct platform_driver i2s_mic2_driver = {
	.probe = i2s_mic2_probe,
	.remove = i2s_mic2_remove,
	.driver = {
		.name = "syna-i2s-mic2",
		.of_match_table = i2s_mic2_dt_ids,
	},
};
module_platform_driver(i2s_mic2_driver);

MODULE_DESCRIPTION("Synaptics I2S MIC2 ALSA driver");
MODULE_ALIAS("platform:i2s-mic2");
MODULE_LICENSE("GPL v2");
