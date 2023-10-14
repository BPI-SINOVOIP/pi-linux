// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>

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

/* Enable the buffer status debugging. As some debug function may be called from isr, in case printk console log
 * level is high, isr would be waiting in console driver which would make the underrun situation more worst.
 * Enable it only when needed.
 */
//#define BUF_STATE_DEBUG

#define DMA_BUFFER_SIZE        (256 * 1024)
#define DMA_BUFFER_MIN         (512)
#define MAX_BUFFER_SIZE        (DMA_BUFFER_SIZE << 1)

#define ZERO_DMA_BUFFER_SIZE   (32)
#define DHUB_DMA_DEPTH           4

enum data_format {
	data_format_pcm,
	data_format_iec61937,
};

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
	unsigned int in_dma_size;

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
	 * Indicates if bitstream data is parsed, ignore pcm
	 */
	bool data_parsed;

	/*
	 * Instance lock between ISR and higher contexts.
	 */
	spinlock_t lock;
	unsigned int data_format;

	/* spdif DMA buffer */
	unsigned char *spdif_dma_area;  		/* dma buf for spdifo */
	dma_addr_t spdif_dma_addr;  			/* phy addr of spdif buf */
	unsigned int spdif_buf_size;		   /* size of dma area */
	unsigned int spdif_ratio;

	/* PCM DMA buffer */
	unsigned char *pcm_dma_area;
	dma_addr_t pcm_dma_addr;
	unsigned int pcm_buf_size;
	unsigned int pcm_ratio;
	unsigned int pcm_ratio_div;

	/* HDMI DMA buffer */
	unsigned char *hdmi_dma_area;
	dma_addr_t hdmi_dma_addr;
	unsigned int hdmi_buf_size;
	unsigned int hdmi_ratio;
	unsigned int hdmi_ratio_denominator;

	/* hw parameter */
	unsigned int sample_rate;
	unsigned int sample_format;
	unsigned int channel_num;
	ssize_t buf_size;
	ssize_t period_size;

	/* for spdif encoding */
	unsigned int spdif_frames;
	unsigned char channel_status[24];
//	struct hdmi_spdif_frame stSpdifDataHeader[SPDIF_BLOCK_SIZE * 2 * 3];
	/* for hdmi encoding */
	unsigned int hdmi_spdif_frames;
	unsigned char hdmi_channel_status[24];
	struct iec61937_burst_header bh;
	enum burst_data_type burst_type;
	/* playback status */
	unsigned int output_mode;
	unsigned int intr_updates;  	// tracing the src interrupt happened

	struct snd_pcm_substream *ss;
	struct snd_pcm_indirect pcm_indirect;
	unsigned int spdif_ch;
	unsigned int i2s_ch;
	unsigned int hdmi_ch;
	struct berlin_chip *chip;
	struct workqueue_struct *wq;
	struct delayed_work hdmi_enable_work;

};

//Sample rate fixed to 48k now, avpll setting
static unsigned int berlin_playback_rates[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
	64000, 88200, 96000, 176400, 192000, 352800, 384000
};

static struct snd_pcm_hw_constraint_list berlin_constraints_rates = {
	.count  = ARRAY_SIZE(berlin_playback_rates),
	.list   = berlin_playback_rates,
	.mask   = 0,
};

static void *zero_dma_buf;
static dma_addr_t zero_dma_addr;

static void spdif_enc_subframe(uint32_t *subframe,
				   const uint32_t data, uint32_t sync_type,
				   uint8_t v, uint8_t u, uint8_t c)
{
	struct spdif_frame *spdif = (struct spdif_frame *)subframe;
	uint16_t high_16 = (data >> 16), low_16 = ((data >> 8) & 0xff);
	int32_t parity = ((v + u + c) & 1);

	*subframe = 0x00;
	spdif->sync = sync_type;
	spdif->validflag = v;
	spdif->user = u;
	spdif->channelstatus = c;
	spdif->paritybit = parity;
	spdif->audiosample2 = high_16;
	spdif->audiosample1 = low_16;
}

static inline u32 get_p_bit(u32 x)
{
	x = (x >> 16) ^ x;
	x = (x >> 8) ^ x;
	x = (x >> 4) ^ x;
	x = (x >> 2) ^ x;
	x = (x >> 1) ^ x;
	return (x & 1);
}

static void hdmi_spdif_enc_subframe(u32 *subframe, u16 data, uint32_t sync_type, uint8_t c)
{
	struct hdmi_spdif_frame *pData = (struct hdmi_spdif_frame *)subframe;
	/* reset */
	*subframe = 0;

	pData->unParityBit = 0;
	pData->reserve = 0;
	pData->reserve_b = 0;
	pData->unChannelStatus = c;
	pData->unBbit = sync_type;
	pData->unValidFlag = 1;
	pData->unUserData = 0;
	pData->Data_Low = data & 0x1F;
	pData->Data_Mid = (data >> 5) & 0xFF;
	pData->Data_High = (data >> 13) & 0x7;
	pData->unParityBit = get_p_bit((*((u32 *)pData)) & 0x7FFFFFFF) ? 1 : 0;
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

dma_begin:
	if (bp->pcm_indirect.hw_ready < (bp->period_size + bp->in_dma_size) ||
		bp->in_dma_size == (DHUB_DMA_DEPTH * bp->period_size)) {
#ifdef BUF_STATE_DEBUG
		if(bp->in_dma_size == 0)
			snd_printd("%s: mode %x, underrun! hw_ready: %d\n", __func__,
			    bp->output_mode, bp->pcm_indirect.hw_ready);
#endif
		return;
	}

	if (bp->ma_dma_pending && bp->spdif_dma_pending)
		return;

	if ((bp->output_mode & I2SO_MODE)
	    && !bp->ma_dma_pending) {
		dma_source_address = bp->pcm_dma_addr +
					bp->dma_offset * bp->pcm_ratio / bp->pcm_ratio_div;
		dma_size = bp->period_size * bp->pcm_ratio / bp->pcm_ratio_div;
		bp->ma_dma_pending = true;

		dhub_channel_write_cmd(bp->chip->dhub,
					   bp->i2s_ch,
					   dma_source_address, dma_size,
					   0, 0, 0, 1, 0, 0);
	}
	if ((bp->output_mode & SPDIFO_MODE)
		&& !bp->spdif_dma_pending) {
		dma_source_address = bp->spdif_dma_addr + bp->dma_offset * bp->spdif_ratio;
		dma_size = bp->period_size * bp->spdif_ratio;
		bp->spdif_dma_pending = true;
		dhub_channel_write_cmd(bp->chip->dhub,
					   bp->spdif_ch,
					   dma_source_address, dma_size,
					   0, 0, 0, 1, 0, 0);
	}

	if (bp->output_mode & HDMIO_MODE) {
		unsigned int pushed_dma_offset;

		pushed_dma_offset = bp->dma_offset + bp->in_dma_size;
		pushed_dma_offset %= bp->buf_size;
		dma_source_address = bp->hdmi_dma_addr +
			pushed_dma_offset * bp->hdmi_ratio / bp->hdmi_ratio_denominator;
		dma_size = bp->period_size * bp->hdmi_ratio / bp->hdmi_ratio_denominator;
		bp->in_dma_size += bp->period_size;
		dhub_channel_write_cmd(bp->chip->dhub,
					   bp->hdmi_ch,
					   dma_source_address, dma_size,
					   0, 0, 0, 1, 0, 0);
		goto dma_begin;
	}
}

void berlin_playback_hdmi_bitstream_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct berlin_playback *bp = runtime->private_data;
	unsigned long flags;

	spin_lock_irqsave(&bp->lock, flags);
	start_dma_if_needed(bp);
	spin_unlock_irqrestore(&bp->lock, flags);
}

static inline u32 get_chanid(struct berlin_playback *bp)
{
	u32 chanId = 0;

	if (bp->output_mode & HDMIO_MODE) {
		chanId = bp->hdmi_ch;
	}

	if (bp->output_mode & SPDIFO_MODE) {
		chanId = bp->spdif_ch;
	}

	if (bp->output_mode & I2SO_MODE) {
		chanId = bp->i2s_ch;
	}

	return chanId;
}

static int channel_enable(void *hdl, u32 chanId, u32 enable)
{
	HDL_semaphore *pSemHandle = NULL;
	u32 uiInstate;

	pSemHandle = dhub_semaphore(hdl);
	if (!enable) {
		uiInstate = semaphore_chk_full(pSemHandle, chanId);
		if ((uiInstate >> chanId) & 1) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
		}
	}
	semaphore_intr_enable(pSemHandle,
			  chanId, 0,
			  enable ? 1 : 0,
			  0,
			  0,
			  0
		);
	return 0;
}

static void bitstream_params_adapt(struct berlin_playback *bp)
{
	/* set the defalut burst data type according to the input hw params
	 * for mat,  since the platform can support fs up to 192K, we set
	 * 8 channel + 192K instead of 2 channel + 768K
	 */

	if (bp->channel_num == 8) {
		bp->burst_type = BURST_MAT;
		snd_printd("pcm hw params for MAT\n");
		bp->channel_num = 2;
	} else  {
		if (bp->channel_num != 2) {
			snd_printk("error hw params: channel %u, fs %u\n",
				bp->channel_num, bp->sample_rate);
			bp->channel_num = 2;
		}

		if (bp->sample_rate <= eAUD_Freq_48K) {
			bp->burst_type = BURST_AC3;
			snd_printd("pcm hw params for AC3\n");
		} else {
			bp->burst_type = BURST_DDP;
			snd_printd("pcm hw params for DDP\n");
		}
	}
	bp->sample_format = SNDRV_PCM_FORMAT_S16_LE;
}

static void init_hdmi_channel_status(struct berlin_playback *bp)
{
	u32 spdif_fs;
	struct spdif_cs *chnsts;

	switch (bp->burst_type) {
	case BURST_MAT:
		spdif_fs = bp->sample_rate << 2;
		break;
	case BURST_AC3:
	case BURST_DDP:
	default:
		spdif_fs = bp->sample_rate;
		break;
	}

	chnsts = (struct spdif_cs *)&(bp->hdmi_channel_status[0]);
	hdmi_init_channel_status(chnsts, spdif_fs, NULL);
	snd_printd("hdmi spdif fs %d\n", spdif_fs);
}

static void berlin_playback_trigger_start(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;

	snd_printd("%s: done. \n", __func__);
	channel_enable(bp->chip->dhub, get_chanid(bp), true);
}

static void berlin_playback_trigger_stop(struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;

	snd_printd("%s: done.\n", __func__);
	dhub_channel_clear_done(bp->chip->dhub, get_chanid(bp));
	DhubChannelClear(bp->chip->dhub, get_chanid(bp), 0);
	channel_enable(bp->chip->dhub, get_chanid(bp), false);

	if (bp->output_mode & HDMIO_MODE) {
		bp->hdmi_spdif_frames = 0;
	}
	if (bp->output_mode & SPDIFO_MODE) {
		spdif_init_channel_status(
			(struct spdif_cs *)&(bp->channel_status[0]),
			bp->sample_rate, 0);
		bp->spdif_frames = 0;
	}
}

static const struct snd_pcm_hardware berlin_playback_hw = {
	.info   		= (SNDRV_PCM_INFO_MMAP
				   | SNDRV_PCM_INFO_INTERLEAVED
				   | SNDRV_PCM_INFO_MMAP_VALID
				   | SNDRV_PCM_INFO_PAUSE
				   | SNDRV_PCM_INFO_RESUME),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE
				   | SNDRV_PCM_FMTBIT_S24_LE
				   | SNDRV_PCM_FMTBIT_S24_3LE
				   | SNDRV_PCM_FMTBIT_S32_LE),
	.rates  		= (SNDRV_PCM_RATE_8000_384000
				   | SNDRV_PCM_RATE_KNOT),
	.channels_min   	= 1,
	.channels_max   	= 8,
	.buffer_bytes_max   = MAX_BUFFER_SIZE,
	.period_bytes_min   = DMA_BUFFER_MIN,
	.period_bytes_max   = DMA_BUFFER_SIZE,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size  	= 0
};

static const struct snd_pcm_hardware berlin_playback_multi_codecs = {
	.info   		= (SNDRV_PCM_INFO_MMAP
				   | SNDRV_PCM_INFO_INTERLEAVED
				   | SNDRV_PCM_INFO_MMAP_VALID
				   | SNDRV_PCM_INFO_PAUSE
				   | SNDRV_PCM_INFO_RESUME),
	.formats		= (SNDRV_PCM_FMTBIT_S16_LE
					| SNDRV_PCM_FMTBIT_S24_LE
					| SNDRV_PCM_FMTBIT_S24_3LE
					| SNDRV_PCM_FMTBIT_S32_LE),
	.rates  		= SNDRV_PCM_RATE_8000_384000,
	.channels_min   	= 4,
	.channels_max   	= 4,
	.buffer_bytes_max   = MAX_BUFFER_SIZE * 2,
	.period_bytes_min   = DMA_BUFFER_MIN * 2,
	.period_bytes_max   = DMA_BUFFER_SIZE * 2,
	.periods_min		= 2,
	.periods_max		= MAX_BUFFER_SIZE / DMA_BUFFER_MIN,
	.fifo_size  	= 0
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

		if (bp->hdmi_dma_area) {
			dma_free_coherent(NULL, bp->hdmi_buf_size,
					  bp->hdmi_dma_area,
					  bp->hdmi_dma_addr);
			bp->hdmi_dma_area = NULL;
			bp->hdmi_dma_addr = 0;
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
	snd_iprintf(buffer, "indirect.hw_ready:\t%d in_dma:\t%u\n",
			bp->pcm_indirect.hw_ready, bp->in_dma_size);
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

int berlin_playback_open(struct snd_pcm_substream *ss, int passthrough)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct berlin_playback *bp;
	int err;

	snd_printd("%s: start (passthrough %d).\n", __func__, passthrough);

	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
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
	bp->data_parsed = false;
	bp->intr_updates = 0;
	bp->ss = ss;
	bp->i2s_ch = -1;
	bp->spdif_ch = -1;
	bp->hdmi_ch = -1;
	bp->data_format = passthrough ? data_format_iec61937 : data_format_pcm;

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

		if (bp->hdmi_dma_area) {
			dma_free_coherent(&pdev->dev,
					  bp->hdmi_buf_size,
					  bp->hdmi_dma_area,
					  bp->hdmi_dma_addr);
			bp->hdmi_dma_area = NULL;
			bp->hdmi_dma_addr = 0;
		}

	}

	if (bp->pages_allocated == true) {
		snd_pcm_lib_free_pages(ss);
		bp->pages_allocated = false;
	}

	if (bp->wq) {
		cancel_delayed_work_sync(&bp->hdmi_enable_work);
		destroy_workqueue(bp->wq);
		bp->wq = NULL;
	}

	return 0;
}

extern void hdmi_port_int_enable(void);
//static void hdmi_enable(struct work_struct *work)
void hdmi_enable(struct work_struct *work)
{
	//hdmi_port_int_enable();
	return;
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

	snd_printd("%s: fs:%d ch:%d width:%d format:%s, period bytes:%d buffer bytes: %d format: %s\n",
		__func__,
		params_rate(p), params_channels(p),
		params_width(p),
		snd_pcm_format_name(params_format(p)),
		params_period_bytes(p),
		params_buffer_bytes(p),
		snd_pcm_format_name(params_format(p)));

	bp->chip = chip;
	berlin_playback_hw_free(ss);

	err = snd_pcm_lib_malloc_pages(ss, params_buffer_bytes(p));
	if (err < 0) {
		snd_printk("failed to allocated pages for buffers\n");
		return err;
	}
	bp->pages_allocated = true;

	INIT_DELAYED_WORK(&bp->hdmi_enable_work, hdmi_enable);
	bp->wq = alloc_workqueue("berlin_bp_que", WQ_HIGHPRI | WQ_UNBOUND, 1);
	if (bp->wq == NULL) {
		snd_printd("alloc_workqueue failed.");
		err = -ENOMEM;
		return err;
	}

	snd_printd("%s: sample_rate:%d channels:%d format:%s\n", __func__,
		   params_rate(p), params_channels(p),
		   snd_pcm_format_name(params_format(p)));

	bp->sample_rate = params_rate(p);
	bp->sample_format = params_format(p);
	bp->channel_num = params_channels(p);
	bp->period_size = params_period_bytes(p);
	bp->buf_size = params_buffer_bytes(p);
	bp->pcm_ratio = 1;
	bp->pcm_ratio_div = 1;
	if (bp->data_format == data_format_iec61937)
		bitstream_params_adapt(bp);

	snd_printd("%s: period_size:%d buf_size:%d\n", __func__,
		   params_period_bytes(p), params_buffer_bytes(p));
	if (bp->sample_format == SNDRV_PCM_FORMAT_S16_LE)
		bp->pcm_ratio *= 2;
	if (params_width(p)/8 == 3) {
		bp->pcm_ratio *= 4;
		bp->pcm_ratio_div = 3;
	}

	bp->pcm_buf_size = bp->buf_size * bp->pcm_ratio / bp->pcm_ratio_div;

	snd_printd("%s: pcm_ratio:%d pcm_ratio_div:%d buf_size:%ld pcm_buf_size:%d\n",
		__func__,
		bp->pcm_ratio,
		bp->pcm_ratio_div,
		bp->buf_size,
		bp->pcm_buf_size);

	bp->pcm_dma_area =
		dma_alloc_coherent(&pdev->dev, bp->pcm_buf_size,
				&bp->pcm_dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!bp->pcm_dma_area) {
		snd_printk("%s: failed to allocate PCM DMA area\n", __func__);
		goto err_pcm_dma;
	}

	bp->hdmi_ratio = 1;
	bp->hdmi_ratio_denominator = 1;
	if (bp->data_format == data_format_iec61937) {
		snd_printd("16bit stream maybe need to pack 32bit\n");
		bp->hdmi_ratio *= 2;
	} else { /* pcm data */
		if (bp->channel_num == 1)
			bp->hdmi_ratio *= 2;
		else if (bp->channel_num > 2) {
			bp->hdmi_ratio = 8;
			bp->hdmi_ratio_denominator = bp->channel_num;
		}
	}

	bp->hdmi_buf_size = bp->buf_size * bp->hdmi_ratio / bp->hdmi_ratio_denominator;

	bp->hdmi_dma_area =
		dma_alloc_coherent(&pdev->dev, bp->hdmi_buf_size,
				&bp->hdmi_dma_addr, GFP_KERNEL | __GFP_ZERO);
	if (!bp->hdmi_dma_area) {
		snd_printk("%s: failed to allocate HDMI DMA area\n", __func__);
		goto err_hdmi_dma;
	}

	/* initialize hdmi spdif channel status */
	init_hdmi_channel_status(bp);

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
	spdif_init_channel_status(chnsts, bp->sample_rate, 0);

	if (zero_dma_buf == NULL) {
		zero_dma_buf = devm_kzalloc(&pdev->dev, ZERO_DMA_BUFFER_SIZE,
					    GFP_KERNEL);
		if (!zero_dma_buf) {
			snd_printk("%s: failed to allocate zero DMA area\n",
				   __func__);
			goto err_zero_dma;
		}
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
	if (bp->hdmi_dma_area)
		dma_free_coherent(&pdev->dev, bp->hdmi_buf_size,
				  bp->hdmi_dma_area, bp->hdmi_dma_addr);
	bp->hdmi_dma_area = NULL;
	bp->hdmi_dma_addr = 0;
err_hdmi_dma:
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
	bp->in_dma_size = 0;
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
		snd_printd("trigger cmd %d\n", cmd);
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
		uint8_t v = bp->data_format == data_format_iec61937 ? 1 : 0;

#ifdef SPDIF_CLOCK_PATTERN
		spdif_enc_subframe(&spdif_out[i * 4],
				   pcm_in[i * 2], sync_word, v, 0,
				   channel_status);

		sync_word = TYPE_W;
		spdif_enc_subframe(&spdif_out[(i * 4) + 2],
				   pcm_in[(i * 2) + 1], sync_word, v, 0,
				   channel_status);
#else
		spdif_enc_subframe(&spdif_out[i * 2],
				   pcm_in[i * 2], sync_word, v, 0,
				   channel_status);

		sync_word = TYPE_W;
		spdif_enc_subframe(&spdif_out[(i * 2) + 1],
				   pcm_in[(i * 2) + 1], sync_word, v, 0,
				   channel_status);

#endif
		++bp->spdif_frames;
		bp->spdif_frames %= SPDIF_BLOCK_SIZE;
	}
}

static int search_iec61937_header(char *buf, u32 size, u32 *consumed)
{
	u16 unSyncPa = 0, unSyncPb = 0;
	u32 unBytesLeft, uiConsumed = 0;
	bool bNextSyncFind = false;
	u16 *pSearch = NULL;
	u16 Pa = 0xF872, Pb = 0x4E1F;

	if (!buf || !consumed)
		return -EINVAL;

	pSearch = (u16 *)buf;
	unBytesLeft = size;

	do {
		if (unBytesLeft < sizeof(u16) * 2) {
			*consumed = uiConsumed;
			return -EINVAL;
		}

		unSyncPa = *pSearch;
		unSyncPb = *(pSearch + 1);
		if (((Pa == unSyncPa) && (Pb == unSyncPb))
			|| ((0x72F8 == unSyncPa) && (0x1F4E == unSyncPb))) {
			bNextSyncFind = true;
		} else {
			uiConsumed += sizeof(u16);
			pSearch = (u16 *)(buf + uiConsumed);
			unBytesLeft -= sizeof(u16);
			continue;
		}
	} while (!bNextSyncFind);

	*consumed = uiConsumed;
	return 0;
}

static bool parse_iec61937(struct berlin_playback *bp, const u16 *buf, int buf_size, int *offset)
{
	char *p = (char *)buf;
	int ret;
	u32 consumed = 0;
	struct iec61937_burst_header *header;

	ret = search_iec61937_header(p, buf_size, &consumed);
	if (ret < 0) {
		snd_printk("%s,IEC61937SearchSync, failed, %d, %d\n",
			bp->output_mode & HDMIO_MODE ? "HDMI" : "SPDIF",
			 ret, consumed);
		return false;
	}

	header = (struct iec61937_burst_header *)(p + consumed);
	snd_printd("%s IEC61937 header found @%d, [0x%04x 0x%04x 0x%04x 0x%04x]\n",
		bp->output_mode & HDMIO_MODE ? "HDMI" : "SPDIF",
		consumed, header->syncword1, header->syncword2,
		header->burstinfo, header->len);
	bp->bh = *header;
	*offset = consumed/(sizeof(u16) * 2);

	return true;
}

static void hdmi_spdif_encode(struct berlin_playback *bp,
	 const u16 *pcm_in, u32 *pcm_out, int frames)
{
	int i;

	for (i = 0; i < frames; ++i) {
		unsigned char channel_status =
			hdmi_get_channel_status(bp->hdmi_channel_status,
						 bp->hdmi_spdif_frames);
		uint32_t sync_word = bp->hdmi_spdif_frames == 0 ? 1 : 0;

		hdmi_spdif_enc_subframe(&pcm_out[i * 2], pcm_in[i * 2], sync_word, channel_status);
		hdmi_spdif_enc_subframe(&pcm_out[i * 2 + 1], pcm_in[i * 2 + 1], 0, channel_status);

		++bp->hdmi_spdif_frames;
		bp->hdmi_spdif_frames %= SPDIF_BLOCK_SIZE;
	}
}

static void init_hdmi_tx_passthrough(struct berlin_playback *bp,
					void *buf, u32 len)
{
	int offset = -1;

	bp->data_parsed = parse_iec61937(bp, buf, len, &offset);
	if (bp->data_parsed) {
		u32 burst_type = bp->bh.burstinfo & 0x1f;

		if (bp->burst_type != burst_type) {
			snd_printk("Warnning: get burst type %u diff with original %u\n",
				burst_type, bp->burst_type);
			bp->burst_type = burst_type;
			/* reinitialize spdif channel status for bitstream */
			init_hdmi_channel_status(bp);
		}
	}
}

static int berlin_playback_copy(struct snd_pcm_substream *ss,
				int channel, snd_pcm_uframes_t pos,
				void *buf, size_t bytes)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct berlin_playback *bp = runtime->private_data;
	int32_t *pcm_buf;
	const int frames = bytes /
			   (bp->channel_num *
			    snd_pcm_format_width(bp->sample_format) / 8);
	int i, j;
	int channels = bp->channel_num;

	if (pos >= bp->buf_size)
		return -EINVAL;

#ifdef BUF_STATE_DEBUG
	if (bp->pcm_indirect.hw_ready >= bp->period_size) {
		const unsigned long dma_b = bp->dma_offset;
		const unsigned long dma_e = (dma_b + bp->in_dma_size) % bp->buf_size;
		const unsigned long write_b = pos;
		const unsigned long write_e = write_b + bytes - 1;

		// Write begin position shouldn't be in DMA area.
		if ((dma_b <= write_b) && (write_b < dma_e)) {
			snd_printd("%s: db:%lu <= wb:%lu < de:%lu\n",
				   __func__, dma_b, write_b, dma_e);
		}
		// Write end position shouldn't be in DMA area.
		if ((dma_b <= write_e) && (write_e < dma_e)) {
			snd_printd("%s: db:%lu <= we:%lu < de:%lu\n",
				   __func__, dma_b, write_e, dma_e);
		}
	}
#endif

	if (bp->output_mode & I2SO_MODE) {
		pcm_buf = (int32_t *)(bp->pcm_dma_area +
					pos * bp->pcm_ratio / bp->pcm_ratio_div);
	} else if (bp->output_mode & SPDIFO_MODE) {
		pcm_buf = (int32_t *)(bp->spdif_dma_area +
							pos * bp->spdif_ratio);
	} else if (bp->output_mode & HDMIO_MODE) {
		pcm_buf = (int32_t *)(bp->hdmi_dma_area +
					pos * bp->hdmi_ratio / bp->hdmi_ratio_denominator);
	} else {
		snd_printd("output mode %d", bp->output_mode);
		return -EINVAL;
	}

	if (!bp->data_parsed) {
		printk_ratelimited("berlin_pcm begin to parse data\n");
		if (bp->data_format == data_format_pcm)
			bp->data_parsed = true;
	}

	if (bp->sample_format == SNDRV_PCM_FORMAT_S16_LE) {
		const int16_t *s16_pcm_source = (int16_t *)buf;

		if (bp->output_mode & HDMIO_MODE) {

			if (bp->data_format == data_format_iec61937) {
				if (!bp->data_parsed)
					init_hdmi_tx_passthrough(bp, buf, frames * 4);
			} else {
				u32 consumed = 0;
				int ret;
				/* debug here */
				ret = search_iec61937_header(buf, frames * 4, &consumed);
				if (!ret) {
					snd_printk("IEC61937SearchSync found @%x, fatal error\n", consumed);
				}
			}

			if (bp->data_format == data_format_pcm) {
				int16_t *s16_pcm_dest = (int16_t *)pcm_buf;
				u8 chan;

				if (bp->channel_num % 2)
					printk_ratelimited("channel_num %d odd\n", bp->channel_num);

				if (bp->channel_num == 1) {
					// Shift sample to 32-bits, and upmix to stereo.
					for (i = 0; i < frames; ++i) {
						s16_pcm_dest[i * 2] = s16_pcm_source[i];
						s16_pcm_dest[(i * 2) + 1] = 0x00;
					}
				} else if (bp->channel_num == 2) {
					for (i = 0; i < frames; ++i) {
						s16_pcm_dest[i * 2] = s16_pcm_source[i * 2];
						s16_pcm_dest[(i * 2) + 1] = s16_pcm_source[i * 2 + 1];
					}
				} else {
					for (i = 0; i < frames; ++i) {
						for (chan = 0; chan < HDMI_PCM_MAX_CHANNEL_NUM; chan++) {
							if (chan < bp->channel_num)
								s16_pcm_dest[i * HDMI_PCM_MAX_CHANNEL_NUM + chan]
									= s16_pcm_source[i * bp->channel_num + chan];
							else
								s16_pcm_dest[i * HDMI_PCM_MAX_CHANNEL_NUM + chan]
									= 0x00;
						}
					}
				}
			} else {
				hdmi_spdif_encode(bp, s16_pcm_source, pcm_buf, frames);
			}
		} else if (bp->output_mode & SPDIFO_MODE) {

			if (bp->data_format == data_format_iec61937 && !bp->data_parsed) {
				int offset = -1;

				bp->data_parsed = parse_iec61937(bp, buf, frames * 4, &offset);
				if (bp->data_parsed) {
					spdif_init_channel_status(
						(struct spdif_cs *)&(bp->channel_status[0]),
						bp->sample_rate, 1);
				} else {
					snd_printk("fatal error\n");
					return -EINVAL;
				}
			}

			for (i = 0; i < frames; i++) {
				pcm_buf[i * 2] = s16_pcm_source[i * 2] << 16;
				pcm_buf[(i * 2) + 1] = s16_pcm_source[i * 2 + 1] << 16;
			}

		} else {
			for (i = 0; i < frames; i++) {
				for (j = 0; j < channels; j++)
					pcm_buf[i * channels + j] =
						s16_pcm_source[i * channels + j] << 16;
			}
		}
	} else if (bp->sample_format == SNDRV_PCM_FORMAT_S32_LE) {
		const int32_t *s32_pcm_source = (int32_t *)buf;

		if (bp->output_mode & HDMIO_MODE) {
			int32_t *s32_pcm_dest = (int32_t *)pcm_buf;
			u8 chan;

			if (bp->channel_num % 2)
				printk_ratelimited("channel_num %d odd\n", bp->channel_num);

			if (bp->channel_num == 1) {
				// Upmix each sample to stereo.
				for (i = 0; i < frames; ++i) {
					s32_pcm_dest[i * 2] = s32_pcm_source[i];
					s32_pcm_dest[(i * 2) + 1] = s32_pcm_source[i];
				}
			} else if (bp->channel_num == 2) {
				// Copy the left and right samples straight over.
				for (i = 0; i < frames; ++i) {
					s32_pcm_dest[i * 2] = s32_pcm_source[i * 2];
					s32_pcm_dest[(i * 2) + 1] = s32_pcm_source[(i * 2) + 1];
				}
			} else {
				for (i = 0; i < frames; ++i)
					for (chan = 0; chan < HDMI_PCM_MAX_CHANNEL_NUM; chan++)
						if (chan < bp->channel_num)
							s32_pcm_dest[i * HDMI_PCM_MAX_CHANNEL_NUM + chan]
								= s32_pcm_source[i * bp->channel_num + chan];
						else
							s32_pcm_dest[i * HDMI_PCM_MAX_CHANNEL_NUM + chan]
								= 0x00;
			}
		} else {
			for (i = 0; i < frames; i++) {
				for (j = 0; j < channels; j++)
					pcm_buf[i * channels + j] =
						s32_pcm_source[i * channels + j];
			}
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

		if (bp->output_mode & HDMIO_MODE) {
			int32_t *s32_pcm_dest = (int32_t *)pcm_buf;
			u8 chan;

			if (bp->channel_num == 1) {
				// Upmix each sample to stereo.
				for (i = 0; i < frames; ++i) {
					s32_pcm_dest[i * 2] = s32_pcm_source[i] << 8;
					s32_pcm_dest[(i * 2) + 1] = s32_pcm_source[i] << 8;
				}
			} else if (bp->channel_num == 2) {
				// Copy the left and right samples straight over.
				for (i = 0; i < frames; ++i) {
					s32_pcm_dest[i * 2] = s32_pcm_source[i * 2] << 8;
					s32_pcm_dest[(i * 2) + 1] =
						s32_pcm_source[(i * 2) + 1] << 8;
				}
			} else {
				for (i = 0; i < frames; ++i)
					for (chan = 0; chan < HDMI_PCM_MAX_CHANNEL_NUM; chan++)
						if (chan < bp->channel_num)
							s32_pcm_dest[i * HDMI_PCM_MAX_CHANNEL_NUM + chan]
								= s32_pcm_source[i * bp->channel_num + chan] << 8;
						else
							s32_pcm_dest[i * HDMI_PCM_MAX_CHANNEL_NUM + chan]
								= 0x00;
			}
		} else {
			for (i = 0; i < frames; i++) {
				for (j = 0; j < channels; j++)
					pcm_buf[i * channels + j] =
						s32_pcm_source[i * channels + j] << 8;
			}
		}
	} else {
		snd_printd("Unsupported format:%s\n",
			snd_pcm_format_name(bp->sample_format));
		return -EINVAL;
	}


	if (bp->output_mode & SPDIFO_MODE)
		spdif_encode(bp, pcm_buf, pcm_buf, frames);
//	if (bp->output_mode & HDMIO_MODE)
//		printk_ratelimited("output %d frames\n", frames);

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
		if (mode == I2SO_MODE && bp->data_format == data_format_pcm)
			bp->i2s_ch = ch ? ch[0] : 0;
		else if (mode == SPDIFO_MODE)
			bp->spdif_ch = ch ? ch[0] : 0;
		else if (mode == HDMIO_MODE)
			bp->hdmi_ch = ch ? ch[0] : 0;
		else if (mode == 0) {
			bp->output_mode = 0;
			bp->i2s_ch = 0;
			bp->spdif_ch = 0;
			bp->hdmi_ch = 0;
		} else {
			snd_printk("invalid mode setting, mod %u\n", mode);
			return;
		}
		bp->output_mode |= mode;
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

	if (!(bp->output_mode & HDMIO_MODE)) {
		spin_lock_irqsave(&bp->lock, flags);
		start_dma_if_needed(bp);
		spin_unlock_irqrestore(&bp->lock, flags);
	}
	else {
		queue_delayed_work(bp->wq, &bp->hdmi_enable_work, 0);
	}

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
	if (!bp->ma_dma_pending && !bp->spdif_dma_pending && !bp->in_dma_size) {
		spin_unlock(&bp->lock);
#ifdef BUF_STATE_DEBUG
		snd_printd("stream %p no dma pending\n", ss);
#endif
		return 0;
	}

	if (chanId == bp->i2s_ch) {
#ifdef BUF_STATE_DEBUG
		if (bp->intr_updates & I2SO_MODE)
			snd_printd("stream %p dual i2s intrupt\n", ss);
#endif
		bp->ma_dma_pending = false;
	}

	if (chanId == bp->spdif_ch) {
#ifdef BUF_STATE_DEBUG
		if (bp->intr_updates & SPDIFO_MODE)
			snd_printd("stream %p dual spdif interrupt\n", ss);
#endif
		bp->spdif_dma_pending = false;
	}

	if (chanId == bp->hdmi_ch) {
		if (bp->intr_updates & HDMIO_MODE)
			snd_printd("stream %p dual hdmi interrupt\n", ss);
		bp->in_dma_size -= bp->period_size;
	}
	/* Roll the DMA pointer, and chain if needed */
	if (chanId == bp->i2s_ch)
		bp->intr_updates |= I2SO_MODE;

	if (chanId == bp->spdif_ch)
		bp->intr_updates |= SPDIFO_MODE;

	if (chanId == bp->hdmi_ch)
		bp->intr_updates |= HDMIO_MODE;

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

bool berlin_playback_passthrough_check(struct snd_pcm_substream *substream,
			       u32 *data_type)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct berlin_playback *bp = runtime->private_data;

	if (bp->data_format != data_format_iec61937)
		return false;

	*data_type = bp->burst_type;
	return true;
}
