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
#include "avio_common.h"

#define HDMI_LPBK_RATES   (SNDRV_PCM_RATE_8000_192000)
#define HDMI_LPBK_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
		      | SNDRV_PCM_FMTBIT_S32_LE)

static atomic_t hdmi_lpbk_cnt = ATOMIC_INIT(0);

struct hdmi_lpbk_priv {
	struct platform_device *pdev;
	const char *dev_name;
	unsigned int irq;
	u32 chid;
	bool irq_requested;
	bool dummy_data;
	bool disable_mic_mute;
	void *aio_handle;
};

static void hdmi_lpbk_set_rx_port_en(struct hdmi_lpbk_priv *lpbk, bool en)
{
	aio_enablerxport(lpbk->aio_handle, AIO_ID_MIC5_RX, en);
}

static void hdmi_lpbk_ch_flush(struct hdmi_lpbk_priv *lpbk, bool en)
{
	aio_set_aud_ch_flush(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD0, en);
	aio_set_aud_ch_flush(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD1, en);
	aio_set_aud_ch_flush(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD2, en);
	aio_set_aud_ch_flush(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD3, en);
}

static void hdmi_lpbk_ch_en(struct hdmi_lpbk_priv *lpbk, bool en)
{
	aio_set_aud_ch_en(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD0, en);
	aio_set_aud_ch_en(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD1, en);
	aio_set_aud_ch_en(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD2, en);
	aio_set_aud_ch_en(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD3, en);
}

static void hdmi_lpbk_ch_mute(struct hdmi_lpbk_priv *lpbk, bool en)
{
	aio_set_aud_ch_mute(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD0, en);
	aio_set_aud_ch_mute(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD1, en);
	aio_set_aud_ch_mute(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD2, en);
	aio_set_aud_ch_mute(lpbk->aio_handle, AIO_ID_MIC5_RX, AIO_TSD3, en);
}

static void hdmi_lpbk_enable(struct hdmi_lpbk_priv *lpbk, bool en)
{
	if (en && atomic_inc_return(&hdmi_lpbk_cnt) > 1)
		return;
	else if (!en && atomic_dec_if_positive(&hdmi_lpbk_cnt) != 0)
		return;

	if (en) {
		hdmi_lpbk_ch_flush(lpbk, 0);
		hdmi_lpbk_set_rx_port_en(lpbk, 1);
	} else {
		hdmi_lpbk_ch_flush(lpbk, 1);
		hdmi_lpbk_set_rx_port_en(lpbk, 0);
	}
}

static struct snd_kcontrol_new i2s_hdmi_loopback_ctrls[] = {
	//TODO: add dai controls here
};

static int i2s_hdmi_loopback_startup(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct hdmi_lpbk_priv *lpbk = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s: start %p %p\n", __func__, ss, dai);
	aio_sw_rst(lpbk->aio_handle, AIO_SW_RST_MIC5, 0);
	aio_sw_rst(lpbk->aio_handle, AIO_SW_RST_MIC5, 1);

	hdmi_lpbk_ch_en(lpbk, 1);
	hdmi_lpbk_ch_mute(lpbk, 1);

	return 0;
}

static void i2s_hdmi_loopback_shutdown(struct snd_pcm_substream *ss,
				struct snd_soc_dai *dai)
{
	struct hdmi_lpbk_priv *lpbk = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s: start %p %p\n", __func__, ss, dai);
	hdmi_lpbk_ch_mute(lpbk, 1);
	hdmi_lpbk_ch_en(lpbk, 0);
}

static int i2s_hdmi_loopback_hw_params(struct snd_pcm_substream *ss,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct hdmi_lpbk_priv *lpbk = snd_soc_dai_get_drvdata(dai);
	struct berlin_ss_params ssparams;
	int ret;

	aio_set_loopback_clk_gate(lpbk->aio_handle,
		   AIO_LOOPBACK_CLK_GATE_MIC5, 0);
	ret = aio_configure_loopback(lpbk->aio_handle, AIO_ID_MIC5_RX,
		 params_channels(params), lpbk->dummy_data);

	if (ret < 0) {
		snd_printk("failed to configure loopback %d\n", ret);
		return ret;
	}

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = I2SI_MODE;
	ssparams.enable_mic_mute = !lpbk->disable_mic_mute;
	ssparams.irq = &lpbk->irq;
	ssparams.interleaved = true;
	ssparams.dummy_data = lpbk->dummy_data;
	ssparams.dev_name = lpbk->dev_name;
	ret = berlin_pcm_request_dma_irq(ss, &ssparams);

	if (ret == 0)
		lpbk->irq_requested = true;

	return ret;
}

static int i2s_hdmi_loopback_set_dai_fmt(struct snd_soc_dai *dai,
				  unsigned int fmt)
{
	struct hdmi_lpbk_priv *lpbk = snd_soc_dai_get_drvdata(dai);
	struct device *dev = &lpbk->pdev->dev;
	int ret = 0;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_PDM:
		//TODO
		break;
	default:
		dev_err(dev, "%s: Unknown DAI format mask %x\n",
			__func__, fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_NB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_IB_IF:
		//TODO
		break;
	default:
		dev_err(dev, "%s: Unknown DAI invert mask %x\n",
			__func__, fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_CBS_CFS:
	case SND_SOC_DAIFMT_CBM_CFS:
	case SND_SOC_DAIFMT_CBS_CFM:
		//TODO
		break;
	default:
		dev_err(dev, "%s: Unknown DAI master mask %x\n",
			__func__, fmt);
		return -EINVAL;
	}

	return ret;
}

static int i2s_hdmi_loopback_hw_free(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct hdmi_lpbk_priv *lpbk = snd_soc_dai_get_drvdata(dai);

	if (lpbk->irq_requested && lpbk->irq >= 0) {
		berlin_pcm_free_dma_irq(ss, 1, &lpbk->irq);
		lpbk->irq_requested = false;
	}

	hdmi_lpbk_ch_flush(lpbk, 1);

	return 0;
}

static int i2s_hdmi_loopback_trigger(struct snd_pcm_substream *ss,
			      int cmd, struct snd_soc_dai *dai)
{
	struct hdmi_lpbk_priv *lpbk = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		hdmi_lpbk_ch_mute(lpbk, 0);
		hdmi_lpbk_enable(lpbk, 1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		hdmi_lpbk_enable(lpbk, 0);
		hdmi_lpbk_ch_mute(lpbk, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2s_hdmi_loopback_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, i2s_hdmi_loopback_ctrls,
				 ARRAY_SIZE(i2s_hdmi_loopback_ctrls));

	return 0;
}

static struct snd_soc_dai_ops i2s_dai_hdmi_lpbk_ops = {
	.startup = i2s_hdmi_loopback_startup,
	.hw_params = i2s_hdmi_loopback_hw_params,
	.set_fmt = i2s_hdmi_loopback_set_dai_fmt,
	.hw_free = i2s_hdmi_loopback_hw_free,
	.trigger = i2s_hdmi_loopback_trigger,
	.shutdown = i2s_hdmi_loopback_shutdown,
};

static struct snd_soc_dai_driver i2s_hdmi_loopback_dai = {
	.name = "i2s-hdmi-loopback",
	.probe = i2s_hdmi_loopback_dai_probe,
	.capture = {
		.stream_name = "HDMI-Loopback-Capture",
		.channels_min = 2,
		.channels_max = 8,
		.rates = HDMI_LPBK_RATES,
		.formats = HDMI_LPBK_FORMATS,
	},
	.ops = &i2s_dai_hdmi_lpbk_ops,
};

static const struct snd_soc_component_driver i2s_hdmi_loopback_component = {
	.name = "i2s-hdmi-loopback",
};

static int i2s_hdmi_loopback_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct hdmi_lpbk_priv *lpbk;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	lpbk = devm_kzalloc(dev, sizeof(struct hdmi_lpbk_priv), GFP_KERNEL);
	if (!lpbk)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		snd_printk("fail to get irq for node %s\n", pdev->name);
		return irq;
	}
	lpbk->irq = irq;
	lpbk->chid = irqd_to_hwirq(irq_get_irq_data(irq));
	if (lpbk->chid < 0) {
		snd_printk("got invalid dhub chid %d\n", lpbk->chid);
		return -EINVAL;
	}
	snd_printd("got irq %d chid %d\n", lpbk->irq, lpbk->chid);

	lpbk->dummy_data = of_property_read_bool(np, "dummy_data");
	lpbk->disable_mic_mute = of_property_read_bool(np, "disablemicmute");
	lpbk->dev_name = dev_name(dev);
	lpbk->pdev = pdev;
	lpbk->aio_handle = open_aio(lpbk->dev_name);
	if (unlikely(lpbk->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n",
				    lpbk->aio_handle);
		return -EBUSY;
	}
	dev_set_drvdata(dev, lpbk);

	ret = devm_snd_soc_register_component(dev,
					      &i2s_hdmi_loopback_component,
					      &i2s_hdmi_loopback_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		goto fail_register_dai;
	}
	snd_printd("%s: done irq %d chid %d dummy_data %d\n", __func__,
		lpbk->irq, lpbk->chid, lpbk->dummy_data);
	return ret;
fail_register_dai:
	close_aio(lpbk->aio_handle);
	lpbk->aio_handle = NULL;
	return ret;
}

static int i2s_hdmi_loopback_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hdmi_lpbk_priv *lpbk;

	lpbk = (struct hdmi_lpbk_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (lpbk && lpbk->aio_handle) {
		close_aio(lpbk->aio_handle);
		lpbk->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id i2s_hdmi_loopback_dt_ids[] = {
	{ .compatible = "syna,vs680-i2s-hdmi-lpbk",  },
	{ .compatible = "syna,vs640-i2s-hdmi-lpbk",  },
	{}
};
MODULE_DEVICE_TABLE(of, i2s_hdmi_loopback_dt_ids);

static struct platform_driver i2s_hdmi_loopback_driver = {
	.probe = i2s_hdmi_loopback_probe,
	.remove = i2s_hdmi_loopback_remove,
	.driver = {
		.name = "syna-i2s-hdmi-lpbk",
		.of_match_table = i2s_hdmi_loopback_dt_ids,
	},
};
module_platform_driver(i2s_hdmi_loopback_driver);

MODULE_DESCRIPTION("Synaptics I2S HDMI Loopback ALSA driver");
MODULE_ALIAS("platform:i2s-hdmi-lpbk");
MODULE_LICENSE("GPL v2");
