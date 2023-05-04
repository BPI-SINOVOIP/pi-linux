// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for Synopsys DesignWare Cores Mobile Storage Host Controller
 *
 * Copyright (C) 2018 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mmc/mmc.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <linux/sizes.h>

#include "sdhci-pltfm.h"

#define SDHCI_DWCMSHC_ARG2_STUFF	GENMASK(31, 16)

/* DWCMSHC specific Mode Select value */
#define DWCMSHC_CTRL_HS400		0x7

#define SDHCI_VENDOR_PTR_R		0xe8
#define SDHCI_VENDOR2_PTR_R		0xea

/* phy */
#define PHY_CNFG			0x0
#define  PHY_RSTN			(1 << 0)
#define  PHY_PWRGOOD			(1 << 1)
#define CMDPAD_CNFG			0x4
#define DATPAD_CNFG			0x6
#define CLKPAD_CNFG			0x8
#define STBPAD_CNFG			0xa
#define RSTNPAD_CNFG			0xc
#define COMMDL_CNFG			0x1c
#define  DLSTEP_SEL			(1 << 0)
#define  DLOUT_EN			(1 << 1)
#define SDCLKDL_CNFG			0x1d
#define  EXTDLY_EN			(1 << 0)
#define  BYPASS_EN			(1 << 1)
#define  INPSEL_CNFG_SFT		2
#define  INPSEL_CNFG_MSK		(0x3 << INPSEL_CNFG_SFT)
#define  UPDATE_DC			(1 << 4)
#define SDCLKDL_DC			0x1e
#define  CCKDL_DC_SFT			0
#define  CCKDL_DC_MSK			(0x7f << CCKDL_DC_SFT)
#define SMPLDL_CNFG			0x20
#define  SEXTDLY_EN			(1 << 0)
#define  SBYPASS_EN			(1 << 1)
#define  SINPSEL_CNFG_SFT		2
#define  SINPSEL_CNFG_MSK		(0x3 << SINPSEL_CNFG_SFT)
#define  SINPSEL_OVERRIDE		(1 << 4)
#define ATDL_CNFG			0x21
#define  AEXTDLY_EN			(1 << 0)
#define  ABYPASS_EN			(1 << 1)
#define  AINPSEL_CNFG_SFT		2
#define  AINPSEL_CNFG_MSK		(0x3 << AINPSEL_CNFG_SFT)
#define DLL_CTRL			0x24
#define  DLL_EN				(1 << 0)
#define DLL_CNFG1			0x25
#define  WAITCYCLE_SFT			0
#define  WAITCYCLE_MSK			(0x7 << WAITCYCLE_SFT)
#define  SLVDLY_SFT			4
#define  SLVDLY_MSK			(0x3 << SLVDLY_SFT)
#define DLL_CNFG2			0x26
#define DLLDL_CNFG			0x28
#define  SLV_INPSEL_SFT			5
#define  SLV_INPSEL_MSK			(0x3 << SLV_INPSEL_SFT)
#define DLLLBT_CNFG			0x2c
#define DLL_STATUS			0x2e
#define  LOCK_STS			(1 << 0)
#define  ERROR_STS			(1 << 1)
#define DLLDBG_MLKDC			0x30
#define DLLDBG_SLKDC			0x32

/* PHY RX SEL modes */
#define RXSELOFF			0x0
#define SCHMITT1P8			0x1
#define SCHMITT3P3			0x2
#define SCHMITT1P2			0x3

#define PAD_SP_8			0x8
#define PAD_SP_9			0x9
#define PAD_SN_8			0x8

#define WPE_DISABLE			0x0
#define WPE_PULLUP			0x1
#define WPE_PULLDOWN			0x2

#define TX_SLEW_P_0			0x0
#define TX_SLEW_P_2			0x2
#define TX_SLEW_P_3			0x3

#define TX_SLEW_N_2			0x2
#define TX_SLEW_N_3			0x3

/* vendor */
#define EMMC_CTRL_R			0x2c
#define  CARD_IS_EMMC			(1 << 0)
#define  ENH_STROBE_ENABLE		(1 << 8)
#define AT_CTRL_R			0x40
#define  AT_EN				(1 << 0)
#define  CI_SEL				(1 << 1)
#define  SWIN_TH_EN			(1 << 2)
#define  RPT_TUNE_ERR			(1 << 3)
#define  SW_TUNE_EN			(1 << 4)
#define  TUNE_CLK_STOP_EN		(1 << 16)
#define  PRE_CHANGE_DLY_SFT		17
#define  PRE_CHANGE_DLY_MSK		(0x3 << PRE_CHANGE_DLY_SFT)
#define  POST_CHANGE_DLY_SFT		19
#define  POST_CHANGE_DLY_MSK		(0x3 << POST_CHANGE_DLY_SFT)
#define  SWIN_TH_VAL_SFT		24
#define  SWIN_TH_VAL_MSK		(0x3f << SWIN_TH_VAL_SFT)


#define BOUNDARY_OK(addr, len) \
	((addr | (SZ_128M - 1)) == ((addr + len - 1) | (SZ_128M - 1)))

struct dwcmshc_priv {
	u32			phy_offset;
	u32			vendor_ptr;
	struct clk		*bus_clk;
	struct reset_control	*rst;
	struct reset_control	*phy_rst;
	u8			init_vol;
	u8			sdclkdl_dc;
	u8			dc_200m;
	u8			dc_pre200m;
	u8			pad_sn;
	u8			pad_sp;
	u8			drv_strength;
	bool			dll_cal;
	bool			mode1_tune;
	u32			dll_delay_offset;
};

/*
 * If DMA addr spans 128MB boundary, we split the DMA transfer into two
 * so that each DMA transfer doesn't exceed the boundary.
 */
static void dwcmshc_adma_write_desc(struct sdhci_host *host, void **desc,
				    dma_addr_t addr, int len, unsigned int cmd)
{
	int tmplen, offset;

	if (likely(!len || BOUNDARY_OK(addr, len))) {
		sdhci_adma_write_desc(host, desc, addr, len, cmd);
		return;
	}

	offset = addr & (SZ_128M - 1);
	tmplen = SZ_128M - offset;
	sdhci_adma_write_desc(host, desc, addr, tmplen, cmd);

	addr += tmplen;
	len -= tmplen;
	sdhci_adma_write_desc(host, desc, addr, len, cmd);
}

static inline u32 phy_rdl(struct sdhci_host *host, u32 phy_offset, u32 offset)
{
	return readl(host->ioaddr + phy_offset + offset);
}

static inline void phy_wrl(struct sdhci_host *host, u32 phy_offset,
			   u32 offset, u32 data)
{
	writel(data, host->ioaddr + phy_offset + offset);
}

static inline u16 phy_rdw(struct sdhci_host *host, u32 phy_offset, u32 offset)
{
	return readw(host->ioaddr + phy_offset + offset);
}

static inline void phy_wrw(struct sdhci_host *host, u32 phy_offset,
			   u32 offset, u16 data)
{
	writew(data, host->ioaddr + phy_offset + offset);
}

static inline u8 phy_rdb(struct sdhci_host *host, u32 phy_offset, u32 offset)
{
	return readb(host->ioaddr + phy_offset + offset);
}

static inline void phy_wrb(struct sdhci_host *host, u32 phy_offset,
			   u32 offset, u8 data)
{
	writeb(data, host->ioaddr + phy_offset + offset);
}

static inline u32 vendor_rdl(struct sdhci_host *host, u32 vendor_ptr,
			u32 offset)
{
	return readl(host->ioaddr + vendor_ptr + offset);
}

static inline void vendor_wrl(struct sdhci_host *host, u32 vendor_ptr,
			   u32 offset, u32 data)
{
	writel(data, host->ioaddr + vendor_ptr + offset);
}

static void dwcmshc_phy_init(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 phy_offset = priv->phy_offset;
	u32 vendor_ptr = priv->vendor_ptr;
	u32 val;
	u8 valb;

	if (!phy_offset)
		return;

	/* phy delay line setup */
	valb = phy_rdb(host, phy_offset, COMMDL_CNFG);
	valb &= ~DLSTEP_SEL;
	valb &= ~DLOUT_EN;
	phy_wrb(host, phy_offset, COMMDL_CNFG, valb);

	valb = phy_rdb(host, phy_offset, SDCLKDL_CNFG);
	valb &= ~EXTDLY_EN;
	valb &= ~BYPASS_EN;
	valb &= ~INPSEL_CNFG_MSK;
	valb &= ~UPDATE_DC;
	phy_wrb(host, phy_offset, SDCLKDL_CNFG, valb);

	valb = phy_rdb(host, phy_offset, SMPLDL_CNFG);
	valb &= ~SEXTDLY_EN;
	valb &= ~SBYPASS_EN;
	valb &= ~SINPSEL_OVERRIDE;
	valb &= ~SINPSEL_CNFG_MSK;
	valb |= (3 << SINPSEL_CNFG_SFT);
	phy_wrb(host, phy_offset, SMPLDL_CNFG, valb);

	valb = phy_rdb(host, phy_offset, ATDL_CNFG);
	valb &= ~AEXTDLY_EN;
	valb &= ~ABYPASS_EN;
	valb &= ~AINPSEL_CNFG_MSK;
	valb |= (3 << AINPSEL_CNFG_SFT);
	phy_wrb(host, phy_offset, ATDL_CNFG, valb);

	valb = phy_rdb(host, phy_offset, SDCLKDL_CNFG);
	valb |= UPDATE_DC;
	phy_wrb(host, phy_offset, SDCLKDL_CNFG, valb);

	valb = phy_rdb(host, phy_offset, SDCLKDL_DC);
	valb &= ~CCKDL_DC_MSK;
	valb |= (127 << CCKDL_DC_SFT);
	phy_wrb(host, phy_offset, SDCLKDL_DC, valb);

	valb = phy_rdb(host, phy_offset, SDCLKDL_CNFG);
	valb &= ~UPDATE_DC;
	phy_wrb(host, phy_offset, SDCLKDL_CNFG, valb);

	/* phy tuning setup */
	val = vendor_rdl(host, vendor_ptr, AT_CTRL_R);
	val |= TUNE_CLK_STOP_EN;
	val &= ~POST_CHANGE_DLY_MSK;
	val |= (3 << POST_CHANGE_DLY_SFT);
	val &= ~PRE_CHANGE_DLY_MSK;
	val |= (3 << PRE_CHANGE_DLY_SFT);
	vendor_wrl(host, vendor_ptr, AT_CTRL_R, val);

	reset_control_deassert(priv->phy_rst);

	/* deassert phy reset */
	val = phy_rdl(host, phy_offset, PHY_CNFG);
	val |= PHY_RSTN;
	phy_wrl(host, phy_offset, PHY_CNFG, val);
}

static void dwcmshc_retune_setup(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 vendor_ptr = priv->vendor_ptr;
	u32 reg;
	u32 clk;

	reg = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	reg &= ~SDHCI_INT_RETUNE;
	sdhci_writel(host, reg, SDHCI_SIGNAL_ENABLE);
	reg = sdhci_readl(host, SDHCI_INT_ENABLE);
	reg &= ~SDHCI_INT_RETUNE;
	sdhci_writel(host, reg, SDHCI_INT_ENABLE);

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	sdhci_writew(host, clk & ~SDHCI_CLOCK_CARD_EN, SDHCI_CLOCK_CONTROL);

	reg = vendor_rdl(host, vendor_ptr, AT_CTRL_R);
	reg &= ~SWIN_TH_EN;
	reg &= ~RPT_TUNE_ERR;
	reg &= ~AT_EN;
	reg |= CI_SEL;
	reg |= TUNE_CLK_STOP_EN;
	reg &= ~POST_CHANGE_DLY_MSK;
	reg |= (3 << POST_CHANGE_DLY_SFT);
	reg &= ~PRE_CHANGE_DLY_MSK;
	reg |= (1 << PRE_CHANGE_DLY_SFT);
	reg &= ~SWIN_TH_VAL_MSK;
	vendor_wrl(host, vendor_ptr, AT_CTRL_R, reg);

	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	host->tuning_mode = SDHCI_TUNING_MODE_1;
}

static void dwcmshc_reset(struct sdhci_host *host, u8 mask)
{
	sdhci_reset(host, mask);

	if (mask & SDHCI_RESET_ALL) {
		struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
		struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
		u32 tmp = MMC_CAP2_NO_SD | MMC_CAP2_NO_SDIO;

		dwcmshc_phy_init(host);
		if (priv->mode1_tune)
			dwcmshc_retune_setup(host);
		if ((host->mmc->caps2 & tmp) == tmp) {
			u32 val;
			u32 vendor_ptr = priv->vendor_ptr;

			val = vendor_rdl(host, vendor_ptr, EMMC_CTRL_R);
			val |= CARD_IS_EMMC;
			vendor_wrl(host, vendor_ptr, EMMC_CTRL_R, val);
		}
	}
}

static const u8 phy_reg_val[][22] = {
	{
		/* 3.3V */
		/* pad general */
		PAD_SP_9, PAD_SN_8,
		/* RXSEL */
		SCHMITT3P3, SCHMITT3P3, RXSELOFF, SCHMITT3P3, SCHMITT3P3,
		/* WEAKPULL_EN */
		WPE_PULLUP, WPE_PULLUP, WPE_DISABLE, WPE_PULLDOWN, WPE_PULLUP,
		/* TXSLEW_CTRL_P */
		TX_SLEW_P_3, TX_SLEW_P_3, TX_SLEW_P_3, TX_SLEW_P_3, TX_SLEW_P_3,
		/* TXSLEW_CTRL_N */
		TX_SLEW_N_2, TX_SLEW_N_2, TX_SLEW_N_2, TX_SLEW_N_2, TX_SLEW_N_2,
	},
	{
		/* 1.8V */
		/* pad general */
		PAD_SP_8, PAD_SN_8,
		/* RXSEL */
		SCHMITT1P8, SCHMITT1P8, RXSELOFF, SCHMITT1P8, SCHMITT1P8,
		/* WEAKPULL_EN */
		WPE_PULLUP, WPE_PULLUP, WPE_DISABLE, WPE_PULLDOWN, WPE_PULLUP,
		/* TXSLEW_CTRL_P */
		TX_SLEW_P_0, TX_SLEW_P_0, TX_SLEW_P_0, TX_SLEW_P_0, TX_SLEW_P_0,
		/* TXSLEW_CTRL_N */
		TX_SLEW_N_3, TX_SLEW_N_3, TX_SLEW_N_3, TX_SLEW_N_3, TX_SLEW_N_3,
	},
};

static int dwcmshc_start_signal_voltage_switch(struct mmc_host *mmc,
					       struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 phy_offset = priv->phy_offset;
	int i = ios->signal_voltage;
	u32 val, count;
	u16 valw, clk;

	if (i != MMC_SIGNAL_VOLTAGE_330 && i != MMC_SIGNAL_VOLTAGE_180)
		return -EINVAL;

	if (priv->init_vol == MMC_SIGNAL_VOLTAGE_180 &&
			i == MMC_SIGNAL_VOLTAGE_330)
		return -EINVAL;

	clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	sdhci_writew(host, clk & ~SDHCI_CLOCK_CARD_EN, SDHCI_CLOCK_CONTROL);

	/* general */
	val = phy_rdl(host, phy_offset, PHY_CNFG);
	val &= ~(0xf << 16);
	if (priv->pad_sp)
		val |= (priv->pad_sp << 16);
	else
		val |= (phy_reg_val[i][0] << 16);
	val &= ~(0xf << 20);
	if (priv->pad_sn)
		val |= (priv->pad_sn << 20);
	else
		val |= (phy_reg_val[i][1] << 20);
	phy_wrl(host, phy_offset, PHY_CNFG, val);

	/* RXSEL */
	valw = phy_rdw(host, phy_offset, CMDPAD_CNFG);
	valw &= ~(0x7 << 0);
	valw |= (phy_reg_val[i][2] << 0);
	phy_wrw(host, phy_offset, CMDPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, DATPAD_CNFG);
	valw &= ~(0x7 << 0);
	valw |= (phy_reg_val[i][3] << 0);
	phy_wrw(host, phy_offset, DATPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, CLKPAD_CNFG);
	valw &= ~(0x7 << 0);
	valw |= (phy_reg_val[i][4] << 0);
	phy_wrw(host, phy_offset, CLKPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, STBPAD_CNFG);
	valw &= ~(0x7 << 0);
	valw |= (phy_reg_val[i][5] << 0);
	phy_wrw(host, phy_offset, STBPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, RSTNPAD_CNFG);
	valw &= ~(0x7 << 0);
	valw |= (phy_reg_val[i][6] << 0);
	phy_wrw(host, phy_offset, RSTNPAD_CNFG, valw);

	/* WEAKPULL_EN */
	valw = phy_rdw(host, phy_offset, CMDPAD_CNFG);
	valw &= ~(0x3 << 3);
	valw |= (phy_reg_val[i][7] << 3);
	phy_wrw(host, phy_offset, CMDPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, DATPAD_CNFG);
	valw &= ~(0x3 << 3);
	valw |= (phy_reg_val[i][8] << 3);
	phy_wrw(host, phy_offset, DATPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, CLKPAD_CNFG);
	valw &= ~(0x3 << 3);
	valw |= (phy_reg_val[i][9] << 3);
	phy_wrw(host, phy_offset, CLKPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, STBPAD_CNFG);
	valw &= ~(0x3 << 3);
	valw |= (phy_reg_val[i][10] << 3);
	phy_wrw(host, phy_offset, STBPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, RSTNPAD_CNFG);
	valw &= ~(0x3 << 3);
	valw |= (phy_reg_val[i][11] << 3);
	phy_wrw(host, phy_offset, RSTNPAD_CNFG, valw);

	/* TXSLEW_CTRL_P */
	valw = phy_rdw(host, phy_offset, CMDPAD_CNFG);
	valw &= ~(0xf << 5);
	valw |= (phy_reg_val[i][12] << 5);
	phy_wrw(host, phy_offset, CMDPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, DATPAD_CNFG);
	valw &= ~(0xf << 5);
	valw |= (phy_reg_val[i][13] << 5);
	phy_wrw(host, phy_offset, DATPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, CLKPAD_CNFG);
	valw &= ~(0xf << 5);
	valw |= (phy_reg_val[i][14] << 5);
	phy_wrw(host, phy_offset, CLKPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, STBPAD_CNFG);
	valw &= ~(0xf << 5);
	valw |= (phy_reg_val[i][15] << 5);
	phy_wrw(host, phy_offset, STBPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, RSTNPAD_CNFG);
	valw &= ~(0xf << 5);
	valw |= (phy_reg_val[i][16] << 5);
	phy_wrw(host, phy_offset, RSTNPAD_CNFG, valw);

	/* TXSLEW_CTRL_N */
	valw = phy_rdw(host, phy_offset, CMDPAD_CNFG);
	valw &= ~(0xf << 9);
	valw |= (phy_reg_val[i][17] << 9);
	phy_wrw(host, phy_offset, CMDPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, DATPAD_CNFG);
	valw &= ~(0xf << 9);
	valw |= (phy_reg_val[i][18] << 9);
	phy_wrw(host, phy_offset, DATPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, CLKPAD_CNFG);
	valw &= ~(0xf << 9);
	valw |= (phy_reg_val[i][19] << 9);
	phy_wrw(host, phy_offset, CLKPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, STBPAD_CNFG);
	valw &= ~(0xf << 9);
	valw |= (phy_reg_val[i][20] << 9);
	phy_wrw(host, phy_offset, STBPAD_CNFG, valw);

	valw = phy_rdw(host, phy_offset, RSTNPAD_CNFG);
	valw &= ~(0xf << 9);
	valw |= (phy_reg_val[i][21] << 9);
	phy_wrw(host, phy_offset, RSTNPAD_CNFG, valw);

	/* wait for phy pwrgood */
	count = 100;
	while (count) {
		val = phy_rdl(host, phy_offset, PHY_CNFG);
		if (val & PHY_PWRGOOD)
			break;
		udelay(100);
		count--;
	}

	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);

	if (!count) {
		dev_err(mmc_dev(mmc), "phy config timeout\n");
		return -ETIMEDOUT;
	}
	return sdhci_start_signal_voltage_switch(mmc, ios);
}

static void dwcmshc_hs400_enhanced_strobe(struct mmc_host *mmc,
					  struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 val, vendor_ptr = priv->vendor_ptr;

	val = vendor_rdl(host, vendor_ptr, EMMC_CTRL_R);
	if (ios->enhanced_strobe)
		val |= ENH_STROBE_ENABLE;
	else
		val &= ~ENH_STROBE_ENABLE;
	vendor_wrl(host, vendor_ptr, EMMC_CTRL_R, val);
}

static int dwcmshc_select_drive_strength(struct mmc_card *card,
					 unsigned int max_dtr, int host_drv,
					 int card_drv, int *drv_type)
{
	struct sdhci_host *host = mmc_priv(card->host);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);

	if (!(mmc_driver_type_mask(priv->drv_strength) & card_drv))
		return 0;

	return priv->drv_strength;
}

static void dwcmshc_check_auto_cmd23(struct mmc_host *mmc,
					struct mmc_request *mrq)
{
	struct sdhci_host *host = mmc_priv(mmc);

	if (mrq->sbc && (mrq->sbc->arg & SDHCI_DWCMSHC_ARG2_STUFF))
		host->flags &= ~SDHCI_AUTO_CMD23;
	else
		host->flags |= SDHCI_AUTO_CMD23;
}

static void dwcmshc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	dwcmshc_check_auto_cmd23(mmc, mrq);

	sdhci_request(mmc, mrq);
}

static int dwcmshc_phy_dll_cal(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 phy_offset = priv->phy_offset;
	u8 val, valm, vals;
	int ret;

	val = phy_rdb(host, phy_offset, DLL_CTRL);
	val &= ~DLL_EN;
	phy_wrb(host, phy_offset, DLL_CTRL, val);
	val |= DLL_EN;
	phy_wrb(host, phy_offset, DLL_CTRL, val);

	ret = readb_poll_timeout(host->ioaddr + phy_offset + DLL_STATUS, val,
				 (val & LOCK_STS), 1000, 20000);
	if (ret)
		return ret;

	valm = phy_rdb(host, phy_offset, DLLDBG_MLKDC);
	vals = phy_rdb(host, phy_offset, DLLDBG_SLKDC);
	if (!valm || !vals)
		return -EINVAL;
	val = valm / vals;
	if (val == 4)
		ret = 5000 / valm;
	else if (val == 2)
		ret = 5000 / 2 / valm;
	else
		ret = 5000 / 4 / vals;
	if (!ret)
		return -EINVAL;
	ret = (1400 + priv->dll_delay_offset) / ret;
	ret++;

	dev_info(mmc_dev(host->mmc), "dll-calibration result: %d\n", ret);
	return ret;
}

static void dwcmshc_set_phy_tx_delay(struct sdhci_host *host, u8 delay)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 phy_offset = priv->phy_offset;
	u8 valb, valdc;

	valdc = phy_rdb(host, phy_offset, SDCLKDL_DC);
	if ((valdc & CCKDL_DC_MSK) >> CCKDL_DC_SFT == delay)
		return;

	valb = phy_rdb(host, phy_offset, SDCLKDL_CNFG);
	valb |= UPDATE_DC;
	phy_wrb(host, phy_offset, SDCLKDL_CNFG, valb);

	valdc &= ~CCKDL_DC_MSK;
	valdc |= (delay << CCKDL_DC_SFT);
	phy_wrb(host, phy_offset, SDCLKDL_DC, valdc);

	valb = phy_rdb(host, phy_offset, SDCLKDL_CNFG);
	valb &= ~UPDATE_DC;
	phy_wrb(host, phy_offset, SDCLKDL_CNFG, valb);
}

static inline u8 dwcmshc_choose_hs400_txdelay(struct sdhci_host *host,
					      struct dwcmshc_priv *priv)
{
	int ret;

	if (!priv->dll_cal)
		return priv->sdclkdl_dc;

	if ((host->mmc->ios.clock < MMC_HS200_MAX_DTR) && priv->dc_pre200m)
		return priv->dc_pre200m;
	if ((host->mmc->ios.clock >= MMC_HS200_MAX_DTR) && priv->dc_200m)
		return priv->dc_200m;

	ret = dwcmshc_phy_dll_cal(host);
	if (ret > 0) {
		if (host->mmc->ios.clock < MMC_HS200_MAX_DTR)
			priv->dc_pre200m = ret;
		else
			priv->dc_200m = ret;
		return ret;
	}

	return priv->sdclkdl_dc;
}

static void dwcmshc_set_uhs_signaling(struct sdhci_host *host,
				      unsigned int timing)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u8 txdelay = 0;
	u16 ctrl_2;

	ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
	if ((timing == MMC_TIMING_MMC_HS200) ||
	    (timing == MMC_TIMING_UHS_SDR104))
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
	else if (timing == MMC_TIMING_UHS_SDR12)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
	else if ((timing == MMC_TIMING_UHS_SDR25) ||
		 (timing == MMC_TIMING_MMC_HS))
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
	else if (timing == MMC_TIMING_UHS_SDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
	else if ((timing == MMC_TIMING_UHS_DDR50) ||
		 (timing == MMC_TIMING_MMC_DDR52))
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
	else if (timing == MMC_TIMING_MMC_HS400)
		ctrl_2 |= DWCMSHC_CTRL_HS400;
	sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);

	if (timing == MMC_TIMING_MMC_HS400) {
		u32 phy_offset = priv->phy_offset;
		u8 valb;

		phy_wrw(host, phy_offset, DLLLBT_CNFG, 0x8000);

		valb = 0;
		valb |= (0x3 << SLVDLY_SFT);
		valb |= (0x3 << WAITCYCLE_SFT);
		phy_wrb(host, phy_offset, DLL_CNFG1, valb);

		valb = 0;
		valb |= 0xa;
		phy_wrb(host, phy_offset, DLL_CNFG2, valb);

		valb = 0;
		valb |= (0x3 << SLV_INPSEL_SFT);
		phy_wrb(host, phy_offset, DLLDL_CNFG, valb);

		valb = phy_rdb(host, phy_offset, DLL_CTRL);
		valb |= DLL_EN;
		phy_wrb(host, phy_offset, DLL_CTRL, valb);
	}

	if (timing == MMC_TIMING_UHS_SDR104)
		txdelay = priv->sdclkdl_dc;
	else if (timing == MMC_TIMING_MMC_HS)
		txdelay = 100;
	else if (timing == MMC_TIMING_MMC_DDR52)
		txdelay = 90;
	else if (timing == MMC_TIMING_MMC_HS200)
		txdelay = 40;
	else if (timing == MMC_TIMING_MMC_HS400)
		txdelay = dwcmshc_choose_hs400_txdelay(host, priv);

	if (txdelay)
		dwcmshc_set_phy_tx_delay(host, txdelay);
}

static int dwcmshc_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 vendor_ptr = priv->vendor_ptr;
	u16 clk;
	u32 val;

	if (priv->mode1_tune) {
		if (host->tuning_mode != SDHCI_TUNING_MODE_1)
			dwcmshc_retune_setup(host);
	} else {
		sdhci_reset_tuning(host);
		clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
		sdhci_writew(host, clk & ~SDHCI_CLOCK_CARD_EN, SDHCI_CLOCK_CONTROL);
		val = vendor_rdl(host, vendor_ptr, AT_CTRL_R);
		val &= ~SWIN_TH_EN;
		val &= ~RPT_TUNE_ERR;
		val |= AT_EN;
		vendor_wrl(host, vendor_ptr, AT_CTRL_R, val);
		sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	}

	return sdhci_execute_tuning(mmc, opcode);
}

static const struct sdhci_ops sdhci_dwcmshc_ops = {
	.set_clock		= sdhci_set_clock,
	.set_bus_width		= sdhci_set_bus_width,
	.set_uhs_signaling	= dwcmshc_set_uhs_signaling,
	.get_max_clock		= sdhci_pltfm_clk_get_max_clock,
	.reset			= dwcmshc_reset,
	.adma_write_desc	= dwcmshc_adma_write_desc,
};

static const struct sdhci_pltfm_data sdhci_dwcmshc_pdata = {
	.ops = &sdhci_dwcmshc_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN,
};

static int dwcmshc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host;
	struct dwcmshc_priv *priv;
	int err;
	u32 extra;

	host = sdhci_pltfm_init(pdev, &sdhci_dwcmshc_pdata,
				sizeof(struct dwcmshc_priv));
	if (IS_ERR(host))
		return PTR_ERR(host);

	host->tuning_loop_count = 128;

	/*
	 * extra adma table cnt for cross 128M boundary handling.
	 */
	extra = DIV_ROUND_UP_ULL(dma_get_required_mask(&pdev->dev), SZ_128M);
	if (extra > SDHCI_MAX_SEGS)
		extra = SDHCI_MAX_SEGS;
	host->adma_table_cnt += extra;

	pltfm_host = sdhci_priv(host);
	priv = sdhci_pltfm_priv(pltfm_host);

	priv->rst = devm_reset_control_get_optional(&pdev->dev, "host");
	if (IS_ERR(priv->rst) && PTR_ERR(priv->rst) == -EPROBE_DEFER)
		return -EPROBE_DEFER;
	if (!IS_ERR(priv->rst))
		reset_control_reset(priv->rst);

	priv->phy_rst = devm_reset_control_get_optional(&pdev->dev, "phy");
	if (IS_ERR(priv->phy_rst) && PTR_ERR(priv->phy_rst) == -EPROBE_DEFER)
		return -EPROBE_DEFER;
	if (!IS_ERR(priv->phy_rst))
		reset_control_assert(priv->phy_rst);

	pltfm_host->clk = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR(pltfm_host->clk)) {
		err = PTR_ERR(pltfm_host->clk);
		dev_err(&pdev->dev, "failed to get core clk: %d\n", err);
		goto free_pltfm;
	}
	err = clk_prepare_enable(pltfm_host->clk);
	if (err)
		goto free_pltfm;

	priv->bus_clk = devm_clk_get(&pdev->dev, "bus");
	if (!IS_ERR(priv->bus_clk))
		clk_prepare_enable(priv->bus_clk);

	err = mmc_of_parse(host->mmc);
	if (err)
		goto err_clk;

	sdhci_get_of_property(pdev);

	of_property_read_u32(np, "phy-offset", &priv->phy_offset);
	of_property_read_u8(np, "initial-voltage", &priv->init_vol);
	of_property_read_u8(np, "sdclkdl-dc", &priv->sdclkdl_dc);
	of_property_read_u8(np, "pad-sn", &priv->pad_sn);
	of_property_read_u8(np, "pad-sp", &priv->pad_sp);
	of_property_read_u8(np, "drv-strength", &priv->drv_strength);
	err = of_property_read_u32(np, "dll-delay-offset",
				   &priv->dll_delay_offset);
	if (err < 0)
		priv->dll_delay_offset = 400;
	priv->dll_cal = of_property_read_bool(np, "dll-calibration");
	priv->mode1_tune = of_property_read_bool(np, "mode1-tune");
	if (priv->phy_offset)
		host->mmc_host_ops.start_signal_voltage_switch =
				dwcmshc_start_signal_voltage_switch;
	host->mmc_host_ops.hs400_enhanced_strobe =
		dwcmshc_hs400_enhanced_strobe;
	host->mmc_host_ops.select_drive_strength =
		dwcmshc_select_drive_strength;
	host->mmc_host_ops.request = dwcmshc_request;
	host->mmc_host_ops.execute_tuning = dwcmshc_execute_tuning;

	priv->vendor_ptr = sdhci_readw(host, SDHCI_VENDOR_PTR_R);

	host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;

	err = sdhci_add_host(host);
	if (err)
		goto err_clk;

	return 0;

err_clk:
	clk_disable_unprepare(pltfm_host->clk);
	clk_disable_unprepare(priv->bus_clk);
free_pltfm:
	sdhci_pltfm_free(pdev);
	return err;
}

static int dwcmshc_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);

	sdhci_remove_host(host, 0);

	clk_disable_unprepare(pltfm_host->clk);
	clk_disable_unprepare(priv->bus_clk);

	sdhci_pltfm_free(pdev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int dwcmshc_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret;

	ret = sdhci_suspend_host(host);
	if (ret)
		return ret;

	clk_disable_unprepare(pltfm_host->clk);
	if (!IS_ERR(priv->bus_clk))
		clk_disable_unprepare(priv->bus_clk);

	return ret;
}

static int dwcmshc_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret;

	ret = clk_prepare_enable(pltfm_host->clk);
	if (ret)
		return ret;

	if (!IS_ERR(priv->bus_clk)) {
		ret = clk_prepare_enable(priv->bus_clk);
		if (ret)
			return ret;
	}

	return sdhci_resume_host(host);
}
#endif

static SIMPLE_DEV_PM_OPS(dwcmshc_pmops, dwcmshc_suspend, dwcmshc_resume);

static const struct of_device_id sdhci_dwcmshc_dt_ids[] = {
	{ .compatible = "snps,dwcmshc-sdhci" },
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_dwcmshc_dt_ids);

static struct platform_driver sdhci_dwcmshc_driver = {
	.driver	= {
		.name	= "sdhci-dwcmshc",
		.of_match_table = sdhci_dwcmshc_dt_ids,
		.pm = &dwcmshc_pmops,
	},
	.probe	= dwcmshc_probe,
	.remove	= dwcmshc_remove,
};
module_platform_driver(sdhci_dwcmshc_driver);

MODULE_DESCRIPTION("SDHCI platform driver for Synopsys DWC MSHC");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_LICENSE("GPL v2");
