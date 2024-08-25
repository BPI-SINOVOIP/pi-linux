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
#ifndef __MMNGR_PRIVATE_H__
#define __MMNGR_PRIVATE_H__

#include "mmngr_private_cmn.h"

/* IPMMU (MMU) */
#if defined(IPMMU_MMU_SUPPORT_1GB_PGTABLE)
#define IPMMU_MMU_SUPPORT
#endif

struct MM_DRVDATA {
	struct device *mm_dev;
	struct device *mm_dev_reserve;
	unsigned long	reserve_size;
	phys_addr_t	reserve_phy_addr;
	unsigned long	reserve_kernel_virt_addr;
};

struct BM {
	phys_addr_t	top_phy_addr;
	unsigned long	order;
	unsigned long	end_bit;
	unsigned long	*bits;
};

struct LOSSY_INFO {
	u32 magic;
	u32 a0;
	u32 b0;
};

struct LOSSY_DATA {
	u32 fmt;
	struct BM *bm_lossy;
};

#ifdef IPMMU_MMU_SUPPORT
enum {
	DO_IOREMAP,
	DO_IOUNMAP,
	ENABLE_UTLB,
	DISABLE_UTLB,
	ENABLE_MMU_MM,
	DISABLE_MMU_MM,
	ENABLE_MMU,
	DISABLE_MMU_TLB,
	DISABLE_MMU,
	SET_TRANSLATION_TABLE,
	CLEAR_MMU_STATUS_REGS,
	PRINT_MMU_DEBUG,
	START_MMU_PERF_MON,
	STOP_MMU_PERF_MON,
	PRINT_MMU_PERF_MON,
	BACKUP_MMU_REGS,
	RESTORE_MMU_REGS,
	INVALIDATE_TLB,
};

struct hw_register {
	const char *reg_name;
	unsigned int reg_offset;
	unsigned int reg_val;
};

struct ip_master {
	const char *ip_name;
	unsigned int utlb_no;
};

struct rcar_ipmmu {
	const char *ipmmu_name;
	unsigned int base_addr;
	void __iomem *virt_addr;
	unsigned int reg_count;
	unsigned int masters_count;
	struct hw_register *ipmmu_reg;
	struct ip_master *ip_masters;
};

struct rcar_ipmmu_data {
	struct rcar_ipmmu **ipmmu_data;
};
#endif /* IPMMU_MMU_SUPPORT */

#ifdef CONFIG_COMPAT
struct COMPAT_MM_PARAM {
	compat_size_t	size;
	compat_u64	phy_addr;
	compat_uint_t	hard_addr;
	compat_ulong_t	user_virt_addr;
	compat_ulong_t	kernel_virt_addr;
	compat_uint_t	flag;
};

#define COMPAT_MM_IOC_ALLOC	_IOWR(MM_IOC_MAGIC, 0, struct COMPAT_MM_PARAM)
#define COMPAT_MM_IOC_FREE	_IOWR(MM_IOC_MAGIC, 1, struct COMPAT_MM_PARAM)
#define COMPAT_MM_IOC_SET	_IOWR(MM_IOC_MAGIC, 2, struct COMPAT_MM_PARAM)
#define COMPAT_MM_IOC_GET	_IOWR(MM_IOC_MAGIC, 3, struct COMPAT_MM_PARAM)
#define COMPAT_MM_IOC_ALLOC_CO	_IOWR(MM_IOC_MAGIC, 4, struct COMPAT_MM_PARAM)
#define COMPAT_MM_IOC_FREE_CO	_IOWR(MM_IOC_MAGIC, 5, struct COMPAT_MM_PARAM)
#define COMPAT_MM_IOC_SHARE	_IOWR(MM_IOC_MAGIC, 6, struct COMPAT_MM_PARAM)

static long compat_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg);
#endif

#define DEVNAME		"rgnmm"
#define DRVNAME		DEVNAME
#define CLSNAME		DEVNAME
#define DEVNUM		1

static int mm_ioc_alloc(struct device *mm_dev,
			int __user *in,
			struct MM_PARAM *out);
static void mm_ioc_free(struct device *mm_dev, struct MM_PARAM *p);
static int mm_ioc_set(int __user *in, struct MM_PARAM *out);
static int mm_ioc_get(struct MM_PARAM *in, int __user *out);
static int alloc_bm(struct BM *pb,
		phys_addr_t top_phy_addr,
		unsigned long size,
		unsigned long order);
static void free_bm(struct BM *pb);
static int mm_ioc_alloc_co(struct BM *pb, int __user *in, struct MM_PARAM *out);
static int mm_ioc_alloc_co_select(int __user *in, struct MM_PARAM *out);
static void mm_ioc_free_co(struct BM *pb, struct MM_PARAM *p);
static void mm_ioc_free_co_select(struct MM_PARAM *p);
static int mm_ioc_share(int __user *in, struct MM_PARAM *out);
static void mmngr_dev_set_cma_area(struct device *dev, struct cma *cma);
static int init_lossy_info(void);
static int find_lossy_entry(unsigned int flag, int *entry);
static int _parse_reserved_mem_dt(struct device_node *np,
				  const char *phandle_name,
				  const char *match,
				  u64 *addr, u64 *size);
static int parse_reserved_mem_dt(struct device_node *np);
static int open(struct inode *inode, struct file *file);
static int close(struct inode *inode, struct file *file);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int mmap(struct file *filp, struct vm_area_struct *vma);
static int mm_probe(struct platform_device *pdev);
static int mm_remove(struct platform_device *pdev);
static int mm_init(void);
static void mm_exit(void);

static int validate_memory_map(void);

#ifdef MMNGR_SALVATORX
#define MM_OMXBUF_SIZE		(128 * 1024 * 1024)
#endif /* MMNGR_SALVATORX */

#ifdef MMNGR_EBISU
#define MM_OMXBUF_SIZE		(64 * 1024 * 1024)
#endif /* MMNGR_EBISU */

#ifdef MMNGR_V3MSK
#define MM_OMXBUF_ADDR		(0x7F000000UL)
#define MM_OMXBUF_SIZE		(16 * 1024 * 1024)
#endif /* MMNGR_V3MSK */

#define	MM_CO_ORDER		(12)

#ifdef MMNGR_SSP_ENABLE
#ifdef MMNGR_SALVATORX
#define MM_SSPBUF_ADDR		(0x53000000UL)
#define MM_SSPBUF_SIZE		(16 * 1024 * 1024)
#endif

#ifdef MMNGR_EBISU
#error "R8A77990 (R-Car E3) EBISU does not support SSPBUF"
#endif
#endif /* MMNGR_SSP_ENABLE */

#define MAX_LOSSY_ENTRIES		(16)
#define MM_LOSSY_INFO_MAGIC		(0x12345678UL)
#define MM_LOSSY_ADDR_MASK		(0x0003FFFFUL)  /* [17:0] */
#define MM_LOSSY_FMT_MASK		(0x60000000UL)  /* [30:29] */
#define MM_LOSSY_ENABLE_MASK		(0x80000000UL)  /* [31] */
#define MM_LOSSY_SHARED_MEM_ADDR	(0x47FD7000UL)
#define MM_LOSSY_SHARED_MEM_SIZE	(MAX_LOSSY_ENTRIES \
					* sizeof(struct LOSSY_INFO))

#ifdef IPMMU_MMU_SUPPORT

#define IPMMUVP_BASE		(0xFE990000UL)
#define IPMMUVI_BASE		(0xFEBD0000UL)
#define IPMMUVC0_BASE		(0xFE6B0000UL)
#define IPMMUVC1_BASE		(0xFE6F0000UL)
#define IPMMUMM_BASE		(0xE67B0000UL)
/* Available in H3 2.0 */
#define IPMMUVP0_BASE		IPMMUVP_BASE
#define IPMMUVP1_BASE		(0xFE980000UL)

#ifdef MMNGR_SSP_ENABLE
#define IPMMUSY_BASE		(0xE7730000UL)
#define IPMMUDS1_BASE		(0xE7740000UL)
#endif

#define EAE			BIT(31)
#define SL_BIT7			BIT(7)
#define TSZ0_32BIT		0 /* 2^(32-0) */
#define MMUEN			BIT(0)
#define FLUSH			BIT(1)
#define IMTTLBR_MASK		GENMASK(31, 12)

#define CUR_TTSEL		7	/* Pagetable no.7 */

#define TTSEL(n)		(0x40 * (n))
#define IMCTRn_OFFSET(n)	(0x0000 + TTSEL(n))
#define IMTTBCRn_OFFSET(n)	(0x0008 + TTSEL(n))
#define IMTTUBR0n_OFFSET(n)	(0x0014 + TTSEL(n))
#define IMTTLBR0n_OFFSET(n)	(0x0010 + TTSEL(n))
#define IMTTUBR1n_OFFSET(n)	(0x001c + TTSEL(n))
#define IMTTLBR1n_OFFSET(n)	(0x0018 + TTSEL(n))
#define IMSTRn_OFFSET(n)	(0x0020 + TTSEL(n))
#define IMMAIR0n_OFFSET(n)	(0x0028 + TTSEL(n))
#define IMMAIR1n_OFFSET(n)	(0x002c + TTSEL(n))
#define IMELARn_OFFSET(n)	(0x0030 + TTSEL(n))
#define IMEUARn_OFFSET(n)	(0x0034 + TTSEL(n))
#define IMUCTR0(n)		(0x300 + ((n) * 16))
#define IMUCTR32(n)		(0x600 + (((n) - 32) * 16))
#define IMUCTRn_OFFSET(n)	((n) < 32 ? IMUCTR0(n) : IMUCTR32(n))
#define IMSCTLR_OFFSET		(0x0500)

#define MAX_UTLB		(48)

#define REG_SIZE		IMUCTRn_OFFSET(MAX_UTLB)
#define IMCTR_VAL		(MMUEN | FLUSH)
#define IMCTR_MM_VAL		(IMCTR_VAL)
#define IMTTBCR_VAL		(EAE | SL_BIT7 | TSZ0_32BIT)
#define IMMAIR0_VAL		(0x5500)
#define IMUCTR_VAL		((CUR_TTSEL << 4) | MMUEN | FLUSH)
#define IMTTLBR_VAL		(__pa(ipmmu_mmu_pgd) & IMTTLBR_MASK)
#define IMTTUBR_VAL		(__pa(ipmmu_mmu_pgd) >> 32)

/* Page entry setting */
#define BLOCK_ENTRY_CONFIG	(0x721 | BIT(2))
#define IPMMU_BLOCK_PGDVAL(phys_addr)	((phys_addr) | BLOCK_ENTRY_CONFIG)

/* Table entries for H3N - Swap mode */
#define H3_IPMMU_ADDR_SECTION_0	0x0540000000ULL
#define H3_IPMMU_ADDR_SECTION_1	0x0040000000ULL
#define H3_IPMMU_ADDR_SECTION_2	0x0080000000ULL
#define H3_IPMMU_ADDR_SECTION_3	0x0500000000ULL
/* Table entries for M3 */
#define M3_IPMMU_ADDR_SECTION_0	0x0640000000ULL
#define M3_IPMMU_ADDR_SECTION_1	0x0040000000ULL
#define M3_IPMMU_ADDR_SECTION_2	0x0080000000ULL
#define M3_IPMMU_ADDR_SECTION_3	0x0600000000ULL
/* Table entries for M3N */
#define M3N_IPMMU_ADDR_SECTION_0	0x04C0000000ULL
#define M3N_IPMMU_ADDR_SECTION_1	0x0040000000ULL
#define M3N_IPMMU_ADDR_SECTION_2	0x0080000000ULL
#define M3N_IPMMU_ADDR_SECTION_3	0x0480000000ULL
/* Table entries for E3 */
#define E3_IPMMU_ADDR_SECTION_0	0x0
#define E3_IPMMU_ADDR_SECTION_1	0x0040000000ULL
#define E3_IPMMU_ADDR_SECTION_2	0x0080000000ULL
#define E3_IPMMU_ADDR_SECTION_3	0x0

#define IPMMU_PGDVAL_SECTION_0	IPMMU_BLOCK_PGDVAL(ipmmu_addr_section_0)
#define IPMMU_PGDVAL_SECTION_1	IPMMU_BLOCK_PGDVAL(ipmmu_addr_section_1)
#define IPMMU_PGDVAL_SECTION_2	IPMMU_BLOCK_PGDVAL(ipmmu_addr_section_2)
#define IPMMU_PGDVAL_SECTION_3	IPMMU_BLOCK_PGDVAL(ipmmu_addr_section_3)

static int __handle_registers(struct rcar_ipmmu *ipmmu, unsigned int handling);
static int handle_registers(struct rcar_ipmmu **ipmmu, unsigned int handling);
static int ipmmu_probe(struct platform_device *pdev);
static int ipmmu_remove(struct platform_device *pdev);
static void create_l1_pgtable(void);
static void free_lx_pgtable(void);
static void ipmmu_mmu_startup(void);
static void ipmmu_mmu_cleanup(void);
static int ipmmu_mmu_initialize(void);
static void ipmmu_mmu_deinitialize(void);
static unsigned int ipmmu_mmu_phys2virt(phys_addr_t paddr);
static phys_addr_t ipmmu_mmu_virt2phys(unsigned int vaddr);
static int mm_ipmmu_suspend(struct device *dev);
static int mm_ipmmu_resume(struct device *dev);

#endif /* IPMMU_MMU_SUPPORT */

#endif	/* __MMNGR_PRIVATE_H__ */
