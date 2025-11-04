/*************************************************************************/ /*
 MMNGR

 Copyright (C) 2015-2019 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

 The contents of this file are subject to the MIT license as set out below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 Alternatively, the contents of this file may be used under the terms of
 the GNU General Public License Version 2 ("GPL") in which case the provisions
 of GPL are applicable instead of those above.

 If you wish to allow use of your version of this file only under the terms of
 GPL, and not to allow others to use your version of this file under the terms
 of the MIT license, indicate your decision by deleting the provisions above
 and replace them with the notice and other provisions required by GPL as set
 out in the file called "GPL-COPYING" included in this distribution. If you do
 not delete the provisions above, a recipient may use your version of this file
 under the terms of either the MIT license or GPL.

 This License is also included in this distribution in the file called
 "MIT-COPYING".

 EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/
#include <linux/compat.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/bitmap.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/dma-map-ops.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/sizes.h>
#include <linux/sys_soc.h>

#include "mmngr_public.h"
#include "mmngr_private.h"

static spinlock_t		lock;
static struct BM		bm;
static struct BM		bm_ssp;
static struct LOSSY_DATA	lossy_entries[MAX_LOSSY_ENTRIES];
static struct MM_DRVDATA	*mm_drvdata;
static struct cma		*mm_cma_area;
static u64			mm_kernel_reserve_addr;
static u64			mm_kernel_reserve_size;
static u64			mm_lossybuf_addr;
static u64			mm_lossybuf_size;
static bool			have_lossy_entries;
#ifdef MMNGR_SSP_ENABLE
static bool			is_sspbuf_valid;
#endif
#ifndef IPMMU_MMU_SUPPORT
static u64			legacy_memory_addr;
static u64			legacy_memory_size;
#else
static bool			ipmmu_common_init_done;
static bool			is_mmu_tlb_disabled;
static u64			ipmmu_addr_section_0;
static u64			ipmmu_addr_section_1;
static u64			ipmmu_addr_section_2;
static u64			ipmmu_addr_section_3;
static phys_addr_t		*ipmmu_mmu_trans_table;
static pgdval_t			*ipmmu_mmu_pgd;

/* Translation table for all IPMMU in R-Car H3 */
static phys_addr_t h3_mmu_table[4] = {
	H3_IPMMU_ADDR_SECTION_0,
	H3_IPMMU_ADDR_SECTION_1,
	H3_IPMMU_ADDR_SECTION_2,
	H3_IPMMU_ADDR_SECTION_3,
};

/* Translation table for all IPMMU in R-Car M3 */
static phys_addr_t m3_mmu_table[4] = {
	M3_IPMMU_ADDR_SECTION_0,
	M3_IPMMU_ADDR_SECTION_1,
	M3_IPMMU_ADDR_SECTION_2,
	M3_IPMMU_ADDR_SECTION_3,
};

/* Translation table for all IPMMU in R-Car M3N */
static phys_addr_t m3n_mmu_table[4] = {
	M3N_IPMMU_ADDR_SECTION_0,
	M3N_IPMMU_ADDR_SECTION_1,
	M3N_IPMMU_ADDR_SECTION_2,
	M3N_IPMMU_ADDR_SECTION_3,
};

/* Translation table for all IPMMU in R-Car E3 */
static phys_addr_t e3_mmu_table[4] = {
	E3_IPMMU_ADDR_SECTION_0,
	E3_IPMMU_ADDR_SECTION_1,
	E3_IPMMU_ADDR_SECTION_2,
	E3_IPMMU_ADDR_SECTION_3,
};

/* Attribute structs describing Salvator-X revisions */
/* H3 */
static const struct soc_device_attribute r8a7795[]  = {
	{ .soc_id = "r8a7795" },
	{}
};

/* H3 WS1.0 and WS1.1 */
static const struct soc_device_attribute r8a7795es1[]  = {
	{ .soc_id = "r8a7795", .revision = "ES1.*" },
	{}
};

/* H3 ES2.0 */
static const struct soc_device_attribute r8a7795es2[]  = {
	{ .soc_id = "r8a7795", .revision = "ES2.0" },
	{}
};

/* M3 */
static const struct soc_device_attribute r8a7796[]  = {
	{ .soc_id = "r8a77961" },
	{}
};

/* M3 Ver.1.x */
static const struct soc_device_attribute r8a7796es1[]  = {
	{ .soc_id = "r8a7796", .revision = "ES1.*" },
	{}
};

/* M3N */
static const struct soc_device_attribute r8a77965[]  = {
	{ .soc_id = "r8a77965" },
	{}
};

/* M3N ES1.x*/
static const struct soc_device_attribute r8a77965es1[]  = {
	{ .soc_id = "r8a77965", .revision = "ES1.*" },
	{}
};

/* E3 */
static const struct soc_device_attribute r8a77990[]  = {
	{ .soc_id = "r8a77990" },
	{}
};

/* E3 ES1.x */
static const struct soc_device_attribute r8a77990es1[]  = {
	{ .soc_id = "r8a77990", .revision = "ES1.*" },
	{}
};

/* For IPMMU Main Memory (IPMMUMM) */
static struct hw_register ipmmumm_ip_regs[] = {
	{"IMCTR",	IMCTRn_OFFSET(CUR_TTSEL)},
	{"IMTTBCR",	IMTTBCRn_OFFSET(CUR_TTSEL)},
	{"IMTTUBR",	IMTTUBR0n_OFFSET(CUR_TTSEL)},
	{"IMTTLBR",	IMTTLBR0n_OFFSET(CUR_TTSEL)},
	{"IMMAIR0",	IMMAIR0n_OFFSET(CUR_TTSEL)},
	{"IMELAR",	IMELARn_OFFSET(CUR_TTSEL)},
	{"IMEUAR",	IMEUARn_OFFSET(CUR_TTSEL)},
	{"IMSTR",	IMSTRn_OFFSET(CUR_TTSEL)},
};

/* For each IPMMU cache */
static struct hw_register ipmmu_ip_regs[] = {
	{"IMCTR",	IMCTRn_OFFSET(CUR_TTSEL)},
	{"IMSCTLR",	IMSCTLR_OFFSET},
	/*
	 * IMUCTRn_OFFSET(n) is not defined here
	 * The register is calculated base on IP utlb_no value
	 */
};

static struct rcar_ipmmu ipmmumm = {
	.ipmmu_name	= "IPMMUMM",
	.base_addr	= IPMMUMM_BASE,
	.reg_count	= ARRAY_SIZE(ipmmumm_ip_regs),
	.ipmmu_reg	= ipmmumm_ip_regs,
	.masters_count	= 0,
	.ip_masters	= NULL,
};

static struct rcar_ipmmu **rcar_gen3_ipmmu;

/* R-Car H3 (R8A7795 ES1.x) */
static struct ip_master r8a7795es1_ipmmuvc0_masters[] = {
	{"FCP-CS", 0},
};

static struct rcar_ipmmu r8a7795es1_ipmmuvc0 = {
	.ipmmu_name	= "IPMMUVC0",
	.base_addr	= IPMMUVC0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795es1_ipmmuvc0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795es1_ipmmuvc0_masters,
};

static struct ip_master r8a7795es1_ipmmuvc1_masters[] = {
	{"FCP-CI ch0",	4},
	{"FCP-CI ch1",	5},
};

static struct rcar_ipmmu r8a7795es1_ipmmuvc1 = {
	.ipmmu_name	= "IPMMUVC1",
	.base_addr	= IPMMUVC1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795es1_ipmmuvc1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795es1_ipmmuvc1_masters,
};

static struct ip_master r8a7795es1_ipmmuvp_masters[] = {
	{"FCP-F ch0",	0},
	{"FCP-F ch1",	1},
	{"FCP-F ch2",	2},
	{"FCP-VB ch0",	5},
	{"FCP-VB ch1",	7},
	{"FCP-VI ch0",	8},
	{"FCP-VI ch1",	9},
	{"FCP-VI ch2",	10},
};

static struct rcar_ipmmu r8a7795es1_ipmmuvp = {
	.ipmmu_name	= "IPMMUVP",
	.base_addr	= IPMMUVP_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795es1_ipmmuvp_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795es1_ipmmuvp_masters,
};

#ifdef MMNGR_SSP_ENABLE
static struct ip_master r8a7795es1_ipmmusy_masters[] = {
	{"SSP1",	4},
};

static struct rcar_ipmmu r8a7795es1_ipmmusy = {
	.ipmmu_name	= "IPMMUSY",
	.base_addr	= IPMMUSY_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795es1_ipmmusy_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795es1_ipmmusy_masters,
};
#endif

static struct rcar_ipmmu *r8a7795es1_ipmmu[] = {
#ifdef MMNGR_SSP_ENABLE
	&r8a7795es1_ipmmusy,
#endif
	&r8a7795es1_ipmmuvp,
	&r8a7795es1_ipmmuvc0,
	&r8a7795es1_ipmmuvc1,
	NULL, /* End of list */
};

/* R-Car H3 (R8A7795 ES2.0) */
static struct ip_master r8a7795_ipmmuvc0_masters[] = {
	{"FCP-CS osid0", 8},
};

static struct rcar_ipmmu r8a7795_ipmmuvc0 = {
	.ipmmu_name	= "IPMMUVC0",
	.base_addr	= IPMMUVC0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795_ipmmuvc0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795_ipmmuvc0_masters,
};

static struct ip_master r8a7795_ipmmuvc1_masters[] = {
	{"FCP-CS osid0", 8},
};

static struct rcar_ipmmu r8a7795_ipmmuvc1 = {
	.ipmmu_name	= "IPMMUVC1",
	.base_addr	= IPMMUVC1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795_ipmmuvc1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795_ipmmuvc1_masters,
};

static struct ip_master r8a7795_ipmmuvp0_masters[] = {
	{"FCP-F ch0",	0},
	{"FCP-VB ch0",	5},
	{"FCP-VI ch0",	8},
};

static struct rcar_ipmmu r8a7795_ipmmuvp0 = {
	.ipmmu_name	= "IPMMUVP0",
	.base_addr	= IPMMUVP0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795_ipmmuvp0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795_ipmmuvp0_masters,
};

static struct ip_master r8a7795_ipmmuvp1_masters[] = {
	{"FCP-F ch1",	1},
	{"FCP-VB ch1",	7},
	{"FCP-VI ch1",	9},
};

static struct rcar_ipmmu r8a7795_ipmmuvp1 = {
	.ipmmu_name	= "IPMMUVP1",
	.base_addr	= IPMMUVP1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795_ipmmuvp1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795_ipmmuvp1_masters,
};

#ifdef MMNGR_SSP_ENABLE
static struct ip_master r8a7795_ipmmuds1_masters[] = {
	{"SSP1",	36},
};

static struct rcar_ipmmu r8a7795_ipmmuds1 = {
	.ipmmu_name	= "IPMMUDS1",
	.base_addr	= IPMMUDS1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7795_ipmmuds1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7795_ipmmuds1_masters,
};
#endif

static struct rcar_ipmmu *r8a7795_ipmmu[] = {
#ifdef MMNGR_SSP_ENABLE
	&r8a7795_ipmmuds1,
#endif
	&r8a7795_ipmmuvp0,
	&r8a7795_ipmmuvp1,
	&r8a7795_ipmmuvc0,
	&r8a7795_ipmmuvc1,
	NULL, /* End of list */
};

/* R-Car M3 (R8A7796) */
static struct ip_master r8a7796_ipmmuvi_masters[] = {
	{"FCP-VB",  5},
};

static struct rcar_ipmmu r8a7796_ipmmuvi = {
	.ipmmu_name	= "IPMMUVI",
	.base_addr	= IPMMUVI_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7796_ipmmuvi_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7796_ipmmuvi_masters,
};

static struct ip_master r8a7796_ipmmuvc0_masters[] = {
	{"FCP-CS",  1},
	{"FCP-CS",  8},
	{"FCP-F",  16},
	{"FCP-VI", 19},
};

static struct rcar_ipmmu r8a7796_ipmmuvc0 = {
	.ipmmu_name	= "IPMMUVC0",
	.base_addr	= IPMMUVC0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7796_ipmmuvc0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7796_ipmmuvc0_masters,
};

#ifdef MMNGR_SSP_ENABLE
static struct ip_master r8a7796_ipmmuds1_masters[] = {
	{"SSP1",	36},
};

static struct rcar_ipmmu r8a7796_ipmmuds1 = {
	.ipmmu_name	= "IPMMUDS1",
	.base_addr	= IPMMUDS1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7796_ipmmuds1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7796_ipmmuds1_masters,
};
#endif

static struct rcar_ipmmu *r8a7796_ipmmu[] = {
#ifdef MMNGR_SSP_ENABLE
	&r8a7796_ipmmuds1,
#endif
	&r8a7796_ipmmuvi,
	&r8a7796_ipmmuvc0,
	NULL, /* End of list */
};

/* R-Car M3 (R8A7796 Ver.1.x) */
static struct ip_master r8a7796es1_ipmmuvi_masters[] = {
	{"FCP-VB",  5},
};

static struct rcar_ipmmu r8a7796es1_ipmmuvi = {
	.ipmmu_name	= "IPMMUVI",
	.base_addr	= IPMMUVI_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7796es1_ipmmuvi_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7796es1_ipmmuvi_masters,
};

static struct ip_master r8a7796es1_ipmmuvc0_masters[] = {
	{"FCP-CI",  4},
	{"FCP-CS",  8},
	{"FCP-F",  16},
	{"FCP-VI", 19},
};

static struct rcar_ipmmu r8a7796es1_ipmmuvc0 = {
	.ipmmu_name	= "IPMMUVC0",
	.base_addr	= IPMMUVC0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7796es1_ipmmuvc0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7796es1_ipmmuvc0_masters,
};

#ifdef MMNGR_SSP_ENABLE
static struct ip_master r8a7796es1_ipmmuds1_masters[] = {
	{"SSP1",	36},
};

static struct rcar_ipmmu r8a7796es1_ipmmuds1 = {
	.ipmmu_name	= "IPMMUDS1",
	.base_addr	= IPMMUDS1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a7796es1_ipmmuds1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a7796es1_ipmmuds1_masters,
};
#endif

static struct rcar_ipmmu *r8a7796es1_ipmmu[] = {
#ifdef MMNGR_SSP_ENABLE
	&r8a7796es1_ipmmuds1,
#endif
	&r8a7796es1_ipmmuvi,
	&r8a7796es1_ipmmuvc0,
	NULL, /* End of list */
};

/* R-Car M3N (R8A77965 ES1.0) */
static struct ip_master r8a77965_ipmmuvc0_masters[] = {
	{"FCP-CS osid0", 8},
};

static struct rcar_ipmmu r8a77965_ipmmuvc0 = {
	.ipmmu_name	= "IPMMUVC0",
	.base_addr	= IPMMUVC0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a77965_ipmmuvc0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a77965_ipmmuvc0_masters,
};

static struct ip_master r8a77965_ipmmuvp0_masters[] = {
	{"FCP-F ch0",	0},
	{"FCP-VB ch0",	5},
	{"FCP-VI ch0",	8},
};

static struct rcar_ipmmu r8a77965_ipmmuvp0 = {
	.ipmmu_name	= "IPMMUVP0",
	.base_addr	= IPMMUVP0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a77965_ipmmuvp0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a77965_ipmmuvp0_masters,
};

#ifdef MMNGR_SSP_ENABLE
static struct ip_master r8a77965_ipmmuds1_masters[] = {
	{"SSP1",	36},
};

static struct rcar_ipmmu r8a77965_ipmmuds1 = {
	.ipmmu_name	= "IPMMUDS1",
	.base_addr	= IPMMUDS1_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a77965_ipmmuds1_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a77965_ipmmuds1_masters,
};
#endif

static struct rcar_ipmmu *r8a77965_ipmmu[] = {
#ifdef MMNGR_SSP_ENABLE
	&r8a77965_ipmmuds1,
#endif
	&r8a77965_ipmmuvp0,
	&r8a77965_ipmmuvc0,
	NULL, /* End of list */
};

/* R-Car E3 (R8A77990) */
static struct ip_master r8a77990_ipmmuvc0_masters[] = {
	{"FCP-CS osid0", 8},
};

static struct rcar_ipmmu r8a77990_ipmmuvc0 = {
	.ipmmu_name	= "IPMMUVC0",
	.base_addr	= IPMMUVC0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a77990_ipmmuvc0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a77990_ipmmuvc0_masters,
};

static struct ip_master r8a77990_ipmmuvp0_masters[] = {
	{"FCP-F ch0",	0},
	{"FCP-VB ch0",	5},
	{"FCP-VI ch0",	8},
};

static struct rcar_ipmmu r8a77990_ipmmuvp0 = {
	.ipmmu_name	= "IPMMUVP0",
	.base_addr	= IPMMUVP0_BASE,
	.reg_count	= ARRAY_SIZE(ipmmu_ip_regs),
	.masters_count	= ARRAY_SIZE(r8a77990_ipmmuvp0_masters),
	.ipmmu_reg	= ipmmu_ip_regs,
	.ip_masters	= r8a77990_ipmmuvp0_masters,
};

static struct rcar_ipmmu *r8a77990_ipmmu[] = {
	&r8a77990_ipmmuvp0,
	&r8a77990_ipmmuvc0,
	NULL, /* End of list */
};

static struct rcar_ipmmu *r8a77990_disable_mmu_tlb[] = {
	&r8a77990_ipmmuvc0,
	NULL, /* End of list */
};

#endif /* IPMMU_MMU_SUPPORT */

static int mm_ioc_alloc(struct device *mm_dev,
			int __user *in,
			struct MM_PARAM *out)
{
	int		ret = 0;
	struct MM_PARAM	tmp;

	if (copy_from_user(&tmp, (void __user *)in, sizeof(struct MM_PARAM))) {
		pr_err("%s EFAULT\n", __func__);
		ret = -EFAULT;
		return ret;
	}
	out->size = tmp.size;
	out->kernel_virt_addr = (unsigned long)dma_alloc_coherent(mm_dev,
						out->size,
						(dma_addr_t *)&out->phy_addr,
						GFP_KERNEL);
	if (!out->kernel_virt_addr) {
		pr_err("%s ENOMEM\n", __func__);
		ret = -ENOMEM;
		out->phy_addr = 0;
		out->hard_addr = 0;
		return ret;
	}

#ifdef IPMMU_MMU_SUPPORT
	out->hard_addr = ipmmu_mmu_phys2virt(out->phy_addr);
#else
	out->hard_addr = (unsigned int)out->phy_addr;
#endif

	out->flag = tmp.flag;

	return ret;
}

static void mm_ioc_free(struct device *mm_dev, struct MM_PARAM *p)
{
	dma_free_coherent(mm_dev, p->size, (void *)p->kernel_virt_addr,
			(dma_addr_t)p->phy_addr);
	memset(p, 0, sizeof(struct MM_PARAM));
}

static int mm_ioc_set(int __user *in, struct MM_PARAM *out)
{
	int		ret = 0;
	struct MM_PARAM	tmp;

	if (copy_from_user(&tmp, in, sizeof(struct MM_PARAM))) {
		ret = -EFAULT;
		return ret;
	}

	out->user_virt_addr = tmp.user_virt_addr;

	return ret;
}

static int mm_ioc_get(struct MM_PARAM *in, int __user *out)
{
	int	ret = 0;

	if (copy_to_user(out, in, sizeof(struct MM_PARAM))) {
		ret = -EFAULT;
		return ret;
	}

	return ret;
}

static int alloc_bm(struct BM *pb,
		phys_addr_t top_phy_addr,
		unsigned long size,
		unsigned long order)
{
	unsigned long	nbits;
	unsigned long	nbytes;

	pr_debug("MMNGR: CO from 0x%llx to 0x%llx\n",
	top_phy_addr, top_phy_addr + size - 1);

	if (pb == NULL)
		return -1;

	nbits = (size + ((1UL << order) - 1)) >> order;
	nbytes = (nbits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	pb->bits = kzalloc(nbytes, GFP_KERNEL);
	if (pb->bits == NULL)
		return -1;
	pb->order = order;
	pb->top_phy_addr = top_phy_addr;
	pb->end_bit = nbits;

	return 0;
}

static void free_bm(struct BM *pb)
{
	kfree(pb->bits);
}

static int mm_ioc_alloc_co(struct BM *pb, int __user *in, struct MM_PARAM *out)
{
	int		ret = 0;
	unsigned long	nbits;
	unsigned long	start_bit;
	struct MM_PARAM	tmp;

	if (copy_from_user(&tmp, in, sizeof(struct MM_PARAM))) {
		pr_err("%s EFAULT\n", __func__);
		ret = -EFAULT;
		return ret;
	}

	out->size = tmp.size;
	nbits = (out->size + (1UL << pb->order) - 1) >> pb->order;

	spin_lock(&lock);
	start_bit = bitmap_find_next_zero_area(pb->bits, pb->end_bit, 0,
						nbits, 0);
	if (start_bit >= pb->end_bit) {
		pr_err("start(%ld), end(%ld)\n", start_bit,
			pb->end_bit);
		spin_unlock(&lock);
		out->phy_addr = 0;
		out->hard_addr = 0;
		return -ENOMEM;
	}
	bitmap_set(pb->bits, start_bit, nbits);
	spin_unlock(&lock);

	out->phy_addr = pb->top_phy_addr + (start_bit << pb->order);

#ifdef IPMMU_MMU_SUPPORT
	out->hard_addr = ipmmu_mmu_phys2virt(out->phy_addr);
#else
	out->hard_addr = (unsigned int)out->phy_addr;
#endif
	out->flag = tmp.flag;

	return 0;
}

static int find_lossy_entry(unsigned int flag, int *entry)
{
	u32	i, fmt;
	int		ret;

	if (!have_lossy_entries)
		return -EINVAL; /* Not supported */

	fmt = ((flag & 0xF0) >> 4) - 1;
	pr_debug("Requested format 0x%x.\n", fmt);

	for (i = 0; i < MAX_LOSSY_ENTRIES; i++) {
		if (lossy_entries[i].fmt == fmt)
			break;
	}

	if (i < MAX_LOSSY_ENTRIES) {
		*entry = i;
		ret = 0;
		pr_debug("Found entry no.%d\n", i);
	} else {
		*entry = -1;
		ret = -EINVAL; /* Not supported */
		pr_debug("No entry is found!\n");
	}

	return ret;
}

static int mm_ioc_alloc_co_select(int __user *in, struct MM_PARAM *out)
{
	int		ret = 0;
	int		entry = 0;
	struct MM_PARAM	tmp;

	if (copy_from_user(&tmp, in, sizeof(struct MM_PARAM))) {
		pr_err("%s EFAULT\n", __func__);
		ret = -EFAULT;
		return ret;
	}

	if (tmp.flag == MM_CARVEOUT)
		ret = mm_ioc_alloc_co(&bm, in, out);
#ifndef MMNGR_SSP_ENABLE
	else if (tmp.flag == MM_CARVEOUT_SSP) {
		pr_err("%s EINVAL\n", __func__);
		ret = -EINVAL;
	}
#else
	else if (tmp.flag == MM_CARVEOUT_SSP)
		if (is_sspbuf_valid)
			ret = mm_ioc_alloc_co(&bm_ssp, in, out);
		else
			ret = -ENOMEM;
#endif
	else if ((tmp.flag & 0xF) == MM_CARVEOUT_LOSSY) {
		ret = find_lossy_entry(tmp.flag, &entry);
		if (ret)
			return ret;
		ret = mm_ioc_alloc_co(lossy_entries[entry].bm_lossy, in, out);
	}

	return ret;
}

static void mm_ioc_free_co(struct BM *pb, struct MM_PARAM *p)
{
	unsigned long	nbits;
	unsigned long	start_bit;

	start_bit = (p->phy_addr - pb->top_phy_addr) >> pb->order;
	nbits = (p->size + (1UL << pb->order) - 1) >> pb->order;

	spin_lock(&lock);
	bitmap_clear(pb->bits, start_bit, nbits);
	spin_unlock(&lock);
	memset(p, 0, sizeof(struct MM_PARAM));
}

static void mm_ioc_free_co_select(struct MM_PARAM *p)
{
	int		entry = 0;

	if (p->flag == MM_CARVEOUT) {
		mm_ioc_free_co(&bm, p);
	} else if (p->flag == MM_CARVEOUT_SSP) {
		mm_ioc_free_co(&bm_ssp, p);
	} else if ((p->flag & 0xF) == MM_CARVEOUT_LOSSY) {
		find_lossy_entry(p->flag, &entry);
		if (entry >= 0)
			mm_ioc_free_co(lossy_entries[entry].bm_lossy, p);
	}
}

static int mm_ioc_share(int __user *in, struct MM_PARAM *out)
{
	int		ret = 0;
	struct MM_PARAM	tmp;

	if (copy_from_user(&tmp, in, sizeof(struct MM_PARAM))) {
		ret = -EFAULT;
		return ret;
	}

	out->phy_addr = tmp.phy_addr;
	out->size = tmp.size;

	return ret;
}

/* change virtual address to physical address */
static void mm_cnv_addr(struct MM_PARAM *tmp)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;
	unsigned long start;
	unsigned long pfn;
	int ret = 0;

	mmap_read_lock(mm);
	vma = find_vma(mm, tmp->user_virt_addr);
	start = tmp->user_virt_addr & PAGE_MASK;
	ret = follow_pfn(vma, start, &pfn);
	if(ret != 0)
		 pr_warn("Could not convert virt[0x%lx] addr", tmp->user_virt_addr);
	tmp->hard_addr = pfn << PAGE_SHIFT;
	mmap_read_unlock(mm);

	return;
}

static void mmngr_dev_set_cma_area(struct device *dev, struct cma *cma)
{
	if (dev)
		dev->cma_area = cma;
}

static int open(struct inode *inode, struct file *file)
{
	struct MM_PARAM	*p;

	p = kzalloc(sizeof(struct MM_PARAM), GFP_KERNEL);
	if (!p)
		return -1;

	file->private_data = p;

	return 0;
}

static int close(struct inode *inode, struct file *file)
{
	struct MM_PARAM	*p = file->private_data;
	struct BM	*pb;
	struct device	*mm_dev;
	int		entry = 0;

	if (p) {
		if ((p->flag == MM_KERNELHEAP || p->flag == MM_KERNELHEAP_CACHED)
		&& (p->kernel_virt_addr != 0)) {
			pr_warn("%s MMD kernelheap warning\n", __func__);
			mm_dev = mm_drvdata->mm_dev;
			dma_free_coherent(mm_dev, p->size,
					(void *)p->kernel_virt_addr,
					(dma_addr_t)p->phy_addr);
		} else if ((p->flag == MM_CARVEOUT)
		&& (p->phy_addr != 0)) {
			pr_warn("%s MMD carveout warning\n", __func__);
			pb = &bm;
			mm_ioc_free_co(pb, p);
		} else if ((p->flag == MM_CARVEOUT_SSP)
		&& (p->phy_addr != 0)) {
#ifdef MMNGR_SSP_ENABLE
			if (is_sspbuf_valid) {
				pr_warn("%s MMD carveout SSP warning\n",
					__func__);
				pb = &bm_ssp;
				mm_ioc_free_co(pb, p);
			}
#endif
		} else if (((p->flag & 0xF) == MM_CARVEOUT_LOSSY)
		&& (p->phy_addr != 0)) {
			pr_warn("%s MMD carveout LOSSY warning\n", __func__);
			find_lossy_entry(p->flag, &entry);
			if (entry >= 0) {
				pb = lossy_entries[entry].bm_lossy;
				mm_ioc_free_co(pb, p);
			}
		}

		kfree(p);
		file->private_data = NULL;
	}

	return 0;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int		ercd;
	int		ret;
	struct MM_PARAM	*p = file->private_data;
	struct MM_CACHE_PARAM cachep;
	struct device	*mm_dev;

	mm_dev = mm_drvdata->mm_dev;

	switch (cmd) {
	case MM_IOC_ALLOC:
		ercd = mm_ioc_alloc(mm_dev, (int __user *)arg, p);
		if (ercd) {
			pr_err("%s MMD ALLOC ENOMEM\n", __func__);
			ret = ercd;
			goto exit;
		}
		break;
	case MM_IOC_FREE:
		mm_ioc_free(mm_dev, p);
		break;
	case MM_IOC_SET:
		ercd = mm_ioc_set((int __user *)arg, p);
		if (ercd) {
			pr_err("%s MMD SET EFAULT\n", __func__);
			ret = ercd;
			goto exit;
		}
		break;
	case MM_IOC_GET:
		ercd = mm_ioc_get(p, (int __user *)arg);
		if (ercd) {
			pr_err("%s MMD GET EFAULT\n", __func__);
			ret = ercd;
			goto exit;
		}
		break;
	case MM_IOC_ALLOC_CO:
		ercd = mm_ioc_alloc_co_select((int __user *)arg, p);
		if (ercd) {
			pr_err("%s MMD C ALLOC ENOMEM\n", __func__);
			ret = ercd;
			goto exit;
		}
		break;
	case MM_IOC_FREE_CO:
		mm_ioc_free_co_select(p);
		break;
	case MM_IOC_SHARE:
		ercd = mm_ioc_share((int __user *)arg, p);
		if (ercd) {
			pr_err("%s MMD C SHARE EFAULT\n", __func__);
			ret = ercd;
			goto exit;
		}
		break;
	case MM_IOC_FLUSH:
		if (copy_from_user(&cachep, (void __user *)arg, sizeof(struct MM_CACHE_PARAM)))
			return -EFAULT;

		dma_sync_single_for_device(mm_dev, p->hard_addr + cachep.offset,
					cachep.len, DMA_FROM_DEVICE);
		break;
	case MM_IOC_INVAL:
		if (copy_from_user(&cachep, (void __user *)arg, sizeof(struct MM_CACHE_PARAM)))
			return -EFAULT;

		dma_sync_single_for_cpu(mm_dev, p->hard_addr + cachep.offset,
					cachep.len, DMA_TO_DEVICE);
		break;
	case MM_IOC_VTOP:   /* change virtual address to physical address */
		{
			struct MM_PARAM temp;

			mm_ioc_set((int __user *) arg, &temp);
			mm_cnv_addr(&temp);
			mm_ioc_get(&temp, (int __user *)arg);
			break;
		}
	default:
		pr_err("%s MMD CMD EFAULT\n", __func__);
		ret = -EFAULT;
		goto exit;
	}

	return 0;

exit:
	return ret;
}

static int mmap(struct file *filp, struct vm_area_struct *vma)
{
	phys_addr_t	start;
	phys_addr_t	off;
	unsigned long	len;
	struct MM_PARAM	*p = filp->private_data;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;

	off = vma->vm_pgoff << PAGE_SHIFT;
#ifdef IPMMU_MMU_SUPPORT
	start = ipmmu_mmu_virt2phys((unsigned int)p->phy_addr);
	if (!start)
		return -EINVAL;
#else
	start = p->phy_addr;
#endif

	len = PAGE_ALIGN((start & ~PAGE_MASK) + p->size);

	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;

	if (p->flag != MM_KERNELHEAP_CACHED) {
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
	} else {
		vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
	}

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int validate_memory_map(void)
{
	int ret = 0;
#ifdef MMNGR_SSP_ENABLE
	unsigned long buf_size;
	const char *buf_name;
#endif

#ifndef IPMMU_MMU_SUPPORT
	if ((mm_kernel_reserve_addr + mm_kernel_reserve_size) > 
	    (legacy_memory_addr + legacy_memory_size)) {
		pr_warn("The kernel reserved area (0x%09llx - 0x%09llx) is out of "\
			"the legacy area of memory (0x%09llx - 0x%09llx) \n",
			mm_kernel_reserve_addr,
			mm_kernel_reserve_addr + mm_kernel_reserve_size,
			legacy_memory_addr,
			legacy_memory_addr + legacy_memory_size);
		return -1;
	}
#endif

	if (mm_kernel_reserve_size < MM_OMXBUF_SIZE) {
		pr_warn("The size (0x%x) of OMXBUF is over "\
			"the kernel reserved size (0x%llx) for Multimedia.\n",
			MM_OMXBUF_SIZE, mm_kernel_reserve_size);
		pr_warn("Failed to initialize MMNGR.\n");
		ret = -1;
	}

#ifdef MMNGR_SSP_ENABLE
	buf_size = MM_OMXBUF_SIZE + MM_SSPBUF_SIZE;
	buf_name = "OMXBUF and SSPBUF";

	if (mm_kernel_reserve_size >= buf_size) {
		if ((MM_SSPBUF_ADDR >= mm_kernel_reserve_addr) &&
		    (MM_SSPBUF_ADDR <= (mm_kernel_reserve_addr
					+ mm_kernel_reserve_size
					- MM_SSPBUF_SIZE))) {
			is_sspbuf_valid = true;
		} else {
			pr_warn("The SSPBUF (0x%lx - 0x%lx) is out of range of"\
				"the kernel reserved size (0x%llx - 0x%llx) for Multimedia.\n",
				MM_SSPBUF_ADDR, MM_SSPBUF_ADDR + MM_SSPBUF_SIZE,
				mm_kernel_reserve_addr,
				mm_kernel_reserve_addr
				+ mm_kernel_reserve_size);
			pr_warn("Not able to allocate buffer in SSPBUF.\n");

			is_sspbuf_valid = false;
		}
	} else {
		pr_warn("The total size (0x%lx) of %s is over "\
			"the kernel reserved size (0x%llx) for Multimedia.\n",
			buf_size, buf_name, mm_kernel_reserve_size);
		pr_warn("Not able to allocate buffer in SSPBUF.\n");

		is_sspbuf_valid = false;
	}
#endif
	return ret;
}

static int _parse_reserved_mem_dt(struct device_node *np,
				  const char *phandle_name,
				  const char *match,
				  u64 *addr, u64 *size)
{
	const __be32 *regaddr_p = NULL;
	struct device_node *node = NULL;
	int index;
	int prop_size = 0;
	int ret = 0;
	*addr = 0;
	*size = 0;

	of_get_property(np, phandle_name, &prop_size);
	for (index = 0; index < prop_size; index++) {
		node = of_parse_phandle(np, phandle_name, index);
		if (node) {
			if (strstr(match, node->name)) {
				regaddr_p = of_get_address(node, 0, size, NULL);
				break;
			}
		}
	}

	if (regaddr_p)
		*addr = of_translate_address(node, regaddr_p);
	else
		ret = -1;

	of_node_put(node);
	return ret;
}

#ifndef IPMMU_MMU_SUPPORT
static int parse_legacy_memory_node(struct device_node *np)
{
	int ret = -1;
	u64 memory_addr;
	u64 memory_size;
	struct device_node *node = NULL;

	for_each_child_of_node(np, node) {
		if(of_property_match_string(node, "device_type", "memory") >= 0){
			const __be32 *regaddr_p = NULL;
			regaddr_p = of_get_address(node, 0, &memory_size, NULL);
			if (regaddr_p)
				memory_addr = of_translate_address(node, regaddr_p);
			if ( memory_size && ((memory_addr & 0xF00000000) == 0x0)) {
				legacy_memory_addr = memory_addr;
				legacy_memory_size = memory_size;
				ret = 0;
				break;
			}
		}
	}
	return ret;
}
#endif

static const struct soc_device_attribute rzv2n_match[] = {
	{ .family = "RZ/V2N" },
	{ /* sentinel*/ }
};

static int parse_reserved_mem_dt(struct device_node *np)
{
	int ret = 0;

	/* Parse reserved memory for multimedia */
	ret = _parse_reserved_mem_dt(np, "memory-region",
				     "linux,multimedia",
				     &mm_kernel_reserve_addr,
				     &mm_kernel_reserve_size);
	if (ret) {
		pr_warn("Failed to parse MMP reserved area" \
			 "(linux,multimedia) from DT\n");
		return ret;
	}

	if (!soc_device_match(rzv2n_match)) {
		/* Parse reserved memory for lossy compression feature */
		ret = _parse_reserved_mem_dt(np, "memory-region",
					     "linux,lossy_decompress",
					     &mm_lossybuf_addr,
					     &mm_lossybuf_size);
		if (ret) {
			pr_warn("Failed to parse Lossy reserved area" \
				"(linux,lossy_decompress) from DT\n");
			ret = 0; /* Let MMNGR support other features */
		}
	}
	return ret;
}

static int init_lossy_info(void)
{
	int ret = 0;
	void __iomem *mem;
	u32 i, fmt;
	u64 start, end;
	struct BM *bm_lossy;
	struct LOSSY_INFO *p;
	u32 total_lossy_size = 0;

	have_lossy_entries = false;

	mem = ioremap(MM_LOSSY_SHARED_MEM_ADDR,
		      MM_LOSSY_SHARED_MEM_SIZE);
	if (mem == NULL)
		return -1;

	p = (struct LOSSY_INFO __force *)mem;

	for (i = 0; i < MAX_LOSSY_ENTRIES; i++) {
		/* Validate the entry */
		if ((p->magic != MM_LOSSY_INFO_MAGIC)
		|| (p->a0 == 0) || (p->b0 == 0)
		|| ((p->a0 & MM_LOSSY_ENABLE_MASK) == 0))
			break;

		/* Parse entry information */
		start = (p->a0 & MM_LOSSY_ADDR_MASK) << 20;
		end = (p->b0 & MM_LOSSY_ADDR_MASK) << 20;
		fmt = (p->a0 & MM_LOSSY_FMT_MASK) >> 29;

		/* Validate kernel reserved mem for Lossy */
		if (i == 0 && start != mm_lossybuf_addr) {
			pr_warn("Mismatch between the start address (0x%llx) "\
				"of reserved mem and start address (0x%llx) "\
				"of Lossy enabled area.\n", mm_lossybuf_addr,
				start);
			break;
		}

		total_lossy_size += end - start;
		if (total_lossy_size > mm_lossybuf_size) {
			pr_warn("Size of Lossy enabled areas (0x%x) is over "\
				"the size of reserved mem(0x%x).\n",
				total_lossy_size,
				(unsigned int)mm_lossybuf_size);
			break;
		}

		/* Allocate bitmap for entry */
		bm_lossy = kzalloc(sizeof(struct BM), GFP_KERNEL);
		if (bm_lossy == NULL)
			break;

		ret = alloc_bm(bm_lossy, start, end - start, MM_CO_ORDER);
		if (ret)
			break;

		pr_debug("Support entry %d with format 0x%x.\n", i, fmt);

		lossy_entries[i].fmt = fmt;
		lossy_entries[i].bm_lossy = bm_lossy;

		p++;
	}

	if (i > 0)
		have_lossy_entries = true;

	iounmap(mem);
	return ret;
}

#ifdef IPMMU_MMU_SUPPORT
static int __handle_registers(struct rcar_ipmmu *ipmmu, unsigned int handling)
{
	int ret = 0;
	unsigned int j;

	phys_addr_t base_addr = ipmmu->base_addr;
	void __iomem *virt_addr = ipmmu->virt_addr;
	unsigned int reg_count = ipmmu->reg_count;
	unsigned int masters_count = ipmmu->masters_count;
	struct hw_register *ipmmu_reg = ipmmu->ipmmu_reg;
	struct ip_master *ip_masters = ipmmu->ip_masters;

	if (handling == DO_IOREMAP) { /* ioremap */
		/* IOREMAP registers in an IPMMU */
		ipmmu->virt_addr = ioremap(base_addr, REG_SIZE);
		if (ipmmu->virt_addr == NULL)
			ret = -1;

		pr_debug("\n%s: DO_IOREMAP: %s, virt_addr 0x%lx\n",
			 __func__, ipmmu->ipmmu_name,
			 (unsigned long)ipmmu->virt_addr);

	} else if (handling == DO_IOUNMAP) { /* iounmap*/
		/* IOUNMAP registers in an IPMMU */
		iounmap(ipmmu->virt_addr);
		ipmmu->virt_addr = NULL;
		pr_debug("%s: DO_IOUNMAP: %s, virt_addr 0x%lx\n",
			 __func__, ipmmu->ipmmu_name,
			 (unsigned long)ipmmu->virt_addr);

	} else if (handling == ENABLE_UTLB) { /* Enable utlb for IP master */
		for (j = 0; j < masters_count; j++)
			iowrite32(IMUCTR_VAL,
				  virt_addr +
				  IMUCTRn_OFFSET(ip_masters[j].utlb_no));

	} else if (handling == DISABLE_UTLB) { /* Disable utlb for IP master */
		for (j = 0; j < masters_count; j++)
			iowrite32(~IMUCTR_VAL & ioread32(
				virt_addr + IMUCTRn_OFFSET(
						ip_masters[j].utlb_no)),
				virt_addr + IMUCTRn_OFFSET(
						ip_masters[j].utlb_no));

	} else if (handling == ENABLE_MMU_MM) { /* Enable MMU of IPMMUMM */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMCTR"))
				break;
		}

		if (j < reg_count) /* Found IMCTR */
			iowrite32(IMCTR_MM_VAL | ioread32(
				  virt_addr + ipmmu_reg[j].reg_offset),
				  virt_addr + ipmmu_reg[j].reg_offset);
		else
			ret = -1;

	} else if (handling == DISABLE_MMU_MM) { /* Disable MMU of IPMMUMM */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMCTR"))
				break;
		}

		if (j < reg_count) /* Found IMCTR */
			iowrite32(~IMCTR_MM_VAL & ioread32(
				  virt_addr + ipmmu_reg[j].reg_offset),
				  virt_addr + ipmmu_reg[j].reg_offset);
		else
			ret = -1;

	} else if (handling == ENABLE_MMU) { /* Enable MMU of IPMMU */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMCTR"))
				break;
		}

		if (j < reg_count) /* Found IMCTR */
			iowrite32(IMCTR_VAL | ioread32(
				  virt_addr + ipmmu_reg[j].reg_offset),
				  virt_addr + ipmmu_reg[j].reg_offset);
		else
			ret = -1;

	} else if (handling == DISABLE_MMU) { /* Disable MMU of IPMMU */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMCTR"))
				break;
		}

		if (j < reg_count) /* Found IMCTR */
			iowrite32(~IMCTR_VAL & ioread32(
				  virt_addr + ipmmu_reg[j].reg_offset),
				  virt_addr + ipmmu_reg[j].reg_offset);
		else
			ret = -1;

	} else if (handling == DISABLE_MMU_TLB) { /* Disable MMU TLB */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMSCTLR"))
				break;
		}

		if (j < reg_count) /* Found IMSCTLR */
			iowrite32(0xE0000000 | ioread32(
				  virt_addr + ipmmu_reg[j].reg_offset),
				  virt_addr + ipmmu_reg[j].reg_offset);
		else
			ret = -1;

	} else if (handling == SET_TRANSLATION_TABLE) {
		/* Enable MMU translation for IPMMU */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMTTBCR"))
				break;
		}

		iowrite32(IMTTUBR_VAL,
			  virt_addr + ipmmu_reg[j + 1].reg_offset);
		iowrite32(IMTTLBR_VAL,
			  virt_addr + ipmmu_reg[j + 2].reg_offset);
		iowrite32(IMTTBCR_VAL, virt_addr + ipmmu_reg[j].reg_offset);
		iowrite32(IMMAIR0_VAL,
			  virt_addr + ipmmu_reg[j + 3].reg_offset);

	} else if (handling == CLEAR_MMU_STATUS_REGS) { /* Clear MMU status */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMSTR"))
				break;
		}

		iowrite32(0, virt_addr + ipmmu_reg[j].reg_offset);

	} else if (handling == INVALIDATE_TLB) { /* Invalidate TLB */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMCTR"))
				break;
		}

		if (j < reg_count) /* Found IMCTR */
			iowrite32(FLUSH | ioread32(
				  virt_addr + ipmmu_reg[j].reg_offset),
				  virt_addr + ipmmu_reg[j].reg_offset);
		else
			ret = -1;

	} else if (handling == PRINT_MMU_DEBUG) { /* Print MMU status */
		for (j = 0; j < reg_count; j++) {
			if (j == 0)
				pr_debug("---\n"); /* delimiter */
			if (!strcmp(ipmmu_reg[j].reg_name, "IMSTR") ||
			    !strcmp(ipmmu_reg[j].reg_name, "IMELAR") ||
			    !strcmp(ipmmu_reg[j].reg_name, "IMEUAR"))
				pr_err("%s: %s(%08x)\n", ipmmu->ipmmu_name,
				       ipmmu_reg[j].reg_name,
				       ioread32(virt_addr +
						ipmmu_reg[j].reg_offset));
			else
				pr_debug("%s: %s(%08x)\n", ipmmu->ipmmu_name,
					 ipmmu_reg[j].reg_name,
					 ioread32(virt_addr +
						  ipmmu_reg[j].reg_offset));
		}

	} else if (handling == BACKUP_MMU_REGS) { /* Backup IPMMU(MMU) regs */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMSTR"))
				break;
		}

		if (j < reg_count) /* Found IMSTR */
			for (; j < reg_count; j++) {
				ipmmu_reg[j].reg_val = ioread32(virt_addr +
						       ipmmu_reg[j].reg_offset);
				pr_debug("%s: reg value 0x%08x\n",
					 ipmmu_reg[j].reg_name,
					 ipmmu_reg[j].reg_val);
			}
		else
			ret = -1;

	} else if (handling == RESTORE_MMU_REGS) { /* Restore IPMMU(MMU) regs */
		for (j = 0; j < reg_count; j++) {
			if (!strcmp(ipmmu_reg[j].reg_name, "IMSTR"))
				break;
		}

		if (j < reg_count) /* Found IMSTR */
			for (; j < reg_count; j++) {
				iowrite32(ipmmu_reg[j].reg_val,
					  virt_addr + ipmmu_reg[j].reg_offset);
				pr_debug("%s: reg value 0x%08x\n",
					 ipmmu_reg[j].reg_name,
					 ioread32(virt_addr +
					 ipmmu_reg[j].reg_offset));
			}
		else
			ret = -1;

	} else { /* Invalid */
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -1;
	}

	return ret;
}

/*
 * Handle the ioremap/iounmap of IP registers
 *   handling: Flag of processing
 *     0: ioremap
 *     1: iounmap
 */
static int handle_registers(struct rcar_ipmmu **ipmmu, unsigned int handling)
{
	struct rcar_ipmmu *working_ipmmu;
	unsigned int i = 0;
	int ret = 0;

	while (ipmmu[i] != NULL) {
		working_ipmmu = ipmmu[i];
		ret = __handle_registers(working_ipmmu, handling);
		i++;
	}

	return ret;
}
#endif /* IPMMU_MMU_SUPPORT */

#ifdef IPMMU_MMU_SUPPORT
static int ipmmu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct rcar_ipmmu_data *data;

	/*
	 * Although MMNGR driver supports some IPMMU cache(s),
	 * below processing should only run once.
	 */
	if (ipmmu_common_init_done)
		return 0;

	data = of_device_get_match_data(dev);
	if (!data)
		return -1;

	if (soc_device_match(r8a7795es1))
		rcar_gen3_ipmmu = r8a7795es1_ipmmu;
	else if (soc_device_match(r8a7796es1))
		rcar_gen3_ipmmu = r8a7796es1_ipmmu;
	else
		rcar_gen3_ipmmu = data->ipmmu_data;

	if (soc_device_match(r8a7795es2) ||
	    soc_device_match(r8a77965es1) ||
	    soc_device_match(r8a77990es1))
		is_mmu_tlb_disabled = true;
	else
		is_mmu_tlb_disabled = false;

	if (soc_device_match(r8a7796))
		ipmmu_mmu_trans_table = m3_mmu_table;
	else if (soc_device_match(r8a77965))
		ipmmu_mmu_trans_table = m3n_mmu_table;
	else if (soc_device_match(r8a77990))
		ipmmu_mmu_trans_table = e3_mmu_table;
	else /* H3 */
		ipmmu_mmu_trans_table = h3_mmu_table;

	ipmmu_addr_section_0 = ipmmu_mmu_trans_table[0];
	ipmmu_addr_section_1 = ipmmu_mmu_trans_table[1];
	ipmmu_addr_section_2 = ipmmu_mmu_trans_table[2];
	ipmmu_addr_section_3 = ipmmu_mmu_trans_table[3];

	ipmmu_common_init_done = true;

	return 0;
}

static int ipmmu_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct rcar_ipmmu_data r8a7795_ipmmu_data = {
	.ipmmu_data = r8a7795_ipmmu,
};

static const struct rcar_ipmmu_data r8a7796_ipmmu_data = {
	.ipmmu_data = r8a7796_ipmmu,
};

static const struct rcar_ipmmu_data r8a77965_ipmmu_data = {
	.ipmmu_data = r8a77965_ipmmu,
};

static const struct rcar_ipmmu_data r8a77990_ipmmu_data = {
	.ipmmu_data = r8a77990_ipmmu,
};

static const struct of_device_id ipmmu_of_match[] = {
	{
	  .compatible	= "renesas,ipmmu-mmu-r8a7795",
	  .data		= &r8a7795_ipmmu_data
	},
	{
	  .compatible	= "renesas,ipmmu-mmu-r8a7796",
	  .data = &r8a7796_ipmmu_data
	},
	{
	  .compatible	= "renesas,ipmmu-mmu-r8a77961",
	  .data = &r8a7796_ipmmu_data
	},
	{
	  .compatible	= "renesas,ipmmu-mmu-r8a77965",
	  .data = &r8a77965_ipmmu_data
	},
	{
	  .compatible	= "renesas,ipmmu-mmu-r8a77990",
	  .data = &r8a77990_ipmmu_data
	},
	{ },
};

#ifdef CONFIG_PM_SLEEP
static int mm_ipmmu_suspend(struct device *dev)
{
	handle_registers(rcar_gen3_ipmmu, BACKUP_MMU_REGS);

	return 0;
}

static int mm_ipmmu_resume(struct device *dev)
{
	__handle_registers(&ipmmumm, RESTORE_MMU_REGS);
	__handle_registers(&ipmmumm, SET_TRANSLATION_TABLE);
	__handle_registers(&ipmmumm, ENABLE_MMU_MM);

	handle_registers(rcar_gen3_ipmmu, ENABLE_MMU);
	handle_registers(rcar_gen3_ipmmu, ENABLE_UTLB);

	return 0;
}

static SIMPLE_DEV_PM_OPS(mm_ipmmu_pm_ops,
			mm_ipmmu_suspend, mm_ipmmu_resume);
#define DEV_PM_OPS (&mm_ipmmu_pm_ops)
#else
#define DEV_PM_OPS NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver ipmmu_driver = {
	.driver = {
		.name = DEVNAME "_ipmmu_drv",
		.pm	= DEV_PM_OPS,
		.owner = THIS_MODULE,
		.of_match_table = ipmmu_of_match,
	},
	.probe = ipmmu_probe,
	.remove = ipmmu_remove,
};
#endif /* IPMMU_MMU_SUPPORT */

static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.open		= open,
	.release	= close,
	.unlocked_ioctl	= ioctl,
	.mmap		= mmap,
};

static struct miscdevice misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVNAME,
	.fops		= &fops,
};

static int mm_probe(struct platform_device *pdev)
{
	int			ret = 0;
	struct MM_DRVDATA	*p = NULL;
	phys_addr_t		phy_addr;
	void			*pkernel_virt_addr;
	struct device		*dev = &pdev->dev;
	struct device_node	*np = dev->of_node;
	unsigned long		mm_omxbuf_size;

#ifndef IPMMU_MMU_SUPPORT
	ret = parse_legacy_memory_node(np->parent);
	if (ret) {
		pr_err("%s MMD ERROR\n", __func__);
		return -1;
	}
#endif

	ret = parse_reserved_mem_dt(np);
	if (ret) {
		pr_err("%s MMD ERROR\n", __func__);
		return -1;
	}

	ret = validate_memory_map();
	if (ret) {
		pr_err("%s MMD ERROR\n", __func__);
		return -1;
	}

#ifndef MMNGR_SSP_ENABLE
	mm_omxbuf_size = mm_kernel_reserve_size;
#else
	mm_omxbuf_size = mm_kernel_reserve_size - MM_SSPBUF_SIZE;
#endif
	ret = alloc_bm(&bm, mm_kernel_reserve_addr, mm_omxbuf_size, MM_CO_ORDER);
	if (ret) {
		pr_err("%s MMD ERROR\n", __func__);
		return -1;
	}

#ifdef MMNGR_SSP_ENABLE
	if (is_sspbuf_valid) {
		ret = alloc_bm(&bm_ssp, MM_SSPBUF_ADDR, MM_SSPBUF_SIZE,
				MM_CO_ORDER);
		if (ret) {
			pr_err("%s MMD ERROR\n", __func__);
			return -1;
		}
	}
#endif

	if (!soc_device_match(rzv2n_match)) {
		ret = init_lossy_info();
		if (ret) {
			pr_err("MMD mm_init ERROR\n");
			return -1;
		}
	}

	p = kzalloc(sizeof(struct MM_DRVDATA), GFP_KERNEL);
	if (p == NULL)
		return -1;

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));

#ifdef IPMMU_MMU_SUPPORT
	if (!rcar_gen3_ipmmu) {
		pr_err("%s MMD ERROR\n", __func__);
		return -1;
	}

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(40));
	ipmmu_mmu_startup();
	ipmmu_mmu_initialize();
#endif

	misc_register(&misc);

	/* Handler for mem alloc in 2nd CMA area */
	p->mm_dev_reserve = dev;
	of_reserved_mem_device_init(p->mm_dev_reserve);

	pkernel_virt_addr = dma_alloc_coherent(p->mm_dev_reserve,
					mm_kernel_reserve_size,
					(dma_addr_t *)&phy_addr,
					GFP_KERNEL);
	if (pkernel_virt_addr == NULL) {
		pr_err("MMD mm_init ERROR\n");
		goto err_alloc;
	}

	p->reserve_size = mm_kernel_reserve_size;
	p->reserve_kernel_virt_addr = (unsigned long)pkernel_virt_addr;
	p->reserve_phy_addr = (unsigned long)phy_addr;
	pr_debug("MMD reserve area from 0x%pK to 0x%pK at virtual\n",
		pkernel_virt_addr,
		pkernel_virt_addr + mm_kernel_reserve_size - 1);
	pr_debug("MMD reserve area from 0x%lx to 0x%llx at physical\n",
		(unsigned long)phy_addr,
		(unsigned long)phy_addr + mm_kernel_reserve_size - 1);

	/* Handler for mem alloc in 1st CMA area */
	mm_cma_area = dev->cma_area;
	dev->cma_area = NULL;
	p->mm_dev = dev;
	mm_drvdata = p;

	spin_lock_init(&lock);

	return 0;

err_alloc:
	misc_deregister(&misc);
	return -1;
}

static int mm_remove(struct platform_device *pdev)
{
	u32 i;

	misc_deregister(&misc);

#ifdef IPMMU_MMU_SUPPORT
	ipmmu_mmu_deinitialize();
	ipmmu_mmu_cleanup();
#endif

#ifdef MMNGR_SSP_ENABLE
	if (is_sspbuf_valid)
		free_bm(&bm_ssp);
#endif

	for (i = 0; i < MAX_LOSSY_ENTRIES; i++) {
		if (lossy_entries[i].bm_lossy == NULL)
			break;
		free_bm(lossy_entries[i].bm_lossy);
	}

	free_bm(&bm);

	mmngr_dev_set_cma_area(mm_drvdata->mm_dev_reserve,
				mm_cma_area);
	dma_free_coherent(mm_drvdata->mm_dev_reserve,
			mm_drvdata->reserve_size,
			(void *)mm_drvdata->reserve_kernel_virt_addr,
			(dma_addr_t)mm_drvdata->reserve_phy_addr);

	kfree(mm_drvdata);

	return 0;
}

static const struct of_device_id mm_of_match[] = {
	{ .compatible = "renesas,mmngr" },
	{ },
};

static struct platform_driver mm_driver = {
	.driver = {
		.name = DEVNAME "_drv",
		.owner = THIS_MODULE,
		.of_match_table = mm_of_match,
	},
	.probe = mm_probe,
	.remove = mm_remove,
};

#ifdef IPMMU_MMU_SUPPORT
static void create_l1_pgtable(void)
{
	pgdval_t	*pgdval_addr = NULL;
	int i = 0;

	pgdval_addr = kzalloc(PAGE_SIZE, GFP_ATOMIC);

	if (pgdval_addr != NULL) {
		pgdval_addr[0] = IPMMU_PGDVAL_SECTION_0;
		pgdval_addr[1] = IPMMU_PGDVAL_SECTION_1;
		pgdval_addr[2] = IPMMU_PGDVAL_SECTION_2;
		pgdval_addr[3] = IPMMU_PGDVAL_SECTION_3;

		ipmmu_mmu_pgd = pgdval_addr;

		isb();
		dma_rmb();

		for (i = 0; i < 4; i++)
			pr_debug("L1: ipmmu_mmu_pgd[%d] 0x%llx\n", i,
				 ipmmu_mmu_pgd[i]);

		for (i = 0; i < 4; i++)
			pr_debug("ipmmu_mmu_trans_table[%d]: 0x%llx\n", i,
				 ipmmu_mmu_trans_table[i]);
	}
}

static void free_lx_pgtable(void)
{
	kfree(ipmmu_mmu_pgd);
}

static void ipmmu_mmu_startup(void)
{
	create_l1_pgtable();
}

static void ipmmu_mmu_cleanup(void)
{
	free_lx_pgtable();
}

static int ipmmu_mmu_initialize(void)
{
	__handle_registers(&ipmmumm, DO_IOREMAP);
	handle_registers(rcar_gen3_ipmmu, DO_IOREMAP);
	__handle_registers(&ipmmumm, CLEAR_MMU_STATUS_REGS);

	__handle_registers(&ipmmumm, SET_TRANSLATION_TABLE);
	__handle_registers(&ipmmumm, ENABLE_MMU_MM);

	if (is_mmu_tlb_disabled) {
		if (soc_device_match(r8a77990es1))
			handle_registers(r8a77990_disable_mmu_tlb,
					 DISABLE_MMU_TLB);
		else
			handle_registers(rcar_gen3_ipmmu, DISABLE_MMU_TLB);
	}
	handle_registers(rcar_gen3_ipmmu, ENABLE_MMU);
	handle_registers(rcar_gen3_ipmmu, ENABLE_UTLB);
	__handle_registers(&ipmmumm, CLEAR_MMU_STATUS_REGS);

	return 0;
}

static void ipmmu_mmu_deinitialize(void)
{
	__handle_registers(&ipmmumm, PRINT_MMU_DEBUG);
	handle_registers(rcar_gen3_ipmmu, PRINT_MMU_DEBUG);

	handle_registers(rcar_gen3_ipmmu, DISABLE_UTLB);
	handle_registers(rcar_gen3_ipmmu, INVALIDATE_TLB);
	handle_registers(rcar_gen3_ipmmu, DISABLE_MMU);

	__handle_registers(&ipmmumm, INVALIDATE_TLB);
	__handle_registers(&ipmmumm, DISABLE_MMU_MM);
	__handle_registers(&ipmmumm, CLEAR_MMU_STATUS_REGS);

	handle_registers(rcar_gen3_ipmmu, DO_IOUNMAP);
	__handle_registers(&ipmmumm, DO_IOUNMAP);
}

static unsigned int ipmmu_mmu_phys2virt(phys_addr_t paddr)
{
	unsigned int	vaddr = 0;
	int		section = 0;

	pr_debug("p2v: before: paddr 0x%llx vaddr 0x%x\n", paddr, vaddr);

	for (section = 0; section < 4; section++) {
		if ((paddr >= ipmmu_mmu_trans_table[section]) &&
		    (paddr < ipmmu_mmu_trans_table[section] + SZ_1G)) {
			vaddr = section * SZ_1G;
			vaddr |= paddr & 0x3fffffff;
		}
	}

	pr_debug("p2v: after: paddr 0x%llx vaddr 0x%x\n", paddr, vaddr);

	return vaddr;
}

static phys_addr_t ipmmu_mmu_virt2phys(unsigned int vaddr)
{
	phys_addr_t	paddr = 0;
	unsigned int	section_start_addr = 0;
	unsigned int	section = 0;

	pr_debug("v2p: before: vaddr 0x%x paddr 0x%llx\n", vaddr, paddr);

	section = (vaddr >> 30);
	section_start_addr = section * SZ_1G;

	paddr = ipmmu_mmu_trans_table[section]
		+ (vaddr - section_start_addr);

	pr_debug("v2p: after: vaddr 0x%x paddr 0x%llx\n", vaddr, paddr);

	return paddr;
}
#endif /* IPMMU_MMU_SUPPORT */

static int mm_init(void)
{
#ifdef IPMMU_MMU_SUPPORT
	platform_driver_register(&ipmmu_driver);
#endif
	return platform_driver_register(&mm_driver);
}

static void mm_exit(void)
{
	platform_driver_unregister(&mm_driver);
#ifdef IPMMU_MMU_SUPPORT
	platform_driver_unregister(&ipmmu_driver);
#endif
}

module_init(mm_init);
module_exit(mm_exit);

MODULE_LICENSE("Dual MIT/GPL");
