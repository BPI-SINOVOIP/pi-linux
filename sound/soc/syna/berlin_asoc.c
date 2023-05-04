// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/gpio/consumer.h>
#include <linux/of_platform.h>
#include <linux/module.h>

#include <sound/pcm_params.h>
#include <sound/soc.h>
#include "avio_common.h"

/* TODO: add correct route and widgets here or dts */
static const struct snd_soc_dapm_route berlin_asoc_route[] = {
};

/* TODO: Add all possible widgets here or dts */
static const struct snd_soc_dapm_widget berlin_asoc_widgets[] = {
};

static int berlin_asoc_link_hwparam(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec = rtd->codec_dai;
	int i = 0, cret = 0;

	if (rtd->num_codecs == 2) {
		for (i = 0; i < rtd->num_codecs; i++) {
			struct snd_soc_dai *codec_dai = rtd->codec_dais[i];

			if (strstr(codec->name, "cx2072x") || strstr(codec->name, "cx9000")) {
				cret = snd_soc_dai_set_sysclk(codec_dai, 1, 6144000, 0);
				if (cret)
					dev_err(rtd->dev, "error %d set codec %s\n",
						cret, codec_dai->name);
				cret = snd_soc_dai_set_bclk_ratio(codec_dai, 64);
				if (cret)
					dev_err(rtd->dev, "error %d set codec %s bclkratio \n",
						cret, codec_dai->name);
			}
		}
	} else {
		/* try to set codec dai fmt */
		if (strstr(codec->name, "tas2770")) {
			/* set codec DAI slots, 2 channels, slot width 32 */
			cret = snd_soc_dai_set_tdm_slot(codec, 0xFF, 0xFF, 2, 32);
			if (cret)
				dev_err(rtd->dev, "setting codec %s slot err %d\n",
					codec->name, cret);
		}

		if (strstr(codec->name, "cx9000")) {
			cret = snd_soc_dai_set_sysclk(codec, 1, 3072000, 0);
			if (cret)
				dev_err(rtd->dev, "error %d set codec %s\n",
					cret, codec->name);

			cret = snd_soc_dai_set_bclk_ratio(codec, 64);
			if (cret)
				dev_err(rtd->dev, "error %d set codec %s bclkratio \n",
					cret, codec->name);
		}
	}

	return cret;
}

static struct snd_soc_ops asoc_link_ops = {
	.hw_params = &berlin_asoc_link_hwparam,
};

static int snd_berlin_rate_control_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = -500;
	uinfo->value.integer.max = 500;
	uinfo->value.integer.step = 1;
	return 0;
}

static int snd_berlin_rate_control_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	//TODO: add ppm func back here
#if 0
	int ppm_base, ppm_now;

	AVPLL_GetPPM(&ppm_base, &ppm_now);
	ucontrol->value.integer.value[0] = (ppm_now - ppm_base);
	snd_printd("%s: get ppm %d\n", __func__, ppm_now - ppm_base);
#endif
	return 0;
}

static int snd_berlin_rate_control_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	//TODO: add ppm func back here
#if 0
	int ppm_base, ppm_now;
	int ppm, current_ppm;

	ppm = ucontrol->value.integer.value[0];
	if ((ppm < -500) || (ppm > 500))
		return -1;

	AVPLL_GetPPM(&ppm_base, &ppm_now);
	current_ppm = (ppm_now - ppm_base);
	if (ppm != current_ppm) {
		AVPLL_AdjustPPM(ppm_base - ppm_now + ppm);
		snd_printd("%s: adjust ppm %d -> %d\n", __func__,
			   current_ppm, ppm);
		return 1;
	}
#endif
	return 0;
}

static struct snd_kcontrol_new snd_berlin_rate_control = {
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = "PCM Playback Rate Offset",
	.index = 0,
	.device = 0,
	.subdevice = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xffff,
	.get = snd_berlin_rate_control_get,
	.put = snd_berlin_rate_control_put,
	.info = snd_berlin_rate_control_info
};

struct berlin_asoc_priv {
	struct snd_soc_dai_link dai_link[16];
	struct snd_soc_dai_link_component dai_link_comp_cpu[16];
	struct snd_soc_dai_link_component dai_link_comp_codec[16];
	struct snd_soc_dai_link_component dai_link_comp_platform[16];
	u32 link_num;
	struct platform_device *pdev;
	struct snd_soc_card card;
	struct gpio_desc *mute_status_gpio;
	bool mute_status_gpio_output;
	char name[32];
};

static struct berlin_asoc_priv *dev_to_berlin_asoc(struct device *dev)
{
	struct snd_card *card = NULL;

	card = dev_to_snd_card(dev);
	if (card == NULL)
		return NULL;
	return (struct berlin_asoc_priv *)card->private_data;
}

static ssize_t mic_mute_state_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct berlin_asoc_priv *priv = dev_to_berlin_asoc(dev);
	ssize_t status;

	if (priv == NULL)
		return -ENODEV;

	status = sprintf(buf, "%d\n", gpiod_get_value_cansleep(priv->mute_status_gpio));
	return status;
}

static ssize_t mic_mute_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct berlin_asoc_priv *priv = dev_to_berlin_asoc(dev);
	ssize_t status;
	long value;

	if (priv == NULL)
		return -ENODEV;

	if (!priv->mute_status_gpio_output)
		return -EINVAL;

	status = kstrtol(buf, 0, &value);
	if (status == 0) {
		gpiod_set_value_cansleep(priv->mute_status_gpio, value);
		status = size;
	}
	return status;
}

static DEVICE_ATTR_RW(mic_mute_state);

static struct snd_soc_dai_link_component soundbar_multi_codecs[] = {
	{
		.dai_name = "cx9000-amplifier",
	},
	{
		.dai_name = "cx2072x-hifi",
	},
};

static int berlin_asoc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *cpu_np, *codec_np, *platform_np, *iter;
	struct device_node *np = pdev->dev.of_node;
	struct berlin_asoc_priv *priv;
	struct snd_kcontrol *kctl;
	int ret;
	int i;
	u32 multi_codecs = 0;
	const char *cpstr;
	char *chip_name;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	for (i = 0; i < 16; i++) {
		priv->dai_link[i].cpus = &(priv->dai_link_comp_cpu[i]);
		priv->dai_link[i].codecs = &(priv->dai_link_comp_codec[i]);
		priv->dai_link[i].platforms = &(priv->dai_link_comp_platform[i]);
	}

	priv->mute_status_gpio_output = of_property_read_bool(np, "mute-gpio-output");
	if (priv->mute_status_gpio_output)
		priv->mute_status_gpio = devm_gpiod_get_optional(dev, "mute", GPIOD_OUT_LOW);
	else
		priv->mute_status_gpio = devm_gpiod_get_optional(dev, "mute", GPIOD_IN);

	if (IS_ERR(priv->mute_status_gpio))
		return PTR_ERR(priv->mute_status_gpio);

	device_property_read_u32(dev, "codec_nums", &multi_codecs);

	for_each_child_of_node(np, iter) {
		struct snd_soc_dai_link *plink;

		if (!of_device_is_available(iter))
			continue;
		snd_printd("iter %s available\n", iter->name);

		/* dai-link */
		plink = &priv->dai_link[priv->link_num];

		/* Either cpu of_node or name */
		cpu_np = of_parse_phandle(iter, "cpu-node", 0);
		if (cpu_np) {
			plink->cpus->of_node = cpu_np;
			plink->num_cpus++;
		} else {
			of_property_read_string(iter, "cpu-name",
						&plink->cpus->name);
			if (plink->cpus->name == NULL)
				continue;
		}

		/* Either codec of_node or name */
		codec_np = of_parse_phandle(iter, "codec-node", 0);
		if (codec_np) {
			plink->codecs->of_node = codec_np;
			plink->num_codecs++;
			//TODO: add slot parse
			//snd_soc_of_parse_tdm_slot(codec_np, &);
		} else {
			of_property_read_string(iter, "codec-name",
						&plink->codecs->name);
			if (plink->codecs->name != NULL)
				plink->num_codecs++;
			if ((multi_codecs == 0) && (plink->codecs->name == NULL))
				continue;
		}

		of_property_read_string(iter, "codec-dai-name",
					&plink->codecs->dai_name);

		/* Either platform of_node or name */
		platform_np = of_parse_phandle(iter, "platform-node", 0);
		if (platform_np) {
			plink->platforms->of_node = platform_np;
			plink->num_platforms++;
		} else {
			of_property_read_string(iter, "platform-name",
						&plink->platforms->name);
			if (plink->platforms->name == NULL)
				continue;
			else
				plink->num_platforms++;
		}

		of_property_read_string(iter, "link-name",
					&plink->name);
		of_property_read_string(iter, "stream-name",
					&plink->stream_name);
		snprintf(priv->name, sizeof(priv->name), "%s-%s",
				(plink->cpus->name != NULL) ?
					plink->cpus->name : plink->cpus->of_node->name,
				(plink->codecs->name != NULL) ?
					plink->codecs->name : plink->codecs->of_node->name);
		plink->ops = &asoc_link_ops;
		plink->dai_fmt = snd_soc_of_parse_daifmt(iter, NULL,
							 NULL, NULL);
		snd_printd("dai %d: cpu dai %s codec dai %s\n",
				priv->link_num,
				(plink->cpus->name != NULL) ?
						plink->cpus->name : plink->cpus->of_node->name,
				(plink->codecs->name != NULL) ?
						plink->codecs->name : plink->codecs->of_node->name);
		priv->link_num++;
	}
	if (multi_codecs == 2) {
		soundbar_multi_codecs[0].of_node = of_parse_phandle(pdev->dev.of_node, "cnxt,cx9000", 0);
		if (soundbar_multi_codecs[0].of_node == NULL)
			snd_printk("get codec cx9000 dev node failed\n");

		soundbar_multi_codecs[1].of_node = of_parse_phandle(pdev->dev.of_node, "cnxt,cx20721", 0);
		if (soundbar_multi_codecs[0].of_node == NULL)
			snd_printk("get codec cx20721 dev node failed\n");

		priv->dai_link[1].codecs = soundbar_multi_codecs;
		priv->dai_link[1].num_codecs = multi_codecs;
	}

	priv->pdev = pdev;
	priv->card.dev = &pdev->dev;
	priv->card.dai_link = priv->dai_link;
	priv->card.num_links = priv->link_num;
	priv->card.dapm_routes = berlin_asoc_route;
	priv->card.num_dapm_routes = ARRAY_SIZE(berlin_asoc_route);
	priv->card.dapm_widgets = berlin_asoc_widgets;
	priv->card.num_dapm_widgets = ARRAY_SIZE(berlin_asoc_widgets);

	ret = device_property_read_string(dev, "compatible", &cpstr);
	if (ret)
		return ret;

	chip_name = strstr(cpstr, "syna,");
	if (chip_name) {
		chip_name = chip_name + 5;
		priv->card.name = chip_name;
	} else
		priv->card.name = cpstr;

	//TODO: add audio routing in dts and enable this parse

	snd_soc_card_set_drvdata(&priv->card, priv);
	ret = devm_snd_soc_register_card(&pdev->dev, &priv->card);
	if (ret)
		return ret;
	priv->card.snd_card->private_data = priv;

	kctl = snd_ctl_new1(&snd_berlin_rate_control, NULL);
	if (kctl) {
		ret = snd_ctl_add(priv->card.snd_card, kctl);
		if (ret < 0)
			snd_printk("error adding rate control: %d\n", ret);
	} else
		snd_printk("snd_ctl_new1 FAIL!\n");

	ret = sysfs_create_file(&priv->card.snd_card->card_dev.kobj,
				&dev_attr_mic_mute_state.attr);

	snd_printd("%s: probe done\n", __func__);

	return ret;
}

static int berlin_asoc_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	sysfs_remove_file(&card->snd_card->card_dev.kobj,
		&dev_attr_mic_mute_state.attr);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int berlin_asoc_suspend(struct device *dev)
{
	struct snd_soc_card *soc_card = dev_get_drvdata(dev);

	snd_soc_suspend(soc_card->dev);

	return 0;
}

static int berlin_asoc_resume(struct device *dev)
{
	struct snd_soc_card *soc_card = dev_get_drvdata(dev);

	snd_soc_resume(soc_card->dev);

	return 0;
}
#endif

static const struct of_device_id berlin_asoc_dt_ids[] = {
	{ .compatible = "syna,berlin-asoc",  },
	{ .compatible = "syna,as370-asoc",  },
	{ .compatible = "syna,vs680-asoc", },
	{ .compatible = "syna,as470-asoc", },
	{ .compatible = "syna,vs640-asoc", },
	{}
};
MODULE_DEVICE_TABLE(of, berlin_asoc_dt_ids);


static SIMPLE_DEV_PM_OPS(berlin_asoc_pmops, berlin_asoc_suspend,
			 berlin_asoc_resume);

static struct platform_driver berlin_asoc_driver = {
	.probe = berlin_asoc_probe,
	.remove = berlin_asoc_remove,
	.driver = {
		.name = "syna-berlin-asoc",
		.of_match_table = berlin_asoc_dt_ids,
		.pm = &berlin_asoc_pmops,
	},
};
module_platform_driver(berlin_asoc_driver);

MODULE_DESCRIPTION("Synaptics Berlin ASoC ALSA driver");
MODULE_ALIAS("platform:berlin-asoc");
MODULE_LICENSE("GPL v2");
