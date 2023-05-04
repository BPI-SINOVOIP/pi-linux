// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/irq.h>

#include <sound/core.h>
#include <sound/info.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/pcm-indirect.h>
#include <sound/soc.h>

#include "berlin_pcm.h"
#include "berlin_spdif.h"
#include "aio_hal.h"
#include "avio_dhub_drv.h"


#define DMA_BUFFER_SIZE        (4 * 1024)
#define DMA_BUFFER_MIN         (DMA_BUFFER_SIZE >> 2)
#define MAX_BUFFER_SIZE        (DMA_BUFFER_SIZE << 1)

#define ZERO_DMA_BUFFER_SIZE   (32)

struct berlin_playback {
	/*
	 * Driver-specific debug proc entry
	 */
	struct snd_info_entry *entry;

	/*
	 * Tracks the base address of the last submitted DMA block.
	 * Moved to next period in ISR.
	 * read in berlin_playback_pointer.
	 *
	 * Total size of ALSA buffer in the format received from userspace.
	 * Note that 16 bit input is padded out to 32.
	 */
	unsigned int dma_offset;

	/*
	 * Is there a submitted DMA request?
	 * Set when a DMA request is submitted to DHUB.
	 * Cleared on 'stop' or ISR.
	 */
	bool ma_dma_pending;
	bool spdif_dma_pending;
	/*
	 * Indicates if page memory is allocated
	 */
	bool pages_allocated;

	/*
	 * Instance lock between ISR and higher contexts.
	 */
	spinlock_t lock;

	/* spdif DMA buffer */
	unsigned char *spdif_dma_area;          /* dma buf for spdifo */
	dma_addr_t spdif_dma_addr;              /* phy addr of spdif buf */
	unsigned int spdif_buf_size;           /* size of dma area */
	unsigned int spdif_ratio;

	/* PCM DMA buffer */
	unsigned char *pcm_dma_area;
	dma_addr_t pcm_dma_addr;
	unsigned int pcm_buf_size;
	unsigned int pcm_ratio;

	/* hw parameter */
	unsigned int sample_rate;
	unsigned int sample_format;
	unsigned int channel_num;
	ssize_t buf_size;
	ssize_t period_size;

	/* for spdif encoding */
	unsigned int spdif_frames;
	unsigned char channel_status[24];

	/* playback status */
	unsigned int output_mode;
	unsigned int intr_updates;      // tracing the src interrupt happened

	struct snd_pcm_substream *ss;
	struct snd_pcm_indirect pcm_indirect;
	unsigned int spdif_ch;
	unsigned int i2s_ch;
	struct berlin_chip *chip;
};

//Sample rate fixed to 48k now, avpll setting
static unsigned int berlin_playback_rates[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
	64000, 88200, 96000
};

static struct snd_pcm_hw_constraint_list berlin_constraints_rates = {
	.count	= ARRAY_SIZE(berlin_playback_rates),
	.list	= berlin_playback_rates,
	.mask	= 0,
};

static void *zero_dma_buf;
static dma_addr_t zero_dma_addr;

static void spdif_enc_subframe(uint32_t *subframe,
			       const uint32_t data, uint32_t sync_type,
			       uint8_t v, uint8_t u, uint8_t c)
{
	struct spdif_frame *spdif = (struct spdif_frame *)subframe;
	int16_t high_16 = (data >> 16), low_16 = ((data >> 8) & 0xff);
	int32_t parity = ((v + u + c) & 1);

	spdif->sync = sync_type;
	spdif->validflag = v;
	spdif->user = u;
	spdif->channelstatus = c;
	spdif->paritybit = parity;
	spdif->audiosample2 = high_16;
	spdif->audiosample1 = low_16;
}

/*
 * Kicks off a DMA transfer to audio IO interface for the |berlin_pcm|.
 * Must be called with instance spinlock held.
 * Must be called only when instance is in playing state.
 */
static void start_dma_if_needed(struct berlin_playback *bp)
{
	dma_addr_t dma_source_address;
	int dma_size;

	assert_spin_locked(&bp->lock);
	if (bp->pcm_indirect.hw_ready < bp->period_size) {
		snd_printk("%s: underrun! hw_ready: %d\n", __func__,
			   bp->pcm_indirect.hw_ready);
		return;
	}

	if (bp->ma_dma_pending && bp->spdif_dma_pending)
		return;

	if ((bp->output_mode & I2SO_MODE)
	    && !bp->ma_dma_pending) {
		dma_source_address =
			bp->pcm_dma_addr + bp->dma_offset * bp->pcm_ratio;
		dma_size = bp->period_size * bp->pcm_ratio;
		bp->ma_dma_pending = true;

		dhub_channel_write_cmd(bp->chip->dhub,
				       bp->i2s_ch,
				       dma_source_address, dma_size,
				       0, 0, 0, 1, 0, 0);
	}
	if ((bp->output_mode & SPDIFO_MODE)
	    && !bp->spdif_dma_pending) {
		dma_source_address =
			bp->spdif_dma_addr + bp->dma_offset * bp->spdif_ratio;
		dma_size = bp->period_size * bp->spdif_ratio;
		bp->spdif_dma_pending = true;
		dhub_channel_write_cmd(bp->chip->dhub,
				       bp->spdif_ch,
				       dma_source_address, dma_size,
				       0, 0, 0, 1, 0, 0);
	}
}

static void berlin_playback_trigger_start(struct snd_pcm_substream *ss)
{
	snd_printd("%s: finished.\n", __func__);
}

static void berlin_playback_trigger_stop(struct snd_pcm_substream *ss)
{
	snd_printd("%s: finished.\n", __func__);
}

static const struct snd_pcm_hardware berlin_playback_hw = {
	.info			= (SNDRV_PCM_INFO_MMAP
				   | SNDRV_PCM_INFO_INTERLEAVED
				   | SNDRV_PCM_INFO_MMAP_VALID
				   | SNDRV_PCM_INFO_PAUSE
				   | SNDRV_PCM_INFO_RESUME),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE
				   | SNDRV_PCM_FMTBIT_S24_LE
				   | SNDRV_PCM_FMTBIT_S24_3LE
				   | SNDRV_PCM_FMTBIT_S32_LE),
	.rates			= (SNDRV_PCM_RATE_8000_96000
				   | SNDRV_PCM_RATE_KNOT),
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= MAX_BUFFER_SIZE,
	.period_bytes_min	= DMA_BUFFER_MIN,
	.period_bytes_max	= DMA_BUFFER_SIZE,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size		= 0
};

static const struct snd_pcm_hardware berlin_playback_multi_codecs = {
	.info			= (SNDRV_PCM_INFO_MMAP
				   | SNDRV_PCM_INFO_INTERLEAVED
				   | SNDRV_PCM_INFO_MMAP_VALID
				   | SNDRV_PCM_INFO_PAUSE
				   | SNDRV_PCM_INFO_RESUME),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE
					| SNDRV_PCM_FMTBIT_S24_LE
					| SNDRV_PCM_FMTBIT_S24_3LE
					| SNDRV_PCM_FMTBIT_S32_LE),
	.rates			= SNDRV_PCM_RATE_48000,
	.channels_min		= 4,
	.channels_max		= 4,
	.buffer_bytes_max	= MAX_BUFFER_SIZE * 2,
	.period_bytes_min	= DMA_BUFFER_MIN * 2,
	.period_bytes_max	= DMA_BUFFER_SIZE * 2,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size		= 0
};

static void berlin_runtime_free(struct snd_pcm_runtime *runtime)
{
	struct berlin_playback *bp = runtime->private_data;

	if (bp) {
		if (bp->spdif_dma_area) {
			dma_free_coherent(NULL, bp->spdif_buf_size,
					  bp->spdif_dma_area,
					  bp->spdif_dma_addr);
			bp->spdif_dma_area = NULL;
			bp->spdif_dma_addr = 0;
		}

		if (bp->pcm_dma_area) {
			dma_free_coherent(NULL, bp->pcm_buf_size,
					  bp->pcm_dma_area,
					  bp->pcm_dma_addr);
			bp->pcm_dma_area = NULL;
			bp->pcm_dma_addr = 0;
		}

		if (bp->entry)
			snd_info_free_entry(bp->entry);

		kfree(bp);
		runtime->private_data = NULL;
	}
}

#ifdef CONFIG_SND_VERBOSE_PROCFS
static void debug_entry(struct snd_info_entry *entry,
			struct snd_info_buffer *buffer)
{
	struct berlin_playback *bp =
		(struct berlin_playback *)entry->private_data;
	unsigned long flags;

	spin_lock_irqsave(&bp->lock, flags);
	snd_iprintf(buffer, "dma_pending:\t\tma-%d spdif-%d\n",
		    bp->ma_dma_pending, bp->spdif_dma_pending);
	snd_iprintf(buffer, "output_mode:\t\t%d\n", bp->output_mode);
	snd_iprintf(buffer, "current_dma_offset:\t%u\n",
		    bp->dma_offset);
	snd_iprintf(buffer, "\n");
	snd_iprintf(buffer, "indirect.hw_data:\t%u\n",
		    bp->pcm_indirect.hw_data);
	snd_iprintf(buffer, "indirect.hw_io:\t\t%u\n",
		    bp->pcm_indirect.hw_io);
	snd_iprintf(buffer, "indirect.hw_ready:\t%d\n",
		    bp->pcm_indirect.hw_ready);
	snd_iprintf(buffer, "indirect.sw_data:\t%u\n",
		    bp->pcm_indirect.hw_data);
	snd_iprintf(buffer, "indirect.sw_io:\t\t%u\n",
		    bp->pcm_indirect.hw_io);
	snd_iprintf(buffer, "indirect.sw_ready:\t%d\n",
		    bp->pcm_indirect.sw_ready);
	snd_iprintf(buffer, "indirect.appl_ptr:\t%lu\n",
		    bp->pcm_indirect.appl_ptr);
	snd_iprintf(buffer, "\n");
	snd_iprintf(buffer, "substream->runtime->control->appl_ptr:\t%lu\n",
		    bp->ss->runtime->control->appl_ptr);
	snd_iprintf(buffer, "substream->runtime->status->hw_ptr:\t%lu\n",
		    bp->ss->runtime->status->hw_ptr);
	spin_unlock_irqrestore(&bp->lock, flags);
}
#endif

int berlin_playback_open(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct berlin_playback *bp;
	int err;

	snd_printd("%s: start.\n", __func__);

	err = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE,
			&berlin_constraints_rates);
	if (err < 0)
		return err;

	bp = kzalloc(sizeof(*bp), GFP_KERNEL);
	if (bp == NULL)
		return -ENOMEM;

	runtime->private_data = bp;
	runtime->private_free = berlin_runtime_free;
	if (rtd->num_codecs == 2)
		runtime->hw = berlin_playback_multi_codecs;
	else
		runtime->hw = berlin_playback_hw;

	/* buffer and period size are multiple of minimum depth size */
	err = snd_pcm_hw_constraint_step(ss->runtime, 0,
					 SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
					 runtime->hw.period_bytes_min);
	if (err)
		return -EINVAL;

	err = snd_pcm_hw_constraint_step(ss->runtime, 0,
					 SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
					 runtime->hw.period_bytes_min);
	if (err)
		return -EINVAL;

/* TODO: remove this dependency after split spdif/i2s play */
#ifdef CONFIG_SND_VERBOSE_PROCFS
	bp->entry = snd_info_create_card_entry(ss->pcm->card,
					       "debug",
					       ss->proc_root);
	if (!bp->entry)
		snd_printd("%s: couldn't create debug entry\n", __func__);
	else {
		snd_info_set_text_ops(bp->entry, bp, debug_entry);
		snd_info_register(bp->entry);
	}
#endif

	bp->ma_dma_pending = false;
	bp->spdif_dma_pending = false;
	bp->pages_allocated = false;
	bp->intr_updates = 0;
	bp->ss = ss;
	bp->i2s_ch = -1;
	bp->spdif_ch = -1;

	spin_lock_init(&bp->lock);

	snd_printd("%s: finished.\n", __func__);
	return 0;
}

int berlin_playback_close(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;

	if (bp) {
		if (bp->entry) {
			snd_info_free_entry(bp->entry);
			bp->entry = NULL;
		}
		kfree(bp);
		runtime->private_data = NULL;
	}

	snd_printd("%s: finished.\n", __func__);
	return 0;
}

int berlin_playback_hw_free(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	struct berlin_chip *chip = bp->chip;

	if (chip) {
		struct platform_device *pdev = chip->pdev;

		if (bp->spdif_dma_area) {
			dma_free_coherent(&pdev->dev,
					  bp->spdif_buf_size,
					  bp->spdif_dma_area,
					  bp->spdif_dma_addr);
			bp->spdif_dma_area = NULL;
			bp->spdif_dma_addr = 0;
		}

		if (bp->pcm_dma_area) {
			dma_free_coherent(&pdev->dev,
					  bp->pcm_buf_size,
					  bp->pcm_dma_area,
					  bp->pcm_dma_addr);
			bp->pcm_dma_area = NULL;
			bp->pcm_dma_addr = 0;
		}
	}

	if (bp->pages_allocated == true) {
		snd_pcm_lib_free_pages(ss);
		bp->pages_allocated = false;
	}

	return 0;
}

int berlin_playback_hw_params(struct snd_pcm_substream *ss,
			      struct snd_pcm_hw_params *p)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, "syna-berlin-pcm");
	struct device *dev = component->dev;
	struct berlin_chip *chip = dev_get_drvdata(dev);
	struct platform_device *pdev = chip->pdev;
	struct spdif_cs *chnsts;
	int err;

	snd_printk("%s: fs:%d ch:%d width:%d format:%s, period bytes:%d\n",
		__func__,
		params_rate(p), params_channels(p),
		params_width(p),
		snd_pcm_format_name(params_format(p)),
		params_period_bytes(p));

	bp->chip = chip;
	berlin_playback_hw_free(ss);

	err = snd_pcm_lib_malloc_pages(ss, params_buffer_bytes(p));
	if (err < 0) {
		snd_printk("failed to allocated pages for buffers\n");
		return err;
	}
	bp->pages_allocated = true;

	snd_printd("%s: sample_rate:%d channels:%d format:%s\n", __func__,
		   params_rate(p), params_channels(p),
		   snd_pcm_format_name(params_format(p)));

	bp->sample_rate = params_rate(p);
	bp->sample_format = params_format(p);
	bp->channel_num = params_channels(p);
	bp->period_size = params_period_bytes(p);
	bp->buf_size = params_buffer_bytes(p);
	bp->pcm_ratio = 1;

	if (bp->sample_format == SNDRV_PCM_FORMAT_S16_LE)
		bp->pcm_ratio *= 2;

	bp->pcm_buf_size = bp->buf_size * bp->pcm_ratio;

	bp->pcm_dma_area =
		dma_alloc_coherent(&pdev->dev, bp->pcm_buf_size,
				&bp->pcm_dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!bp->pcm_dma_area) {
		snd_printk("%s: failed to allocate PCM DMA area\n", __func__);
		goto err_pcm_dma;
	}
#ifdef SPDIF_CLOCK_PATTERN
	bp->spdif_ratio = bp->pcm_ratio * 2;
#else
	bp->spdif_ratio = bp->pcm_ratio;
#endif

	bp->spdif_buf_size = bp->buf_size * bp->spdif_ratio;

	bp->spdif_dma_area =
		dma_alloc_coherent(&pdev->dev, bp->spdif_buf_size,
				&bp->spdif_dma_addr, GFP_KERNEL | __GFP_ZERO);

	if (!bp->spdif_dma_area) {
		snd_printk("%s: failed to allocate SPDIF DMA area\n", __func__);
		goto err_spdif_dma;
	}

	/* initialize spdif channel status */
	chnsts = (struct spdif_cs *)&(bp->channel_status[0]);
	spdif_init_channel_status(chnsts, bp->sample_rate);

	if (zero_dma_buf == NULL) {
		zero_dma_buf = devm_kzalloc(&pdev->dev, ZERO_DMA_BUFFER_SIZE,
					    GFP_KERNEL);
		if (!zero_dma_buf)
			goto err_zero_dma;

		zero_dma_addr = dma_map_single(&pdev->dev, zero_dma_buf,
					       ZERO_DMA_BUFFER_SIZE, DMA_TO_DEVICE);
	}

	return 0;

err_zero_dma:
	if (bp->spdif_dma_area)
		dma_free_coherent(&pdev->dev, bp->spdif_buf_size,
				bp->spdif_dma_area, bp->spdif_dma_addr);
	bp->spdif_dma_area = NULL;
	bp->spdif_dma_addr = 0;

err_spdif_dma:
	if (bp->pcm_dma_area)
		dma_free_coherent(&pdev->dev, bp->pcm_buf_size,
				  bp->pcm_dma_area, bp->pcm_dma_addr);
	bp->pcm_dma_area = NULL;
	bp->pcm_dma_addr = 0;
err_pcm_dma:
	snd_pcm_lib_free_pages(ss);
	return -ENOMEM;
}

int berlin_playback_prepare(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	unsigned long flags;

	spin_lock_irqsave(&bp->lock, flags);
	bp->dma_offset = 0;
	memset(&bp->pcm_indirect, 0, sizeof(bp->pcm_indirect));
	bp->pcm_indirect.hw_buffer_size = bp->buf_size;
	bp->pcm_indirect.sw_buffer_size = snd_pcm_lib_buffer_bytes(ss);
	spin_unlock_irqrestore(&bp->lock, flags);

	snd_printd("%s finished. buffer: %zd period: %zd hw %u sw %u\n",
		   __func__,
		   snd_pcm_lib_buffer_bytes(ss),
		   snd_pcm_lib_period_bytes(ss),
		   bp->pcm_indirect.hw_buffer_size,
		   bp->pcm_indirect.sw_buffer_size);
	return 0;
}

int berlin_playback_trigger(struct snd_pcm_substream *ss, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		berlin_playback_trigger_start(ss);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		berlin_playback_trigger_stop(ss);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

snd_pcm_uframes_t
berlin_playback_pointer(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	uint32_t buf_pos;
	unsigned long flags;

	spin_lock_irqsave(&bp->lock, flags);
	buf_pos = bp->dma_offset;
	spin_unlock_irqrestore(&bp->lock, flags);

	return snd_pcm_indirect_playback_pointer(ss,
						 &bp->pcm_indirect, buf_pos);
}

// Encodes |frames| number of stereo S32LE frames from |pcm_in|
// to |spdif_out| SPDIF frames (64-bits per frame)
static void spdif_encode(struct berlin_playback *bp,
			 const int32_t *pcm_in, int32_t *spdif_out,
			 int frames)
{
	int i;

	for (i = 0; i < frames; ++i) {
		unsigned char channel_status =
			spdif_get_channel_status(bp->channel_status,
						 bp->spdif_frames);
		uint32_t sync_word = bp->spdif_frames ? TYPE_M : TYPE_B;
#ifdef SPDIF_CLOCK_PATTERN
		spdif_enc_subframe(&spdif_out[i * 4],
				   pcm_in[i * 2], sync_word, 0, 0,
				   channel_status);

		sync_word = TYPE_W;
		spdif_enc_subframe(&spdif_out[(i * 4) + 2],
				   pcm_in[(i * 2) + 1], sync_word, 0, 0,
				   channel_status);
#else
		spdif_enc_subframe(&spdif_out[i * 2],
				   pcm_in[i * 2], sync_word, 0, 0,
				   channel_status);

		sync_word = TYPE_W;
		spdif_enc_subframe(&spdif_out[(i * 2) + 1],
				   pcm_in[(i * 2) + 1], sync_word, 0, 0,
				   channel_status);

#endif
		++bp->spdif_frames;
		bp->spdif_frames %= SPDIF_BLOCK_SIZE;
	}
}

static int berlin_playback_copy(struct snd_pcm_substream *ss,
				int channel, snd_pcm_uframes_t pos,
				void *buf, size_t bytes)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	int32_t *pcm_buf = (int32_t *)(bp->pcm_dma_area +
				      pos * bp->pcm_ratio);
	int32_t *spdif_buf = (int32_t *)(bp->spdif_dma_area +
					pos * bp->spdif_ratio);
	const int frames = bytes /
			   (bp->channel_num *
			    snd_pcm_format_width(bp->sample_format) / 8);
	const int channels = bp->channel_num;
	int i, j;

	if (pos >= bp->buf_size)
		return -EINVAL;

	if (bp->pcm_indirect.hw_ready >= bp->period_size) {
		const unsigned long dma_b = bp->dma_offset;
		const unsigned long dma_e = dma_b + bp->period_size - 1;
		const unsigned long write_b = pos;
		const unsigned long write_e = write_b + bytes - 1;

		// Write begin position shouldn't be in DMA area.
		if ((dma_b <= write_b) && (write_b <= dma_e)) {
			snd_printk("%s: db:%lu <= wb:%lu <= de:%lu\n",
				   __func__, dma_b, write_b, dma_e);
		}
		// Write end position shouldn't be in DMA area.
		if ((dma_b <= write_e) && (write_e <= dma_e)) {
			snd_printk("%s: db:%lu <= we:%lu <= de:%lu\n",
				   __func__, dma_b, write_e, dma_e);
		}
		// Write shouldn't overlap DMA area.
		if ((write_b <= dma_b) && (write_e >= dma_e)) {
			snd_printk("%s: wb:%lu <= db:%lu && we:%lu >= de:%lu\n",
				   __func__, write_b, dma_b,
				   write_e, dma_e);
		}
	}

	if (bp->sample_format == SNDRV_PCM_FORMAT_S16_LE) {
		const int16_t *s16_pcm_source = (int16_t *)buf;

		for (i = 0; i < frames; i++) {
			for (j = 0; j < channels; j++)
				pcm_buf[i * channels + j] =
					s16_pcm_source[i * channels + j] << 16;
		}
	} else if (bp->sample_format == SNDRV_PCM_FORMAT_S32_LE) {
		const int32_t *s32_pcm_source = (int32_t *)buf;

		for (i = 0; i < frames; i++) {
			for (j = 0; j < channels; j++)
				pcm_buf[i * channels + j] =
					s32_pcm_source[i * channels + j];
		}
	} else if (bp->sample_format == SNDRV_PCM_FORMAT_S24_3LE) {
		const char *pcm_source = (char *)buf;
		int ch_bytes =
			snd_pcm_format_physical_width(bp->sample_format) / 8;
		int ch_offset;

		for (i = 0; i < frames; i++) {
			for (j = 0; j < channels; j++) {
				ch_offset = (i * channels + j) * ch_bytes;
				pcm_buf[i * channels + j] =
					(pcm_source[ch_offset] << 8) |
					(pcm_source[ch_offset + 1] << 16) |
					(pcm_source[ch_offset + 2] << 24);
			}
		}
	} else if (bp->sample_format == SNDRV_PCM_FORMAT_S24_LE) {
		const int32_t *s32_pcm_source = (int32_t *)buf;

		for (i = 0; i < frames; i++) {
			for (j = 0; j < channels; j++)
				pcm_buf[i * channels + j] =
					s32_pcm_source[i * channels + j] << 8;
		}
	} else {
		snd_printk("Unsupported format:%s\n",
		snd_pcm_format_name(bp->sample_format));
		return -EINVAL;
	}

	if (bp->output_mode & SPDIFO_MODE)
		spdif_encode(bp, pcm_buf, spdif_buf, frames);

	return 0;
}

static void berlin_playback_transfer(struct snd_pcm_substream *ss,
				     struct snd_pcm_indirect *rec,
				     size_t bytes)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	void *src = (void *)(runtime->dma_area + rec->sw_data);

	if (!src)
		return;

	berlin_playback_copy(ss, bp->channel_num,
			     rec->hw_data, src, bytes);
}

static inline void
berlin_direct_playback_transfer(struct snd_pcm_substream *substream,
				struct snd_pcm_indirect *rec,
				snd_pcm_indirect_copy_t copy)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_pcm_uframes_t appl_ptr = runtime->control->appl_ptr;
	snd_pcm_sframes_t diff = appl_ptr - rec->appl_ptr;
	int qsize;

	if (diff) {
		if (diff < -(snd_pcm_sframes_t) (runtime->boundary / 2))
			diff += runtime->boundary;
		if (diff < 0)
			return;
		rec->sw_ready += (int)frames_to_bytes(runtime, diff);
		rec->appl_ptr = appl_ptr;
	}

	if (runtime->stop_threshold == runtime->boundary)
		rec->sw_ready = snd_pcm_lib_period_bytes(substream);

	qsize = rec->hw_queue_size ? rec->hw_queue_size : rec->hw_buffer_size;
	while (rec->hw_ready < qsize) {
		unsigned int hw_to_end = rec->hw_buffer_size - rec->hw_data;
		unsigned int sw_to_end = rec->sw_buffer_size - rec->sw_data;
		unsigned int bytes = qsize - rec->hw_ready;

		if (hw_to_end < bytes)
			bytes = hw_to_end;
		if (sw_to_end < bytes)
			bytes = sw_to_end;
		if (!bytes)
			break;
		copy(substream, rec, bytes);
		rec->hw_data += bytes;
		if (rec->hw_data == rec->hw_buffer_size)
			rec->hw_data = 0;
		rec->sw_data += bytes;
		if (rec->sw_data == rec->sw_buffer_size)
			rec->sw_data = 0;
		rec->hw_ready += bytes;
	}
}

void berlin_playback_set_ch_mode(struct snd_pcm_substream *ss,
				 u32 ch_num, u32 *ch, u32 mode)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;

	if (ch_num > 1) {
		snd_printk("only support 1 dhub channel play");
		return;
	}

	if (bp) {
		bp->output_mode |= mode;
		if (mode == I2SO_MODE)
			bp->i2s_ch = ch ? ch[0] : 0;
		else if (mode == SPDIFO_MODE)
			bp->spdif_ch = ch ? ch[0] : 0;
		else if (mode == 0) {
			bp->output_mode = 0;
			bp->i2s_ch = 0;
			bp->spdif_ch = 0;
		} else
			snd_printk("invalid mode setting\n");
	}
}

int berlin_playback_ack(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	struct snd_pcm_indirect *pcm_indirect = &bp->pcm_indirect;
	unsigned long flags;

	pcm_indirect->hw_queue_size = bp->buf_size;
	if (runtime->tstamp_mode == SNDRV_PCM_TSTAMP_ENABLE)
		berlin_direct_playback_transfer(ss, pcm_indirect,
					   berlin_playback_transfer);
	else
		snd_pcm_indirect_playback_transfer(ss, pcm_indirect,
					   berlin_playback_transfer);
	spin_lock_irqsave(&bp->lock, flags);
	start_dma_if_needed(bp);
	spin_unlock_irqrestore(&bp->lock, flags);
	return 0;
}

int berlin_playback_isr(struct snd_pcm_substream *ss,
			unsigned int chanId)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;

	bool all_update = false;

	spin_lock(&bp->lock);

	/* If we were not pending, avoid pointer manipulation */
	if (!bp->ma_dma_pending && !bp->spdif_dma_pending) {
		spin_unlock(&bp->lock);
		snd_printd("stream %p no dma pending\n", ss);
		return 0;
	}

	if (chanId == bp->i2s_ch) {
		if (bp->intr_updates & I2SO_MODE)
			snd_printd("stream %p dual i2s intrupt\n", ss);
		bp->ma_dma_pending = false;
	} else if (chanId == bp->spdif_ch) {
		if (bp->intr_updates & SPDIFO_MODE)
			snd_printd("stream %p dual spdif interrupt\n", ss);
		bp->spdif_dma_pending = false;
	}
	/* Roll the DMA pointer, and chain if needed */
	if (chanId == bp->i2s_ch)
		bp->intr_updates |= I2SO_MODE;

	if (chanId == bp->spdif_ch)
		bp->intr_updates |= SPDIFO_MODE;

	if ((bp->intr_updates & bp->output_mode) == bp->output_mode) {
		bp->dma_offset += bp->period_size;
		bp->dma_offset %= bp->buf_size;
		all_update = true;
		bp->intr_updates = 0;
	}
	spin_unlock(&bp->lock);

	if (all_update)
		snd_pcm_period_elapsed(ss);

	spin_lock(&bp->lock);
	start_dma_if_needed(bp);
	spin_unlock(&bp->lock);
	return 0;
}
