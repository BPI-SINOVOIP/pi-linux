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
    #define        AVIO_MEMMAP_VPP128B_DHUB_REG_BASE           0x0
    #define        AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_BASE       0x40000
    #define        AVIO_MEMMAP_VPP_MP_REG_BASE                 0x60000
    #define        AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE            0x80000
    #define        AVIO_MEMMAP_VPP_REG_BASE                    0x90000
    #define        AVIO_MEMMAP_AVIO_GBL_BASE                   0x98000
    #define        AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE            0xA0000
    #define        AVIO_MEMMAP_AVIO_EARC_RX_REG_BASE           0xA8000
    #define        AVIO_MEMMAP_AVIO_I2S_REG_BASE               0xAC000
    #define        AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE        0xAC400
    #define        AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_BASE       0xAC800
    #define        AVIO_MEMMAP_AVIO_BCM_REG_BASE               0xACC00
    #define        AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE       0xAD000
    #define        AVIO_MEMMAP_AVIO_SNPS_TRNG_REG_BASE         0xAD400
    #define        AVIO_MEMMAP_AVIO_HDMIRX_WRAP_REG_BASE       0xAD800
    #define        AVIO_MEMMAP_AIO64B_DHUB_REG_BASE            0xB0000
    #define        AVIO_MEMMAP_AVIO_RESERVED0_REG_BASE         0xC0000
    #define        AVIO_MEMMAP_AVIO_RESERVED1_REG_BASE         0x100000
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
