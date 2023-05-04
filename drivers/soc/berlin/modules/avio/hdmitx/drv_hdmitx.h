// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _DRV_HDMITX_H_
#define _DRV_HDMITX_H_

#include "drv_vpp.h"

int drv_hdmitx_init(VPP_CTX *hVppCtx);
int drv_hdmitx_set_state(struct extcon_dev *hdmitx_dev, unsigned int state);

#endif
