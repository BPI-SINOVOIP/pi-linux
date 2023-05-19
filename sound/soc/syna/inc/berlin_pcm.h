/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __BERLIN_PCM_H__
#define __BERLIN_PCM_H__

#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "aio_priv.h"

enum berlin_xrun_t {
	PCM_OVERRUN,
	FIFO_OVERRUN,
	PCM_UNDERRUN,
	FIFO_UNDERRUN,
	IRQ_DISABLE,
	XRUN_T_MAX
};

/* Each capture substream support either I2S OR PDM, not BOTH! */
#define I2SI_MODE      (1)
#define PDMI_MODE      (I2SI_MODE << 1)
#define SPDIFI_MODE    (I2SI_MODE << 2)
#define DMICI_MODE     (I2SI_MODE << 3)
#define FLAG_EARC_MODE (I2SI_MODE << 0xF)
/*
 * Playback substream may masked to I2S AND SPDIF at the sametime
 * The reason is we need to consider the customer use case, currently
 * they only feed one output stream but want playback to i2s and spdif.
 * Can split into I2S and SPDIF dai in further if customer use case allow
 */
#define I2SO_MODE     (I2SI_MODE << 4)
#define SPDIFO_MODE   (I2SI_MODE << 5)

// Support up to MAX_CHANNELS audio channels, MAX_CHANNELS/2 dhub channels
#define MAX_CHID        (MAX_CHANNELS >> 1)

struct berlin_chip {
	struct snd_card *card;
	struct platform_device *pdev;
	struct snd_hwdep *hwdep;
	void __iomem *pll_base;
	atomic_long_t xruns[XRUN_T_MAX];
	void *dhub;
	struct mutex dhub_lock;
};

struct berlin_ss_params {
	u32 irq_num;
	u32 chid_num;
	u32 mode;
	u32 channel_map;
	bool interleaved;
	bool dummy_data;
	bool enable_mic_mute;
	bool ch_shift_check; /* for dmic multi-channel shift check */
	unsigned int *irq;
	const char *dev_name;
};

void berlin_report_xrun(struct berlin_chip *chip, enum berlin_xrun_t xrun_type);
int berlin_pcm_request_dma_irq(struct snd_pcm_substream *substream,
			       struct berlin_ss_params *params);
void berlin_pcm_free_dma_irq(struct snd_pcm_substream *substream,
			     u32 irq_num, unsigned int *irq);
void berlin_pcm_max_ch_inuse(struct snd_pcm_substream *ss,
			     u32 ch_num);

#endif
