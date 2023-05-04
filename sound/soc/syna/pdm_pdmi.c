// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <sound/soc.h>

#include "pdm.h"
#include "berlin_pcm.h"
#include "aio_hal.h"
#include "avio_common.h"

static struct snd_kcontrol_new pdm_pdmi_ctrls[] = {
	//TODO: add dai control
};

static int pdm_pdmi_startup(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	snd_printd("%s: start 0x%p 0x%p\n", __func__, ss, dai);

	return 0;
}

static void pdm_pdmi_shutdown(struct snd_pcm_substream *ss,
				struct snd_soc_dai *dai)
{
	snd_printd("%s: end 0x%p 0x%p\n", __func__, ss, dai);
}

static int pdm_pdmi_hw_params(struct snd_pcm_substream *ss,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct pdm_pdmi_priv *pdmi = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params), chid_num;
	int ret = 0, i;
	struct berlin_ss_params ssparams;

	/* AVPLL configuration */
	//aio_set_pll(fs, 4, 0);
	/* AIO configuration */
	aio_pdm_set_ctrl1_div(pdmi->aio_handle, fs);

	for (i = 0; i < pdmi->irqc; i++) {
		aio_pdm_ch_mute(pdmi->aio_handle, i, 1);
		aio_pdm_ch_en(pdmi->aio_handle, i, 1);
		aio_pdm_rdfd_set(pdmi->aio_handle, i, 0x3, 0x3);
	}

	chid_num = (params_channels(params)+1) / 2;
	if (chid_num > pdmi->irqc) {
		snd_printk("max %d ch support by dts\n", 2 * pdmi->irqc);
		return -EINVAL;
	}

	ssparams.irq_num = 1;
	ssparams.chid_num = chid_num;
	ssparams.mode = PDMI_MODE;
	ssparams.enable_mic_mute = true;
	ssparams.irq = pdmi->irq;
	ssparams.interleaved = false;
	ssparams.dummy_data = false;
	ssparams.dev_name = pdmi->dev_name;
	ret = berlin_pcm_request_dma_irq(ss, &ssparams);

	if (ret == 0)
		pdmi->irq_requested = true;

	berlin_pcm_max_ch_inuse(ss, pdmi->max_ch_inuse);

	return ret;
}

static int pdm_pdmi_hw_free(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	struct pdm_pdmi_priv *pdmi = snd_soc_dai_get_drvdata(dai);
	int i;

	for (i = 0; i < pdmi->irqc; i++) {
		aio_pdm_ch_mute(pdmi->aio_handle, i, 1);
		aio_pdm_ch_en(pdmi->aio_handle, i, 0);
	}

	if (pdmi->irq_requested) {
		berlin_pcm_free_dma_irq(ss, 1, pdmi->irq);
		pdmi->irq_requested = false;
	}

	return 0;
}

static void pdm_pdmi_trigger_start(struct snd_pcm_substream *ss,
				     struct snd_soc_dai *dai)
{
	struct pdm_pdmi_priv *pdmi = snd_soc_dai_get_drvdata(dai);
	int i;

	for (i = 0; i < pdmi->irqc; i++) {
		aio_pdm_ch_mute(pdmi->aio_handle, i, 0);
	}

	aio_pdm_global_en(pdmi->aio_handle, 1);
}

static void pdm_pdmi_trigger_stop(struct snd_pcm_substream *ss,
				    struct snd_soc_dai *dai)
{
	struct pdm_pdmi_priv *pdmi = snd_soc_dai_get_drvdata(dai);
	aio_pdm_global_en(pdmi->aio_handle, 0);
}

static int pdm_pdmi_trigger(struct snd_pcm_substream *ss,
			      int cmd,
			      struct snd_soc_dai *dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pdm_pdmi_trigger_start(ss, dai);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pdm_pdmi_trigger_stop(ss, dai);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pdm_pdmi_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, pdm_pdmi_ctrls,
				 ARRAY_SIZE(pdm_pdmi_ctrls));

	return 0;
}

static struct snd_soc_dai_ops pdm_dai_pdmi_ops = {
	.startup = pdm_pdmi_startup,
	.hw_params = pdm_pdmi_hw_params,
	.hw_free = pdm_pdmi_hw_free,
	.trigger = pdm_pdmi_trigger,
	.shutdown = pdm_pdmi_shutdown,
};

static struct snd_soc_dai_driver pdm_pdmi_dai = {
	.name = "pdm-pdmi",
	.probe = pdm_pdmi_dai_probe,
	.capture = {
		.stream_name = "PDMI-Capture",
		.channels_min = MIN_CHANNELS,
		.channels_max = MAX_CHANNELS,
		.rates = PDM_PDMI_RATES,
		.formats = PDM_PDMI_FORMATS,
	},
	.ops = &pdm_dai_pdmi_ops,
};

static const struct snd_soc_component_driver pdm_pdmi_component = {
	.name = "pdm-pdmi",
};

static int pdm_pdmi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct pdm_pdmi_priv *pdmi;
	int ret;
	u32 i = 0;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	pdmi = devm_kzalloc(dev, sizeof(struct pdm_pdmi_priv), GFP_KERNEL);
	if (!pdmi)
		return -ENOMEM;

	pdmi->irqc = platform_irq_count(pdev);
	if (pdmi->irqc == 0) {
		snd_printk("no pdm channel defined in dts, do nothing\n");
		return 0;
	}
	for (i = 0; i < pdmi->irqc; i++) {
		pdmi->irq[i] = platform_get_irq(pdev, i);
		if (pdmi->irq[i] <= 0) {
			snd_printk("fail to get irq %d for %s\n",
				   pdmi->irq[i], pdev->name);
			return -EINVAL;
		}
		pdmi->chid[i] = irqd_to_hwirq(irq_get_irq_data(pdmi->irq[i]));
		if (pdmi->chid[i] < 0) {
			snd_printk("got invalid dhub chid %d\n", pdmi->chid[i]);
			return -EINVAL;
		}
	}

	ret = of_property_read_u32(np, "max-ch-inuse", &pdmi->max_ch_inuse);
	if (ret != 0)
		pdmi->max_ch_inuse = MAX_CHANNELS;

	pdmi->dev_name = dev_name(dev);
	pdmi->aio_handle = open_aio(pdmi->dev_name);
	if (unlikely(pdmi->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", pdmi->aio_handle);
		return -ENODEV;
	}

	dev_set_drvdata(dev, pdmi);

	ret = devm_snd_soc_register_component(dev,
					      &pdm_pdmi_component,
					      &pdm_pdmi_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}

	snd_printd("%s done irqc %d, max ch inuse %d\n", __func__,
		   pdmi->irqc, pdmi->max_ch_inuse);
	return ret;
}

static int pdm_pdmi_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pdm_pdmi_priv *pdmi;

	pdmi = (struct pdm_pdmi_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if(pdmi && pdmi->aio_handle) {
		close_aio(pdmi->aio_handle);
		pdmi->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id pdm_pdmi_dt_ids[] = {
	{ .compatible = "syna,as370-pdmi",},
	{ .compatible = "syna,vs680-pdm-pdmi",},
	{}
};
MODULE_DEVICE_TABLE(of, pdm_pdmi_dt_ids);

static struct platform_driver pdm_pdmi_driver = {
	.probe = pdm_pdmi_probe,
	.remove = pdm_pdmi_remove,
	.driver = {
		.name = "syna-pdmi-dai",
		.of_match_table = pdm_pdmi_dt_ids,
	},
};
module_platform_driver(pdm_pdmi_driver);

MODULE_DESCRIPTION("Synaptics PDM Capture ALSA driver");
MODULE_ALIAS("platform:pdm-pdmi");
MODULE_LICENSE("GPL v2");
