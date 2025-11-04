/*
 * Driver for the Renesas RZ/V2H, RZ/V2N DRP unit
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

#ifdef __KERNEL__
#include <linux/types.h>  /* for stdint */
#include <asm/io.h>       /* for ioread/iowrite */
#include <linux/delay.h>  /* for mdelay */
#include <linux/module.h> /* for MODULE macro */
#else
#include <stdint.h>
#endif

#include "drp-core.h"
#include "drp-reg.h"

//------------------------------------------------------------------------------------------------------------------
// Parameter setting
//------------------------------------------------------------------------------------------------------------------
#define NUT_VERSION                 "2023.02.24.a"

#define DRP_DFCENA                  (0x1)
#define DRP_DIVFIX                  (0x02)
#define DRP_MIN_DIVFIX              (0x02)
#define DRP_ADDR_RELOCATABLE_ENA    (0x0)
#define AIMAC_DIVFIX                (0x01)
#define AIMAC_ADDR_RELOCATABLE_ENA  (0x0)
#define DRP_ECC_ENA                 (0x1)
#define DRP_CDCC_PRERE              (0x1)
#define DRP_CDCC_HFCFGW             (0x1)
#define INIT_SET_BP_MODE            (0)
#define INIT_SET_BP_CSTART          (0)
#define INIT_SET_BP_ITR             (0)
#define INIT_SET_BP_INST            (0)
#define STP_ERRINT_STATUS_REG_NUM   (5)
#define AIMAC_ERRINT_STATUS_REG_NUM (8)
#define EXDx_ERRINT_STATUS_REG_NUM  (4)
#define ENABLE_BUILD_SET_DRP_DBGCTRL_FUNC  (0)  // If 0, disable to build set_drp_dbgctrl()

//------------------------------------------------------------------------------------------------------------------
// Do not change
//------------------------------------------------------------------------------------------------------------------
#define DEF_DRP_SIDE                (0)
#define DEF_AIMAC_SIDE              (1)


//------------------------------------------------------------------------------------------------------------------
// for Linux
//------------------------------------------------------------------------------------------------------------------
#ifndef __KERNEL__
#define ioread32(addr)              (*((volatile uint32_t *)(addr)))
#define ioread16(addr)              (*((volatile uint16_t *)(addr)))
#define ioread8(addr)               (*((volatile uint8_t *)(addr)))
#define iowrite32(value, addr)      (*((volatile uint32_t *)(addr)) = (uint32_t)(value))
#define iowrite16(value, addr)      (*((volatile uint16_t *)(addr)) = (uint16_t)(value))
#define iowrite8(value, addr)       (*((volatile uint8_t *)(addr))  = (uint8_t)(value))
#endif

//------------------------------------------------------------------------------------------------------------------
// Prototype
//------------------------------------------------------------------------------------------------------------------
static void set_drpclkgen_freq(addr_t drp_base_addr, uint32_t bit_divfix, uint32_t bit_dfcena);
static void start_drp_clk(addr_t drp_base_addr);
static void stop_drp_clk(addr_t drp_base_addr);
static void disable_drp_swreset(addr_t drp_base_addr);
static void enable_drp_swreset(addr_t drp_base_addr);
static void enable_addr_relocatable_drp(addr_t drp_base_addr, uint64_t* addr);
static void disable_adrconv_drp(addr_t drp_base_addr);
static void enable_adrconv_drp(addr_t drp_base_addr);
#if defined(CONFIG_ARCH_R9A09G056)
static void disable_addr_relocatable_func(addr_t drp_base_addr, addr_t aimac_base_addr);
#else
static void disable_addr_relocatable_func(addr_t drp_base_addr);
#endif
static void disable_addr_relocatable(addr_t addr);
static int32_t stop_desc_prefetch(addr_t dmactl_addr);
static int32_t dma_stop(addr_t dmactl_addr);
static void start_drp_dmac(addr_t drp_base_addr);
static int32_t stop_drp_dmac(addr_t drp_base_addr);
static int32_t stop_drp_dmacw(addr_t drp_base_addr);
static void disable_drp_intmask(addr_t drp_base_addr);
static void enable_drp_intmask(addr_t drp_base_addr);
static void setdmaoffset_drp(addr_t drp_base_addr);
static void drp_bootseq_drp(addr_t drp_base_addr, uint32_t *drp_addr_relocatable_tbl);
static void start_prefetch_drp_drp(addr_t drp_base_addr);
static int32_t stop_prefetch_drp_drp(addr_t drp_base_addr);
static void aimac_bootseq(addr_t aimac_base_addr, uint8_t *aimac_addr_relocatable_tbl);
static void enable_addr_relocatable_aimac(addr_t aimac_base_addr, uint64_t* addr);
static void disable_adrconv_aimac(addr_t aimac_base_addr);
static void enable_adrconv_aimac(addr_t aimac_base_addr);
static void set_drp_desc_drp(addr_t drp_base_addr, uint64_t drp_desc_addr);
static void set_aimac_desc(addr_t aimac_base_addr, uint64_t cmd_desc_addr, uint64_t param_desc_addr);
static void start_prefetch_aimac(addr_t aimac_base_addr);
static int32_t stop_prefetch_aimac(addr_t aimac_base_addr);
static int32_t aimac_finalize(addr_t aimac_base_addr);
static void set_aimac_freq(addr_t aimac_base_addr, uint32_t divfix);
static void start_aimac_dmac(addr_t aimac_base_addr);
static void disable_aimac_intmask(addr_t aimac_base_addr);
static int32_t stop_aimac_dmac(addr_t aimac_base_addr);
static void enable_addr_relocatable(addr_t addr, uint8_t* tbl);

static void set_drp_maxfreq(addr_t drp_base_addr, uint32_t mindiv);

static int32_t loop_w(addr_t raddr, uint32_t exp_data, uint32_t mask);
static int32_t check_reg(addr_t raddr, uint32_t exp, uint32_t mask0, uint32_t mask1);
static void cpg_set(addr_t addr, int32_t bit, uint32_t val);
static int32_t cpg_check(addr_t addr, int32_t bit, uint32_t val);
#ifndef __KERNEL__
static void mdelay(uint32_t msecs);
#endif

const static uint32_t stp_errint_status_reg_tbl[STP_ERRINT_STATUS_REG_NUM] =
{
    DRP_ERRINT_STATUS_ADDR, IDIF_EINT_ADDR, ODIF_EINT_ADDR, IDMAC_INTSE_ADDR, ODMAC_INTSE_ADDR,
};

const static char* stp_errint_status_reg_name_tbl[STP_ERRINT_STATUS_REG_NUM] =
{
    "DRP_ERRINT_STATUS","IDIF_EINT", "ODIF_EINT", "IDMAC_INTSE", "ODMAC_INTSE",
};

//------------------------------------------------------------------------------------------------------------------
// IF functions
//------------------------------------------------------------------------------------------------------------------
int32_t R_DRP_DRP_Open(addr_t drp_base_addr, int32_t ch, spinlock_t *lock)
{
    drp_bootseq_drp(drp_base_addr, 0);
    return R_DRP_SUCCESS;
}

int32_t R_DRP_AIMAC_Open(addr_t aimac_base_addr, int32_t ch)
{
    aimac_bootseq(aimac_base_addr, 0);
    return R_DRP_SUCCESS;
}

int32_t R_DRP_DRP_SetAdrConv(addr_t drp_base_addr, int32_t ch, uint64_t* addr)
{

    disable_adrconv_drp(drp_base_addr);
    enable_addr_relocatable_drp(drp_base_addr, addr);
    enable_adrconv_drp(drp_base_addr);
    
    return R_DRP_SUCCESS;
}

int32_t R_DRP_AIMAC_SetAdrConv(addr_t aimac_base_addr, int32_t ch, uint64_t* addr)
{

    disable_adrconv_aimac(aimac_base_addr);
    enable_addr_relocatable_aimac(aimac_base_addr, addr);
    enable_adrconv_aimac(aimac_base_addr);

    return R_DRP_SUCCESS;
}

int32_t R_DRP_AIMAC_DisableAdrConv(addr_t aimac_base_addr, int32_t ch, uint64_t* addr)
{
    disable_adrconv_aimac(aimac_base_addr);
    return R_DRP_SUCCESS;
}

int32_t R_DRP_DRP_ResetDmaoffset(addr_t drp_base_addr, int32_t ch)
{

    setdmaoffset_drp(drp_base_addr);
    
    return R_DRP_SUCCESS;
}

int32_t R_DRP_SetFreq(addr_t drp_base_addr, int32_t ch, uint32_t divfix)
{
    set_drp_maxfreq(drp_base_addr, divfix);
    return R_DRP_SUCCESS;
}

int32_t R_DRP_DRP_GetLastDescAddr(addr_t drp_base_addr, int32_t page, uint64_t* addr)
{
    uint32_t r_data0, addr_conv_en, desc_page, desc_tbl_data;
    int32_t ret = R_DRP_ERR_REG;
    /* Check Addr Conv is enabled */
    addr_conv_en = ioread32(drp_base_addr + STP_ADRCONV_BADDR + ADRCONV_TBL_EN);
    if( ( addr_conv_en & 0x00000001) == 0x00000001) // Addr conv function is Enabled
    {
        /* Get Descriptor address */
        r_data0 = ioread32(drp_base_addr + STP_DSCC_BADDR + DSCC_DPA_REG);      // Descriptor addr bit0  - 32
        desc_page = (r_data0 >> 24) & 0xFF;
        if( desc_page == page ){
            desc_tbl_data = ioread32(drp_base_addr + STP_ADRCONV_BADDR + ADRCONV_TBL + page * 4);  //'4' means 'sizeof(ADRCONV_TBLn register)'
            if( (desc_tbl_data & 0x00000071) == 0x00000001)  // Addr conv (each page) is Enabled, and Page size = 16MB
            {
                *addr = ((uint64_t)(desc_tbl_data & 0x0000FF00)) << 24 | (desc_tbl_data & 0xFF000000) | (r_data0 & 0x00FFFFFF);
                ret = R_DRP_SUCCESS;
            }
        }
    }

    return ret;
}

int32_t R_DRP_DRP_Start(addr_t drp_base_addr, int32_t ch, uint64_t desc)
{
    set_drp_desc_drp(drp_base_addr, desc);
    start_prefetch_drp_drp(drp_base_addr);
    return R_DRP_SUCCESS;
}

int32_t R_DRP_AIMAC_Start(addr_t aimac_base_addr, int32_t ch, uint64_t cmd_desc, uint64_t param_desc)
{
    set_aimac_desc(aimac_base_addr, cmd_desc, param_desc);
    start_prefetch_aimac(aimac_base_addr);
    return R_DRP_SUCCESS;
}

int32_t R_DRP_DRP_Stop(addr_t drp_base_addr, int32_t ch, spinlock_t *lock)
{
    int32_t ret = R_DRP_SUCCESS;

    if (0 != stop_prefetch_drp_drp(drp_base_addr))
    {
        ret = R_DRP_ERR_RESET;
        goto end;
    }

    goto end;

end:
    return ret;
}

int32_t R_DRP_AIMAC_Reset(addr_t aimac_base_addr, int32_t ch)
{
    int32_t ret = R_DRP_SUCCESS;

    if (0 != stop_prefetch_aimac(aimac_base_addr))
    {
        ret = R_DRP_ERR_RESET;
        goto end;
    }
    if (0 != aimac_finalize(aimac_base_addr))
    {
        ret = R_DRP_ERR_RESET;
        goto end;
    }

    goto end;

end:
    return ret;
}

int32_t R_DRP_DRP_Nmlint(addr_t drp_base_addr, int32_t ch, drp_odif_intcnto_t *odif_intcnto)
{
    uint32_t rdata;
    rdata = ioread32(drp_base_addr + STP_STPC_BADDR + STPC_INT_STS);
    
    if( 1 == ((rdata >> 9) & 1) )
    {
        rdata = ioread32(drp_base_addr + STP_ODIF_BADDR + ODIF_INT_REG);
        iowrite32(rdata,drp_base_addr + STP_ODIF_BADDR + ODIF_INT_REG);
        rdata = ioread32(drp_base_addr + STP_ODIF_BADDR + ODIF_INT_REG);    //Blank read

        odif_intcnto->ch0 = ioread32(drp_base_addr + STP_ODIF_BADDR + ODIF_INTCNTO0_REG);
    }
    else
    {
        odif_intcnto->ch0 = 0;
        odif_intcnto->ch1 = 0;
        odif_intcnto->ch2 = 0;
        odif_intcnto->ch3 = 0;
    }
    return R_DRP_SUCCESS;
}

int32_t R_DRP_DRP_CLR_Nmlint(addr_t drp_base_addr, int32_t ch)
{
    return stop_desc_prefetch(drp_base_addr + STP_DSCC_BADDR + DSCC_DCTLI0_REG);
}

void R_DRP_DRP_Errint(addr_t drp_base_addr, int32_t ch)
{
    volatile uint32_t dscc_pamon;
    volatile uint32_t stpc_errint_sts;
    uint32_t index;
    volatile uint32_t error_status;
    volatile uint32_t dummy;

    printk(KERN_ERR "DRP1 Error Interrupt\n");

    dscc_pamon      = ioread32(drp_base_addr   + STP_DSCC_BADDR  + DSCC_PAMON_REG);

    printk(KERN_ERR "DSCC_PAMON : 0x%08X\n",      dscc_pamon);

    stpc_errint_sts = ioread32(drp_base_addr + STP_STPC_ERRINT_STS);
    printk(KERN_ERR "STPC_ERRINT_STS : 0x%08X\n", stpc_errint_sts);


    // Enable interrupt masks to prevent multiple error interrupts.
    enable_drp_intmask(drp_base_addr);

    /**
     * 1. Check the processing completion criteria
     * (1) Identify the interrupt factor and clear the interrupt factor.
     */
    for (index = 0; index < STP_ERRINT_STATUS_REG_NUM; index++)
    {
        error_status = ioread32(drp_base_addr + stp_errint_status_reg_tbl[index]);
        iowrite32(error_status, drp_base_addr + stp_errint_status_reg_tbl[index]);
        dummy = ioread32(drp_base_addr + stp_errint_status_reg_tbl[index]);

        printk(KERN_ERR "%s : 0x%08X\n", stp_errint_status_reg_name_tbl[index], error_status);
    }

    /**
     * 2. Stop a prefetch of descriptor
     * (1) Set the DSCEN field of the DSCC_DCTL register to 0.
     */
    iowrite8(0x00, drp_base_addr + STP_DSCC_BADDR + DSCC_DCTLI0_REG);
}

int32_t R_DRP_DRP_RegRead(addr_t drp_base_addr, uint32_t offset, uint32_t* pvalue)
{
    int32_t ret;

    if (0 == pvalue)
    {
        ret = R_DRP_ERR_INVALID_ARG;
    }
    else
    {
        *pvalue = ioread32(drp_base_addr + offset);
        ret = R_DRP_SUCCESS;
    }

    return ret;
}

void R_DRP_DRP_RegWrite(addr_t drp_base_addr, uint32_t offset, uint32_t value)
{
    iowrite32(value, drp_base_addr + offset);
}

/** Check whether ACT field of the DSCC_DCTL register is 0.
 * This function is implemented according to Section 7.2.2 - (2) of UM
 * `(2) Repeat the reading until the ACT field of the DSCC_DCTL register is 0.`
 * @param drp_base_addr DRP base address
 * @return Whether ACT field of the DSCC_DCTL register is 0 or not.
 * @retval 0 ACT field is 0.
 * @retval -1 ACT field is not 0.
 */
int32_t R_DRP_DRP_IsActFieldOfDsccDctlZero(addr_t drp_base_addr)
{
    addr_t dmactl_addr = drp_base_addr + STP_DSCC_BADDR + DSCC_DCTLI0_REG;
    uint32_t rdata;
    uint32_t exp_data = 0x00000000;
    uint32_t mask = ~(0xFFFFFFFD);
    int32_t result = -1;

    rdata = ioread32(dmactl_addr);
    if ((rdata & mask) == (exp_data & mask))
    {
        result = 0;
    }

    return result;
}

//------------------------------------------------------------------------------------------------------------------
// CLKGEN module setting
//------------------------------------------------------------------------------------------------------------------

static void set_drpclkgen_freq(addr_t drp_base_addr, uint32_t bit_divfix, uint32_t bit_dfcena)
{
    uint32_t BIT_STBYWT;

    BIT_STBYWT = 0x1;
    iowrite32(BIT_STBYWT,                       drp_base_addr + STP_STPC_BADDR + STPC_CLKGEN_STBYWAIT);
    iowrite32((bit_divfix << 16) | bit_dfcena,  drp_base_addr + STP_STPC_BADDR + STPC_CLKGEN_DIV);
    BIT_STBYWT = 0x0;
    iowrite32(BIT_STBYWT,                       drp_base_addr + STP_STPC_BADDR + STPC_CLKGEN_STBYWAIT);
}

static void stop_aimac_clk(addr_t aimac_base_addr)
{
    iowrite32(0x00000000, aimac_base_addr + EXD0_STPC_BADDR + STPC_CLKE);
    iowrite32(0x00000000, aimac_base_addr + EXD1_STPC_BADDR + STPC_CLKE);
    iowrite32(0x00000000, aimac_base_addr + CLKRSTCON_BADDR + CLKRSTCON_CLKE_REG);
}

//------------------------------------------------------------------------------------------------------------------
// Enable/Disable debug function
//------------------------------------------------------------------------------------------------------------------
#if ENABLE_BUILD_SET_DRP_DBGCTRL_FUNC
static void set_drp_dbgctrl(addr_t drp_base_addr)
{
    set_drp_maxfreq(drp_base_addr, DRP_MIN_DIVFIX);
    iowrite32((DRP_CDCC_PRERE << 8) + 0x00000001,   drp_base_addr + DRP_CDCC_DBGCTL);
    iowrite32(0x00070000 + DRP_CDCC_HFCFGW,         drp_base_addr + DRP_CDCC_CCCTL);
}
#endif

static void stop_mclkgen(addr_t aimac_base_addr)
{
    uint32_t BIT_MCLKGEN = 0x01;

    iowrite32(0x7F << 16,   aimac_base_addr + EXD0_STPC_BADDR + STPC_CLKSW_CONFIG);
    iowrite32(BIT_MCLKGEN,  aimac_base_addr + EXD0_STPC_BADDR + STPC_CLKGEN_RST);
}

//------------------------------------------------------------------------------------------------------------------
// Enable/Disable DMA channel clock
//------------------------------------------------------------------------------------------------------------------

static void start_drp_clk(addr_t drp_base_addr)
{
    uint32_t BIT_CMN_CLKE  = 0x1;
    uint32_t BIT_STBL_CLKE = 0x1;
    uint32_t BIT_DSCC_CLKE = 0x1;
    uint32_t BIT_CFGW_CLKE = 0x1;
    uint32_t BIT_CHOx_CLKE = 0xF;
    uint32_t BIT_CHIx_CLKE = 0xF;

    iowrite32((BIT_STBL_CLKE << 29) |
              (BIT_CMN_CLKE  << 28) |
              (BIT_CHOx_CLKE << 16) |
              (BIT_DSCC_CLKE <<  9) |
              (BIT_CFGW_CLKE <<  8) |
              (BIT_CHIx_CLKE <<  0),    drp_base_addr + STP_STPC_BADDR + STPC_CLKE);
}

static void stop_drp_clk(addr_t drp_base_addr)
{
    uint32_t BIT_CMN_CLKE  = 0x0;
    uint32_t BIT_STBL_CLKE = 0x0;
    uint32_t BIT_DSCC_CLKE = 0x0;
    uint32_t BIT_CFGW_CLKE = 0x0;
    uint32_t BIT_CHOx_CLKE = 0x0;
    uint32_t BIT_CHIx_CLKE = 0x0;

    iowrite32(0xFFFFFFFF & ((BIT_STBL_CLKE << 29) |
                            (BIT_CMN_CLKE  << 28) |
                            (BIT_CHOx_CLKE << 16) |
                            (BIT_DSCC_CLKE <<  9) |
                            (BIT_CFGW_CLKE <<  8) |
                            (BIT_CHIx_CLKE <<  0)), drp_base_addr + STP_STPC_BADDR + STPC_CLKE);
}

static void start_aimac_clk(addr_t aimac_base_addr)
{
    uint32_t BIT_CMN_CLKE  = 0x1;
    uint32_t BIT_DSCC_CLKE = 0x1;
    uint32_t BIT_MCMD_CLKE = 0x1;
    uint32_t BIT_CHOx_CLKE = 0x3;
    uint32_t BIT_CHIx_CLKE = 0x3;
    uint32_t BIT_MCLK_AIM  = 0x1;
    uint32_t BIT_DCLK_AIM  = 0x1;
    uint32_t BIT_ACLK_AIM  = 0x1;

    iowrite32((BIT_CMN_CLKE  << 28) |
              (BIT_CHOx_CLKE << 16) |
              (BIT_DSCC_CLKE <<  9) |
              (BIT_MCMD_CLKE <<  2) |
              (BIT_CHIx_CLKE <<  0),    aimac_base_addr + EXD0_STPC_BADDR + STPC_CLKE);

    iowrite32((BIT_CMN_CLKE  << 28) |
              (BIT_CHOx_CLKE << 16) |
              (BIT_CHIx_CLKE <<  0),    aimac_base_addr + EXD1_STPC_BADDR + STPC_CLKE);

    iowrite32((BIT_MCLK_AIM << 2) |
              (BIT_DCLK_AIM << 1) |
              (BIT_ACLK_AIM << 0),      aimac_base_addr + CLKRSTCON_BADDR + CLKRSTCON_CLKE_REG);
}

//------------------------------------------------------------------------------------------------------------------
// Enable/Disable SW reset
//------------------------------------------------------------------------------------------------------------------

static void disable_drp_swreset(addr_t drp_base_addr)
{
    uint32_t BIT_DRP_RST    = 0x0;
    uint32_t BIT_DRPOIF_RST = 0x0;
    uint32_t BIT_DRPIIF_RST = 0x0;
    uint32_t BIT_CMN_RST    = 0x0;
    uint32_t BIT_STBL_RST   = 0x0;
    uint32_t BIT_DSCC_RST   = 0x0;
    uint32_t BIT_CFGW_RST   = 0x0;
    uint32_t BIT_CHOx_RST   = 0x0;
    uint32_t BIT_CHIx_RST   = 0x0;
    
    iowrite32(0xFFFFFFFF & ((BIT_DRP_RST   << 31) |
                            (BIT_STBL_RST  << 29) |
                            (BIT_CMN_RST   << 28) |
                            (BIT_DRPOIF_RST<< 27) |
                            (BIT_DRPIIF_RST<< 26) |
                            (BIT_CHOx_RST  << 16) |
                            (BIT_DSCC_RST  <<  9) |
                            (BIT_CFGW_RST  <<  8) |
                            (BIT_CHIx_RST  <<  0)), drp_base_addr + STP_STPC_BADDR + STPC_SFTRST);
}

static void enable_drp_swreset(addr_t drp_base_addr)
{
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_STPC_BADDR + STPC_SFTRST);
}

static void disable_aimac_swreset(addr_t aimac_base_addr)
{
    uint32_t BIT_CMN_RST  = 0x0;
    uint32_t BIT_DSCC_RST = 0x0;
    uint32_t BIT_MCMD_RST = 0x0;
    uint32_t BIT_CHOx_RST = 0x0;
    uint32_t BIT_CHIx_RST = 0x0;
    uint32_t BIT_STBL     = 0x1;
    uint32_t BIT_MAC      = 0x0;
    uint32_t BIT_PRAM     = 0x0;
    uint32_t BIT_CMDS     = 0x0;
    uint32_t BIT_ADMA     = 0x0;

    iowrite32((BIT_CMN_RST  << 28) |
              (BIT_CHOx_RST << 16) |
              (BIT_DSCC_RST <<  9) |
              (BIT_MCMD_RST <<  2) |
              (BIT_CHIx_RST <<  0), aimac_base_addr + EXD0_STPC_BADDR + STPC_SFTRST);
    iowrite32((BIT_CMN_RST  << 28) |
              (BIT_CHOx_RST << 16) |
              (BIT_CHIx_RST <<  0), aimac_base_addr + EXD1_STPC_BADDR + STPC_SFTRST);
    iowrite32((BIT_STBL     <<  4) |
              (BIT_MAC      <<  3) |
              (BIT_PRAM     <<  2) |
              (BIT_CMDS     <<  1) |
              (BIT_ADMA     <<  0), aimac_base_addr + CLKRSTCON_BADDR + CLKRSTCON_SFTRST_REG);
}

static void enable_aimac_swreset(addr_t aimac_base_addr)
{
    iowrite32(0xB1FF03FF, aimac_base_addr + EXD0_STPC_BADDR + STPC_SFTRST);
    iowrite32(0xB1FF03FF, aimac_base_addr + EXD1_STPC_BADDR + STPC_SFTRST);
    iowrite32(0x0000001F, aimac_base_addr + CLKRSTCON_BADDR + CLKRSTCON_SFTRST_REG);
}

//------------------------------------------------------------------------------------------------------------------
// AI-MAC individual setting
//------------------------------------------------------------------------------------------------------------------

static void aimac_inidividual_setting(addr_t aimac_base_addr)
{
    uint32_t BIT_EO_CEN = 0x1;

    iowrite32(BIT_EO_CEN << 11, aimac_base_addr + FMBUF_CLK_CTRL);
}

//------------------------------------------------------------------------------------------------------------------
// Address relocatable
//------------------------------------------------------------------------------------------------------------------

static void enable_addr_relocatable_func(addr_t drp_base_addr, addr_t aimac_base_addr, uint8_t* tbl)
{
    if (0 != drp_base_addr)
    {
        enable_addr_relocatable(drp_base_addr + STP_ADRCONV_BADDR, tbl);
    }
    if (0 != aimac_base_addr)
    {
        enable_addr_relocatable(aimac_base_addr + EXD0_ADRCONV_BADDR, tbl);
    }
}

static void enable_addr_relocatable(addr_t addr, uint8_t* tbl)
{
    if (0 == tbl)
    {
        iowrite32(0x00000000, addr + ADRCONV_TBL_EN);
    }
    else
    {
        uint32_t i;
        uint32_t BIT_VLD;
        uint32_t BIT_PG;
        uint32_t BIT_MAP_ADR_24to31;
        uint32_t BIT_MAP_ADR_32to39;

        for (i = 0; i < 256; i++)
        {
            BIT_VLD            = ((uint32_t)tbl[i * 4 + 0] & 0xFF);
            BIT_PG             = ((uint32_t)tbl[i * 4 + 1] & 0xFF);
            BIT_MAP_ADR_24to31 = ((uint32_t)tbl[i * 4 + 2] & 0xFF);
            BIT_MAP_ADR_32to39 = ((uint32_t)tbl[i * 4 + 3] & 0xFF);
            iowrite32((BIT_MAP_ADR_24to31 << 24) |
                      (BIT_MAP_ADR_32to39 <<  8) |
                      (BIT_PG             <<  4) |
                      (BIT_VLD            <<  0),   addr + ADRCONV_TBL + i * 4);
        }
        iowrite32(0x00000001, addr + ADRCONV_TBL_EN);
    }
}

#if defined(CONFIG_ARCH_R9A09G056)
static void disable_addr_relocatable_func(addr_t drp_base_addr, addr_t aimac_base_addr)
{
    if (0 != drp_base_addr)
    {
        disable_addr_relocatable(drp_base_addr + STP_ADRCONV_BADDR);
    }
    if (0 != aimac_base_addr)
    {
        disable_addr_relocatable(aimac_base_addr + EXD0_ADRCONV_BADDR);
    }
}
#else
static void disable_addr_relocatable_func(addr_t drp_base_addr)
{
    if (0 != drp_base_addr)
    {
        disable_addr_relocatable(drp_base_addr + STP_ADRCONV_BADDR);
    }
}
#endif

static void disable_addr_relocatable(addr_t addr)
{
    iowrite32(0x00000000, addr + ADRCONV_TBL_EN);
}

static void enable_addr_relocatable_drp(addr_t drp_base_addr, uint64_t* addr)
{
    int i;

    for( i=0; i<256; i++ )
    {
        uint32_t bit32_24 = (uint32_t)(*addr)       & 0xFF000000;
        uint32_t bit15_8  = (uint32_t)(*addr >> 24) & 0x0000FF00;
        uint32_t bit6_4   = 0x00000000;
        uint32_t bit0     = 0x00000001;
        iowrite32(bit32_24 | bit15_8 | bit6_4 | bit0, drp_base_addr + STP_ADRCONV_BADDR + ADRCONV_TBL + i*4);
        addr++;
    }
}

static void enable_addr_relocatable_aimac(addr_t aimac_base_addr, uint64_t* addr)
{
    int i;

    for( i=0; i<256; i++ )
    {
        uint32_t bit32_24 = (uint32_t)(*addr)       & 0xFF000000;
        uint32_t bit15_8  = (uint32_t)(*addr >> 24) & 0x0000FF00;
        uint32_t bit6_4   = 0x00000000;
        uint32_t bit0     = 0x00000001;
        iowrite32(bit32_24 | bit15_8 | bit6_4 | bit0, aimac_base_addr + EXD0_ADRCONV_BADDR + ADRCONV_TBL + i*4);
        addr++;
    }
}

static void disable_adrconv_drp(addr_t drp_base_addr)
{
    iowrite32(0, drp_base_addr + STP_ADRCONV_BADDR + ADRCONV_TBL_EN);
}

static void disable_adrconv_aimac(addr_t aimac_base_addr)
{
    iowrite32(0, aimac_base_addr + EXD0_ADRCONV_BADDR + ADRCONV_TBL_EN);
}

static void enable_adrconv_drp(addr_t drp_base_addr)
{
    iowrite32(1, drp_base_addr + STP_ADRCONV_BADDR + ADRCONV_TBL_EN);
}

static void enable_adrconv_aimac(addr_t aimac_base_addr)
{
    iowrite32(1, aimac_base_addr + EXD0_ADRCONV_BADDR + ADRCONV_TBL_EN);
}

static void setdmaoffset_drp(addr_t drp_base_addr)
{
    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADRCW_REG);
    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADR2CW_REG);

    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADRI0_REG);
    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADR2I0_REG);

    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADRI1_REG);
    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADR2I1_REG);

    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADRI2_REG);
    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADR2I2_REG);

    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADRI3_REG);
    iowrite32(0, drp_base_addr + STP_IDIF_BADDR + IDIF_BADR2I3_REG);

    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADRO0_REG);
    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADR2O0_REG);

    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADRO1_REG);
    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADR2O1_REG);

    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADRO2_REG);
    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADR2O2_REG);

    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADRO3_REG);
    iowrite32(0, drp_base_addr + STP_ODIF_BADDR + ODIF_BADR2O3_REG);
}

//------------------------------------------------------------------------------------------------------------------
// Start/Stop prefetch descriptor
//------------------------------------------------------------------------------------------------------------------
static int32_t stop_desc_prefetch(addr_t dmactl_addr)
{
    iowrite8(0x00, dmactl_addr);
    return loop_w(dmactl_addr, 0x00000000, 0xFFFFFFFD);
}

static int32_t dma_stop(addr_t dmactl_addr)
{
    int32_t ret = -1;
    uint32_t loop;

    iowrite8(0x00, dmactl_addr + 0x2);
    iowrite8(0x00, dmactl_addr + 0x0);

    loop = 0;
    while ((100 > loop) && (0 != ret))
    {
        if (0 == check_reg(dmactl_addr, 0x00080000, 0x00000002, 0x00080000))
        {
            ret = 0;
            break;
        }
        udelay(1);
        loop++;
    }

    loop = 0;
    while ((100 > loop) && (0 != ret))
    {
        if (0 == check_reg(dmactl_addr, 0x00080000, 0x00000002, 0x00080000))
        {
            ret = 0;
            break;
        }
        usleep_range(100, 200);
        loop++;
    }

    return ret;
}

static void start_drp_dmac(addr_t drp_base_addr)
{
    uint32_t BIT_DEN       = 0x1;
    uint32_t BIT_REQEN     = 0x1;
    uint32_t BIT_DSEL_DESC = 0x0;
    uint32_t BIT_DSEL_CMD  = 0x1;

    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_DESC << 8) | (BIT_DEN << 0), drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI0_REG);
    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI1_REG);
    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI2_REG);
    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI3_REG);

    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO0_REG);
    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO1_REG);
    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO2_REG);
    iowrite32((BIT_REQEN << 18) | (BIT_DSEL_CMD  << 8) | (BIT_DEN << 0), drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO3_REG);

    iowrite32((BIT_REQEN << 18) | (BIT_DEN << 0),                        drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLCW_REG);
}

static int32_t stop_drp_dmac(addr_t drp_base_addr)
{
    int32_t ret = 0;

    if (0 != dma_stop(drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI1_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI2_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLI3_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO1_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO2_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(drp_base_addr + STP_ODIF_BADDR + ODIF_DMACTLO3_REG))
    {
        goto error_stop;
    }

    goto end;

error_stop:
    ret = -1;
    goto end;

end:
    return ret;
}

static int32_t stop_drp_dmacw(addr_t drp_base_addr)
{
    addr_t dmactl_addr = drp_base_addr + STP_IDIF_BADDR + IDIF_DMACTLCW_REG;

    iowrite8(0x00, dmactl_addr);

    return loop_w(dmactl_addr, 0x00000000, 0xFFFFFFFD);
}

static void start_aimac_dmac(addr_t aimac_base_addr)
{
    uint32_t BIT_DEN   = 0x1;
    uint32_t BIT_REQEN = 0x1;
    uint32_t wdata     = (BIT_REQEN << 18) | (BIT_DEN << 0);

    iowrite32(wdata, aimac_base_addr + EXD0_IDIF_BADDR + IDIF_DMACTLMCMD_REG);
    iowrite32(wdata, aimac_base_addr + EXD0_IDIF_BADDR + IDIF_DMACTLI0_REG);
    iowrite32(wdata, aimac_base_addr + EXD0_IDIF_BADDR + IDIF_DMACTLI1_REG);
    iowrite32(wdata, aimac_base_addr + EXD1_IDIF_BADDR + IDIF_DMACTLI0_REG);
    iowrite32(wdata, aimac_base_addr + EXD1_IDIF_BADDR + IDIF_DMACTLI1_REG);
    iowrite32(wdata, aimac_base_addr + EXD0_ODIF_BADDR + ODIF_DMACTLO0_REG);
    iowrite32(wdata, aimac_base_addr + EXD0_ODIF_BADDR + ODIF_DMACTLO1_REG);
    iowrite32(wdata, aimac_base_addr + EXD1_ODIF_BADDR + ODIF_DMACTLO0_REG);
    iowrite32(wdata, aimac_base_addr + EXD1_ODIF_BADDR + ODIF_DMACTLO1_REG);

    iowrite32(wdata, aimac_base_addr + AID0_IDIF_BADDR + IDIF_DMACTLI0_REG);
    iowrite32(wdata, aimac_base_addr + AID0_IDIF_BADDR + IDIF_DMACTLI1_REG);
    iowrite32(wdata, aimac_base_addr + AID0_IDIF2_BADDR + IDIF_DMACTPCMD_REG);
    iowrite32(wdata, aimac_base_addr + AID1_IDIF_BADDR + IDIF_DMACTLI0_REG);
}

static int32_t stop_aimac_dmac(addr_t aimac_base_addr)
{
    int32_t ret = 0;

    if (0 != dma_stop(aimac_base_addr + AID0_IDIF2_BADDR + IDIF_DMACTPCMD_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD0_IDIF_BADDR + IDIF_DMACTLMCMD_REG))
    {
        goto error_stop;
    }

    if (0 != dma_stop(aimac_base_addr + AID0_IDIF_BADDR + IDIF_DMACTLI0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + AID0_IDIF_BADDR + IDIF_DMACTLI1_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + AID1_IDIF_BADDR + IDIF_DMACTLI0_REG))
    {
        goto error_stop;
    }

    if (0 != dma_stop(aimac_base_addr + EXD0_IDIF_BADDR + IDIF_DMACTLI0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD0_IDIF_BADDR + IDIF_DMACTLI1_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD1_IDIF_BADDR + IDIF_DMACTLI0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD1_IDIF_BADDR + IDIF_DMACTLI1_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD0_ODIF_BADDR + ODIF_DMACTLO0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD0_ODIF_BADDR + ODIF_DMACTLO1_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD1_ODIF_BADDR + ODIF_DMACTLO0_REG))
    {
        goto error_stop;
    }
    if (0 != dma_stop(aimac_base_addr + EXD1_ODIF_BADDR + ODIF_DMACTLO1_REG))
    {
        goto error_stop;
    }

    goto end;

error_stop:
    ret = -1;
    goto end;

end:
    return ret;
}

//------------------------------------------------------------------------------------------------------------------
// DRP-AI interrupt mask setting
//------------------------------------------------------------------------------------------------------------------
static void disable_drp_intmask(addr_t drp_base_addr)
{
#if (0 != DRP_ECC_ENA)
    iowrite32(0x0000073F, drp_base_addr + DRP_ERRINT_ENABLE);
#else
    iowrite32(0x00000000, drp_base_addr + DRP_ERRINT_ENABLE);
    iowrite32(0x00000007, drp_base_addr + DRP_ECC);
#endif

    iowrite32(0xF8F0F0F0, drp_base_addr + STP_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFEFEFE, drp_base_addr + STP_IDIF_BADDR  + IDIF_EINTMSK_DSC_REG);
    iowrite32(0xFFF0F0F0, drp_base_addr + STP_ODIF_BADDR  + ODIF_EINTMSK_REG);
    iowrite32(0xFFFFFFF8, drp_base_addr + STP_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFC, drp_base_addr + STP_ODMAC_BADDR + ODMAC_INTME_REG);

    iowrite32(0xFFFFFFF0, drp_base_addr + STP_ODIF_BADDR + ODIF_INTMSK_REG);

    // setting ELC0
    iowrite32(0x0000000F, drp_base_addr + STP_ODIF_BADDR + ODIF_ELCCTL_REG);
}

static void enable_drp_intmask(addr_t drp_base_addr)
{
    // enable normal interrupt
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_ODIF_BADDR + ODIF_INTMSK_REG);

    // disable DRP error interrupt
    iowrite32(0x00000000, drp_base_addr + DRP_ERRINT_ENABLE);

    // enable abnormal interrupt
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_IDIF_BADDR  + IDIF_EINTMSK_DSC_REG);
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_ODIF_BADDR  + ODIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, drp_base_addr + STP_ODMAC_BADDR + ODMAC_INTME_REG);

    // disable ELC0 output
    iowrite32(0x00000000, drp_base_addr + STP_ODIF_BADDR + ODIF_ELCCTL_REG);
}

static void disable_aimac_intmask(addr_t aimac_base_addr)
{
    // unmask normal interrupt
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_ODIF_BADDR  + ODIF_INTMSK_REG);
    iowrite32(0xFFFFFFFD, aimac_base_addr + EXD1_ODIF_BADDR  + ODIF_INTMSK_REG);

    // unmask abnormal interrupt
    iowrite32(0xFFFCFCFC, aimac_base_addr + AID0_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFEFEFE, aimac_base_addr + AID0_IDIF2_BADDR + IDIF_EINTMSK_DSC_REG);
    iowrite32(0xFFFFFFF8, aimac_base_addr + AID0_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFEFEFE, aimac_base_addr + AID1_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFFFFF8, aimac_base_addr + AID1_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFCFCFC, aimac_base_addr + EXD0_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFBFBFB, aimac_base_addr + EXD0_IDIF_BADDR  + IDIF_EINTMSK_DSC_REG);
    iowrite32(0xFFFCFCFC, aimac_base_addr + EXD0_ODIF_BADDR  + ODIF_EINTMSK_REG);
    iowrite32(0xFFFFFFF8, aimac_base_addr + EXD0_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFC, aimac_base_addr + EXD0_ODMAC_BADDR + ODMAC_INTME_REG);
    iowrite32(0xFFFCFCFC, aimac_base_addr + EXD1_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFCFCFC, aimac_base_addr + EXD1_ODIF_BADDR  + ODIF_EINTMSK_REG);
    iowrite32(0xFFFFFFF8, aimac_base_addr + EXD1_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFC, aimac_base_addr + EXD1_ODMAC_BADDR + ODMAC_INTME_REG);

    iowrite32(0xFFFF0FCC, aimac_base_addr + PRAM_INTMSK);
    iowrite32(0xFFFFFE0C, aimac_base_addr + FMBUF_ERR_MSK);
    iowrite32(0xFFFFFFF0, aimac_base_addr + MACTOP_MACCTL_ERR_MSK);

    // setup MAC_ELC0 output
    iowrite32(0x00000003, aimac_base_addr + EXD0_ODIF_BADDR  + ODIF_ELCCTL_REG);
    iowrite32(0x00000003, aimac_base_addr + EXD1_ODIF_BADDR  + ODIF_ELCCTL_REG);
}

static void enable_aimac_intmask(addr_t aimac_base_addr)
{
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_ODIF_BADDR  + ODIF_INTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD1_ODIF_BADDR  + ODIF_INTMSK_REG);

    iowrite32(0xFFFFFFFF, aimac_base_addr + AID0_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + AID0_IDIF2_BADDR + IDIF_EINTMSK_DSC_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + AID0_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + AID1_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + AID1_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_IDIF_BADDR  + IDIF_EINTMSK_DSC_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_ODIF_BADDR  + ODIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD0_ODMAC_BADDR + ODMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD1_IDIF_BADDR  + IDIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD1_ODIF_BADDR  + ODIF_EINTMSK_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD1_IDMAC_BADDR + IDMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + EXD1_ODMAC_BADDR + ODMAC_INTME_REG);
    iowrite32(0xFFFFFFFF, aimac_base_addr + PRAM_INTMSK);
    iowrite32(0xFFFFFFFF, aimac_base_addr + FMBUF_ERR_MSK);
    iowrite32(0xFFFFFFFF, aimac_base_addr + MACTOP_MACCTL_ERR_MSK);

    iowrite32(0x00000000, aimac_base_addr + EXD0_ODIF_BADDR  + ODIF_ELCCTL_REG);
    iowrite32(0x00000000, aimac_base_addr + EXD1_ODIF_BADDR  + ODIF_ELCCTL_REG);
}

//==================================================================================================================
// Initialize (DRP)
//==================================================================================================================
static void drp_bootseq_drp(addr_t drp_base_addr, uint32_t *drp_addr_relocatable_tbl)
{
    uint32_t BIT_DRPCLKGEN_RST = 0x0;

    // 1. release DRPCLKGEN module reset
    iowrite32(BIT_DRPCLKGEN_RST, drp_base_addr + STP_STPC_BADDR + STPC_CLKGEN_RST);

    // 2. setup DRP clock frequency
    set_drpclkgen_freq(drp_base_addr, DRP_DIVFIX, DRP_DFCENA);

    // 3. enable DMA channel clock
    start_drp_clk(drp_base_addr);

    // 4. release soft reset
    disable_drp_swreset(drp_base_addr);

    // debug on/off
    // set_drp_dbgctrl(drp_base_addr);

    // 5. initialize SYNCTBL
    // Not required for V2H

    // 6. setup address relocatable table
#if defined(CONFIG_ARCH_R9A09G056)
    disable_addr_relocatable_func(drp_base_addr, 0);
#else
    disable_addr_relocatable_func(drp_base_addr);
#endif

    // 7. setup DMA channel
    start_drp_dmac(drp_base_addr);

    // 8. unmask interrupt
    disable_drp_intmask(drp_base_addr);
}

static void aimac_bootseq(addr_t aimac_base_addr, uint8_t *aimac_addr_relocatable_tbl)
{
    uint32_t BIT_MCLKGEN_RST = 0x0;

    // 1. release MCLKGEN module reset
    iowrite32(BIT_MCLKGEN_RST, aimac_base_addr + EXD0_STPC_BADDR + STPC_CLKGEN_RST);

    // 2. setup AIMAC clock frequency
    set_aimac_freq(aimac_base_addr, AIMAC_DIVFIX);

    // 3. Supply/Stop clock (initialize multi cycle FF)
    iowrite32(0x00000007, aimac_base_addr + CLKRSTCON_BADDR + CLKRSTCON_CLKE_REG);
    iowrite32(0x00000000, aimac_base_addr + EXD0_DRPIN_M_BADDR + DRPIN_DUMMY); // dummy access
    iowrite32(0x00000000, aimac_base_addr + CLKRSTCON_BADDR + CLKRSTCON_CLKE_REG);

    // 4. release soft reset
    disable_aimac_swreset(aimac_base_addr);

    // 5. enable clock
    start_aimac_clk(aimac_base_addr);

    // 6. AI-MAC version specific setting
    aimac_inidividual_setting(aimac_base_addr);

    // 7. address relocatable setting
    enable_addr_relocatable_func(0, aimac_base_addr, aimac_addr_relocatable_tbl);

    // 8. DMA channel setting
    start_aimac_dmac(aimac_base_addr);

    // 9. unmask interrupt
    disable_aimac_intmask(aimac_base_addr);
}

//==================================================================================================================
// Procedure of DRP/AIMAC
//==================================================================================================================
static void set_drp_desc_drp(addr_t drp_base_addr, uint64_t drp_desc_addr)
{
    iowrite32(drp_desc_addr & 0xFFFFFFFF, drp_base_addr + STP_DSCC_BADDR + DSCC_DPA_REG);
    iowrite32(drp_desc_addr >> 32,        drp_base_addr + STP_DSCC_BADDR + DSCC_DPA2_REG);
}

static void set_aimac_desc(addr_t aimac_base_addr, uint64_t cmd_desc_addr, uint64_t param_desc_addr)
{
    iowrite32(cmd_desc_addr   & 0xFFFFFFFF, aimac_base_addr + EXD0_DSCC_BADDR + DSCC_DPA_REG);
    iowrite32(cmd_desc_addr   >> 32,        aimac_base_addr + EXD0_DSCC_BADDR + DSCC_DPA2_REG);
    iowrite32(param_desc_addr & 0xFFFFFFFF, aimac_base_addr + AID0_DSCC_BADDR + DSCC_DPA_REG);
    iowrite32(param_desc_addr >> 32,        aimac_base_addr + AID0_DSCC_BADDR + DSCC_DPA2_REG);
}

static void start_prefetch_drp_drp(addr_t drp_base_addr)
{
    uint32_t BIT_DSCEN = 0x1;

    iowrite32(BIT_DSCEN << 0, drp_base_addr + STP_DSCC_BADDR + DSCC_DCTLI0_REG);
}

static void start_prefetch_aimac(addr_t aimac_base_addr)
{
    uint32_t BIT_DSCEN = 0x1;

    iowrite32(BIT_DSCEN << 0, aimac_base_addr + EXD0_DSCC_BADDR + DSCC_DCTLI0_REG);
    iowrite32(BIT_DSCEN << 0, aimac_base_addr + AID0_DSCC_BADDR + DSCC_DCTLI0_REG);
}

//==================================================================================================================
// Stop procedure
//==================================================================================================================
static int32_t stop_prefetch_drp_drp(addr_t drp_base_addr)
{
    int32_t ret = 0;

    // 1. Stop descriptor prefetch.
    if( 0 != stop_desc_prefetch(drp_base_addr + STP_DSCC_BADDR + DSCC_DCTLI0_REG))
    {
        ret = -1;
        goto end;
    }

    // 2. Stop writing configuration data.
    if (0 != stop_drp_dmacw(drp_base_addr))
    {
        ret = -1;
        goto end;
    }

    // 3. Mask interrput.
    enable_drp_intmask(drp_base_addr);

    // 4. Stop data input/output.
    if (0 != stop_drp_dmac(drp_base_addr))
    {
        ret = -1;
        goto end;
    }

    // 5. Disable address relocation table.
#if defined(CONFIG_ARCH_R9A09G056)
    disable_addr_relocatable_func(drp_base_addr, 0);
#else
    disable_addr_relocatable_func(drp_base_addr);
#endif

    // 6. Set up DRP core to fixed frequency mode.
    set_drpclkgen_freq(drp_base_addr, 0x7F, 0x0);

    // 7. Software reset.
    enable_drp_swreset(drp_base_addr);

    // 8. Stop DMA channel clock.
    stop_drp_clk(drp_base_addr);

    // 9. Reset DRPCLKGEN module.
    iowrite32(0x00000001, drp_base_addr + STP_STPC_BADDR + STPC_CLKGEN_RST);

    // 10.System reset
    /* DO NOTHING */
    goto end;

end:
    return ret;
}

static int32_t stop_prefetch_aimac(addr_t aimac_base_addr)
{
    int32_t ret = 0;

    // 1. Stop descriptor prefetch.
    ret = stop_desc_prefetch(aimac_base_addr + AID0_DSCC_BADDR + DSCC_DCTLI0_REG);
    if (0 != ret)
    {
        goto end;
    }
    ret = stop_desc_prefetch(aimac_base_addr + EXD0_DSCC_BADDR + DSCC_DCTLI0_REG);
    if (0 != ret)
    {
        goto end;
    }
    goto end;

end:
    return ret;
}

static int32_t aimac_finalize(addr_t aimac_base_addr)
{
    int32_t ret = 0;

    // 2. mask interrupt.
    enable_aimac_intmask(aimac_base_addr);

    // 3. Stop command input.
    /* TBD */

    // 4. Stop parameter (weight, bias) input.
    /* TBD */

    // 5. Forced stop data input/output
    if (0 != stop_aimac_dmac(aimac_base_addr))
    {
        ret = -1;
        goto end;
    }

    // 6. Disable address relocatable table.
    disable_addr_relocatable_func(0, aimac_base_addr);

    // 7. Stop clock.
    stop_aimac_clk(aimac_base_addr);

    // 8. Software reset.
    enable_aimac_swreset(aimac_base_addr);

    // 9. Stop MCLKGEN
    stop_mclkgen(aimac_base_addr);

    goto end;

end:
    return ret;
}

int32_t cpg_reset_drp(addr_t cpg_base_addr, int32_t ch)
{
    int32_t ret = R_DRP_SUCCESS;
    int32_t BIT_NUM_RST    = (0 == ch) ? 13 : 12;
    int32_t BIT_NUM_RSTMON = (0 == ch) ? 14 : 13;

    // Reset on setting.
    cpg_set(cpg_base_addr + CPG_RST_15_REG, BIT_NUM_RST, 0x0u);
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_7_REG, BIT_NUM_RSTMON, 0x1u))
    {
        ret = R_DRP_ERR_RESET;
        goto end;
    }

    // Reset off setting.
    cpg_set(cpg_base_addr + CPG_RST_15_REG, BIT_NUM_RST, 0x1u);
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_7_REG, BIT_NUM_RSTMON, 0x0u))
    {
        ret = R_DRP_ERR_RESET;
        goto end;
    }

    goto end;

end:
    return ret;
}

//==================================================================================================================
// Procedure of changing DRP-AI clock (for debug).
//==================================================================================================================
static void set_drp_maxfreq(addr_t drp_base_addr, uint32_t mindiv)
{
    iowrite32(mindiv, drp_base_addr + DRP_MINDIV);
}

static void set_aimac_freq(addr_t aimac_base_addr, uint32_t divfix)
{
    iowrite32(divfix << 16, aimac_base_addr + EXD0_STPC_BADDR + STPC_CLKSW_CONFIG);
}

//==================================================================================================================
// reg check loop
//==================================================================================================================
static int32_t loop_w(addr_t raddr, uint32_t exp_data, uint32_t mask)
{
    int32_t ret = -1;
    uint32_t loop;

    loop = 0;
    while ((100 > loop) && (0 != ret))
    {
        if (0 == check_reg(raddr, exp_data, ~mask, 0))
        {
            ret = 0;
            break;
        }
        udelay(1);
        loop++;
    }

    loop = 0;
    while ((100 > loop) && (0 != ret))
    {
        if (0 == check_reg(raddr, exp_data, ~mask, 0))
        {
            ret = 0;
            break;
        }
        usleep_range(100, 200);
        loop++;
    }

    return ret;
}

//==================================================================================================================
// CPG function
//==================================================================================================================
#if defined(CONFIG_ARCH_R9A09G056)
/* RZ/V2N conditional compilation */
int32_t initialize_cpg_drp(addr_t cpg_base_addr)
{
        int32_t ret = R_DRP_SUCCESS;

    // PLLETH
    cpg_set(cpg_base_addr + CPG_PLLETH_STBY_REG, 0, 0x1u);

    // PLLETH_MON
    if (0 != loop_w(cpg_base_addr + CPG_PLLETH_MON_REG, 0x00000010u, 0xFFFFFFEFu))
    {
        goto error_init;
    }

    // MSTOP
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG, 12, 0x0u);  // DRP_SRAM0
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG, 13, 0x0u);  // DRP_SRAM1
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG, 14, 0x0u);  // DRP_SRAM2
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG, 15, 0x0u);  // DRP_SRAM3
    cpg_set(cpg_base_addr + CPG_BUS_9_MSTOP_REG,  0, 0x0u);  // DRP_SRAM4
    cpg_set(cpg_base_addr + CPG_BUS_9_MSTOP_REG,  1, 0x0u);  // DRP_SRAM5
    cpg_set(cpg_base_addr + CPG_BUS_9_MSTOP_REG,  2, 0x0u);  // DRP_SRAM6
    cpg_set(cpg_base_addr + CPG_BUS_9_MSTOP_REG,  3, 0x0u);  // DRP_SRAM7
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG,  8, 0x0u);  // AIMAC
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG,  9, 0x0u);  // STP
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG, 10, 0x0u);  // DRP

    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 1, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 2, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 3, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 4, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 5, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 6, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 7, 0x0u);
    cpg_set(cpg_base_addr + CPG_BUS_12_MSTOP_REG, 8, 0x0u);

    // CLK_ON
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG,  8, 0x1u);  // SRAM_0
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG,  9, 0x1u);  // SRAM_1
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG, 10, 0x1u);  // SRAM_2
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG, 11, 0x1u);  // SRAM_3
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG, 12, 0x1u);  // SRAM_4
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG, 13, 0x1u);  // SRAM_5
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG, 14, 0x1u);  // SRAM_6
    cpg_set(cpg_base_addr + CPG_CLKON_1_REG, 15, 0x1u);  // SRAM_7
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 0, 0x1u);  // DRP.DCLKIN
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 1, 0x1u);  // DRP.ACLK
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 2, 0x1u);  // DRP.INITCLK
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 3, 0x1u);  // DRPAI.DCLKIN
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 4, 0x1u);  // DRPAI.ACLK
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 5, 0x1u);  // DRPAI.INITCLK
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 6, 0x1u);  // DRPAI.MCLK

    // CLK_MON
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 24, 0x1u))  // SRAM_0
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 25, 0x1u))  // SRAM_1
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 26, 0x1u))  // SRAM_2
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 27, 0x1u))  // SRAM_3
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 28, 0x1u))  // SRAM_4
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 29, 0x1u))  // SRAM_5
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 30, 0x1u))  // SRAM_6
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_0_REG, 31, 0x1u))  // SRAM_7
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 16, 0x1u))  // DRP.DCLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 17, 0x1u))  // DRP.ACLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 18, 0x1u))  // DRP.INITCLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 19, 0x1u))  // DRPAI.DCLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 20, 0x1u))  // DRPAI.ACLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 21, 0x1u))  // DRPAI.INITCLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 22, 0x1u))  // DRPAI.MCLK
    {
        goto error_init;
    }

    // Reset OFF
    cpg_set(cpg_base_addr + CPG_RST_3_REG,  14, 0x1u);  // SRAM_0
    cpg_set(cpg_base_addr + CPG_RST_3_REG,  15, 0x1u);  // SRAM_1
    cpg_set(cpg_base_addr + CPG_RST_4_REG,   0, 0x1u);  // SRAM_2
    cpg_set(cpg_base_addr + CPG_RST_4_REG,   1, 0x1u);  // SRAM_3
    cpg_set(cpg_base_addr + CPG_RST_4_REG,   2, 0x1u);  // SRAM_4
    cpg_set(cpg_base_addr + CPG_RST_4_REG,   3, 0x1u);  // SRAM_5
    cpg_set(cpg_base_addr + CPG_RST_4_REG,   4, 0x1u);  // SRAM_6
    cpg_set(cpg_base_addr + CPG_RST_4_REG,   5, 0x1u);  // SRAM_7
    cpg_set(cpg_base_addr + CPG_RST_15_REG, 12, 0x1u);  // DRP.ARESETn
    cpg_set(cpg_base_addr + CPG_RST_15_REG, 13, 0x1u);  // DRPAI.ARESETn

    // RSTMON
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 15, 0x0u))  // SRAM_0
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 16, 0x0u))  // SRAM_1
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 17, 0x0u))  // SRAM_2
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 18, 0x0u))  // SRAM_3
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 19, 0x0u))  // SRAM_4
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 20, 0x0u))  // SRAM_5
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 21, 0x0u))  // SRAM_6
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_1_REG, 22, 0x0u))  // SRAM_7
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_7_REG, 13, 0x0u))  // DRP.ARESETn
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_7_REG, 14, 0x0u))  // DRPAI.ARESETn
    {
        goto error_init;
    }

    goto end;

error_init:
    ret = R_DRP_ERR_INIT;
    goto end;

end:
    return ret;
}
#else
/* RZ/V2H conditional compilation */
int32_t initialize_cpg_drp(addr_t cpg_base_addr)
{
    int32_t ret = R_DRP_SUCCESS;

    // PLLETH
    cpg_set(cpg_base_addr + CPG_PLLETH_STBY_REG, 0, 0x1u);

    // PLLETH_MON
    if (0 != loop_w(cpg_base_addr + CPG_PLLETH_MON_REG, 0x00000010u, 0xFFFFFFEFu))
    {
        goto error_init;
    }

    // MSTOP
    cpg_set(cpg_base_addr + CPG_BUS_8_MSTOP_REG, 10, 0x0u);  // DRP

    // CLK_ON
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 0, 0x1u);  // DRP.DCLKIN
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 1, 0x1u);  // DRP.ACLK
    cpg_set(cpg_base_addr + CPG_CLKON_17_REG, 2, 0x1u);  // DRP.INITCLK

    // CLK_MON
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 16, 0x1u))  // DRP.DCLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 17, 0x1u))  // DRP.ACLK
    {
        goto error_init;
    }
    if (0 != cpg_check(cpg_base_addr + CPG_CLKMON_8_REG, 18, 0x1u))  // DRP.INITCLK
    {
        goto error_init;
    }
                                                       
    // Reset OFF
    cpg_set(cpg_base_addr + CPG_RST_15_REG, 12, 0x1u);  // DRP.ARESETn

    // RSTMON
    if (0 != cpg_check(cpg_base_addr + CPG_RSTMON_7_REG, 13, 0x0u))  // DRP.ARESETn
    {
        goto error_init;
    }

    goto end;

error_init:
    ret = R_DRP_ERR_INIT;
    goto end;

end:
    return ret;

}
#endif

static void cpg_set(addr_t addr, int32_t bit, uint32_t val)
{
    uint32_t rdata;

    rdata = ioread32(addr);
    rdata = (rdata >> bit) & 1;

    if (rdata != val)
    {
        uint32_t wdata = (1 << (bit + 16)) + (val << bit);

        iowrite32(wdata, addr);
    }
}

static int32_t check_reg(addr_t raddr, uint32_t exp, uint32_t mask0, uint32_t mask1)
{
    int32_t result = -1;
    uint32_t rdata = ioread32(raddr);

    if ((0 != mask0) && (0 == mask1))
    {
        if ((rdata & mask0) == (exp & mask0))
        {
            result = 0;
        }
    }
    else if ((0 == mask0) && (0 != mask1))
    {
        if ((rdata & mask1) == (exp & mask1))
        {
            result = 0;
        }
    }
    else if ((0 != mask0) && (0 != mask1))
    {
        if (((rdata & mask0) == (exp & mask0)) || ((rdata & mask1) == (exp & mask1)))
        {
            result = 0;
        }
    }

    return result;
}

static int32_t cpg_check(addr_t addr, int32_t bit, uint32_t val)
{
    int32_t ret = -1;
    uint32_t loop;

    loop = 0;
    while ((100 > loop) && (0 != ret))
    {
        if (0 == check_reg(addr, (val << bit), (1 << bit), 0))
        {
            ret = 0;
            break;
        }
        udelay(1);
        loop++;
    }

    loop = 0;
    while ((100 > loop) && (0 != ret))
    {
        if (0 == check_reg(addr, (val << bit), (1 << bit), 0))
        {
            ret = 0;
            break;
        }
        usleep_range(100, 200);
        loop++;
    }

    return ret;
}

#ifndef __KERNEL__
/* about 3.9 sec maximum. */
static void mdelay(uint32_t msecs)
{
    volatile uint32_t i;

    for (i = 0; i < msecs * 1100000; i++);
}
#endif

#ifdef __KERNEL__
#if defined(CONFIG_ARCH_R9A09G057)
/* V2H conditional compilation */
MODULE_DESCRIPTION("RZ/V2H DRPAI driver");
#endif
MODULE_AUTHOR("Renesas Electronics Corporation");
MODULE_LICENSE("GPL v2");
#endif
