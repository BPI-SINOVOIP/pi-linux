/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Driver for the Renesas RZ/V2H, RZ/V2N DRP-AI unit
 *
 * Copyright (C) 2024 Renesas Electronics Corporation
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

#ifndef _UAPI__DRPAI_H
#define _UAPI__DRPAI_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif
#include <linux/ioctl.h>

#define DRPAI_IO_TYPE               (46)
#define DRPAI_ASSIGN                _IOW (DRPAI_IO_TYPE, 0, drpai_data_t)
#define DRPAI_START                 _IOW (DRPAI_IO_TYPE, 1, drpai_data_t)
#define DRPAI_RESET                 _IO  (DRPAI_IO_TYPE, 2)
#define DRPAI_GET_STATUS            _IOR (DRPAI_IO_TYPE, 3, drpai_status_t)
#define DRPAI_REG_DUMP              _IO  (DRPAI_IO_TYPE, 5)
#define DRPAI_ASSIGN_PARAM          _IOW (DRPAI_IO_TYPE, 6, drpai_assign_param_t)  /* Since the sturecture size is different,       */
                                                                                   /* it will be a different ID from DRPAI_SET_SEQ. */
#define DRPAI_PREPOST_CROP          _IOW (DRPAI_IO_TYPE, 7, drpai_crop_t)
#define DRPAI_PREPOST_INADDR        _IOW (DRPAI_IO_TYPE, 8, drpai_inout_t)
#define DRPAI_SET_SEQ               _IOW (DRPAI_IO_TYPE, 6, drpai_seq_t)           /* Since the sturecture size is different,            */
                                                                                   /* it will be a different ID from DRPAI_ASSIGN_PARAM. */
#define DRPAI_GET_DRPAI_AREA        _IOR (DRPAI_IO_TYPE, 11, drpai_data_t)

#define DRPAI_SET_DRP_MAX_FREQ      _IOW (DRPAI_IO_TYPE, 12, uint32_t)
#define DRPAI_SET_DRPAI_FREQ        _IOW (DRPAI_IO_TYPE, 13, uint32_t)

#define DRPAI_SET_ADRCONV           _IOW (DRPAI_IO_TYPE, 14, drpai_adrconv_t)
#define DRPAI_SET_EXTRA_AREA        _IOW (DRPAI_IO_TYPE, 15, uint32_t)

#define DRPAI_READ_DRP_REG          _IOWR(DRPAI_IO_TYPE, 64, drpai_reg_t)
#define DRPAI_WRITE_DRP_REG         _IOW (DRPAI_IO_TYPE, 65, drpai_reg_t)
#define DRPAI_READ_DRPAI_REG        _IOWR(DRPAI_IO_TYPE, 66, drpai_reg_t)
#define DRPAI_WRITE_DRPAI_REG       _IOW (DRPAI_IO_TYPE, 67, drpai_reg_t)
#define DRPAI_READ_CPG_REG          _IOWR(DRPAI_IO_TYPE, 68, drpai_reg_t)
#define DRPAI_WRITE_CPG_REG         _IOW (DRPAI_IO_TYPE, 69, drpai_reg_t)

#define DRPAI_INDEX_NUM                     (10)
#define DRPAI_INDEX_INPUT                   (0)
#define DRPAI_INDEX_DRP_DESC                (1)
#define DRPAI_INDEX_DRP_CFG                 (2)
#define DRPAI_INDEX_DRP_PARAM               (3)
#define DRPAI_INDEX_AIMAC_DESC              (4)
#define DRPAI_INDEX_WEIGHT                  (5)
#define DRPAI_INDEX_OUTPUT                  (6)
#define DRPAI_INDEX_AIMAC_CMD               (7)
#define DRPAI_INDEX_AIMAC_PARAM_DESC        (8)
#define DRPAI_INDEX_AIMAC_PARAM_CMD         (9)
#define DRPAI_STATUS_INIT                   (0)
#define DRPAI_STATUS_IDLE                   (1)
#define DRPAI_STATUS_RUN                    (2)
#define DRPAI_ERRINFO_SUCCESS               (0)
#define DRPAI_ERRINFO_DRP_ERR               (-1)
#define DRPAI_ERRINFO_AIMAC_ERR             (-2)
#define DRPAI_ERRINFO_RESET                 (-3)
#define DRPAI_RESERVED_NUM                  (32)
#define DRPAI_SEQ_NUM                       (20)
#define DRPAI_EXE_AI                        (1)
#define DRPAI_EXE_DRP                       (2)
#define DRPAI_DRP_NOLOAD                    (0)
#define DRPAI_MAX_NODE_NAME                 (256)
#define DRPAI_ADRCONV_MODE_REPLACE          (0)
#define DRPAI_ADRCONV_MODE_ADD              (1)
#define DRPAI_ADRCONV_MODE_DISABLE          (0xFF)
#define DRPAI_ADRCONV_MODE_ALL_CLEAR        (0xFF)


#ifdef __cplusplus
extern "C" {
#endif

typedef struct drpai_data
{
    uint64_t        address;
    uint64_t        size;
} drpai_data_t;

typedef struct drpai_status
{
    uint32_t        status;
    int32_t         err;
    uint32_t        reserved[DRPAI_RESERVED_NUM];
} drpai_status_t;

typedef struct drpai_assign_param
{
    uint32_t        info_size;
    drpai_data_t    obj;
} drpai_assign_param_t;

typedef struct drpai_crop
{
    uint16_t        img_owidth;
    uint16_t        img_oheight;
    uint16_t        pos_x;
    uint16_t        pos_y;
    drpai_data_t    obj;
} drpai_crop_t;

typedef struct drpai_inout
{
    char            name[DRPAI_MAX_NODE_NAME];
    drpai_data_t    data;
    drpai_data_t    obj;
} drpai_inout_t;

typedef struct drpai_seq
{
    uint32_t        num;
    uint32_t        order[DRPAI_SEQ_NUM];
} drpai_seq_t;

typedef struct drpai_adrconv
{
    uint32_t        org_address;
    uint32_t        size;
    uint64_t        conv_address;
    uint8_t         mode;
} drpai_adrconv_t;

typedef struct drpai_reg
{
    uint32_t        offset;
    uint32_t        value;
} drpai_reg_t;

#ifdef __cplusplus
}
#endif

#endif /* _UAPI__DRPAI_H */
