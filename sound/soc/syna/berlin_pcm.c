// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/of_device.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "berlin_pcm.h"
#include "berlin_capture.h"
#include "berlin_playback.h"

#include "aio_hal.h"
#include "avio_dhub_drv.h"
#include "avio_common.h"

/* Enable the buffer status debugging. As some debug function may be called from isr, in case printk console log
 * level is high, isr would be waiting in console driver which would make the underrun situation more worst.
 * Enable it only when needed.
 */
//#define BUF_STATE_DEBUG

static struct berlin_chip *dev_to_berlin_chip(struct device *dev)
{
	struct snd_card *card = NULL;

	card = dev_get_drvdata(dev);
	if (card == NULL)
		return NULL;
	return (struct berlin_chip *)card->private_data;
}

void berlin_report_xrun(struct berlin_chip *chip,
			enum berlin_xrun_t xrun_type)
{
	atomic_long_inc(chip->xruns + xrun_type);
}

static enum berlin_xrun_t berlin_xrun_string_to_type(const char *string)
{
	if (strcmp("pcm_overrun", string) == 0)
		return PCM_OVERRUN;
	if (strcmp("fifo_overrun", string) == 0)
		return FIFO_OVERRUN;
	if (strcmp("pcm_underrun", string) == 0)
		return PCM_UNDERRUN;
	if (strcmp("fifo_underrun", string) == 0)
		return FIFO_UNDERRUN;
	if (strcmp("irq_disable_us", string) == 0)
		return IRQ_DISABLE;
#ifdef BUF_STATE_DEBUG
	snd_printd("%s: unrecognized xrun type: %s\n", __func__, string);
#endif
	return XRUN_T_MAX;
}

static ssize_t
xrun_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long overruns;

	struct berlin_chip *chip = NULL;
	enum berlin_xrun_t xrun_type =
		berlin_xrun_string_to_type(attr->attr.name);

	if (xrun_type == XRUN_T_MAX)
		return -EINVAL;

	chip = dev_to_berlin_chip(dev);
	if (chip == NULL)
		return -ENODEV;
	overruns = atomic_long_read(chip->xruns + xrun_type);
	return snprintf(buf, PAGE_SIZE, "%lu\n", overruns);
}

static ssize_t
xrun_store(struct device *dev,
	   struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long overruns;
	int err;

	struct berlin_chip *chip = NULL;
	enum berlin_xrun_t xrun_type =
		berlin_xrun_string_to_type(attr->attr.name);

	if (xrun_type == XRUN_T_MAX)
		return -EINVAL;

	chip = dev_to_berlin_chip(dev);
	if (chip == NULL)
		return -ENODEV;

	err = kstrtoul(buf, 10, &overruns);
	if (err < 0)
		return err;

	// TODO(yichunko): Remove the following after underrun issue is
	// resolved
	// in IRQ_DISABLE mode, overruns = # of us to block
	if (xrun_type == IRQ_DISABLE) {
		unsigned long flags;

		local_irq_save(flags);
#ifdef BUF_STATE_DEBUG
		snd_printd("block for %lu us.\n", overruns);
#endif
		udelay(overruns);
		local_irq_restore(flags);
	}
	// TODO(yichunko)
	atomic_long_set(chip->xruns + xrun_type, overruns);
	return count;
}

static DEVICE_ATTR(pcm_overrun, 0644, xrun_show, xrun_store);
static DEVICE_ATTR(fifo_overrun, 0644, xrun_show, xrun_store);
static DEVICE_ATTR(pcm_underrun, 0644, xrun_show, xrun_store);
static DEVICE_ATTR(fifo_underrun, 0644, xrun_show, xrun_store);
static DEVICE_ATTR(irq_disable_us, 0644, xrun_show, xrun_store);

static struct attribute *berlin_xrun_sysfs_entries[] = {
	&dev_attr_pcm_overrun.attr,
	&dev_attr_fifo_overrun.attr,
	&dev_attr_pcm_underrun.attr,
	&dev_attr_fifo_underrun.attr,
	&dev_attr_irq_disable_us.attr,
	NULL,
};

static const struct attribute_group berlin_sysfs_group = {
	.name	= "xrun",
	.attrs	= berlin_xrun_sysfs_entries,
};

static irqreturn_t berlin_alsa_io_isr(int irq, void *dev_id)
{
	struct snd_pcm_substream *substream;
	irq_hw_number_t hw_irq;
	int chan_id;

	substream = dev_id;
	hw_irq = irqd_to_hwirq(irq_get_irq_data(irq));
	chan_id = (int)hw_irq;

	if (substream->runtime) {
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			berlin_capture_isr(substream);
		else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			berlin_playback_isr(substream, chan_id);
	}

	return IRQ_HANDLED;
}

int berlin_pcm_request_dma_irq(struct snd_pcm_substream *substream,
			       struct berlin_ss_params *params)
{
	u32 ch[MAX_CHID], i, irq_num, chid_num;
	int err;
	void *dev_id = substream;

	if (!params) {
		snd_printk("%s: null params input\n", __func__);
		return -EINVAL;
	}

	irq_num = params->irq_num;
	chid_num = params->chid_num;
	if (irq_num == 0 || chid_num == 0 || irq_num > chid_num) {
		snd_printk("%s: invalid num irq %d chid %d\n",
			   __func__, irq_num, chid_num);
		return 0;
	}

	for (i = 0; i < irq_num; i++) {
		err = request_irq(params->irq[i], berlin_alsa_io_isr, 0,
				  params->dev_name, dev_id);
		if (unlikely(err < 0)) {
			snd_printk("irq %d request error: %d\n",
				   params->irq[i], err);
			return err;
		}
	}

	for (i = 0; i < chid_num; i++)
		ch[i] = irqd_to_hwirq(irq_get_irq_data(params->irq[i]));

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		berlin_capture_set_ch_mode(substream, chid_num, ch,
					   params->mode,
					   params->enable_mic_mute,
					   params->interleaved,
					   params->dummy_data,
					   params->channel_map,
					   params->ch_shift_check);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		berlin_playback_set_ch_mode(substream, chid_num, ch,
					    params->mode);

	return err;
}
EXPORT_SYMBOL(berlin_pcm_request_dma_irq);

void berlin_pcm_free_dma_irq(struct snd_pcm_substream *substream,
			     u32 irq_num,
			     unsigned int *irq)
{
	u32 i = 0;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		berlin_capture_set_ch_mode(substream, 0, 0, 0,
					false, false, false, 0, 0);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		berlin_playback_set_ch_mode(substream, 0, 0, 0);

	for (i = 0; i < irq_num; i++)
		free_irq(irq[i], (void *)substream);
}
EXPORT_SYMBOL(berlin_pcm_free_dma_irq);

void berlin_pcm_max_ch_inuse(struct snd_pcm_substream *substream,
			     u32 ch_num)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		berlin_capture_set_ch_inuse(substream, ch_num);
}
EXPORT_SYMBOL(berlin_pcm_max_ch_inuse);

bool berline_pcm_passthrough_check(struct snd_pcm_substream *substream,
			       u32 *data_type)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_passthrough_check(substream, data_type);

	return false;
}
EXPORT_SYMBOL(berline_pcm_passthrough_check);

void berlin_pcm_hdmi_bitstream_start(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		berlin_playback_hdmi_bitstream_start(substream);
}
EXPORT_SYMBOL(berlin_pcm_hdmi_bitstream_start);

static int berlin_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *c = snd_soc_rtdcom_lookup(rtd, "syna-berlin-pcm");
	struct device *dev = c->dev;
	struct berlin_chip *chip = dev_get_drvdata(dev);

	snd_printd("%s stream: %s (%p)\n", __func__,
		substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" : "playback", substream);
	/* get dhub handle */
	if (mutex_lock_interruptible(&chip->dhub_lock) != 0)
		return  -EINTR;
	if (chip->dhub == NULL) {
		chip->dhub = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB);
		if (unlikely(chip->dhub == NULL)) {
			snd_printk("chip->dhub: get failed\n");
			mutex_unlock(&chip->dhub_lock);
			return -EBUSY;
		}
	}
	mutex_unlock(&chip->dhub_lock);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_open(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_open(substream, chip->passthrough_enable);

	return 0;
}

static int berlin_pcm_close(struct snd_pcm_substream *substream)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_close(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_close(substream);

	return 0;
}

static int berlin_pcm_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_hw_params(substream, params);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_hw_params(substream, params);

	return 0;
}

static int berlin_pcm_hw_free(struct snd_pcm_substream *substream)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_hw_free(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_hw_free(substream);

	return 0;
}

static int berlin_pcm_prepare(struct snd_pcm_substream *substream)
{
	snd_printd("%s stream: %s (%p)\n", __func__,
		substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" : "playback", substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_prepare(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_prepare(substream);

	return 0;
}

static int berlin_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	snd_printd("%s stream: %s %s (%p)\n", __func__,
		substream->stream == SNDRV_PCM_STREAM_CAPTURE ? "capture" : "playback",
		cmd == SNDRV_PCM_TRIGGER_START ? "start" : "stop",
		substream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_trigger(substream, cmd);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_trigger(substream, cmd);

	return 0;
}

static snd_pcm_uframes_t
berlin_pcm_pointer(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return berlin_capture_pointer(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_pointer(substream);

	return 0;
}

static int berlin_pcm_ack(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return berlin_playback_ack(substream);

	return 0;
}

static const struct snd_pcm_ops berlin_pcm_ops = {
	.open		= berlin_pcm_open,
	.close		= berlin_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= berlin_pcm_hw_params,
	.hw_free	= berlin_pcm_hw_free,
	.prepare	= berlin_pcm_prepare,
	.trigger	= berlin_pcm_trigger,
	.pointer	= berlin_pcm_pointer,
	.ack		= berlin_pcm_ack,
};

#define PREALLOC_BUFFER (2 * 1024 * 1024)
#define PREALLOC_BUFFER_MAX (2 * 1024 * 1024)
/*
 * ALSA API channel-map control callbacks
 */
static int berlin_pcm_chmap_ctl_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 8;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = SNDRV_CHMAP_LAST;
	return 0;
}

static int berlin_pcm_chmap_ctl_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_pcm_chmap *info = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = info->private_data;
	u32 tx_ch[8], tx_num;
	int i;

	if (dai->driver->ops->get_channel_map) {
		dai->driver->ops->get_channel_map(dai, &tx_num, tx_ch, NULL, NULL);
		for (i = 0; i < tx_num; i++)
			ucontrol->value.integer.value[i] = tx_ch[i];
	}

	return 0;
}

static int berlin_pcm_chmap_ctl_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_pcm_chmap *info = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = info->private_data;
	u32 tx_ch[8], tx_num;
	int i;

	if (dai->driver->ops->set_channel_map) {
		tx_num = 8;
		for (i = 0; i < tx_num; i++)
			tx_ch[i] = ucontrol->value.integer.value[i];
		dai->driver->ops->set_channel_map(dai, tx_num, tx_ch, 0, NULL);
	}

	return 0;
}

static int berlin_pcm_chmap_ctl_tlv(struct snd_kcontrol *kcontrol, int op_flag,
			      unsigned int size, unsigned int __user *tlv)
{
	snd_printd("%s enter", __func__);

	return 0;
}

static int berlin_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_pcm_chmap *chmap;
	struct snd_kcontrol *kctl;
	int err, i;

	/* create channel mapping for playback streams only now */
	if (rtd->pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream_count) {
		err = snd_pcm_add_chmap_ctls(rtd->pcm,
					     SNDRV_PCM_STREAM_PLAYBACK,
					     NULL, 0, 0, &chmap);
		if (err < 0)
			return err;
		/* override handlers */
		kctl = chmap->kctl;
		chmap->private_data = rtd->cpu_dai;
		for (i = 0; i < kctl->count; i++)
			kctl->vd[i].access |= SNDRV_CTL_ELEM_ACCESS_WRITE;
		kctl->info = berlin_pcm_chmap_ctl_info;
		kctl->get = berlin_pcm_chmap_ctl_get;
		kctl->put = berlin_pcm_chmap_ctl_put;
		kctl->tlv.c = berlin_pcm_chmap_ctl_tlv;
	}

	snd_pcm_lib_preallocate_pages_for_all(rtd->pcm,
			SNDRV_DMA_TYPE_CONTINUOUS,
			snd_dma_continuous_data
			(GFP_KERNEL),
			PREALLOC_BUFFER,
			PREALLOC_BUFFER_MAX);
	return 0;
}

static void berlin_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static int berlin_pcm_hdmi_spdif_out_switch_get(
					struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *value)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct device *dev = component->dev;
	struct berlin_chip *chip = dev_get_drvdata(dev);

	value->value.integer.value[0] = chip->passthrough_enable;

	return 0;
}

static int berlin_pcm_hdmi_spdif_out_switch_put(
				     struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *value)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct device *dev = component->dev;
	struct berlin_chip *chip = dev_get_drvdata(dev);
	int changed;

	changed = value->value.integer.value[0] != chip->passthrough_enable;
	if (changed) {
		chip->passthrough_enable = value->value.integer.value[0];
		snd_printd("berlin audio passthrough: %s\n",
			value->value.integer.value[0] ? "enable" : "disable");
	}

	return changed;
}

static struct snd_kcontrol_new berlin_pcm_controls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = SNDRV_CTL_NAME_IEC958("", PLAYBACK, SWITCH),
		.info = snd_ctl_boolean_mono_info,
		.get = berlin_pcm_hdmi_spdif_out_switch_get,
		.put = berlin_pcm_hdmi_spdif_out_switch_put,
	}
};

static struct snd_soc_component_driver berlin_pcm_component = {
	.name		= "syna-berlin-pcm",
	.ops		= &berlin_pcm_ops,
	.pcm_new	= berlin_pcm_new,
	.pcm_free	= berlin_pcm_free,
	.controls	= berlin_pcm_controls,
	.num_controls	= ARRAY_SIZE(berlin_pcm_controls),
};

static int berlin_pcm_probe(struct platform_device *pdev)
{
	struct berlin_chip *chip;
	struct device *dev = &pdev->dev;
	int ret;

	snd_printd("berlin-pcm probe %p\n", pdev);

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	chip = devm_kzalloc(dev, sizeof(struct berlin_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->pdev = pdev;

	mutex_init(&chip->dhub_lock);
	of_dma_configure(dev, dev->of_node, true);

	dev_set_drvdata(&pdev->dev, chip);

	ret = devm_snd_soc_register_component(&pdev->dev,
					&berlin_pcm_component,
					NULL, 0);
	if (ret < 0)
		snd_printk("can not do snd soc register\n");

	return ret;
}

static int berlin_pcm_dev_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id berlin_pcm_of_match[] = {
	{.compatible = "syna,berlin-pcm"},
	{.compatible = "syna,as370-pcm"},
	{.compatible = "syna,vs680-pcm"},
	{.compatible = "syna,as470-pcm"},
	{.compatible = "syna,vs640-pcm"},
	{}
};
MODULE_DEVICE_TABLE(of, berlin_pcm_of_match);

static struct platform_driver berlin_pcm_driver = {
	.driver			= {
		.name		= "syna-berlin-pcm",
		.of_match_table = berlin_pcm_of_match,
	},

	.probe			= berlin_pcm_probe,
	.remove			= berlin_pcm_dev_remove,
};
module_platform_driver(berlin_pcm_driver);

MODULE_AUTHOR("Synaptics");
MODULE_DESCRIPTION("Berlin PCM Driver");
MODULE_LICENSE("GPL v2");
