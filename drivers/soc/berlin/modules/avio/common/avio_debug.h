// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _AVIO_DEBUG_H_
#define _AVIO_DEBUG_H_

#define AVIO_DEVICE_TAG    "[avio] "
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define avio_debug(...)		printk(KERN_DEBUG AVIO_DEVICE_TAG __VA_ARGS__)
#else
#define avio_debug(...)
#endif

#define avio_trace(...)		printk(KERN_WARNING AVIO_DEVICE_TAG __VA_ARGS__)
#define avio_error(...)		printk(KERN_ERR AVIO_DEVICE_TAG __VA_ARGS__)

#endif //_AVIO_DEBUG_H_
