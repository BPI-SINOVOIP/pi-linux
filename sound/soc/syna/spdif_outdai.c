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

#define SPDIF_PLAYBACK_RATES   (SNDRV_PCM_RATE_8000_192000)
#define SPDIF_PLAYBACK_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
				| SNDRV_PCM_FMTBIT_S24_LE \
				| SNDRV_PCM_FMTBIT_S24_3LE \
				| SNDRV_PCM_FMTBIT_S32_LE)

struct spdifo_priv {
	struct device *dev;
	const char *dev_name;
	unsigned int spdif_irq;
	u32 spdif_chid;
	u32 mode;
	bool spdif_requested;
	void *aio_handle;
};

static void outdai_set_spdif_clk(struct spdifo_priv *out, u32 div)
{
	int ret;

	ret = aio_setspdifclk(out->aio_handle,  div);
	if (ret != 0)
		snd_printk("aio_setspdifclk() return error(ret=%d)\n", ret);
}

static struct snd_kcontrol_new berlin_outdai_ctrls[] = {
	//TODO: add dai control here
};

/*
 * Applies output configuration of |berlin_pcm| to i2s.
 * Must be called with instance spinlock held.
 * Only one dai instance for playback, so no spin_lock needed
 */
static void outdai_set_aio(struct spdifo_priv *out, u32 fs)
{
	unsigned int analog_div, spdif_div;

	analog_div = berlin_get_div(fs);
	spdif_div =  analog_div - 1;

	outdai_set_spdif_clk(out, spdif_div);
}

static int berlin_outdai_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	//TODO
	return 0;
}

static void berlin_outdai_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	//TODO
}

static int berlin_outdai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct spdifo_priv *outdai = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params);
	int ret;
	struct berlin_ss_params ssparams;

	/* mclk */
	aio_i2s_set_clock(outdai->aio_handle, AIO_ID_SPDIF_TX, 1, AIO_CLK_D3_SWITCH_NOR,
						AIO_CLK_SEL_D8, AIO_APLL_1, 1);

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = SPDIFO_MODE;
	ssparams.irq = &outdai->spdif_irq;
	ssparams.dev_name = outdai->dev_name;
	ret = berlin_pcm_request_dma_irq(substream, &ssparams);
	if (ret == 0)
		outdai->spdif_requested = true;
	else
		return ret;
	aio_setspdif_en(outdai->aio_handle, 1);


	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_SPDIF_TX, AIO_TSD0, 0);

	berlin_set_pll(outdai->aio_handle, AIO_APLL_1, fs);

	outdai_set_aio(outdai, fs);

	return ret;
}

static int berlin_outdai_hw_free(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct spdifo_priv *outdai = snd_soc_dai_get_drvdata(dai);

	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_SPDIF_TX, AIO_TSD0, 1);
	aio_setspdif_en(outdai->aio_handle, 0);

	if (outdai->spdif_requested && outdai->spdif_irq >= 0) {
		berlin_pcm_free_dma_irq(substream, 1, &outdai->spdif_irq);
		outdai->spdif_requested = false;
	}

	return 0;
}

static int berlin_outdai_trigger(struct snd_pcm_substream *substream,
				 int cmd, struct snd_soc_dai *dai)
{
	struct spdifo_priv *outdai = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_SPDIF_TX, AIO_TSD0, 0);
		aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_SPDIF_TX, AIO_TSD0, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_SPDIF_TX, AIO_TSD0, 1);
		aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_SPDIF_TX, AIO_TSD0, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int berlin_outdai_dai_probe(struct snd_soc_dai *dai)
{
	struct spdifo_priv *outdai = snd_soc_dai_get_drvdata(dai);

	aio_setspdif_en(outdai->aio_handle, 1);

	snd_soc_add_dai_controls(dai, berlin_outdai_ctrls,
				 ARRAY_SIZE(berlin_outdai_ctrls));
	return 0;
}

static struct snd_soc_dai_ops berlin_spdif_outdai_ops = {
	.startup   = berlin_outdai_startup,
	.hw_params = berlin_outdai_hw_params,
	.hw_free   = berlin_outdai_hw_free,
	.trigger   = berlin_outdai_trigger,
	.shutdown  = berlin_outdai_shutdown,
};

static struct snd_soc_dai_driver berlin_outdai_dai = {
	.name = "spdif-outdai",
	.probe = berlin_outdai_dai_probe,
	.playback = {
		.stream_name = "SPDIF-Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = SPDIF_PLAYBACK_RATES,
		.formats = SPDIF_PLAYBACK_FORMATS,
	},
	.ops = &berlin_spdif_outdai_ops,
};

static const struct snd_soc_component_driver berlin_outdai_component = {
	.name = "spdif-outdai",
};

static int spdif_outdai_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spdifo_priv *outdai;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	outdai = devm_kzalloc(dev, sizeof(struct spdifo_priv),
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

	irq = platform_get_irq_byname(pdev, "spdifo");
	if (irq >= 0) {
		outdai->mode |= SPDIFO_MODE;
		outdai->spdif_irq = irq;
		outdai->spdif_chid = irqd_to_hwirq(irq_get_irq_data(irq));
		snd_printd("get spdif irq %d for node %s\n",
			   irq, pdev->name);
	}

	if (outdai->mode == 0) {
		snd_printk("non valid irq found in dts\n");
		return -EINVAL;
	}

	ret = devm_snd_soc_register_component(dev,
					      &berlin_outdai_component,
					      &berlin_outdai_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}
	snd_printd("spdif [%d %d]\n", outdai->spdif_irq, outdai->spdif_chid);
	return ret;
}

static int spdif_outdai_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spdifo_priv *outdai;

	outdai = (struct spdifo_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (outdai && outdai->aio_handle) {
		close_aio(outdai->aio_handle);
		outdai->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id spdif_outdai_dt_ids[] = {
	{ .compatible = "syna,vs680-spdifo",  },
	{ .compatible = "syna,vs640-spdifo",  },
	{}
};
MODULE_DEVICE_TABLE(of, spdif_outdai_dt_ids);

static struct platform_driver spdif_outdai_driver = {
	.probe = spdif_outdai_probe,
	.remove = spdif_outdai_remove,
	.driver = {
		.name = "syna-spdif-outdai",
		.of_match_table = spdif_outdai_dt_ids,
	},
};
module_platform_driver(spdif_outdai_driver);

MODULE_DESCRIPTION("Synaptics SPDIF ALSA output dai");
MODULE_ALIAS("platform:spdif-outdai");
MODULE_LICENSE("GPL v2");
