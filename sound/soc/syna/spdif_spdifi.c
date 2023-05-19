// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */
#define DEBUG
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <sound/soc.h>
#include <linux/gpio/consumer.h>

#include "berlin_pcm.h"
#include "aio_hal.h"
#include "avio_common.h"

#define SPDIFI_RATES (SNDRV_PCM_RATE_32000 \
			| SNDRV_PCM_RATE_44100 \
			| SNDRV_PCM_RATE_48000 \
			| SNDRV_PCM_RATE_88200 \
			| SNDRV_PCM_RATE_96000 \
			| SNDRV_PCM_RATE_176400 \
			| SNDRV_PCM_RATE_192000)
#define SPDIFI_FORMATS (SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE \
			   | SNDRV_PCM_FMTBIT_S32_LE)

static const struct of_device_id spdifi_dt_ids[] = {
	{ .compatible = "syna,as370-spdifi", },
	{ .compatible = "syna,as371-spdifi", },
	{ .compatible = "syna,vs680-spdifi", },
	{}
};
MODULE_DEVICE_TABLE(of, spdifi_dt_ids);

struct spdifi_priv {
	const char *dev_name;
	unsigned int irq;
	bool irq_requested;
	u32 chid;
	bool arc_data;
	bool arc_clk;
	bool dynamic_src_ctrl;
	void *aio_handle;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *gate_gpio;
};

static struct snd_kcontrol_new spdifi_ctrls[] = {
	//TODO: add dai control
};

static int spdifi_startup(struct snd_pcm_substream *ss,
			      struct snd_soc_dai *dai)
{
	snd_printd("%s: start 0x%p 0x%p\n", __func__, ss, dai);

	return 0;
}

static void spdifi_shutdown(struct snd_pcm_substream *ss,
				  struct snd_soc_dai *dai)
{
	snd_printd("%s: end 0x%p 0x%p\n", __func__, ss, dai);
}

static int spdifi_hw_params(struct snd_pcm_substream *ss,
				  struct snd_pcm_hw_params *params,
				  struct snd_soc_dai *dai)
{
	struct spdifi_priv *spdifi = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params), width = params_width(params);
	struct berlin_ss_params ssparams;
	int ret = 0;

	/* Trigger spdif reset as h/w init */
	aio_spdifi_sw_reset(spdifi->aio_handle);

	/* dynamic_src_ctrl true: user can switch source dynamically */
	if (!spdifi->dynamic_src_ctrl)
		aio_spdifi_src_sel(spdifi->aio_handle,
			spdifi->arc_clk ? 1 : 0,
			spdifi->arc_data ? 1 : 0);

	ret = aio_spdifi_config(spdifi->aio_handle, fs, width);

	if (ret < 0) {
		snd_printk("failed to config SPDIF DAI: %d\n", ret);
		aio_spdifi_config_reset(spdifi->aio_handle);
		return -EINVAL;
	}

	ssparams.irq_num = 1;
	ssparams.chid_num = (params_channels(params) + 1) / 2;
	ssparams.mode = SPDIFI_MODE | FLAG_EARC_MODE;
	ssparams.enable_mic_mute = false;
	ssparams.irq = &spdifi->irq;
	ssparams.dev_name = spdifi->dev_name;
	ssparams.interleaved = false;
	ssparams.dummy_data = false;
	ret = berlin_pcm_request_dma_irq(ss, &ssparams);
	if (ret == 0)
		spdifi->irq_requested = true;

	return ret;
}
static int spdifi_hw_free(struct snd_pcm_substream *ss,
				struct snd_soc_dai *dai)
{
	struct spdifi_priv *spdifi = snd_soc_dai_get_drvdata(dai);

	/* set back */
	aio_spdifi_config_reset(spdifi->aio_handle);

	if (spdifi->irq_requested) {
		berlin_pcm_free_dma_irq(ss, 1, &spdifi->irq);
		spdifi->irq_requested = false;
	}

	return 0;
}

static int spdifi_trigger(struct snd_pcm_substream *ss,
				int cmd,
				struct snd_soc_dai *dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int spdifi_dai_probe(struct snd_soc_dai *dai)
{
	return snd_soc_add_dai_controls(dai, spdifi_ctrls,
					ARRAY_SIZE(spdifi_ctrls));
}

static struct snd_soc_dai_ops dai_spdifi_ops = {
	.startup = spdifi_startup,
	.hw_params = spdifi_hw_params,
	.hw_free = spdifi_hw_free,
	.trigger = spdifi_trigger,
	.shutdown = spdifi_shutdown,
};

static struct snd_soc_dai_driver spdifi_dai = {
	.name = "berlin-spdifi",
	.probe = spdifi_dai_probe,
	.capture = {
		.stream_name = "spdifi-Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SPDIFI_RATES,
		.formats = SPDIFI_FORMATS,
	},
	.ops = &dai_spdifi_ops,
};

static const struct snd_soc_component_driver spdifi_component = {
	.name = "berlin-spdifi",
};

static int spdifi_get_sample_rate_margin(
	struct device *dev, struct spdifi_priv *spdifi)
{
	u32 val, i;
	int err = 0;
	struct spdifi_sample_rate_margin *p;

	for (i = 0; i < SPDIFI_FS_MAX; i++) {
		p = aio_spdifi_get_srm(i);
		err = device_property_read_u32(dev, p->sample_rate_name,
						&val);
		if (err) {
			snd_printk(
				"%s can't get index %d for %s,use default value 0x%0x\n",
				__func__, i,
				p->sample_rate_name, p->default_value);
		} else {
			snd_printd(
				"%s index %d srm %s value 0x%0x \n",
				__func__, i,
				p->sample_rate_name, val);
			p->config_value = val;
		}
	}
	return 0;
}

static int spdifi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spdifi_priv *spdifi;
	int ret;
	struct device_node *np = dev->of_node;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	spdifi = devm_kzalloc(dev, sizeof(struct spdifi_priv),
			       GFP_KERNEL);
	if (!spdifi)
		return -ENOMEM;

	spdifi->irq = platform_get_irq(pdev, 0);
	if (spdifi->irq <= 0) {
		snd_printk("fail to get irq %d for %s\n",
			   spdifi->irq, pdev->name);
		return -EINVAL;
	}
	spdifi->chid = irqd_to_hwirq(irq_get_irq_data(spdifi->irq));
	if (spdifi->chid < 0) {
		snd_printk("got invalid dhub chid %d\n", spdifi->chid);
		return -EINVAL;
	}

	spdifi->enable_gpio
		= devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
	/*
	when return -EBUSY, it means this gpio is already
	opened by another devcie, so set desc to NULL,
	dont return error, keep probe going on;
	*/
	if (IS_ERR(spdifi->enable_gpio)) {
		if (PTR_ERR(spdifi->enable_gpio) == -EBUSY)
			spdifi->enable_gpio = NULL;
		else
			return PTR_ERR(spdifi->enable_gpio);
	}

	spdifi->gate_gpio
		= devm_gpiod_get_optional(dev, "gate", GPIOD_OUT_HIGH);
	/*
	when return -EBUSY, it means this gpio is already
	opened by another devcie, so set desc to NULL,
	dont return error, keep probe going on;
	*/
	if (IS_ERR(spdifi->gate_gpio)) {
		if (PTR_ERR(spdifi->gate_gpio) == -EBUSY)
			spdifi->gate_gpio = NULL;
		else
			return PTR_ERR(spdifi->gate_gpio);
	}

	gpiod_set_value_cansleep(spdifi->enable_gpio, 1);
	gpiod_set_value_cansleep(spdifi->gate_gpio, 1);

	spdifi->arc_data = of_property_read_bool(np, "arc-data");
	spdifi->arc_clk = of_property_read_bool(np, "arc-clk");
	spdifi->dynamic_src_ctrl =
			of_property_read_bool(np, "dynamic-src-ctrl");

	spdifi->dev_name = dev_name(dev);
	dev_set_drvdata(dev, spdifi);
	spdifi->aio_handle = open_aio(spdifi->dev_name);
	if (unlikely(spdifi->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", spdifi->aio_handle);
		return -EBUSY;
	}

	/* MUST after aio handle got*/
	spdifi_get_sample_rate_margin(dev, spdifi);

	ret = devm_snd_soc_register_component(dev,
					      &spdifi_component,
					      &spdifi_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		goto fail_register_dai;
	}

	snd_printd("%s done irq %d chid %d\n",
		   __func__, spdifi->irq, spdifi->chid);
	return ret;
fail_register_dai:
	close_aio(spdifi->aio_handle);
	spdifi->aio_handle = NULL;
	return ret;
}

static int spdifi_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spdifi_priv *spdifi;

	spdifi = (struct spdifi_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (spdifi && spdifi->aio_handle) {
		close_aio(spdifi->aio_handle);
		spdifi->aio_handle = NULL;
	}

	return 0;
}

static struct platform_driver spdifi_driver = {
	.probe = spdifi_probe,
	.remove = spdifi_remove,
	.driver = {
		.name = "syna-spdif-arc-dai",
		.of_match_table = spdifi_dt_ids,
	},
};
module_platform_driver(spdifi_driver);

MODULE_DESCRIPTION("Synaptics SPDIF/ARC RX ALSA driver");
MODULE_ALIAS("platform:berlin-spdifi");
MODULE_LICENSE("GPL v2");
