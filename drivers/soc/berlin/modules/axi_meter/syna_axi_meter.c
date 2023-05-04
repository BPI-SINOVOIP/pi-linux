// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
*
* Author: Benson Gui <Benson.Gui@synaptics.com>
*
*/

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/fs.h>

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/axi_meter.h>

#define MC_DEVICE_NAME	"mc_meter"

#define RA_MC6Ctrl_perf_log_cnt_ctrl		0x01C0
#define RA_MC6Ctrl_perf_log_cnt_ctrl1		0x01C4
#define RA_MC6Ctrl_perf_log_cnt_ctrl2		0x01C8
#define RA_MC6Ctrl_pc0				0x01D0
#define RA_MC6Ctrl_pc1				0x01D8

#define RA_mc_wrap_AxiPCntCTRL			0x0048
#define RA_mc_wrap_AxiPCntCTRL1			0x004C
#define RA_mc_wrap_AxiMst0_0			0x0050
#define RA_mc_wrap_Mstr0_0_PCnt			0x0080
#define RA_mc_wrap_Mstr0_1_PCnt			0x00B4

#define RA_mc_wrap_perf_log_cnt_ctrl		0x03B4
#define RA_mc_wrap_perf_log_cnt_ctrl_dch1	0x0404
#define RA_mc_wrap_perf_log_cnt_ctrl1		0x03B8
#define RA_mc_wrap_perf_log_cnt_ctrl_dch11	0x0408
#define RA_mc_wrap_perf_log_cnt_ctrl2		0x03BC
#define RA_mc_wrap_perf_log_cnt_ctrl_dch12	0x040C
#define RA_mc_wrap_pc0				0x03C4
#define RA_mc_wrap_pc1				0x03CC
#define RA_mc_wrap_pc0_dch1			0x0414
#define RA_mc_wrap_pc1_dch1			0x041C

#define AS470_AXI_PCNT_CTRL			0x0048
#define AS470_MST0_0_PCNT			0x0064
#define AS470_MST0_1_PCNT			0x0098
#define AS470_AXI_MST0_0			0x004C
#define AS470_MC_CNT_CTRL			0x0240
#define AS470_MC_CNT_CTRL1			0x0244
#define AS470_MC_CNT_CTRL2			0x0248
#define AS470_MC_WRAP_PC0			0x0250
#define AS470_MC_WRAP_PC1			0x0258

#define VS640_AXI_PCNT_CTRL			0x0048
#define VS640_AXI_PCNT_CTRL1			0x004C
#define VS640_AXI_MST0_0			0x0050
#define VS640_MST0_0_PCNT			0x0080
#define VS640_MST0_1_PCNT			0x00B4
#define VS640_MC_CNT_CTRL			0x03D4
#define VS640_MC_CNT_CTRL1			0x03D8
#define VS640_MC_CNT_CTRL2			0x03DC
#define VS640_MC_WRAP_PC0			0x03E4
#define VS640_MC_WRAP_PC1			0x03EC

#define MC_CTRL1_EVT_NUM		5
#define MC_EVT_BW			6
#define MC_EVT_MASK			((1<<MC_EVT_BW) - 1)

#define MC_CTRL_CLR_SHIFT		0
#define MC_CTRL_EN_SHIFT		8
#define MC_CTRL_LATCH_SHIFT		16

#define AS370_AXI_CNT_NUM		3
#define AS470_AXI_CNT_NUM		6
#define VS640_AXI_CNT_NUM		12
#define VS680_AXI_CNT_NUM		12
#define CNT_MST_OFF			4

#define RA_MC6Ctrl_AxiPCntCTRL		0x001C
#define RA_MC6Ctrl_AxiMst0		0x0020
#define RA_MC6Ctrl_AxiMst0DXBAR		0x0024
#define RA_MC6Ctrl_AxiMst1		0x0028

#define RA_MC6Ctrl_Mstr0PCnt		0x002C
#define RA_MC6Ctrl_Mstr0DXBARPCnt	0x0060

#define RA_AxiPCntStat_OF_STATUS	0x002C
#define RA_AxiPCntStat_TOTAL_CNT	0x0000
#define RA_AxiPCntStat_ARWAIT_CNT	0x0004
#define RA_AxiPCntStat_RWAIT_CNT	0x0008
#define RA_AxiPCntStat_RIDLE_CNT	0x000C
#define RA_AxiPCntStat_RDATA_CNT	0x0010
#define RA_AxiPCntStat_AWWAIT_CNT	0x0014
#define RA_AxiPCntStat_WWAIT_CNT	0x0018
#define RA_AxiPCntStat_WIDLE_CNT	0x001C
#define RA_AxiPCntStat_WDATA_CNT	0x0020
#define RA_AxiPCntStat_AWDATA_CNT	0x0024
#define RA_AxiPCntStat_ARDATA_CNT	0x0028

#define CTRL_CLR_SHIFT		0
#define CTRL_EN_SHIFT		8
#define CTRL_LATCH_SHIFT	16

#define OVERFLOW_TOTAL		BIT(0)
#define OVERFLOW_ARWAIT		BIT(1)
#define OVERFLOW_RWAIT		BIT(2)
#define OVERFLOW_RIDLE		BIT(3)
#define OVERFLOW_RDATA		BIT(4)
#define OVERFLOW_AWWAIT		BIT(5)
#define OVERFLOW_WWAIT		BIT(6)
#define OVERFLOW_WIDLE		BIT(7)
#define OVERFLOW_WDATA		BIT(8)
#define OVERFLOW_AWDATA		BIT(9)
#define OVERFLOW_ARDATA		BIT(10)

#define CTL_CH0_BIT		BIT(0)
#define CTL_CH1_BIT		BIT(7)
#define CTL_CH2_BIT		BIT(1)
#define CTL_ALL_CH_BITS		(CTL_CH0_BIT | CTL_CH1_BIT | CTL_CH2_BIT)

struct axi_perf_counter {
	unsigned int total;

	unsigned int arwait;
	unsigned int rwait;
	unsigned int rdata;
	unsigned int ardata;

	unsigned int awwait;
	unsigned int wwait;
	unsigned int wdata;
	unsigned int awdata;

	unsigned int status;
};

struct cnt_hw_data {
	u32 type;
	/* mc counter */
	u32 ctl_off[CNT_SETS_NUM];
	u32 ctl1_off[CNT_SETS_NUM];
	u32 ctl2_off[CNT_SETS_NUM];
	u32 pc0_off[CNT_SETS_NUM];
	u32 pc1_off[CNT_SETS_NUM];
	u32 ovf_off[CNT_SETS_NUM];
	int mc_cnt_num;
	/* axi counter */
	int axi_cnt_num;
	const u32 *ctl_bit;
	u32 ctl_all;
	u32 mst_off;
	const u32 *def_id;

	u32 latch_base;
	u32 latch_lsf;
	u32 en_base;
	u32 en_lsf;
	u32 clr_base;
	u32 clr_lsf;
	u32 axi_cnt_dist;
	u32 axi_cnt_off;

};

static const u32 default_mc_cnt_events[MAX_MC_CNT_NUM] = {
	mc_clk_cyc,
	mc_hif_rd_or_wr,
	mc_hif_wr,
	mc_hif_rd,
	mc_hif_rmw,
	mc_op_is_precharge,
	mc_precharge_for_rdwr,
	mc_precharge_for_other,
	mc_clk_cyc,
	mc_hif_rd_or_wr,
	mc_hif_wr,
	mc_hif_rd,
	mc_hif_rmw,
	mc_op_is_precharge,
	mc_precharge_for_rdwr,
	mc_precharge_for_other,
};

static const u32 as370_axi_cnt_ctrl_bit[AS370_AXI_CNT_NUM + 1] =
{
	CTL_CH0_BIT,
	CTL_CH1_BIT,
	CTL_CH2_BIT,
	CTL_CH0_BIT | CTL_CH1_BIT | CTL_CH2_BIT,
};

static const u32 as470_axi_cnt_ctrl_bit[AS470_AXI_CNT_NUM + 1] =
{
	BIT(0),
	BIT(1),
	BIT(2),
	BIT(3),
	BIT(4),
	BIT(5),
	BIT(AS470_AXI_CNT_NUM) - 1,
};

static const u32 vs640_axi_cnt_ctrl_bit[VS640_AXI_CNT_NUM + 1] =
{
	BIT(0),
	BIT(1),
	BIT(2),
	BIT(3),
	BIT(4),
	BIT(5),
	BIT(6),
	BIT(7),
	BIT(8),
	BIT(9),
	BIT(10),
	BIT(11),
	BIT(VS640_AXI_CNT_NUM) - 1,
};

static const u32 vs680_axi_cnt_ctrl_bit[VS680_AXI_CNT_NUM + 1] =
{
	BIT(0),
	BIT(1),
	BIT(2),
	BIT(3),
	BIT(4),
	BIT(5),
	BIT(6),
	BIT(7),
	BIT(8),
	BIT(9),
	BIT(10),
	BIT(11),
	BIT(VS680_AXI_CNT_NUM) - 1,
};

/* default axi counter mask id*/
static const u32 as370_axi_default_id[AS370_AXI_CNT_NUM] =
{
	(0x0001 << 16) + 0x0001,	/* CPU */
	(0x0001 << 16) + 0x0000,	/* DXBAR */
	(0x0000 << 16) + 0x0000,	/* PORT1 */
};

static const u32 as470_axi_default_id[AS470_AXI_CNT_NUM] =
{
	(0x0003 << 16) + 0x0000,       /* Mstr0_0: CPU */
	(0x0003 << 16) + 0x0003,       /* Mstr0_1: MTEST */
	(0x0003 << 16) + 0x0001,       /* Mstr0_2: DXBAR (CS, PXBAR(EMMC+SDIO+USB2+PB), TXBAR(BCMx)) */
	(0x0007 << 16) + 0x0002,       /* Mstr0_3: AIO */
	(0x0007 << 16) + 0x0006,       /* Mstr0_4: DSP */
	(0x0000 << 16) + 0x0000,       /* Mstr1_0: NPU */
};

static const u32 vs640_axi_default_id[VS640_AXI_CNT_NUM] =
{
	(0x0803 << 16) + 0x0000,	/* Mstr0_0: CPU */
	(0x0F82 << 16) + 0x0002,	/* Mstr0_1: MTEST + DSP0 + DSP1 */
	(0x080B << 16) + 0x0009,	/* Mstr0_2: PERIF (GE+EMMC+SDIO+USBOTG+USB3+PCIE+PB+BUSM) */
	(0x080F << 16) + 0x0005,	/* Mstr0_3: SISS (BCM*+TSP) + BC + IFCP */
	(0x0FEC << 16) + 0x0008,	/* Mstr1_0: AIO */
	(0x0F0C << 16) + 0x0000,	/* Mstr1_1: VPP */
	(0x0F0C << 16) + 0x0004,	/* Mstr1_2: HDCP */
	(0x0FC3 << 16) + 0x0000,	/* Mstr2_0: H1 */
	(0x0FE3 << 16) + 0x0001,	/* Mstr2_1: OVP */
	(0x0F03 << 16) + 0x0002,	/* Mstr2_2: GPU */
	(0x0FE1 << 16) + 0x0001,	/* Mstr3_0: NPU */
	(0x0F81 << 16) + 0x0000,	/* Mstr3_1: V4G */
};

static const u32 vs680_axi_default_id[VS680_AXI_CNT_NUM] =
{
	(0x0C03 << 16) + 0x0000,	/* Mstr0_0: ID_CA73_CPU */
	(0x0FC3 << 16) + 0x0001,	/* Mstr0_1: ID_MTEST */
	(0x000F << 16) + 0x000E,	/* Mstr0_2: (GE+EMMC+SDIO+USBOTG+USB3+PCIE+PB) */
	(0x000F << 16) + 0x0006,	/* Mstr0_3: SISS + BC + IFCP */

	(0x0001 << 16) + 0x0000,	/* Mstr1_0: VPP+AIO */
	(0x0001 << 16) + 0x0001,	/* Mstr1_1: HDMI */

	(0x0002 << 16) + 0x0002,	/* Mstr2_0: V4G */
	(0x0002 << 16) + 0x0000,	/* Mstr2_1: H1/OVP */

	(0x0000 << 16) + 0x0000,	/* Mstr3_0 GPU+NPU */
	(0x0FC3 << 16) + 0x0000,	/* Mstr3_1 NPU */

	(0x0001 << 16) + 0x0001,	/* Mstr4_0 ISP */
	(0x0001 << 16) + 0x0000,	/* Mstr4_1 OTHERS */
};

static const struct cnt_hw_data as37x_hw_data =
{
	.type = AS370_AXI_MC_TYPE,
	/* mc counter content */
	.ctl_off =  {	RA_MC6Ctrl_perf_log_cnt_ctrl,
			0
		    },
	.ctl1_off = {	RA_MC6Ctrl_perf_log_cnt_ctrl1,
			0
		    },
	.ctl2_off = {	RA_MC6Ctrl_perf_log_cnt_ctrl2,
			0
		    },
	.pc0_off =  {	RA_MC6Ctrl_pc0,
			0
		    },
	.pc1_off =  {	RA_MC6Ctrl_pc1,
			0
		    },
	.ovf_off =  {	RA_MC6Ctrl_pc0 + 4,
			0
		    },
	.mc_cnt_num = AS370_MC_CNT_NUM,

	/* axi counter content */
	.axi_cnt_num = AS370_AXI_CNT_NUM,
	.ctl_bit = as370_axi_cnt_ctrl_bit,
	.ctl_all = CTL_ALL_CH_BITS,
	.mst_off = RA_MC6Ctrl_AxiMst0,
	.def_id = as370_axi_default_id,
	.latch_base = RA_MC6Ctrl_AxiPCntCTRL,
	.latch_lsf = CTRL_LATCH_SHIFT,
	.en_base = RA_MC6Ctrl_AxiPCntCTRL,
	.en_lsf = CTRL_EN_SHIFT,
	.clr_base = RA_MC6Ctrl_AxiPCntCTRL,
	.clr_lsf = CTRL_CLR_SHIFT,
	.axi_cnt_dist = RA_MC6Ctrl_Mstr0DXBARPCnt - RA_MC6Ctrl_Mstr0PCnt,
	.axi_cnt_off = RA_MC6Ctrl_Mstr0PCnt,
};

static const struct cnt_hw_data as470_hw_data =
{
	.type = AS470_AXI_MC_TYPE,
	/* mc counter content */
	.ctl_off =  {	AS470_MC_CNT_CTRL,
			0
		    },
	.ctl1_off = {	AS470_MC_CNT_CTRL1,
			0
		    },
	.ctl2_off = {	AS470_MC_CNT_CTRL2,
			0
		    },
	.pc0_off =  {	AS470_MC_WRAP_PC0,
			0
		    },
	.pc1_off =  {	AS470_MC_WRAP_PC1,
			0
		    },
	.ovf_off =  {	AS470_MC_WRAP_PC0 + 4,
			0
		    },
	.mc_cnt_num = AS470_MC_CNT_NUM,

	/* axi counter content */
	.axi_cnt_num = AS470_AXI_CNT_NUM,
	.ctl_bit = as470_axi_cnt_ctrl_bit,
	.ctl_all = BIT(AS470_AXI_CNT_NUM) - 1,
	.mst_off = AS470_AXI_MST0_0,
	.def_id = as470_axi_default_id,
	.latch_base = AS470_AXI_PCNT_CTRL,
	.latch_lsf = 12,
	.en_base = AS470_AXI_PCNT_CTRL,
	.en_lsf = 6,
	.clr_base = AS470_AXI_PCNT_CTRL,
	.clr_lsf = 0,
	.axi_cnt_dist = AS470_MST0_1_PCNT - AS470_MST0_0_PCNT,
	.axi_cnt_off = AS470_MST0_0_PCNT,
};

static const struct cnt_hw_data vs640_hw_data =
{
	.type = VS640_AXI_MC_TYPE,
	/* mc counter content */
	.ctl_off =  {	VS640_MC_CNT_CTRL,
			0
		    },
	.ctl1_off = {	VS640_MC_CNT_CTRL1,
			0
		    },
	.ctl2_off = {	VS640_MC_CNT_CTRL2,
			0
		    },
	.pc0_off =  {	VS640_MC_WRAP_PC0,
			0
		    },
	.pc1_off =  {	VS640_MC_WRAP_PC1,
			0
		    },
	.ovf_off =  {	VS640_MC_WRAP_PC0 + 4,
			0
		    },
	.mc_cnt_num = VS640_MC_CNT_NUM,

	/* axi counter content */
	.axi_cnt_num = VS640_AXI_CNT_NUM,
	.ctl_bit = vs640_axi_cnt_ctrl_bit,
	.ctl_all = BIT(VS640_AXI_CNT_NUM) - 1,
	.mst_off = VS640_AXI_MST0_0,
	.def_id = vs640_axi_default_id,
	.latch_base = VS640_AXI_PCNT_CTRL1,
	.latch_lsf = 0,
	.en_base = VS640_AXI_PCNT_CTRL,
	.en_lsf = 12,
	.clr_base = VS640_AXI_PCNT_CTRL,
	.clr_lsf = 0,
	.axi_cnt_dist = VS640_MST0_1_PCNT - VS640_MST0_0_PCNT,
	.axi_cnt_off = VS640_MST0_0_PCNT,
};

static const struct cnt_hw_data vs680_hw_data =
{
	.type = VS680_AXI_MC_TYPE,
	/* mc counter content */
	.ctl_off =  {	RA_mc_wrap_perf_log_cnt_ctrl,
			RA_mc_wrap_perf_log_cnt_ctrl_dch1
		    },
	.ctl1_off = {	RA_mc_wrap_perf_log_cnt_ctrl1,
			RA_mc_wrap_perf_log_cnt_ctrl_dch11
		    },
	.ctl2_off = {	RA_mc_wrap_perf_log_cnt_ctrl2,
			RA_mc_wrap_perf_log_cnt_ctrl_dch12
		    },
	.pc0_off =  {	RA_mc_wrap_pc0,
			RA_mc_wrap_pc0_dch1
		    },
	.pc1_off =  {	RA_mc_wrap_pc1,
			RA_mc_wrap_pc1_dch1
		    },
	.ovf_off =  {	RA_mc_wrap_pc0 + 4,
			RA_mc_wrap_pc0_dch1 + 4
		    },
	.mc_cnt_num = VS680_MC_CNT_NUM,

	/* axi counter content */
	.axi_cnt_num = VS680_AXI_CNT_NUM,
	.ctl_bit = vs680_axi_cnt_ctrl_bit,
	.ctl_all = BIT(VS680_AXI_CNT_NUM) - 1,
	.mst_off = RA_mc_wrap_AxiMst0_0,
	.def_id = vs680_axi_default_id,
	.latch_base = RA_mc_wrap_AxiPCntCTRL1,
	.latch_lsf = 0,
	.en_base = RA_mc_wrap_AxiPCntCTRL,
	.en_lsf = 12,
	.clr_base = RA_mc_wrap_AxiPCntCTRL,
	.clr_lsf = 0,
	.axi_cnt_dist = RA_mc_wrap_Mstr0_1_PCnt - RA_mc_wrap_Mstr0_0_PCnt,
	.axi_cnt_off = RA_mc_wrap_Mstr0_0_PCnt,
};

struct axi_meter_priv {
	struct platform_device *pdev;
	const char *dev_name;
	void __iomem *base;
	spinlock_t axi_lock;
	spinlock_t mc_lock;
	u32 cnt_ids[MAX_AXI_CNT_NUM];
	u32 mc_evt[MAX_MC_CNT_NUM];
	u64 cnt_en;
	const struct cnt_hw_data *hw;

	/* cdev */
	struct class *drv_class;
	dev_t dev_num;
	struct cdev cdev;
};

static const struct of_device_id syna_axi_meter_dt_ids[] = {
	{ .compatible = "syna,as370-axi-meter",
	  .data = &as37x_hw_data},
	{ .compatible = "syna,vs680-axi-meter",
	  .data = &vs680_hw_data},
	{ .compatible = "syna,as470-axi-meter",
	  .data = &as470_hw_data},
	{ .compatible = "syna,vs640-axi-meter",
	  .data = &vs640_hw_data},
	{}
};
MODULE_DEVICE_TABLE(of, syna_axi_meter_dt_ids);

static int syna_axi_clear_perf_cnt(struct axi_meter_priv *axi_meter, int cnt)
{
	u32 ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;

	/* when cnt == hw->axi_cnt_num, it operates all counters */
	if (cnt > hw->axi_cnt_num)
		return -EINVAL;

	spin_lock(&axi_meter->axi_lock);

	/* clear the cnt, clr -> 1 -> 0*/
	ctrl = readl(axi_meter->base + hw->clr_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->clr_lsf),
				axi_meter->base + hw->clr_base);
	writel(ctrl, axi_meter->base + hw->clr_base);

	/* latch the cnt, latch -> 0 -> 1 */
	ctrl = readl(axi_meter->base + hw->latch_base);
	writel(ctrl & ~(hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);

	spin_unlock(&axi_meter->axi_lock);

	return 0;
}

static int syna_axi_enable_perf_cnt(struct axi_meter_priv *axi_meter,
						int cnt, int en)
{
	u32 ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;

	/* when cnt == hw->axi_cnt_num, it operates all counters */
	if (cnt > hw->axi_cnt_num)
		return -EINVAL;

	spin_lock(&axi_meter->axi_lock);

	if ((axi_meter->cnt_en & hw->ctl_bit[cnt]) ==
		(en ? hw->ctl_bit[cnt] : 0)) {
		/* already enabled or disabled */
		spin_unlock(&axi_meter->axi_lock);
		return 0;
	}

	ctrl = readl(axi_meter->base + hw->en_base);
	if (en) {
		axi_meter->cnt_en |= hw->ctl_bit[cnt];
		ctrl |= hw->ctl_bit[cnt] << hw->en_lsf;
	} else {
		axi_meter->cnt_en &= ~hw->ctl_bit[cnt];
		ctrl &= ~(hw->ctl_bit[cnt] << hw->en_lsf);
	}
	writel(ctrl, axi_meter->base + hw->en_base);
	spin_unlock(&axi_meter->axi_lock);

	return 0;
}

static int syna_axi_set_mask(struct axi_meter_priv *axi_meter,
				    unsigned int cnt, u32 mask_id)
{
	u32 ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;

	if (cnt >= hw->axi_cnt_num)
		return -EINVAL;

	axi_meter->cnt_ids[cnt] = mask_id;

	spin_lock(&axi_meter->axi_lock);

	/*disable the counter*/
	ctrl = readl(axi_meter->base + hw->en_base);
	axi_meter->cnt_en &= ~hw->ctl_bit[cnt];
	ctrl &= ~(hw->ctl_bit[cnt] << hw->en_lsf);
	writel(ctrl, axi_meter->base + hw->en_base);

	/* clear the cnt, clr -> 1 -> 0*/
	ctrl = readl(axi_meter->base + hw->clr_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->clr_lsf),
				axi_meter->base + hw->clr_base);
	writel(ctrl, axi_meter->base + hw->clr_base);

	/* latch the cnt, latch -> 0 -> 1 */
	ctrl = readl(axi_meter->base + hw->latch_base);
	writel(ctrl & ~(hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);

	/*update the mask*/
	writel(mask_id, axi_meter->base + hw->mst_off + cnt * CNT_MST_OFF);

	spin_unlock(&axi_meter->axi_lock);

	return 0;
}

static void syna_axi_init_perf_cnt(struct axi_meter_priv *axi_meter)
{
	const struct cnt_hw_data *hw = axi_meter->hw;
	u32 ctrl;
	int i;

	spin_lock(&axi_meter->axi_lock);
	axi_meter->cnt_en = 0;

	for (i = 0; i < hw->axi_cnt_num; i++) {
		axi_meter->cnt_ids[i] = hw->def_id[i];
		writel(hw->def_id[i], axi_meter->base + hw->mst_off + i * CNT_MST_OFF);
	}

	/* disable all counters */
	ctrl = readl(axi_meter->base + hw->en_base);
	ctrl &= ~(hw->ctl_all << hw->en_lsf);
	writel(ctrl, axi_meter->base + hw->en_base);

	/* clear all counters */
	ctrl = readl(axi_meter->base + hw->clr_base);
	ctrl |= hw->ctl_all << hw->clr_lsf;
	writel(ctrl, axi_meter->base + hw->clr_base);
	ctrl &= ~(hw->ctl_all << hw->clr_lsf);
	writel(ctrl, axi_meter->base + hw->clr_base);

	/* latch all counters */
	ctrl = readl(axi_meter->base + hw->latch_base);
	ctrl &= ~(hw->ctl_all << hw->latch_lsf);
	writel(ctrl, axi_meter->base + hw->latch_base);
	ctrl |= hw->ctl_all << hw->latch_lsf;
	writel(ctrl, axi_meter->base + hw->latch_base);

	spin_unlock(&axi_meter->axi_lock);
}

static void syna_axi_stop_all_cnt(struct axi_meter_priv *axi_meter)
{
	u32 ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;

	spin_lock(&axi_meter->axi_lock);

	/* disable all counters */
	ctrl = readl(axi_meter->base + hw->en_base);
	ctrl &= ~(hw->ctl_all << hw->en_lsf);
	writel(ctrl, axi_meter->base + hw->en_base);

	/* clear all counters */
	ctrl = readl(axi_meter->base + hw->clr_base);
	ctrl |= hw->ctl_all << hw->clr_lsf;
	writel(ctrl, axi_meter->base + hw->clr_base);
	ctrl &= ~(hw->ctl_all << hw->clr_lsf);
	writel(ctrl, axi_meter->base + hw->clr_base);

	/* latch all counters */
	ctrl = readl(axi_meter->base + hw->latch_base);
	ctrl &= ~(hw->ctl_all << hw->latch_lsf);
	writel(ctrl, axi_meter->base + hw->latch_base);
	ctrl |= hw->ctl_all << hw->latch_lsf;
	writel(ctrl, axi_meter->base + hw->latch_base);

	axi_meter->cnt_en = 0;
	spin_unlock(&axi_meter->axi_lock);
}

static int syna_axi_get_cnt(struct axi_meter_priv *axi_meter,
				   struct axi_get_cnt_param *axi_cnt)
{
	u32 group_offset, ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;
	u32 cnt = axi_cnt->cnt;

	if (axi_cnt->cnt >= hw->axi_cnt_num)
		return -EINVAL;

	group_offset = hw->axi_cnt_off + axi_cnt->cnt * hw->axi_cnt_dist;

	spin_lock(&axi_meter->axi_lock);

	/* latch the cnt, latch -> 0 -> 1 */
	ctrl = readl(axi_meter->base + hw->latch_base);
	writel(ctrl & ~(hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);

	axi_cnt->status = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_OF_STATUS);
	axi_cnt->total = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_TOTAL_CNT);
	axi_cnt->rdata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_RDATA_CNT);
	axi_cnt->ardata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_ARDATA_CNT);
	axi_cnt->wdata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_WDATA_CNT);
	axi_cnt->awdata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_AWDATA_CNT);

	spin_unlock(&axi_meter->axi_lock);
	return 0;
}

static int syna_axi_get_clr(struct axi_meter_priv *axi_meter,
				   struct axi_get_cnt_param *axi_cnt)
{
	u32 group_offset, ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;
	u32 cnt = axi_cnt->cnt;

	if (axi_cnt->cnt >= hw->axi_cnt_num)
		return -EINVAL;

	group_offset = hw->axi_cnt_off + axi_cnt->cnt * hw->axi_cnt_dist;

	spin_lock(&axi_meter->axi_lock);

	/* latch the cnt, latch -> 0 -> 1 */
	ctrl = readl(axi_meter->base + hw->latch_base);
	writel(ctrl & ~(hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);

	/* clear the cnt, clr -> 1 -> 0*/
	ctrl = readl(axi_meter->base + hw->clr_base);
	writel(ctrl | (hw->ctl_bit[cnt] << hw->clr_lsf),
				axi_meter->base + hw->clr_base);
	writel(ctrl, axi_meter->base + hw->clr_base);


	axi_cnt->status = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_OF_STATUS);
	axi_cnt->total = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_TOTAL_CNT);
	axi_cnt->rdata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_RDATA_CNT);
	axi_cnt->ardata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_ARDATA_CNT);
	axi_cnt->wdata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_WDATA_CNT);
	axi_cnt->awdata = readl(axi_meter->base + group_offset
					+ RA_AxiPCntStat_AWDATA_CNT);

	spin_unlock(&axi_meter->axi_lock);
	return 0;
}

static int syna_axi_get_clr_all(struct axi_meter_priv *axi_meter,
				   struct axi_get_cnt_param *axi_cnt)
{
	u32 group_offset, ctrl;
	const struct cnt_hw_data *hw = axi_meter->hw;
	int i;

	spin_lock(&axi_meter->axi_lock);

	/* latch all counters, latch -> 0 -> 1 */
	ctrl = readl(axi_meter->base + hw->latch_base);
	writel(ctrl & ~(hw->ctl_bit[hw->axi_cnt_num] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);
	writel(ctrl | (hw->ctl_bit[hw->axi_cnt_num] << hw->latch_lsf),
				axi_meter->base + hw->latch_base);

	/* clear all counters, clr -> 1 -> 0*/
	ctrl = readl(axi_meter->base + hw->clr_base);
	writel(ctrl | (hw->ctl_bit[hw->axi_cnt_num] << hw->clr_lsf),
				axi_meter->base + hw->clr_base);
	writel(ctrl, axi_meter->base + hw->clr_base);

	for (i = 0; i < hw->axi_cnt_num; i++, axi_cnt++) {
		group_offset = hw->axi_cnt_off + i * hw->axi_cnt_dist;
		axi_cnt->cnt = i;
		axi_cnt->status = readl(axi_meter->base + group_offset
						+ RA_AxiPCntStat_OF_STATUS);
		axi_cnt->total = readl(axi_meter->base + group_offset
						+ RA_AxiPCntStat_TOTAL_CNT);
		axi_cnt->rdata = readl(axi_meter->base + group_offset
						+ RA_AxiPCntStat_RDATA_CNT);
		axi_cnt->ardata = readl(axi_meter->base + group_offset
						+ RA_AxiPCntStat_ARDATA_CNT);
		axi_cnt->wdata = readl(axi_meter->base + group_offset
						+ RA_AxiPCntStat_WDATA_CNT);
		axi_cnt->awdata = readl(axi_meter->base + group_offset
						+ RA_AxiPCntStat_AWDATA_CNT);
	}

	spin_unlock(&axi_meter->axi_lock);
	return 0;
}

static int syna_mc_init_cnt(struct axi_meter_priv *axi_meter)
{
	int i;
	u32 ctrl1[CNT_SETS_NUM], ctrl2[CNT_SETS_NUM];
	const struct cnt_hw_data *hw = axi_meter->hw;

	for (i = 0; i < hw->mc_cnt_num; i++) {
		axi_meter->mc_evt[i] = default_mc_cnt_events[i];
	}

	for (i = 0; i < (hw->mc_cnt_num / CNT_NUM_PER_SET); i++) {
		ctrl1[i] =  (axi_meter->mc_evt[0 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (0*MC_EVT_BW);
		ctrl1[i] |= (axi_meter->mc_evt[1 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (1*MC_EVT_BW);
		ctrl1[i] |= (axi_meter->mc_evt[2 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (2*MC_EVT_BW);
		ctrl1[i] |= (axi_meter->mc_evt[3 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (3*MC_EVT_BW);
		ctrl1[i] |= (axi_meter->mc_evt[4 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (4*MC_EVT_BW);
		ctrl2[i] =  (axi_meter->mc_evt[5 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (0*MC_EVT_BW);
		ctrl2[i] |= (axi_meter->mc_evt[6 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (1*MC_EVT_BW);
		ctrl2[i] |= (axi_meter->mc_evt[7 + CNT_NUM_PER_SET * i] & MC_EVT_MASK)
							<< (2*MC_EVT_BW);
	}

	spin_lock(&axi_meter->mc_lock);

	for (i = 0; i < (hw->mc_cnt_num / CNT_NUM_PER_SET); i++) {
		/* clear all counters */
		writel(0xff << MC_CTRL_CLR_SHIFT, axi_meter->base + hw->ctl_off[i]);
		/* latch all counters */
		writel(0xff << MC_CTRL_LATCH_SHIFT,
					axi_meter->base + hw->ctl_off[i]);
		writel(0x0, axi_meter->base + hw->ctl_off[i]);

		writel(ctrl1[i], axi_meter->base + hw->ctl1_off[i]);
		writel(ctrl2[i], axi_meter->base + hw->ctl2_off[i]);
	}

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_clr_cnt(struct axi_meter_priv *axi_meter, u32 cnt)
{
	u32 val, ctl_off;
	const struct cnt_hw_data *hw = axi_meter->hw;

	if (cnt >= hw->mc_cnt_num)
		return -ENODEV;

	ctl_off = hw->ctl_off[cnt/CNT_NUM_PER_SET];
	cnt %= CNT_NUM_PER_SET;

	spin_lock(&axi_meter->mc_lock);

	val = readl(axi_meter->base + ctl_off);
	writel(val | (1 << (cnt + MC_CTRL_CLR_SHIFT)),
			axi_meter->base + ctl_off);
	writel(val, axi_meter->base + ctl_off);

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_clr_all_cnt(struct axi_meter_priv *axi_meter)
{
	u32 val, i;
	const struct cnt_hw_data *hw = axi_meter->hw;

	spin_lock(&axi_meter->mc_lock);

	for (i = 0; i < (hw->mc_cnt_num/CNT_NUM_PER_SET); i++) {
		val = readl(axi_meter->base + hw->ctl_off[i]);
		writel(val | (0xff << MC_CTRL_CLR_SHIFT),
			axi_meter->base + hw->ctl_off[i]);
		writel(val, axi_meter->base + hw->ctl_off[i]);
	}

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_stop_cnt(struct axi_meter_priv *axi_meter, u32 cnt)
{
	u32 val, ctl_off;
	const struct cnt_hw_data *hw = axi_meter->hw;

	if (cnt >= hw->mc_cnt_num)
		return -ENODEV;

	ctl_off = hw->ctl_off[cnt/CNT_NUM_PER_SET];
	cnt %= CNT_NUM_PER_SET;

	spin_lock(&axi_meter->mc_lock);

	val = readl(axi_meter->base + ctl_off);
	val &= ~(1 << (cnt + MC_CTRL_EN_SHIFT));
	writel(val, axi_meter->base + ctl_off);

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_stop_all_cnt(struct axi_meter_priv *axi_meter)
{
	u32 val, i;
	const struct cnt_hw_data *hw = axi_meter->hw;

	spin_lock(&axi_meter->mc_lock);

	for (i = 0; i < (hw->mc_cnt_num/CNT_NUM_PER_SET); i++) {
		val = readl(axi_meter->base + hw->ctl_off[i]);
		val &= ~(0xff << MC_CTRL_EN_SHIFT);
		writel(val, axi_meter->base + hw->ctl_off[i]);
	}

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_start_cnt(struct axi_meter_priv *axi_meter, u32 cnt)
{
	u32 val, ctl_off;
	const struct cnt_hw_data *hw = axi_meter->hw;

	if (cnt >= hw->mc_cnt_num)
		return -ENODEV;

	ctl_off = hw->ctl_off[cnt/CNT_NUM_PER_SET];
	cnt %= CNT_NUM_PER_SET;

	spin_lock(&axi_meter->mc_lock);

	val = readl(axi_meter->base + ctl_off);
	val |= 1 << (cnt + MC_CTRL_EN_SHIFT);
	writel(val, axi_meter->base + ctl_off);

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_start_all_cnt(struct axi_meter_priv *axi_meter)
{
	u32 val, i;
	const struct cnt_hw_data *hw = axi_meter->hw;

	spin_lock(&axi_meter->mc_lock);

	for (i = 0; i < (hw->mc_cnt_num/CNT_NUM_PER_SET); i++) {
		val = readl(axi_meter->base + hw->ctl_off[i]);
		val |= 0xff << MC_CTRL_EN_SHIFT;
		writel(val, axi_meter->base + hw->ctl_off[i]);
	}

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_cnt_set_evt(struct axi_meter_priv *axi_meter, u32 cnt, u32 evt)
{
	u32 val, ctrl, set;
	const struct cnt_hw_data *hw = axi_meter->hw;

	if (cnt >= hw->mc_cnt_num)
		return -ENODEV;

	if (evt >= mc_evt_max)
		return -EINVAL;

	if (axi_meter->mc_evt[cnt] == evt) {
		return 0;
	}
	spin_lock(&axi_meter->mc_lock);

	axi_meter->mc_evt[cnt] = evt;

	set = cnt / CNT_NUM_PER_SET;
	cnt %= CNT_NUM_PER_SET;

	val = readl(axi_meter->base + hw->ctl_off[set]);
	/* stop the counter */
	writel(val & (~(1 << (cnt + MC_CTRL_EN_SHIFT))),
			axi_meter->base + hw->ctl_off[set]);

	if (cnt < MC_CTRL1_EVT_NUM) {
		ctrl = readl(axi_meter->base + hw->ctl1_off[set]);
		ctrl &= ~(MC_EVT_MASK << (cnt*MC_EVT_BW));
		ctrl |= evt << (cnt*MC_EVT_BW);
		writel(ctrl, axi_meter->base + hw->ctl1_off[set]);
	} else {
		cnt -= MC_CTRL1_EVT_NUM;
		ctrl = readl(axi_meter->base + hw->ctl2_off[set]);
		ctrl &= ~(MC_EVT_MASK << (cnt*MC_EVT_BW));
		ctrl |= evt << (cnt*MC_EVT_BW);
		writel(ctrl, axi_meter->base + hw->ctl2_off[set]);
	}

	/* restore CTRL REG */
	writel(val, axi_meter->base + hw->ctl_off[set]);

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_get_all_cnt(struct axi_meter_priv *axi_meter,
					struct mc_get_cnt_param *param)
{
	u32 i, j, read, val, dist;
	const struct cnt_hw_data *hw = axi_meter->hw;
	u32 *cnts = param->cnts;

	spin_lock(&axi_meter->mc_lock);

	/* latch 0 -> 1 -> 0 to update the counter value*/
	for (i = 0; i < (hw->mc_cnt_num/CNT_NUM_PER_SET); i++) {
		val = readl(axi_meter->base + hw->ctl_off[i]);
		writel(val | (0xff << MC_CTRL_LATCH_SHIFT),
				axi_meter->base + hw->ctl_off[i]);
		writel(val, axi_meter->base + hw->ctl_off[i]);
	}

	param->cnt_num = hw->mc_cnt_num;
	for (i = 0; i < (hw->mc_cnt_num/CNT_NUM_PER_SET); i++) {
		param->overflow[i] = 0;
		dist = hw->pc1_off[i] - hw->pc0_off[i];
		for (j = 0; j < CNT_NUM_PER_SET; j++) {
			cnts[i * CNT_NUM_PER_SET + j] = readl(axi_meter->base +
							hw->pc0_off[i] + dist * j);
			read = readl(axi_meter->base + hw->ovf_off[i] + dist * j);
			if (read == 1)
				param->overflow[i] |= (1 << j);
		}
	}

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_get_clr_cnt(struct axi_meter_priv *axi_meter,
					struct mc_get_clr_param *mc_get_clr)
{
	u32 read, val, cnt, set, dist;
	const struct cnt_hw_data *hw = axi_meter->hw;

	cnt = mc_get_clr->cnt_idx;
	if (cnt >= hw->mc_cnt_num)
		return -ENODEV;

	set = cnt / CNT_NUM_PER_SET;
	cnt %= CNT_NUM_PER_SET;
	dist = hw->pc1_off[set] - hw->pc0_off[set];

	mc_get_clr->overflow = 0;

	spin_lock(&axi_meter->mc_lock);

	/* latch 0 -> 1 -> 0 to update the counter value*/
	val = readl(axi_meter->base + hw->ctl_off[set]);
	writel(val | (0x1 << (cnt + MC_CTRL_LATCH_SHIFT)),
			axi_meter->base + hw->ctl_off[set]);
	/* clear the counter */
	writel(val | (0x1 << (cnt + MC_CTRL_CLR_SHIFT)),
			axi_meter->base + hw->ctl_off[set]);

	writel(val, axi_meter->base + hw->ctl_off[set]);

	mc_get_clr->cnt_val = readl(axi_meter->base + hw->pc0_off[set] +
				dist * cnt);

	/* get the overflow bit */
	read = readl(axi_meter->base + hw->ovf_off[set] + dist * cnt);
	if (read == 1)
		mc_get_clr->overflow = 1;

	spin_unlock(&axi_meter->mc_lock);
	return 0;
}

static int syna_mc_cnt_get_evt(struct axi_meter_priv *axi_meter, u32 *evt)
{
	u32 i;
	const struct cnt_hw_data *hw = axi_meter->hw;

	for (i = 0; i < hw->mc_cnt_num; i++) {
		evt[i] = axi_meter->mc_evt[i];
	}

	for (; i < MAX_MC_CNT_NUM; i++) {
		evt[i] = MC_EVT_INVALID;
	}

	return 0;
}

static int syna_get_info(struct axi_meter_priv *axi_meter,
				  struct axi_get_info *info)
{
	const struct cnt_hw_data *hw = axi_meter->hw;

	info->mc_cnt_num = hw->mc_cnt_num;
	info->axi_cnt_num = hw->axi_cnt_num;
	info->type = hw->type;

	return 0;
}

static long syna_mc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct axi_meter_priv *axi_meter = filp->private_data;

	switch (cmd) {
	case MC_IOC_GET_EVT:
	{
		u32 evt[MAX_MC_CNT_NUM];
		ret = syna_mc_cnt_get_evt(axi_meter, evt);
		if (copy_to_user((void __user *)arg, evt, sizeof(evt))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_SET_EVT:
	{
		struct mc_set_evt_param evt_param;

		if (copy_from_user((void *)&evt_param, (void __user *)arg,
				sizeof(evt_param))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_mc_cnt_set_evt(axi_meter, evt_param.cnt, evt_param.evt);
		break;
	}
	case MC_IOC_START_CNT:
	{
		u32 cnt;
		if (copy_from_user((void *)&cnt, (void __user *)arg, sizeof(cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		if (cnt == MC_SET_ALL_CNT) {
			ret = syna_mc_start_all_cnt(axi_meter);
		} else {
			ret = syna_mc_start_cnt(axi_meter, cnt);
		}
		break;
	}
	case MC_IOC_STOP_CNT:
	{
		u32 cnt;
		if (copy_from_user((void *)&cnt, (void __user *)arg, sizeof(cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		if (cnt == MC_SET_ALL_CNT) {
			ret = syna_mc_stop_all_cnt(axi_meter);
		} else {
			ret = syna_mc_stop_cnt(axi_meter, cnt);
		}
		break;
	}
	case MC_IOC_CLR_CNT:
	{
		u32 cnt;
		if (copy_from_user((void *)&cnt, (void __user *)arg, sizeof(cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		if (cnt == MC_SET_ALL_CNT) {
			ret = syna_mc_clr_all_cnt(axi_meter);
		} else {
			ret = syna_mc_clr_cnt(axi_meter, cnt);
		}
		break;
	}
	case MC_IOC_GET_CNT:
	{
		struct mc_get_cnt_param mc_get_cnt;
		memset(&mc_get_cnt, 0, sizeof(mc_get_cnt));
		syna_mc_get_all_cnt(axi_meter, &mc_get_cnt);

		if (copy_to_user((void __user *)arg, &mc_get_cnt, sizeof(mc_get_cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_GET_CLR_CNT:
	{
		struct mc_get_clr_param mc_get_clr;
		if (copy_from_user((void *)&mc_get_clr, (void __user *)arg,
				sizeof(mc_get_clr))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}

		ret = syna_mc_get_clr_cnt(axi_meter, &mc_get_clr);
		if (ret < 0)
			return ret;

		if (copy_to_user((void __user *)arg, &mc_get_clr, sizeof(mc_get_clr))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_AXI_GET_CNT:
	{
		struct axi_get_cnt_param axi_get_cnt;

		if (copy_from_user((void *)&axi_get_cnt, (void __user *)arg,
				sizeof(axi_get_cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_axi_get_cnt(axi_meter, &axi_get_cnt);
		if (ret < 0)
			return ret;

		if (copy_to_user((void __user *)arg, &axi_get_cnt, sizeof(axi_get_cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_AXI_GET_CLR:
	{
		struct axi_get_cnt_param axi_get_cnt;

		if (copy_from_user((void *)&axi_get_cnt, (void __user *)arg,
				sizeof(axi_get_cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_axi_get_clr(axi_meter, &axi_get_cnt);
		if (ret < 0)
			return ret;

		if (copy_to_user((void __user *)arg, &axi_get_cnt, sizeof(axi_get_cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_AXI_GET_CLR_ALL:
	{
		struct axi_get_cnt_param axi_get_cnt[MAX_AXI_CNT_NUM];

		ret = syna_axi_get_clr_all(axi_meter, axi_get_cnt);
		if (ret < 0)
			return ret;

		if (copy_to_user((void __user *)arg, axi_get_cnt,
			sizeof(axi_get_cnt[0]) * axi_meter->hw->axi_cnt_num)) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_AXI_CLR_CNT:
	{
		u32 cnt;
		if (copy_from_user((void *)&cnt, (void __user *)arg, sizeof(cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_axi_clear_perf_cnt(axi_meter, cnt);
		break;
	}
	case MC_IOC_AXI_START_CNT:
	{
		u32 cnt;
		if (copy_from_user((void *)&cnt, (void __user *)arg, sizeof(cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_axi_enable_perf_cnt(axi_meter, cnt, 1);
		break;
	}
	break;
	case MC_IOC_AXI_STOP_CNT:
	{
		u32 cnt;
		if (copy_from_user((void *)&cnt, (void __user *)arg, sizeof(cnt))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_axi_enable_perf_cnt(axi_meter, cnt, 0);
		break;
	}
	case MC_IOC_AXI_GET_MASK:
	{
		u32 mask[MAX_AXI_CNT_NUM];
		int i;

		for (i = 0; i < MAX_AXI_CNT_NUM; i++)
			mask[i] = axi_meter->cnt_ids[i];

		if (copy_to_user((void __user *)arg, mask, sizeof(mask))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	case MC_IOC_AXI_SET_MASK:
	{
		struct axi_set_mask mask_setting;
		if (copy_from_user((void *)&mask_setting, (void __user *)arg,
				sizeof(mask_setting))) {
			dev_err(&axi_meter->pdev->dev, "copy from user failed\n");
			return -EFAULT;
		}
		ret = syna_axi_set_mask(axi_meter, mask_setting.cnt,
					mask_setting.mask_id);
		break;
	}
	case MC_IOC_AXI_GET_INFO:
	{
		struct axi_get_info info;

		ret = syna_get_info(axi_meter, &info);
		if (ret < 0)
			return ret;

		if (copy_to_user((void __user *)arg, &info, sizeof(info))) {
			dev_err(&axi_meter->pdev->dev, "copy to user failed\n");
			return -EFAULT;
		}
		break;
	}
	default:
		return -EINVAL;
	}

	return ret;
}

static int syna_mc_open(struct inode *inode, struct file *fd)
{
	struct axi_meter_priv *mc_meter =
		container_of(inode->i_cdev, struct axi_meter_priv, cdev);

	fd->private_data = mc_meter;

	return 0;
}

static const struct file_operations mem_meter_fops = {
	.owner          = THIS_MODULE,
	.open		= syna_mc_open,
	.unlocked_ioctl = syna_mc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= syna_mc_ioctl,
#endif
};

static int syna_mc_dev_start(struct axi_meter_priv *axi_meter)
{
	int ret;
	struct device *dev = &axi_meter->pdev->dev;
	struct device *class_dev;

	ret = alloc_chrdev_region(&axi_meter->dev_num, 0, 1,
				MC_DEVICE_NAME);
	if (ret < 0) {
		dev_err(dev, "alloc_chrdev_region failed %d\n", ret);
		return ret;
	}

	axi_meter->drv_class = class_create(THIS_MODULE, MC_DEVICE_NAME);
	if (IS_ERR(axi_meter->drv_class)) {
		dev_err(dev, "class create failed\n");
		ret = -ENOMEM;
		goto err_unregister_chrdev_region;
	}

	class_dev = device_create(axi_meter->drv_class,
					NULL, axi_meter->dev_num,
					NULL, MC_DEVICE_NAME);
	if (IS_ERR(class_dev)) {
		dev_err(dev, "device create failed\n");
		ret = -ENOMEM;
		goto err_class_destroy;
	}

	cdev_init(&axi_meter->cdev, &mem_meter_fops);
	axi_meter->cdev.owner = THIS_MODULE;

	ret = cdev_add(&axi_meter->cdev,
			MKDEV(MAJOR(axi_meter->dev_num), 0), 1);
	if (ret < 0) {
		dev_err(dev, "cdev_add failed %d\n", ret);
		goto err_class_dev_destroy;
	}

	return ret;

err_class_dev_destroy:
	device_destroy(axi_meter->drv_class, axi_meter->dev_num);
err_class_destroy:
	class_destroy(axi_meter->drv_class);
err_unregister_chrdev_region:
	unregister_chrdev_region(axi_meter->dev_num, 1);
	return ret;

}

static int syna_mc_dev_stop(struct axi_meter_priv *axi_meter)
{

	device_destroy(axi_meter->drv_class, axi_meter->dev_num);
	class_destroy(axi_meter->drv_class);
	unregister_chrdev_region(axi_meter->dev_num, 1);

	return 0;
}

static int syna_axi_meter_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct axi_meter_priv *axi_meter;
	struct resource *res;
	const struct of_device_id *match;

	axi_meter = devm_kzalloc(dev, sizeof(struct axi_meter_priv), GFP_KERNEL);
	if (!axi_meter) {
		dev_err(dev, "alloc memory failed\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	axi_meter->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(axi_meter->base)) {
		dev_err(dev, "map devmem failed\n");
		return -ENODEV;
	}

	match = of_match_device(syna_axi_meter_dt_ids, dev);
	if (!match || !match->data) {
		dev_err(dev, "no hw parameter found\n");
		return -ENODEV;
	}

	spin_lock_init(&axi_meter->axi_lock);
	spin_lock_init(&axi_meter->mc_lock);
	axi_meter->dev_name = dev_name(dev);
	axi_meter->pdev = pdev;
	axi_meter->hw = match->data;
	dev_set_drvdata(dev, axi_meter);
	syna_axi_init_perf_cnt(axi_meter);

	syna_mc_init_cnt(axi_meter);
	if (syna_mc_dev_start(axi_meter) < 0) {
		dev_err(dev, "init mc counter failed\n");
		return -ENODEV;
	}

	dev_info(dev, "axi meter start\n");
	return 0;
}

static int syna_axi_meter_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct axi_meter_priv *axi_meter;

	axi_meter = (struct axi_meter_priv *)dev_get_drvdata(dev);

	syna_mc_dev_stop(axi_meter);
	syna_axi_stop_all_cnt(axi_meter);

	return 0;
}

static struct platform_driver syna_axi_meter_driver = {
	.probe = syna_axi_meter_probe,
	.remove = syna_axi_meter_remove,
	.driver = {
		.name = "syna-axi-meter",
		.of_match_table = syna_axi_meter_dt_ids,
	},
};
module_platform_driver(syna_axi_meter_driver);

MODULE_DESCRIPTION("Synaptics mc6 axi meter");
MODULE_ALIAS("platform:syna-axi-meter");
MODULE_LICENSE("GPL v2");
