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
	unsigned int spdif_irq;
	u32 spdif_chid;
	u32 mode;
	struct aud_ctrl ctrl;
	bool i2s_requested;
	bool spdif_requested;
	bool keep_clk;
	bool output_mclk;
	void *aio_handle;
};

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

static void outdai_enable_i2s1_mclk_oen(struct outdai_priv *out, bool en)
{
	/*comment out below impl, useless*/
	//u32 offset = RA_avioGbl_CTRL0;
	//u32 val;
	//val = aio_read(out->avioGblbase + offset);
	//SET32avioGbl_CTRL0_I2S1_MCLK_OEN(val, en);
	//aio_write(out->avioGblbase + offset, val);
}

static void outdai_set_spdif_clk(struct outdai_priv *out, u32 div)
{
	int ret;

	ret = aio_setspdifclk(out->aio_handle,  div);
	if (ret != 0)
		snd_printk("aio_setspdifclk() return error(ret=%d)\n", ret);
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
	unsigned int analog_div, spdif_div, cfm, dfm;

	analog_div = berlin_get_div(fs);
	spdif_div =  analog_div - 1;
	cfm = berlin_get_cfm((width == 24 ? 32 : width));
	dfm = berlin_get_dfm((width == 24 ? 32 : width));

	if (out->mode & I2SO_MODE) {
		struct aud_ctrl ctrl;
		unsigned int div, bclk;

		ctrl.chcnt	= chnum;
		ctrl.width_word	= cfm;
		ctrl.width_sample	= dfm;
		ctrl.data_fmt	= out->ctrl.data_fmt;
		ctrl.isleftjfy	= out->ctrl.isleftjfy;
		ctrl.invbclk	= out->ctrl.invbclk;
		ctrl.invfs	= out->ctrl.invfs;
		ctrl.msb	= out->ctrl.msb;
		ctrl.istdm	= out->ctrl.istdm;
		ctrl.islframe	= out->ctrl.islframe;

		if (out->ctrl.istdm) {
			/* TDM */
			bclk = fs * (width == 24 ? 32 : width) * chnum;
		} else {
			/* i2s */
			bclk = fs * 32 * 2;
			ctrl.width_word = AIO_32CFM;
		}
		div = (24576000 * 8) / (8 * bclk);
		div = ilog2(div);

		outdai_set_clk_div(out, div);
		outdai_set_ctl(out, &ctrl);

		/* MIC1 will use the PRIAUD bclk and need to invert it */
		outdai_iosel_set_bclk(out, true);
	}

	if (out->mode & SPDIFO_MODE)
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

static int berlin_outdai_setfmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	outdai->ctrl.isleftjfy = true;
	outdai->ctrl.msb       = true;
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		outdai->ctrl.data_fmt  = 2;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		outdai->ctrl.data_fmt  = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		outdai->ctrl.data_fmt  = 1;
		outdai->ctrl.isleftjfy = false;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		outdai->ctrl.data_fmt  = 1;
		outdai->ctrl.istdm     = true;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		outdai->ctrl.data_fmt  = 2;
		outdai->ctrl.istdm     = true;
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
		ret = -EINVAL;
	}

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

	if (outdai->mode & I2SO_MODE) {
		/* mclk */
		aio_i2s_set_clock(outdai->aio_handle, AIO_ID_PRI_TX,
			1, 0, 4, AIO_APLL_0, 1);

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
		if (!outdai->keep_clk)
			aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 1);
		aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 1);
	}

	if (outdai->mode & SPDIFO_MODE) {
		/* mclk */
		aio_i2s_set_clock(outdai->aio_handle, AIO_ID_SPDIF_TX,
			 1, 0, 4, AIO_APLL_1, 1);

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
	}

	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 0);

	berlin_set_pll(outdai->aio_handle, fs);

	outdai_set_aio(outdai, fs, params_width(params), chnum);

	return ret;
}

static int berlin_outdai_hw_free(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);

	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 1);

	if (!outdai->keep_clk && (outdai->mode & I2SO_MODE))
		aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 0);

	if (outdai->i2s_requested && outdai->i2s_irq >= 0) {
		berlin_pcm_free_dma_irq(substream, 1, &outdai->i2s_irq);
		outdai->i2s_requested = false;
	}

	if (outdai->mode & SPDIFO_MODE)
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
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (outdai->mode & I2SO_MODE) {
			if (outdai->output_mclk)
				outdai_enable_i2s1_mclk_oen(outdai, 1);
			aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 0);
			outdai_enable_primaryport(outdai, 1);
		}
		aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (!outdai->keep_clk && (outdai->mode & I2SO_MODE)) {
			outdai_enable_primaryport(outdai, 0);
			aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 1);
			if (outdai->output_mclk)
				outdai_enable_i2s1_mclk_oen(outdai, 0);
		}
		aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int berlin_outdai_dai_probe(struct snd_soc_dai *dai)
{
	struct outdai_priv *outdai = snd_soc_dai_get_drvdata(dai);

	if (outdai->keep_clk && (outdai->mode & I2SO_MODE))
		aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_PRI_TX, AIO_TSD0, 1);

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
	.name = "berlin-outdai-dai",
	.probe = berlin_outdai_dai_probe,
	.playback = {
		.stream_name = "Playback",
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
	snd_printd("%s: done i2s [%d %d] spdif [%d %d]\n", __func__,
		   outdai->i2s_irq, outdai->i2s_chid,
		   outdai->spdif_irq, outdai->spdif_chid);

	outdai->keep_clk = of_property_read_bool(np, "keep-clk");

	outdai->output_mclk = of_property_read_bool(np, "output-mclk");

	outdai->ctrl.istdm = of_property_read_bool(np, "tdm");
	outdai->ctrl.islframe = of_property_read_bool(np, "long-frame");

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
	{ .compatible = "syna,vs680-i2s-spdif",  },
	{ .compatible = "syna,as470-outdai",  },
	{ .compatible = "syna,vs640-i2s-spdif",  },
	{}
};
MODULE_DEVICE_TABLE(of, i2s_outdai_dt_ids);

static struct platform_driver i2s_outdai_driver = {
	.probe = i2s_outdai_probe,
	.remove = i2s_outdai_remove,
	.driver = {
		.name = "syna-i2s-outdai",
		.of_match_table = i2s_outdai_dt_ids,
	},
};
module_platform_driver(i2s_outdai_driver);

MODULE_DESCRIPTION("Synaptics I2S ALSA output dai");
MODULE_ALIAS("platform:i2s-outdai");
MODULE_LICENSE("GPL v2");
