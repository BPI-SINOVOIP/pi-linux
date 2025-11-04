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

#ifndef DRPAI__H
#define DRPAI__H

//------------------------------------------------------------------------------------------------------------------
// include
//------------------------------------------------------------------------------------------------------------------
#ifdef __KERNEL__
#include <linux/types.h>  /* for stdint */
#else
#include <stdint.h>
#endif

//------------------------------------------------------------------------------------------------------------------
// Macro
//------------------------------------------------------------------------------------------------------------------
#define R_DRPAI_SUCCESS                     (0)
#define R_DRPAI_ERR_INVALID_ARG             (-1)
#define R_DRPAI_ERR_INIT                    (-2)
#define R_DRPAI_ERR_INT                     (-3)
#define R_DRPAI_ERR_STOP                    (-4)
#define R_DRPAI_ERR_RESET                   (-5)

/* reserved */
#define DRPAI_RESERVED_STP_DSCC_PAMON       (0)
#define DRPAI_RESERVED_EXD0_DSCC_PAMON      (1)
#define DRPAI_RESERVED_AID0_DSCC_PAMON      (2)
#define DRPAI_RESERVED_ADRCONV_CTL          (3)
#define DRPAI_RESERVED_EXD0_ADRCONV_CTL     (4)
#define DRPAI_RESERVED_SYNCTBL_TBL13        (5)
#define DRPAI_RESERVED_SYNCTBL_TBL14        (6)
#define DRPAI_RESERVED_SYNCTBL_TBL15        (7)

/* reserved AIMAC Nmlint */
#define DRPAI_RESERVED_INTMON_INT           (16)
#define DRPAI_RESERVED_EXD0_STPC_INT_STS    (17)
#define DRPAI_RESERVED_EXD0_ODMACIF         (18)
#define DRPAI_RESERVED_EXD1_STPC_INT_STS    (19)
#define DRPAI_RESERVED_EXD1_ODMACIF         (20)
#define DRPAI_RESERVED_EXD0_ODIF_INTCNTO0   (21)
#define DRPAI_RESERVED_EXD0_ODIF_INTCNTO1   (22)
#define DRPAI_RESERVED_EXD1_ODIF_INTCNTO0   (23)
#define DRPAI_RESERVED_EXD1_ODIF_INTCNTO1   (24)

/* for CPG reset */
#define CPG_RESET_SUCCESS                   (0)
#define RST_MAX_TIMEOUT                     (100)

/* Debug macro (for only kernel) */
// #define DRPAI_DRV_DEBUG
#ifdef DRPAI_DRV_DEBUG
#define DRPAI_DEBUG_PRINT(fmt, ...) \
            pr_info("[%s: %d](pid: %d) "fmt, \
                            __func__, __LINE__, current->pid, ##__VA_ARGS__)
#else
#define DRPAI_DEBUG_PRINT(...)
#endif

//------------------------------------------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------------------------------------------
#ifdef __KERNEL__
typedef void __iomem* addr_t;
#else
typedef uint64_t addr_t;
#endif

typedef struct drpai_odif_intcnto
{
    uint32_t    ch0;
    uint32_t    ch1;
    uint32_t    ch2;
    uint32_t    ch3;
} drpai_odif_intcnto_t;

//------------------------------------------------------------------------------------------------------------------
// Prototype
//------------------------------------------------------------------------------------------------------------------
void    R_DRPAI_DRP_Open(addr_t drp_base_addr, int32_t ch);
void    R_DRPAI_DRP_Start(addr_t drp_base_addr, int32_t ch, uint64_t desc);
int32_t R_DRPAI_DRP_Stop(addr_t drp_base_addr, int32_t ch);
int32_t R_DRPAI_DRP_SetMaxFreq(addr_t drp_base_addr, int32_t ch, uint32_t mindiv);
int32_t R_DRPAI_DRP_EnableAddrReloc(addr_t drp_base_addr, uint8_t* tbl);
void    R_DRPAI_DRP_DisableAddrReloc(addr_t drp_base_addr);
int32_t R_DRPAI_DRP_Reset(addr_t drp_base_addr, int32_t ch);
void    R_DRPAI_DRP_Errint(addr_t drp_base_addr, addr_t aimac_base_addr, int32_t ch);
void    R_DRPAI_AIMAC_Open(addr_t aimac_base_addr, int32_t ch);
void    R_DRPAI_AIMAC_Start(addr_t aimac_base_addr, int32_t ch, uint64_t cmd_desc, uint64_t param_desc);
int32_t R_DRPAI_AIMAC_Stop(addr_t aimac_base_addr, int32_t ch);
int32_t R_DRPAI_AIMAC_SetFreq(addr_t aimac_base_addr, int32_t ch, uint32_t divfix);
int32_t R_DRPAI_AIMAC_EnableAddrReloc(addr_t aimac_base_addr, uint8_t* tbl);
void    R_DRPAI_AIMAC_DisableAddrReloc(addr_t aimac_base_addr);
int32_t R_DRPAI_AIMAC_Reset(addr_t aimac_base_addr, int32_t ch);
int32_t R_DRPAI_AIMAC_Nmlint(addr_t aimac_base_addr, int32_t ch, uint32_t* reserved);
void    R_DRPAI_AIMAC_Errint(addr_t drp_base_addr, addr_t aimac_base_addr, int32_t ch);
void    R_DRPAI_Status(addr_t drp_base_addr, addr_t aimac_base_addr, int32_t ch, uint32_t* reserved);
int32_t R_DRPAI_DRP_RegRead(addr_t drp_base_addr, uint32_t offset, uint32_t* pvalue);
void    R_DRPAI_DRP_RegWrite(addr_t drp_base_addr, uint32_t offset, uint32_t value);
int32_t R_DRPAI_AIMAC_RegRead(addr_t aimac_base_addr, uint32_t offset, uint32_t* pvalue);
void    R_DRPAI_AIMAC_RegWrite(addr_t aimac_base_addr, uint32_t offset, uint32_t value);
int32_t R_DRPAI_DRP_IsActFieldOfDsccDctlZero(addr_t drp_base_addr);

int32_t initialize_cpg(addr_t cpg_base_addr);

#endif /* DRPAI__H */
