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

#define MIC6_RATES   (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 \
			| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 \
			| SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 \
			| SNDRV_PCM_RATE_192000)
#define MIC6_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
			| SNDRV_PCM_FMTBIT_S32_LE)
static atomic_t mic6_cnt = ATOMIC_INIT(0);

struct mic6_priv {
	struct platform_device *pdev;
	const char *dev_name;
	unsigned int irq;
	u32 chid;
	bool irq_requested;
	bool dummy_data;
	struct mic_common_cfg cfg;
	void *aio_handle;
	u32 channels;
};

void  __weak aio_earc_src_sel(void *hd, u32 sel) { }

void  __weak aio_earc_i2s_frord_sel(void *hd, u32 sel) { }

static void mic6_set_rx_port_en(struct mic6_priv *mic6, bool en)
{
	aio_enablerxport(mic6->aio_handle, AIO_ID_MIC6_RX, en);
}

static void mic6_ch_intl_en(struct mic6_priv *mic6, bool en)
{
	int i;

	for (i = 0; i < (mic6->channels + 1) / 2; i++) {
		aio_set_mic_intlmode(mic6->aio_handle, AIO_ID_MIC6_RX,
				AIO_TSD0 + i, en);
	}

}
static void mic6_ch_flush(struct mic6_priv *mic6, bool en)
{
	int i;

	for (i = 0; i < (mic6->channels + 1) / 2; i++) {
		aio_set_aud_ch_flush(mic6->aio_handle,
				AIO_ID_MIC6_RX, AIO_TSD0 + i, en);
	}
}

static void mic6_ch_en(struct mic6_priv *mic6, bool en)
{
	int i;

	for (i = 0; i < (mic6->channels + 1) / 2; i++) {
		aio_set_aud_ch_en(mic6->aio_handle,
				AIO_ID_MIC6_RX, AIO_TSD0 + i, en);
	}
}

static void mic6_ch_mute(struct mic6_priv *mic6, bool en)
{
	int i;

	for (i = 0; i < (mic6->channels + 1) / 2; i++) {
		aio_set_aud_ch_mute(mic6->aio_handle,
				AIO_ID_MIC6_RX, AIO_TSD0 + i, en);
	}
}

static void mic6_enable(struct mic6_priv *mic6, bool en)
{
	if (en && atomic_inc_return(&mic6_cnt) > 1)
		return;
	else if (!en && atomic_dec_if_positive(&mic6_cnt) != 0)
		return;

	if (en) {
		mic6_ch_flush(mic6, 0);
		mic6_set_rx_port_en(mic6, 1);
	} else {
		mic6_ch_flush(mic6, 1);
		mic6_set_rx_port_en(mic6, 0);
	}
}

static void mic6_set_ctl(struct mic6_priv *mic6, struct aud_ctrl *ctrl)
{
	aio_set_ctl_ext(mic6->aio_handle, AIO_ID_MIC6_RX, ctrl);
}

static struct snd_kcontrol_new i2s_mic6_ctrls[] = {
};

static int i2s_mic6_startup(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct mic6_priv *mic6 = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s: start %p %p\n", __func__, ss, dai);
	aio_sw_rst(mic6->aio_handle, AIO_SW_RST_MIC6, 0);
	aio_sw_rst(mic6->aio_handle, AIO_SW_RST_MIC6, 1);

	return 0;
}

static void i2s_mic6_shutdown(struct snd_pcm_substream *ss,
				struct snd_soc_dai *dai)
{
	struct mic6_priv *mic6 = snd_soc_dai_get_drvdata(dai);
	aio_i2s_clk_sync_reset(mic6->aio_handle, AIO_ID_MIC6_RX);
	snd_printd("%s: start %p %p\n", __func__, ss, dai);
}

static int i2s_mic6_hw_params(struct snd_pcm_substream *ss,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct mic6_priv *mic6 = snd_soc_dai_get_drvdata(dai);
	int ret;
	u32 dfm;
	struct berlin_ss_params ssparams;
	struct aud_ctrl ctrl;

	mic6->channels = params_channels(params);

	aio_earc_i2s_frord_sel(mic6->aio_handle, 0x7);
	aio_earc_src_sel(mic6->aio_handle, 2);
	aio_set_loopback_clk_gate(mic6->aio_handle, AIO_LOOPBACK_CLK_GATE_MIC6, 0);

	dfm = berlin_get_sample_resolution(params_width(params));

	ctrl.chcnt	= mic6->channels;
	ctrl.sample_period_in_bclk	= AIO_32CFM;
	ctrl.sample_resolution	= dfm;
	ctrl.data_fmt	= mic6->cfg.data_fmt;
	ctrl.isleftjfy	= mic6->cfg.isleftjfy;
	ctrl.invbclk	= mic6->cfg.invbclk;
	ctrl.invfs	= mic6->cfg.invfsync;
	ctrl.msb	= true;
	ctrl.istdm	= mic6->cfg.is_tdm;
	ctrl.islframe	= false;
	mic6_set_ctl(mic6, &ctrl);

	mic6_ch_intl_en(mic6, 1);
	mic6_ch_en(mic6, 1);
	mic6_ch_mute(mic6, 1);

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = I2SI_MODE | FLAG_EARC_MODE;
	ssparams.enable_mic_mute = !mic6->cfg.disable_mic_mute;
	ssparams.irq = &mic6->irq;
	ssparams.interleaved = true;
	ssparams.dummy_data = mic6->dummy_data;
	ssparams.dev_name = mic6->dev_name;
	ret = berlin_pcm_request_dma_irq(ss, &ssparams);

	if (ret == 0)
		mic6->irq_requested = true;

	return ret;
}

static int i2s_mic6_set_dai_fmt(struct snd_soc_dai *dai,
				  unsigned int fmt)
{
	struct mic6_priv *outdai = snd_soc_dai_get_drvdata(dai);
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

	snd_printd("%s: data_fmt: %d isleftjfy: %d is_tdm: %d is_master: %d invbclk: %d invfsync: %d\n",
				__func__,
				outdai->cfg.data_fmt, outdai->cfg.isleftjfy,
				outdai->cfg.is_tdm, outdai->cfg.is_master,
				outdai->cfg.invbclk, outdai->cfg.invfsync);

	return ret;
}

static int i2s_mic6_hw_free(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct mic6_priv *mic6 = snd_soc_dai_get_drvdata(dai);

	if (mic6->irq_requested && mic6->irq >= 0) {
		berlin_pcm_free_dma_irq(ss, 1, &mic6->irq);
		mic6->irq_requested = false;
	}
	mic6_ch_flush(mic6, 1);
	mic6_ch_en(mic6, 0);
	mic6_ch_intl_en(mic6, 0);
	aio_earc_src_sel(mic6->aio_handle, 0);

	return 0;
}

static int i2s_mic6_trigger(struct snd_pcm_substream *ss,
			      int cmd, struct snd_soc_dai *dai)
{
	struct mic6_priv *mic6 = snd_soc_dai_get_drvdata(dai);


	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		mic6_ch_mute(mic6, 0);
		mic6_enable(mic6, 1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		mic6_ch_mute(mic6, 1);
		mic6_enable(mic6, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2s_mic6_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, i2s_mic6_ctrls,
				 ARRAY_SIZE(i2s_mic6_ctrls));

	return 0;
}

static struct snd_soc_dai_ops i2s_dai_mic6_ops = {
	.startup = i2s_mic6_startup,
	.hw_params = i2s_mic6_hw_params,
	.set_fmt = i2s_mic6_set_dai_fmt,
	.hw_free = i2s_mic6_hw_free,
	.trigger = i2s_mic6_trigger,
	.shutdown = i2s_mic6_shutdown,
};

static struct snd_soc_dai_driver i2s_mic6_dai = {
	.name = "i2s-mic6",
	.probe = i2s_mic6_dai_probe,
	.capture = {
		.stream_name = "MIC6-Capture",
		.channels_min = 2,
		.channels_max = 8,
		.rates = MIC6_RATES,
		.formats = MIC6_FORMATS,
	},
	.ops = &i2s_dai_mic6_ops,
};

static const struct snd_soc_component_driver i2s_mic6_component = {
	.name = "i2s-mic6",
};

static int i2s_mic6_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mic6_priv *mic6;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	mic6 = devm_kzalloc(dev, sizeof(struct mic6_priv), GFP_KERNEL);
	if (!mic6)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		snd_printk("fail to get irq for node %s\n", pdev->name);
		return irq;
	}
	mic6->irq = irq;
	mic6->chid = irqd_to_hwirq(irq_get_irq_data(irq));
	if (mic6->chid < 0) {
		snd_printk("got invalid dhub chid %d\n", mic6->chid);
		return -EINVAL;
	}
	snd_printd("got irq %d chid %d\n", mic6->irq, mic6->chid);

	mic6->cfg.disable_mic_mute =
			of_property_read_bool(np, "disablemicmute");
	mic6->dummy_data = of_property_read_bool(np, "dummy_data");
	mic6->dev_name = dev_name(dev);
	mic6->pdev = pdev;
	mic6->aio_handle = open_aio(mic6->dev_name);
	if (unlikely(mic6->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", mic6->aio_handle);
		return -EBUSY;
	}
	dev_set_drvdata(dev, mic6);

	ret = devm_snd_soc_register_component(dev,
					      &i2s_mic6_component,
					      &i2s_mic6_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}
	snd_printd("%s: done irq %d chid %d\n", __func__, mic6->irq, mic6->chid);

	return ret;
}

static int i2s_mic6_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mic6_priv *mic6;

	mic6 = (struct mic6_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (mic6 && mic6->aio_handle) {
		close_aio(mic6->aio_handle);
		mic6->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id i2s_mic6_dt_ids[] = {
	{ .compatible = "syna,vs680-i2s-mic6",  },
	{}
};
MODULE_DEVICE_TABLE(of, i2s_mic6_dt_ids);

static struct platform_driver i2s_mic6_driver = {
	.probe = i2s_mic6_probe,
	.remove = i2s_mic6_remove,
	.driver = {
		.name = "syna-i2s-mic6",
		.of_match_table = i2s_mic6_dt_ids,
	},
};
module_platform_driver(i2s_mic6_driver);

MODULE_DESCRIPTION("Synaptics I2S MIC6 ALSA driver");
MODULE_ALIAS("platform:i2s-mic6");
MODULE_LICENSE("GPL v2");
