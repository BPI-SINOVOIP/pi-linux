/*
 * Driver for the Renesas RZ/V2H, RZ/V2N DRP-AI unit
 *
 * Copyright (C) 2023-2024 Renesas Electronics Corporation
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

#ifndef DRP__H
#define DRP__H

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
#define R_DRP_SUCCESS                     (0)
#define R_DRP_ERR_INVALID_ARG             (-1)
#define R_DRP_ERR_INIT                    (-2)
#define R_DRP_ERR_INT                     (-3)
#define R_DRP_ERR_STOP                    (-4)
#define R_DRP_ERR_RESET                   (-5)
#define R_DRP_ERR_REG                     (-6)

/* reserved */
#define DRP_RESERVED_STP_ODIF_INTCNTO0    (0)
#define DRP_RESERVED_STP_ODIF_INTCNTO1    (1)
#define DRP_RESERVED_STP_ODIF_INTCNTO2    (2)
#define DRP_RESERVED_STP_ODIF_INTCNTO3    (3)
#define DRP_RESERVED_EXD0_ODIF_INTCNTO0   (4)
#define DRP_RESERVED_EXD0_ODIF_INTCNTO1   (5)
#define DRP_RESERVED_EXD1_ODIF_INTCNTO0   (6)
#define DRP_RESERVED_EXD1_ODIF_INTCNTO1   (7)
#define DRP_RESERVED_STP_DSCC_PAMON       (8)  /* for debug */
#define DRP_RESERVED_EXD0_DSCC_PAMON      (9)  /* for debug */
#define DRP_RESERVED_AID0_DSCC_PAMON      (10) /* for debug */
#define DRP_RESERVED_STP_ODIF_ELCPLS      (11) /* for debug (initial value) */
#define DRP_RESERVED_EXD0_ODIF_ELCPLS     (12) /* for debug (initial value) */
#define DRP_RESERVED_EXD1_ODIF_ELCPLS     (13) /* for debug (initial value) */

/* for CPG reset */
#define CPG_RESET_SUCCESS                   (0)
#define RST_MAX_TIMEOUT                     (100)

/* Debug macro (for only kernel) */
// #define DRP_DRV_DEBUG
#ifdef DRP_DRV_DEBUG
#define DRP_DEBUG_PRINT(fmt, ...) \
            pr_info("[%s: %d](pid: %d) "fmt, \
                            __func__, __LINE__, current->pid, ##__VA_ARGS__)
#else
#define DRP_DEBUG_PRINT(...)
#endif

//------------------------------------------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------------------------------------------
#ifdef __KERNEL__
typedef void __iomem* addr_t;
#else
typedef uint64_t addr_t;
#endif

typedef struct drp_odif_intcnto
{
    uint32_t    ch0;
    uint32_t    ch1;
    uint32_t    ch2;
    uint32_t    ch3;
} drp_odif_intcnto_t;

//------------------------------------------------------------------------------------------------------------------
// Prototype
//------------------------------------------------------------------------------------------------------------------
int32_t R_DRP_DRP_Open(addr_t drp_base_addr, int32_t ch, spinlock_t *lock);
int32_t R_DRP_DRP_Start(addr_t drp_base_addr, int32_t ch, uint64_t desc);
int32_t R_DRP_DRP_Stop(addr_t drp_base_addr, int32_t ch, spinlock_t *lock);
int32_t R_DRP_DRP_SetMaxFreq(addr_t drp_base_addr, int32_t ch, uint32_t mindiv);
int32_t R_DRP_DRP_Nmlint(addr_t drp_base_addr, int32_t ch, drp_odif_intcnto_t *odif_intcnto);
int32_t R_DRP_DRP_CLR_Nmlint(addr_t drp_base_addr, int32_t ch);
void R_DRP_DRP_Errint(addr_t drp_base_addr, int32_t ch);
int32_t R_DRP_AIMAC_Open(addr_t aimac_base_addr, int32_t ch);
int32_t R_DRP_AIMAC_Start(addr_t aimac_base_addr, int32_t ch, uint64_t cmd_desc, uint64_t param_desc);
int32_t R_DRP_AIMAC_Stop(addr_t aimac_base_addr, int32_t ch);
int32_t R_DRP_AIMAC_SetFreq(addr_t aimac_base_addr, int32_t ch, uint32_t divfix);
int32_t R_DRP_AIMAC_Nmlint(addr_t aimac_base_addr, int32_t ch);
int32_t R_DRP_AIMAC_Errint(addr_t drp_base_addr, addr_t aimac_base_addr, int32_t ch);
int32_t R_DRP_Status(addr_t drp_base_addr, addr_t aimac_base_addr, int32_t ch, uint32_t *reserved);
int32_t R_DRP_AIMAC_Reset(addr_t aimac_base_addr, int32_t ch);
int32_t R_DRP_DRP_RegRead(addr_t drp_base_addr, uint32_t offset, uint32_t* pvalue);
void    R_DRP_DRP_RegWrite(addr_t drp_base_addr, uint32_t offset, uint32_t value);
int32_t R_DRP_AIMAC_RegRead(addr_t aimac_base_addr, uint32_t offset, uint32_t* pvalue);
void    R_DRP_AIMAC_RegWrite(addr_t aimac_base_addr, uint32_t offset, uint32_t value);
int32_t R_DRP_DRP_SetAdrConv(addr_t drp_base_addr, int32_t ch, uint64_t* addr);
int32_t R_DRP_DRP_ResetDmaoffset(addr_t drp_base_addr, int32_t ch);
int32_t R_DRP_DRP_GetLastDescAddr(addr_t drp_base_addr, int32_t page, uint64_t* addr);
int32_t R_DRP_SetFreq(addr_t drp_base_addr, int32_t ch, uint32_t divfix);
int32_t R_DRP_DRP_RegRead(addr_t drp_base_addr, uint32_t offset, uint32_t* pvalue);
void    R_DRP_DRP_RegWrite(addr_t drp_base_addr, uint32_t offset, uint32_t value);
int32_t R_DRP_AIMAC_DisableAdrConv(addr_t aimac_base_addr, int32_t ch, uint64_t* addr);
int32_t R_DRP_AIMAC_SetAdrConv(addr_t aimac_base_addr, int32_t ch, uint64_t* addr);
int32_t R_DRP_DRP_IsActFieldOfDsccDctlZero(addr_t drp_base_addr);
int32_t initialize_cpg_drp(addr_t cpg_base_addr);

#endif /* DRP__H */
