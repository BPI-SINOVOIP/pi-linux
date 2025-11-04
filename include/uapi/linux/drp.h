/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Driver for the Renesas RZ/V2H DRPI unit
 *
 * Copyright (C) 2023 Renesas Electronics Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _UAPI__DRP_H
#define _UAPI__DRP_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif
#include <linux/ioctl.h>

#define DRP_IO_TYPE               (46)
#define DRP_ASSIGN                _IOW (DRP_IO_TYPE, 0, drp_data_t)
#define DRP_START                 _IOW (DRP_IO_TYPE, 1, drp_data_t)
#define DRP_RESET                 _IO  (DRP_IO_TYPE, 2)
#define DRP_GET_STATUS            _IOR (DRP_IO_TYPE, 3, drp_status_t)
#define DRP_SET_SEQ               _IOW (DRP_IO_TYPE, 6, drp_seq_t)           /* Since the sturecture size is different,            */
#define DRP_GET_CODEC_AREA        _IOR (DRP_IO_TYPE, 11, drp_data_t)
#define DRP_GET_OPENCVA_AREA      _IOR (DRP_IO_TYPE, 12, drp_data_t)
#define DRP_SET_DRP_MAX_FREQ      _IOW (DRP_IO_TYPE, 13, uint32_t)
#define DRP_READ_DRP_REG          _IOWR(DRP_IO_TYPE, 64, drp_reg_t)
#define DRP_WRITE_DRP_REG         _IOW (DRP_IO_TYPE, 65, drp_reg_t)

#define DRP_STATUS_INIT                   (0)
#define DRP_STATUS_IDLE                   (1)
#define DRP_STATUS_RUN                    (2)
#define DRP_ERRINFO_SUCCESS               (0)
#define DRP_ERRINFO_DRP_ERR               (-1)
#define DRP_ERRINFO_RESET                 (-3)
#define DRP_RESERVED_NUM                  (10)
#define DRP_SEQ_NUM                       (20)
#define DRP_EXE_DRP_40BIT                 (3)
#define DRP_OPMASK_FORCE_LOAD             (0x8000)
#define PARAM_ADDRESS_NUM                 (120)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct drp_data
{
    uint64_t        address;
    uint32_t        size;
} drp_data_t;

typedef struct drp_status
{
    uint32_t        status;
    int32_t         err;
    uint32_t        reserved[DRP_RESERVED_NUM];
} drp_status_t;

typedef struct iodata_info
{
    uint64_t        address;
    uint32_t        size;
    uint32_t        pos;
} iodata_info_st;

typedef struct drp_seq
{
    uint32_t        num;
    uint32_t        order[DRP_SEQ_NUM];
    uint64_t        address;
    uint32_t        iodata_num;
    iodata_info_st  iodata[PARAM_ADDRESS_NUM];
} drp_seq_t;

typedef struct drp_reg
{
    uint32_t        offset;
    uint32_t        value;
} drp_reg_t;

#ifdef __cplusplus
}
#endif

#endif /* _UAPI__DRP_H */
