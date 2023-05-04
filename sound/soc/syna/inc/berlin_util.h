/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __BERLIN_UITL_H__
#define __BERLIN_UITL_H__

#include <linux/kernel.h>


int berlin_set_pll(void *aio_handle, u32 fs);
u32 berlin_get_div(u32 fs);
u32 berlin_get_dfm(u32 word_w);
u32 berlin_get_cfm(u32 word_s);

#endif
