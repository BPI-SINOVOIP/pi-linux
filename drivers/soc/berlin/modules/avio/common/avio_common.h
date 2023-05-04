// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef	__AVIO_COMMON_H__
#define	__AVIO_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

int is_avio_driver_initialized(void);

int avio_util_get_quiescent_flag(void);

#ifdef __cplusplus
}
#endif

#endif /* __AVIO_COMMON_H__ */
