// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _AIO_PRIV_H_
#define _AIO_PRIV_H_

#define AIO_MIC_CH_NUM (AIO_MIC_CH1 + 1)
#define MAX_DMIC_MODULES (2)

// PCM channel number configuration
#define MIN_CHANNELS    (2)
#define MAX_CHANNELS    (4)

// Default PCM channel map
#define PCM_DEF_CHL_MAP  (0xF)

// PDM to DHUB write size in bytes while in interleave mode & dummy byte inserted
#define PDM_DHUB_OCPF_CHUNK_SZ  (16)

#endif
