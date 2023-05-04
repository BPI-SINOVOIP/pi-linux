// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#ifndef ispDhub_h
#define ispDhub_h (){}

#define     ISPSS_MEMMAP_TSB_DHUB_REG_BASE              0x0
#define     ISPSS_MEMMAP_FWR_DHUB_REG_BASE              0x20000
#define     ISPSS_MEMMAP_BCM_REG_BASE                   0x70000

#define     RA_ispDhubTSB_tcm0                          0x0000
#define     RA_ispDhubTSB_dHub0                         0x4000
#define     RA_ispDhubFWR_tcm0                          0x0000
#define     RA_ispDhubFWR_dHub0                         0x10000

#define     RA_SemaHub_ARR                              0x0100
#define     RA_SemaHub_cell                             0x0100

#define     RA_SemaHub_POP                              0x0404
#define     RA_SemaHub_full                             0x040C
#define     Semaphore_cell_size                         0x18

#define     ispDhubSemMap_TSB_ispIntr                   0x10
#define     ispDhubSemMap_TSB_miIntr                    0x11
#define     ispDhubSemMap_TSB_mipi0Intr                 0x12
#define     ispDhubSemMap_TSB_mipi1Intr                 0x13
#define     ispDhubSemMap_TSB_Fdet_intr0                0x14
#define     ispDhubSemMap_TSB_Fdet_intr1                0x15
#define     ispDhubSemMap_TSB_Scldn_intr0               0x16
#define     ispDhubSemMap_TSB_Scldn_intr1               0x17
#define     ispDhubSemMap_TSB_Scldn_intr2               0x18
#define     ispDhubSemMap_TSB_dwrp_intr                 0x19
#define     ispDhubSemMap_TSB_rot_intr                  0x1A
#define     ispDhubSemMap_TSB_tile_intr                 0x1B
#define     ispDhubSemMap_TSB_bcmInvalidIntr            0x1C

#endif
