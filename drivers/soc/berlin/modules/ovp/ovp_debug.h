// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _OVP_DEBUG_H_
#define _OVP_DEBUG_H_

#define OVP_DEVICE_TAG    "[ovp] "
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define ovp_debug(...)		printk(KERN_DEBUG OVP_DEVICE_TAG __VA_ARGS__)
#else
#define ovp_debug(...)
#endif

#define ovp_trace(...)		ovp_debug(__VA_ARGS__)
#define ovp_error(...)		printk(KERN_ERR OVP_DEVICE_TAG __VA_ARGS__)

#endif //_OVP_DEBUG_H_
