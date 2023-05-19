// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/delay.h>
#include "aio_common.h"
#include "aio_hal.h"
#include "i2s_common.h"
#include "aio_hal.h"
#include "aio_common.h"

#define CTRL_ADDR(a) (RA_AIO_SPDIFRX_CTRL + a)

static struct spdifi_sample_rate_margin
	spdifi_srm[SPDIFI_FS_MAX] = {
	{"sample-rate-32000-margin", 0, 0x1F171717},
	{"sample-rate-44100-margin", 0, 0x1F171717},
	{"sample-rate-48000-margin", 0, 0x0F070707},
	{"sample-rate-88000-margin", 0, 0x1A101010},
	{"sample-rate-96000-margin", 0, 0x1A101010},
	{"sample-rate-17600-margin", 0, 0x0F080808},
	{"sample-rate-192000-margin", 0, 0x0F070707}
};

static u32 spdifi_ctrl[12];

struct spdifi_sample_rate_margin *aio_spdifi_get_srm(u32 idx)
{
	return &spdifi_srm[idx];
}
EXPORT_SYMBOL(aio_spdifi_get_srm);

void aio_spdifi_set_srm(u32 idx, u32 margin)
{
	spdifi_srm[idx].config_value = margin;
	pr_debug("%s: idx:%d, margin:0x%08x\n", __func__, idx, margin);
}
EXPORT_SYMBOL(aio_spdifi_set_srm);

static u32 fs_map(u32 fs)
{
	switch (fs) {
	case 32000:
		return SPDIFI_FS_32KHZ;
	case 44100:
		return SPDIFI_FS_44KHZ;
	case 48000:
		return SPDIFI_FS_48KHZ;
	case 88200:
		return SPDIFI_FS_88KHZ;
	case 96000:
		return SPDIFI_FS_96KHZ;
	case 176000:
		return SPDIFI_FS_176KHZ;
	case 192000:
		return SPDIFI_FS_192KHZ;
	default:
		pr_err("%s, fs(%d) not supported\n", __func__, fs);
		return SPDIFI_FS_MAX;
	}
}

/*
 * spdifi_set_fs()
 * Note:
 * Following table is AS371 CTRL3's values,
 * it can be tunning on different SOC boards.
 * 32KHz	 0x1F171717
 * 44KHz	 0x1F171717
 * 48KHz	 0x1D151515
 * 88KHz	 0x1A101010
 * 96KHz	 0x1A101010
 * 176KHz	 0x0F080808
 * 192KHz	 0x0F070707
 * If CTRL2's ZERO_COUNT, ONE_COUNT,or CTRL3's value is not right,
 * there will be errors on SPDIFRX_STATUS,SPDIFRX_STATUS's ERR_4
 * and FPLL_STAT are set to 1,
 * ERR_4=1:sync Lock is not set by FPLL lock,
 * FPLL_STAT=1:Not locked yet.
 */
static int spdifi_set_fs(struct aio_priv *aio, u32 fs)
{
	T32SPDIFRX_CTRL_CTRL2 ctrl_2;
	T32SPDIFRX_CTRL_CTRL3 ctrl_3;
	u32 fs_id = fs_map(fs);

	if (fs_id >= SPDIFI_FS_MAX) {
		pr_err("fs %d unsupported\n", fs);
		return -EINVAL;
	}

	ctrl_2.u32 = aio_read(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL2));

	switch (fs_id) {
	case SPDIFI_FS_32KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 98;
		ctrl_2.uCTRL2_ONE_COUNT = 98;
		break;
	case SPDIFI_FS_44KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 71;
		ctrl_2.uCTRL2_ONE_COUNT = 71;
		break;
	case SPDIFI_FS_48KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 65;
		ctrl_2.uCTRL2_ONE_COUNT = 65;
		break;
	case SPDIFI_FS_88KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 15;
		ctrl_2.uCTRL2_ONE_COUNT = 15;
		break;
	case SPDIFI_FS_96KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 15;
		ctrl_2.uCTRL2_ONE_COUNT = 15;
		break;
	case SPDIFI_FS_176KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 15;
		ctrl_2.uCTRL2_ONE_COUNT = 15;
		break;
	case SPDIFI_FS_192KHZ:
		ctrl_2.uCTRL2_ZERO_COUNT = 15;
		ctrl_2.uCTRL2_ONE_COUNT = 15;
		break;
	case SPDIFI_FS_MAX:
	default:
		pr_err("fs %d unsupported\n", fs);
		return -EINVAL;
	}
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL2), ctrl_2.u32);

	if (spdifi_srm[fs_id].config_value != 0)
		ctrl_3.u32 =  spdifi_srm[fs_id].config_value;
	else
		ctrl_3.u32 =  spdifi_srm[fs_id].default_value;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL3), ctrl_3.u32);

	return 0;
}

static int aio_spdifi_wait_lock(struct aio_priv *aio, u32 timeout)
{
	u32 address;
	u32 val;

	if (timeout > 10000)
		timeout = 10000;
	address = RA_AIO_SPDIFRX_STATUS + RA_SPDIFRX_STATUS_STATUS;
	aio_write(aio, address, 0);
	do {
		usleep_range(1000, 1010);
		timeout--;
		if (timeout == 0) {
			pr_err("BDET/FPLL lock timeout, status(0x%08x)\n", val);
			return -ETIMEDOUT;
		}
		val = aio_read(aio, address);
	} while (((val&0x400000) != 0x400000) || ((val&0x1000000) != 0));
	return 0;
}

int __weak aio_spdifi_enable_sysclk(void *hd, bool enable)
{
	return 0;
}

int __weak aio_spdifi_enable_refclk(void *hd)
{
	return 0;
}

int aio_spdifi_config(void *hd, u32 fs, u32 width)
{
	u32 i;
	T32SPDIFRX_CTRL_CTRL1 ctrl_1;
	T32SPDIFRX_CTRL_CTRL2 ctrl_2;
	T32SPDIFRX_CTRL_CTRL6 ctrl_6;
	T32SPDIFRX_CTRL_CTRL11 ctrl_11;
	T32SPDIFRX_CTRL_CTRL12 ctrl_12;
	int ret = 0;
	struct aio_priv *aio = hd_to_aio(hd);
	u32 addr;

	aio_spdifi_enable_sysclk(hd, true);

	for (i = 0; i < 12; i++) {
		addr = CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1) + i * 4;
		spdifi_ctrl[i] = aio_read(aio, addr);
	}

	ctrl_1.u32 = spdifi_ctrl[0];
	ctrl_2.u32 = spdifi_ctrl[1];
	ctrl_6.u32 = spdifi_ctrl[5];
	ctrl_11.u32 = spdifi_ctrl[10];
	ctrl_12.u32 = spdifi_ctrl[11];

	pr_debug("%s: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		   __func__, ctrl_1.u32, ctrl_2.u32,
		 ctrl_6.u32, ctrl_11.u32, ctrl_12.u32);

	ctrl_1.uCTRL1_ERR0_CLR = 0x1;
	ctrl_1.uCTRL1_ERR5_CLR = 0x1;
	ctrl_1.uCTRL1_ERR1_EN = 0x1;
	ctrl_1.uCTRL1_ERR4_EN = 0x1;
	ctrl_1.uCTRL1_ERR5_EN = 0x1;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1), ctrl_1.u32);
	/* only vs6XX has refclk control */
	aio_spdifi_enable_refclk(hd);

	/* ctrl2: 0xf0ffa, same as default */
	ctrl_2.uCTRL2_REF_PULSE = 0xfa;
	ctrl_2.uCTRL2_ZERO_COUNT = 0xf;
	ctrl_2.uCTRL2_ONE_COUNT = 0xf;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL2), ctrl_2.u32);

	/* ctrl1: 0xca1 */
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1), ctrl_1.u32);

	/* ctrl2, ctrl3 */
	ret = spdifi_set_fs(aio, fs);

	/* ctrl1: 0x000004a1 */
	ctrl_1.uCTRL1_ERR5_EN = 0;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1), ctrl_1.u32);

	/* ctrl6: 0x12244760 */
	/* On the SPDIF line, when encoded with 32khz sampling,
	 * the count value of bit 0 and bit 1 sampled at high
	 * frequency clock is represented in this register.
	 * Sampling clock = 32Khz
	 * SPDIF line frequency = 32k * 2 * 128 = 8.192Mhz
	 * Since only high or low period is of the interest,
	 * sample time = 8.192M/2 = 4.096Mhz.
	 * If reference clock = 400Mhz
	 * Value = 244.1ns/ 2.5ns = 98 (decimal)
	 */
	ctrl_6.uCTRL6_VAL_32K = 0x60;
	ctrl_6.uCTRL6_VAL_44K = 0x47;
	ctrl_6.uCTRL6_VAL_88K = 0x24;
	ctrl_6.uCTRL6_VAL_176K = 0x12;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL6), ctrl_6.u32);

	/* ctrl1: 0x0c000ca1 */
	ctrl_1.uCTRL1_ERR5_EN = 0x01;
	ctrl_1.uCTRL1_OP_CTRL = 0x3;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1), ctrl_1.u32);

	/* ctrl12: */
	ctrl_12.uCTRL12_FRAME_WIDTH = 0x2;
	ctrl_12.uCTRL12_OP_MODE = 0x2;
	if (width == 16)
		ctrl_12.uCTRL12_DATAVALID_WIDTH = 0x0;
	else if (width == 24)
		ctrl_12.uCTRL12_DATAVALID_WIDTH = 0x3;
	else
		ctrl_12.uCTRL12_DATAVALID_WIDTH = 0x4;

	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL12), ctrl_12.u32);

	/* ctrl1: 0x0c000ca1 */
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1), ctrl_1.u32);

	/* ctrl11: */
	ctrl_11.uCTRL11_CLOSED_LOOP = 0x2;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL11), ctrl_11.u32);

	/* ctrl5: 0x000061a8 */
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL5), 0x61a8);

	ctrl_1.uCTRL1_SW_TRIG = 0x2;
	aio_write(aio, CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1), ctrl_1.u32);

	for (i = 0; i < 12; i++) {
		addr = CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1) + i * 4;
		pr_debug("SPDIFRX_CTRL%d(0x%08x):      0x%08x",
			i + 1, addr, aio_read(aio, addr));
	}
	/* follow diag code, wait clock lock */
	ret = aio_spdifi_wait_lock(aio, 2000);
	return ret;
}
EXPORT_SYMBOL(aio_spdifi_config);

void aio_spdifi_config_reset(void *hd)
{
	u32 i;
	struct aio_priv *aio = hd_to_aio(hd);

	for (i = 0; i < 12; i++)
		aio_write(aio,
			CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1) + i * 4,
			spdifi_ctrl[i]);
	aio_spdifi_enable_sysclk(hd, false);
}
EXPORT_SYMBOL(aio_spdifi_config_reset);

void aio_spdifi_sw_reset(void *hd)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32AIO_SW_RST reg;

	address = RA_AIO_SW_RST;
	reg.u32 = aio_read(aio, address);
	reg.uSW_RST_SPFRX = 0;
	aio_write(aio, address, reg.u32);
	usleep_range(1000, 1010);
	reg.uSW_RST_SPFRX = 1;
	aio_write(aio, address, reg.u32);
	pr_debug("%s: reg: 0x%08x\n", __func__, aio_read(aio, address));
}
EXPORT_SYMBOL(aio_spdifi_sw_reset);

void aio_spdifi_src_sel(void *hd, u32 clk_sel, u32 data_sel)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 address;
	T32SPDIFRX_CTRL_CTRL1 ctrl_1;

	address = CTRL_ADDR(RA_SPDIFRX_CTRL_CTRL1);
	ctrl_1.u32 = aio_read(aio, address);
	/* Clock selection
	 * 0: spdif ref clock 1: ARC PHY sync clock
	 * Not used , if HDMITX - phy does not provide ARC-Clock
	 */
	ctrl_1.uCTRL1_CLK_CTRL = clk_sel;
	/* Data Selection
	 * 0: Spdif pin data
	 * 1: ARC PHY data
	 */
	ctrl_1.uCTRL1_DATA_CTRL = data_sel;
	aio_write(aio, address, ctrl_1.u32);
}
EXPORT_SYMBOL(aio_spdifi_src_sel);
