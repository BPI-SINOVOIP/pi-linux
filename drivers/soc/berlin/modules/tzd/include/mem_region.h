/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ******************************************************************************/

#ifndef _MEM_REGION_H_
#define _MEM_REGION_H_

#include "types.h"
#include "tz_perm.h"

struct master_setting {
	uint32_t		id;
	uint32_t		attr;	/* stage + perm */
	uint32_t		reserved[2];
};

#define MST_M_STAGE			(0xff000000)
#define MST_M_PERM			(0x000000ff)
#define MST_S_STAGE			(24)
#define MST_GET_STAGE(attr)		((attr) >> MST_S_STAGE)
#define MST_GET_PERM(attr)		((attr) & MST_M_PERM)
#define MST_ATTR(stage, perm)		(((stage) << MST_S_STAGE) | \
					((perm) & MST_M_PERM))

/* memory zone */
struct mem_zone {
	uint32_t		id;
	uint32_t		size;
	uint32_t		attr;	/* perm	*/
	uint32_t		reserved;
};

#define	MZ_S_CHIP_ID		5
#define	MZ_M_CHIP_ID		0xe0
#define	MZ_M_ZONE_ID		0x1f
#define MZ_CHIP_ID_MAX		((0xff & MZ_M_CHIP_ID) >> MZ_S_CHIP_ID)
#define	MZ_ID(chip_id, zone_id)	((((chip_id) & MZ_M_CHIP_ID) << MZ_S_CHIP_ID) | \
				((zone_id) & MZ_M_ZONE_ID))

/* register firewall window */
struct rf_window {
	uint32_t		id;
	uint32_t		base;
	uint32_t		mask;
	uint32_t		attr;	/* stage + perm */
};

#define RF_M_STAGE			(0xff000000)
#define RF_M_PERM			(0x000000ff)
#define RF_S_STAGE			(24)
#define RF_GET_STAGE(attr)		((attr) >> RF_S_STAGE)
#define RF_GET_PERM(attr)		((attr) & RF_M_PERM)
#define RF_ATTR(stage, perm)		(((stage) << RF_S_STAGE) | \
					((perm) & RF_M_PERM))

/*
 * Memory Region.
 *
 * Attributes Definitions:
 *
 * bit 31:24: control info
 * bit    31: 0 not premapped, 1 premapped.
 * bit    30: 0 fixed data attributes, 1 support changeable data attributes
 * bit 29:24: reserved
 * bit 23:16: mapping attributes.
 *		[0][ 0][ 0][ 0] [0][ 0][ 1][ 0]	- StrongOrder
 *		[0][ 0][ 0][ 0] [0][ 0][ 1][ 1]	- Device
 *		[0][ 1][ 0][ 0] [0][ 1][ 0][ 0]	- Normal, non-cacheable
 * 		[1][WB][RA][WA]	[1][WB][RA][WA] - Normal, cacheable
 * bit    15: type. 0 memory, 1 register.
 * bit 14: 8: memory zone id or window id (0-127)
 * bit  7: 0: permission, see TZ_Sxx_Nxx
 *
 * compare with linux (CTLR.TRE=1)
 * Here      Linux#n  TR  IR  OR  Attr[7:0] Attributes
 * 00000000  /                              Unknown (On-demand)
 * 00000010  000      00  /   /   00000000  Strong Order (in order)
 * 00000011  100      01  /   /   00000100  Device (in order)
 * 01000100  001      10  00  00  01000100  Normal, Bufferable, non-cacheable
 * 10011001  /                    10011001  Normal, WriteThrough, WA
 * 10101010  010      10  10  10  10101010  Normal, WriteThrough, RA
 * 10111011  /                    10111011  Normal, WriteThrough, RA, WA
 * 11011101  111      10  01  01  11011101  Normal, WriteBack, WA
 * 11101110  011      10  11  11  11101110  Normal, WriteBack, RA
 * 11111111  /                    11111111  Normal, WriteBack, RA, WA
 *
 * Linux doesn't remap Shareable bit, here are the settings:
 * DS0 = PRRR[16] = 0	- device shareable property when S=0, keep 0
 * DS1 = PRRR[17] = 1	- device shareable property when S=1, keep 1
 * NS0 = PRRR[16] = 0	- normal shareable property when S=0, keep 0
 * NS0 = PRRR[16] = 0	- normal shareable property when S=1, keep 1
 * NOS = PRRR[24+n] = 1	- not outer shareable
 * 			note: base on PL310 spec, "Shared only applies to Normal
 * 			Memory outer non-cacheable transactions, where
 * 			ARCACHESx or AWCACHESx= 0010 or 0011. For other values of
 * 			ARCACHESx and AWCACHESx, the cache controller ignores the
 * 			shareable attribute."
 *
 * == Note: ARM Defined Memory types ==
 *
 * Memory Types (TR):
 * 00b - Strong Order (in-order access)
 *       a. it would make *all* access in order too. like every access in
 *          device + DSB.
 *       b. it tends to come with quite large performance penalty.
 *       c. it's highly recommended to use Device Memory. If necessary, insert
 *          DMB instructions ensure write buffer gets drained out to the
 *          peripheral.
 * 01b - Device (in-order access)
 * 10b - Normal (may out-of-order)
 *
 * example 1: load-norm-A, load-dev-B, load-dev-C, load-norm-D could be
 *            performed as ADBC, DBCA, DBAC... (only B->C is guanteed)
 * example 2: load-norm-A, load-so-B, load-so-C, load-norm-D must always
 *            be performed as ABCD.
 * example 3: load-norm-A, load-dev-B, load-dev-C, DMB, load-norm-D can
 *            only be performed as ABCD, BACD, BCAD
 *
 * Normal memory cache attributes (IR/OR):
 * 00b - Non-cacheable
 * 01b - Write-Back, Write-Allocate
 * 10b - Write-Through, no Write-Allocate
 * 11b - Write-Back, no Write-Allocate
 */
/* memory attributes */
#define MR_MEM_UNKNOWN			(0x00)	/* Unknown */
#define MR_MEM_STRONGORDER		(0x02)	/* Strong Order */
#define MR_MEM_DEVICE			(0x03)	/* Device */
#define MR_MEM_NORMAL_OUC_IUC		(0x44)	/* Normal: Non-cacheable, bufferable */
#define MR_MEM_NORMAL_OWTWA_IWTWA	(0x99)	/* Normal: Write Through, Write-Allocate */
#define MR_MEM_NORMAL_OWTRA_IWTRA	(0xaa)	/* Normal: Write Through, Read-Allocate */
#define MR_MEM_NORMAL_OWTRAWA_IWTRAWA	(0xbb)	/* Normal: Write Through, Read-Allocate, Write-Allocate */
#define MR_MEM_NORMAL_OWBWA_IWBWA	(0xdd)	/* Normal: Write Back, Write-Allocate */
#define MR_MEM_NORMAL_OWBRA_IWBRA	(0xee)	/* Normal: Write Back, Read-Allocate */
#define MR_MEM_NORMAL_OWBRAWA_IWBRAWA	(0xff)	/* Normal: Write Back, Read-Allocate, Write-Allocate */
/* alias of normal memory attributes */
#define MR_MEM_UNCACHED			MR_MEM_NORMAL_OUC_IUC
#define MR_MEM_WRITETHROUGH_WA		MR_MEM_NORMAL_OWTWA_IWTWA
#define MR_MEM_WRITETHROUGH_RA		MR_MEM_NORMAL_OWTRA_IWTRA
#define MR_MEM_WRITETHROUGH_RAWA	MR_MEM_NORMAL_OWTRAWA_IWTRAWA
#define MR_MEM_WRITEBACK_WA		MR_MEM_NORMAL_OWBWA_IWBWA
#define MR_MEM_WRITEBACK_RA		MR_MEM_NORMAL_OWBRA_IWBRA
#define MR_MEM_WRITEBACK_RAWA		MR_MEM_NORMAL_OWBRAWA_IWBRAWA

/* mask */
#define MR_M_PERM		(0x000000ff)
#define MR_M_ZONE		(0x00007f00)
#define MR_M_TYPE		(0x00008000)
#define MR_M_MEM_ATTR		(0x00ff0000)
#define MR_M_CACHEABLE		(0x00880000)
#define MR_M_CTRL		(0xff000000)
#define MR_M_DATA_ATTR		(0x40000000)
#define MR_M_PREMAPPED		(0x80000000)

/* shift */
#define MR_S_PERM		(0)
#define MR_S_ZONE		(8)
#define MR_S_TYPE		(15)
#define MR_S_MEM_ATTR		(16)
#define MR_S_CTRL		(24)
#define MR_S_DATA_ATTR		(30)
#define MR_S_PREMAPPED		(31)

/* utils */
#define MR_ATTR(perm, reg, zone, mattr, ctrl)			\
	((perm) | ((zone) << MR_S_ZONE)				\
	 | ((reg) ? MR_M_TYPE : 0)				\
	 | ((mattr) << MR_S_MEM_ATTR)				\
	 | (ctrl))

#define MR_PERM(mr)		(((mr)->attr & MR_M_PERM) >> MR_S_PERM)
#define MR_ZONE(mr)		(((mr)->attr & MR_M_ZONE) >> MR_S_ZONE)
#define MR_WINDOW(mr)		MR_ZONE(mr)
#define MR_MEM_ATTR(mr)		(((mr)->attr & MR_M_MEM_ATTR) >> MR_S_MEM_ATTR)

#define MR_IS_RESTRICTED(mr)	TZ_IS_SNA(MR_PERM(mr))
#define MR_IS_SECURE(mr)	TZ_IS_NNA(MR_PERM(mr))
#define MR_IS_NONSECURE(mr)	(!MR_IS_SECURE(mr))
#define MR_IS_PREMAPPED(mr)	((mr)->attr & MR_M_PREMAPPED)
#define MR_IS_CACHEABLE(mr)	((mr)->attr & MR_M_CACHEABLE)
#define MR_IS_DEVICE(mr)	(MR_MEM_ATTR(mr) == MR_MEM_DEVICE)
#define MR_IS_STRONGORDER(mr)	(MR_MEM_ATTR(mr) == MR_MEM_STRONGORDER)
#define MR_IS_DATA_ATTR(mr)	((mr)->attr & MR_M_DATA_ATTR)
#define MR_IS_REGISTER(mr)	((mr)->attr & MR_M_TYPE)
#define MR_IS_MEMORY(mr)	(!MR_IS_REGISTER(mr))

/* memory region */
struct mem_region {
	char			name[32];
	uint32_t		base;	/* base address */
	uint32_t		size;
	uint32_t		attr;	/* attributes */
	uint32_t		reserved;

	/* bellow fields are user defined data.
	 * it's to support multiple platforms by unified structure.
	 * different platform can use them for different purpose.
	 */
	uint32_t		userdata[4];
};

/*
 * anti-rollback info
 */
struct antirollback
{
	uint32_t comm_ver;	/* common version */
	uint32_t cust_ver;	/* customer version */
	uint32_t reserved[2];
};

/*register region*/
struct reg_region {
	char			name[32];
	uint32_t		base;
	uint32_t		size;
	uint32_t		attr;	/* attributes */
};
#endif	/* _MEM_REGION_H_ */
