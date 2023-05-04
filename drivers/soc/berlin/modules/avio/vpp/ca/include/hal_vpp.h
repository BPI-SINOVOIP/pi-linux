// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2017 Marvell Technology Group Ltd.
 *        http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _HAL_VPP_H_
#define _HAL_VPP_H_            "_HAL_VPP_H_ >>>"
/**    hal_vpp.h for NTZ
 */

#include "avio_io.h"
#include "avio_type.h"
#include "vpp_vbuf.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef E_VPP_BASE
#ifndef E_SUC
#define E_SUC			(0x00000000)
#endif
#ifndef E_ERR
#define E_ERR			(0x80000000)
#endif

#define E_VPP_BASE		(0x0200 << 16)   //VPP base

//defines   --
#define S_VPP(code) (E_SUC | E_VPP_BASE | ((code) & 0xFFFF))
#define E_VPP(code) (E_ERR | E_VPP_BASE | ((code) & 0xFFFF))

#define S_VPP_OK            (S_OK)

/* error code */
#define VPP_E_NODEV         E_VPP(1)
#define VPP_E_BADPARAM      E_VPP(2)
#define VPP_E_BADCALL       E_VPP(3)
#define VPP_E_UNSUPPORT     E_VPP(4)
#define VPP_E_IOFAIL        E_VPP(5)
#define VPP_E_UNCONFIG      E_VPP(6)
#define VPP_E_CMDQFULL      E_VPP(7)
#define VPP_E_FRAMEQFULL    E_VPP(8)
#define VPP_E_BCMBUFFULL    E_VPP(9)
#define VPP_E_NOMEM         E_VPP(10)
#define VPP_EVBIBUFFULL     E_VPP(11)
#define VPP_EHARDWAREBUSY   E_VPP(12)
#define VPP_ENOSINKCNCTED   E_VPP(13)
#define VPP_ENOHDCPENABLED  E_VPP(14)

// error code definitions
#define MV_VPP_OK          S_VPP_OK
#define MV_VPP_ENODEV      VPP_E_NODEV
#define MV_VPP_EBADPARAM   VPP_E_BADPARAM
#define MV_VPP_EBADCALL    VPP_E_BADCALL
#define MV_VPP_EUNSUPPORT  VPP_E_UNSUPPORT
#define MV_VPP_EIOFAIL     VPP_E_IOFAIL
#define MV_VPP_EUNCONFIG   VPP_E_UNCONFIG
#define MV_VPP_ECMDQFULL   VPP_E_CMDQFULL
#define MV_VPP_EFRAMEQFULL VPP_E_FRAMEQFULL
#define MV_VPP_EBCMBUFFULL VPP_E_BCMBUFFULL
#define MV_VPP_ENOMEM      VPP_E_NOMEM
#define MV_VPP_EVBIBUFFULL VPP_EVBIBUFFULL
#define MV_VPP_EHARDWAREBUSY VPP_EHARDWAREBUSY
#define MV_VPP_ENOSINKCNCTED  VPP_ENOSINKCNCTED
#define MV_VPP_ENOHDCPENABLED VPP_ENOHDCPENABLED
#endif

#define MV_DISP_SUCCESS		(0)
#define MV_DISP_E_RES_CFG	(1)
#define MV_DISP_E_CREATE	(2)
#define MV_DISP_E_CFG		(3)
#define MV_DISP_E_RST		(4)
#define MV_DISP_E_WIN		(5)
#define MV_DISP_E_DISP		(6)
#define MV_DISP_E_NOMEM     (7)
#define MV_DISP_E_FAIL		(8)

//fastlogo.ta always uses Internal memory manager
#define VPP_ENABLE_INTERNAL_MEM_MGR

/*definition of global alpha enable or disable (disable means: per pixel alpha)*/
typedef enum {
	GLOBAL_ALPHA_FLAG_DISABLE = 0,
	GLOBAL_ALPHA_FLAG_ENABLE,
} ENUM_GLOBAL_ALPHA_FLAG;


/*definition of various UUID supported*/
typedef enum {
	TA_UUID_FASTLOGO = 0,
	TA_UUID_VPP,
} ENUM_TA_UUID_TYPE;

//struct    --
typedef struct VPP_WIN_T {
	int x;      /* x-coordination of a vpp window top-left corner in pixel, index starting from 0 */
	int y;      /* y-coordination of a vpp window top-left corner in pixle, index starting from 0 */
	int width;  /* width of a vpp window in pixel */
	int height; /* height of a vpp window in pixel */
} VPP_WIN;

typedef struct VPP_WIN_ATTR_T {
	int bgcolor;    /* background color of a vpp window */
	int alpha;      /* global alpha of a vpp window */
	ENUM_GLOBAL_ALPHA_FLAG globalAlphaFlag; /*1:enable plane/global alpha otherwise use per pixel alpha*/
} VPP_WIN_ATTR;

typedef struct VPP_INIT_PARM_T {
	int iHDMIEnable;
	int iVdacEnable;
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
	//Physical address of the shared memory - used for heap/SHM allocation
	unsigned int uiShmPA;
	//Size of the shared memory
	unsigned int uiShmSize;
	unsigned int phShm;
	unsigned int gMemhandle;
#endif
	unsigned int g_vpp;

} VPP_INIT_PARM;

typedef struct VPP_RESOLUTION_DESCRIPTION_T {
	unsigned int   uiActiveWidth;
	unsigned int   uiActiveHeight;
	unsigned int   uiWidth;
	unsigned int   uiHeight;
	unsigned int   uiRefreshRate;
	unsigned int   uiIsInterlaced;
	unsigned int   uiIsDDD;
} VPP_RESOLUTION_DESCRIPTION;

#ifdef __cplusplus
}
#endif
/** _HAL_VPP_H_
 */
#endif
/** ENDOFFILE: hal_vpp.h
 */
