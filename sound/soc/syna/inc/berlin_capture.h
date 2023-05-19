/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __BERLIN_CAPTURE_H__
#define __BERLIN_CAPTURE_H__

#include <sound/soc.h>

void berlin_capture_set_ch_mode(struct snd_pcm_substream *substream,
				u32 ch_num, u32 *chid, u32 mode,
				bool enable_mic_mute, bool interleaved_mode,
				bool dummy_data, u32 channel_map,
				bool ch_shift_check);
int berlin_capture_hw_free(struct snd_pcm_substream *substream);
int berlin_capture_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params);
int berlin_capture_prepare(struct snd_pcm_substream *substream);
int berlin_capture_trigger(struct snd_pcm_substream *substream, int cmd);
snd_pcm_uframes_t
berlin_capture_pointer(struct snd_pcm_substream *substream);
int berlin_capture_isr(struct snd_pcm_substream *substream);
int berlin_capture_open(struct snd_pcm_substream *substream);
int berlin_capture_close(struct snd_pcm_substream *substream);
void berlin_capture_set_ch_inuse(struct snd_pcm_substream *substream,
				 u32 ch_num);
#endif
