/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef avio_memmap_h
#define avio_memmap_h (){}


#include "ctypes.h"

#pragma pack(1)
#ifdef __cplusplus
  extern "C" {
#endif

#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ (){}

    #define _bSETMASK_(b)                                      ((b)<32 ? (1<<((b)&31)) : 0)
    #define _NSETMASK_(msb,lsb)                                (_bSETMASK_((msb)+1)-_bSETMASK_(lsb))
    #define _bCLRMASK_(b)                                      (~_bSETMASK_(b))
    #define _NCLRMASK_(msb,lsb)                                (~_NSETMASK_(msb,lsb))
    #define _BFGET_(r,msb,lsb)                                 (_NSETMASK_((msb)-(lsb),0)&((r)>>(lsb)))
    #define _BFSET_(r,msb,lsb,v)                               do{ (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); }while(0)

#endif



#ifndef h_AVIO_MEMMAP
#define h_AVIO_MEMMAP (){}

    #define        AVIO_MEMMAP_AIO64B_DHUB_REG_BASE            0x0
    #define        AVIO_MEMMAP_AIO64B_DHUB_REG_SIZE            0x20000
    #define        AVIO_MEMMAP_AIO64B_DHUB_REG_DEC_BIT         0x11
    #define        AVIO_MEMMAP_AVIO_GBL_BASE                   0x20000
    #define        AVIO_MEMMAP_AVIO_GBL_SIZE                   0x10000
    #define        AVIO_MEMMAP_AVIO_GBL_DEC_BIT                0x10
    #define        AVIO_MEMMAP_AVIO_BCM_REG_BASE               0x30000
    #define        AVIO_MEMMAP_AVIO_BCM_REG_SIZE               0x10000
    #define        AVIO_MEMMAP_AVIO_BCM_REG_DEC_BIT            0x10
    #define        AVIO_MEMMAP_AVIO_I2S_REG_BASE               0x40000
    #define        AVIO_MEMMAP_AVIO_I2S_REG_SIZE               0x10000
    #define        AVIO_MEMMAP_AVIO_I2S_REG_DEC_BIT            0x10
    #define        AVIO_MEMMAP_AVIO_RESERVED0_REG_BASE         0x50000
    #define        AVIO_MEMMAP_AVIO_RESERVED0_REG_SIZE         0x20000
    #define        AVIO_MEMMAP_AVIO_RESERVED0_REG_DEC_BIT      0x11
    #define        AVIO_MEMMAP_AVIO_RESERVED1_REG_BASE         0x70000
    #define        AVIO_MEMMAP_AVIO_RESERVED1_REG_SIZE         0x8000
    #define        AVIO_MEMMAP_AVIO_RESERVED1_REG_DEC_BIT      0xF

#endif



#ifdef __cplusplus
  }
#endif
#pragma  pack()

#endif

