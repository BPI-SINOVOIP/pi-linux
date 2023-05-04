// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef	__AVIO_BASE_H__
#define	__AVIO_BASE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ctypes.h"

/** SECTION - symbols
 */

#ifdef NULL
#undef NULL
#endif
#define NULL                0

/** ENDOFSECTION
 *//**	SECTION - macros for basic math operations
 */
#ifdef MIN
#undef MIN
#endif
#define	MIN(a, b)			((a) < (b) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define	MAX(a, b)			((a) > (b) ? (a) : (b))

#ifdef ABS
#undef ABS
#endif
#define	ABS(x)				((x) < 0 ? -(x) : (x))

#ifdef SIGN
#undef SIGN
#endif
#define	SIGN(x)				((x) < 0 ? -1 : 1)
#define	ModInc(x, i, mod)	do { (x) += (i); while ((x) >= (mod)) (x) -= (mod); } while (0)

/**	ENDOFSECTION
 *//**	SECTION - macros for bits operations
 */
#define	bTST(x, b)			(((x) >> (b)) & 1)
#define	bSETMASK(b)			((b) < 32 ? (1 << (b)) : 0)
#define	bSET(x, b)			do {	(x) |= bSETMASK(b); } while (0)
#define	bCLRMASK(b)			(~(bSETMASK(b)))
#define	bCLR(x, b)			do {	(x) &= bCLRMASK(b); } while (0)
#define	NSETMASK(msb, lsb)	(bSETMASK((msb) + 1) - bSETMASK(lsb))
#define	NCLRMASK(msb, lsb)	(~(NSETMASK(msb, lsb)))

/** ENDOFSECTION
 *//** SECTION - unified 'xdbg(fmt, ...)'
 */
static inline void nullprint(char *s, ...)
{
	(void)s;
}
#define xdbg				nullprint

/**	ENDOFSECTION
 */

#ifdef __cplusplus
}
#endif
#endif /* __AVIO_BASE_H__ */
