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
	bool bt_sco;
	void *aio_handle;
};

static void sec_set_ctl(struct sec_priv *sec, struct aud_ctrl *ctrl)
{
	aio_set_ctl_ext(sec->aio_handle, AIO_ID_SEC_TX, ctrl);
	aio_set_pcm_mono(sec->aio_handle, AIO_ID_SEC_TX, (ctrl->chcnt == 1));
}

static void sec_set_clk_div(struct sec_priv *sec, u32 div)
{
	aio_setclkdiv(sec->aio_handle, AIO_ID_SEC_TX, div);
}

static void sec_blrclk_oen(struct sec_priv *sec, bool en)
{
	//FIXME later Weizhao Jiang
#if 0
	u32 val;

	//enable bclk and lrclk
	val = aio_read(sec->soen);
	SET32avioGbl_CTRL0_I2S3_BCLK_OEN(val, en);
	SET32avioGbl_CTRL0_I2S3_LRCLK_OEN(val, en);
	aio_write(sec->soen, val);
#endif
}

static void sec_enable_port(struct sec_priv *sec, bool en)
{
	aio_enabletxport(sec->aio_handle, AIO_ID_SEC_TX, en);
}

static struct snd_kcontrol_new i2s_sec_ctrls[] = {
	//TODO: add dai control here
};

/*
 * Applies output configuration of |berlin_pcm| to i2s.
 * Must be called with instance spinlock held.
 * Only one dai instance for playback, so no spin_lock needed
 */
static void sec_set_aio(struct sec_priv *sec,
			u32 fs, int width, int chnum)
{
	unsigned int div, dfm, cfm, bclk;
	struct aud_ctrl ctrl;

	aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
	aio_set_aud_ch_en(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);

	cfm = berlin_get_cfm((width == 24 ? 32 : width));
	dfm = berlin_get_dfm((width == 24 ? 32 : width));

	if (sec->ctrl.istdm) {
		/* TDM */
		bclk = fs * (width == 24 ? 32 : width) * chnum;
	} else {
		/* i2s */
		bclk = fs * 32 * 2;
		cfm = AIO_32CFM;
	}
	div = (24576000 * 8) / (8 * bclk);
	div = ilog2(div);

	ctrl.chcnt	= chnum;
	ctrl.width_word	= cfm;
	ctrl.width_sample	= dfm;
	ctrl.data_fmt	= sec->ctrl.data_fmt;
	ctrl.isleftjfy	= sec->ctrl.isleftjfy;
	ctrl.invbclk	= sec->ctrl.invbclk;
	ctrl.invfs	= sec->ctrl.invfs;
	ctrl.msb	= sec->ctrl.msb;
	ctrl.istdm	= (chnum == 1) ? 0 : sec->ctrl.istdm;
	ctrl.islframe	= sec->ctrl.islframe;

	sec_set_clk_div(sec, div);
	sec_set_ctl(sec, &ctrl);
	aio_set_aud_ch_en(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
}

// Set to slavemode, and sel bclk from external, invert CLK
static void sec_set_slave_mode(struct sec_priv *sec,
			       bool bset, u8 bsel)
{
	aio_set_slave_mode(sec->aio_handle, AIO_ID_SEC_TX, bset);
	aio_set_bclk_sel(sec->aio_handle, AIO_ID_SEC_TX, bsel);
}

static int i2s_sec_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);

	// Enable i2s channel without corresponding disable in close.
	// This is intentional: Avoid SPDIF 'activation delay' problem.
	aio_set_aud_ch_en(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
	aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);

	return 0;
}

static void i2s_sec_shutdown(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);

	/*
	 * Do not set aio_set_aud_ch_en to 0 for bt_sco application
	 * It is the duplex transfer. Need to keep lrck of i2s3
	 */
	if (!sec->bt_sco)
		aio_set_aud_ch_en(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);
}

static int i2s_sec_setfmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sec_priv *outdai = snd_soc_dai_get_drvdata(dai);
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
		aio_clk_enable(sec->aio_handle, AIO_APLL_0, true);
		berlin_set_pll(sec->aio_handle, fs);

		/* mclk */
		aio_i2s_set_clock(sec->aio_handle, AIO_ID_SEC_TX,
			1, 0, 4, AIO_APLL_0, 1);
	} else {
		sec_set_slave_mode(sec, 1, 0);
		sec_blrclk_oen(sec, 0);
	}

	sec_set_aio(sec, fs, params_width(params), params_channels(params));

	return ret;
}

static int i2s_sec_hw_free(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	struct sec_priv *sec = snd_soc_dai_get_drvdata(dai);

	if (!sec->is_master)
		sec_set_slave_mode(sec, 0, 1);

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

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		sec_enable_port(sec, 1);
		sec_blrclk_oen(sec, 1);
		aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		aio_set_aud_ch_mute(sec->aio_handle, AIO_ID_SEC_TX, AIO_TSD0, 1);
		/*
		 * Do not set sec_enable_port to 0 for bt_sco application
		 * It is the duplex transfer. Need to keep lrck of i2s3
		 */
		if (!sec->bt_sco)
			sec_enable_port(sec, 0);
		sec_blrclk_oen(sec, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i2s_sec_dai_probe(struct snd_soc_dai *dai)
{
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
	sec->ctrl.istdm = of_property_read_bool(np, "tdm");
	sec->bt_sco = of_property_read_bool(np, "hfp-bt-sco");
	sec->is_master = of_property_read_bool(np, "master");
	//revert bclk on slave mode
	if (!sec->is_master)
		sec->ctrl.invbclk = 1;

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
	snd_printd("%s: done irq %d chid %d tdm %d master %d\n", __func__,
		   sec->irq, sec->chid, sec->ctrl.istdm, sec->is_master);

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
