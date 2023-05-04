// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef	__AVIO_IO_H__
#define	__AVIO_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ctypes.h"
#include "avio_base.h"
#include "avio_sub_module.h"

 /**	SECTION -  Galois controller register read/write macros
  */
#define AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION

#ifdef AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION
extern void *avio_memmap_phy_to_vir(unsigned int phyaddr);
extern unsigned int avio_memmap_vir_to_phy(void *vir_addr);
#else
#define AVIO_REGMAP_GET_AVIO_CTX() ((AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO))
#define avio_memmap_phy_to_vir(phy_addr)       (((phy_addr) - AVIO_REGMAP_GET_AVIO_CTX()->avio_base) + AVIO_REGMAP_GET_AVIO_CTX()->avio_virt_base)
#define avio_memmap_vir_to_phy(vir_addr)       (((vir_addr) - AVIO_REGMAP_GET_AVIO_CTX()->avio_virt_base) + AVIO_REGMAP_GET_AVIO_CTX()->avio_base)
#endif

#define	GA_REG_WORD32_READ(offset, holder)	(*(holder) = (*((volatile unsigned int *)avio_memmap_phy_to_vir(offset))))
#define	GA_REG_WORD32_WRITE(addr, data)	    ((*((volatile unsigned int *)avio_memmap_phy_to_vir(addr))) = ((unsigned int)(data)))

#define	GA_REG_WORD16_READ(offset, holder)	(*(holder) = (*((volatile unsigned short*)avio_memmap_phy_to_vir(offset))))
#define	GA_REG_WORD16_WRITE(addr, data)	    ((*((volatile unsigned short *)avio_memmap_phy_to_vir(addr))) = ((unsigned short)(data)))

#define	GA_REG_BYTE_READ(offset, holder)	(*(holder) = (*((volatile unsigned char*)avio_memmap_phy_to_vir(offset))))
#define	GA_REG_BYTE_WRITE(addr, data)	    ((*((volatile unsigned char *)avio_memmap_phy_to_vir(addr))) = ((unsigned char)(data)))

#define AVIO_REG_WORD32_READ(offset, holder)    GA_REG_WORD32_READ(offset, holder)
#define AVIO_REG_WORD32_WRITE(addr, data)       GA_REG_WORD32_WRITE(addr, data)

#define AVIO_REG_WORD16_READ(offset, holder)    GA_REG_WORD16_READ(offset, holder)
#define AVIO_REG_WORD16_WRITE(addr, data)       GA_REG_WORD16_WRITE(addr, data)

#define AVIO_REG_BYTE_READ(offset, holder)      GA_REG_BYTE_READ(offset, holder)
#define AVIO_REG_BYTE_WRITE(addr, data)         GA_REG_BYTE_WRITE(addr, data)

/** ENDOFSECTION
 */

/**	SECTION - unified 'IO32RD', 'IO32WR', and 'IO64RD' functions
 */
#define	IO32RD(d, a)				\
	do { (d) = *(volatile UNSG32*)avio_memmap_phy_to_vir(a); } while (0)
#define	IO32WR(d, a)				\
	do { *(volatile UNSG32*)avio_memmap_phy_to_vir(a) = (d); } while (0)
/* Directly write a 32b register or append to 'T64b cfgQ[]', in (adr,data) pairs */
#define	IO32CFG(cfgQ, i, a, d)		do { \
		if (cfgQ) { \
			(cfgQ)[i][0] = (a); (cfgQ)[i][1] = (d); \
		} else { \
			IO32WR(d, a); \
		} \
		(i)++; \
	} while (0)
/**	ENDOFSECTION
 */

#ifdef __cplusplus
}
#endif

#endif /* __AVIO_IO_H__ */
