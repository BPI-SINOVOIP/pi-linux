/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __BERLIN_PLAYBACK_H__
#define __BERLIN_PLAYBACK_H__

#include <sound/soc.h>

void berlin_playback_set_ch_mode(struct snd_pcm_substream *substream,
				 u32 ch_num, u32 *chid, u32 mode);
int berlin_playback_hw_free(struct snd_pcm_substream *substream);
int berlin_playback_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params);
int berlin_playback_prepare(struct snd_pcm_substream *substream);
int berlin_playback_trigger(struct snd_pcm_substream *substream, int cmd);
int berlin_playback_ack(struct snd_pcm_substream *substream);
snd_pcm_uframes_t
berlin_playback_pointer(struct snd_pcm_substream *substream);
int berlin_playback_isr(struct snd_pcm_substream *substream,
			unsigned int chan_id);
int berlin_playback_open(struct snd_pcm_substream *substream);
int berlin_playback_close(struct snd_pcm_substream *substream);
#endif
