/*************************************************************************/ /*
 VCP core module

 Copyright (C) 2015 - 2018 Renesas Electronics Corporation

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

#ifndef MCVX_ARCH_FCPC_H
#define MCVX_ARCH_FCPC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mcvx_register.h"

/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* FCPC PAR					                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/
#define MCVX_FCPC_NO_ADDR		(  0u )		/* to disable read/write access */

#define MCVX_PACK_FC_DATA( FC_ADDR, DATA_L )					\
	do{ 														\
		MCVX_IR_W0 = 0x00010010u;								\
		MCVX_IR_W1 = ( FC_ADDR );								\
		MCVX_IR_W2 = 0u;										\
		MCVX_IR_W3 = ( DATA_L );								\
		tail += MCVX_IR_SIZEOF_4W; 								\
	}while(0)
																			/*	FC_ADDRESS,		DATA[31: 0] */
#define MCVX_MAKE_FCP_CTRL_PICINFO0( DATA_L )				MCVX_PACK_FC_DATA(	0x01000000u,	DATA_L		)
#define MCVX_MAKE_FCP_CTRL_PICINFO1( DATA_L )				MCVX_PACK_FC_DATA(	0x01000001u,	DATA_L		)
#define MCVX_MAKE_FCP_CTRL_INIT_END							MCVX_PACK_FC_DATA(	0x01000010u,	0u			)

#define MCVX_MAKE_FCP_COMP_PICINFO0( DATA_L )				MCVX_PACK_FC_DATA(	0x02000000u,	DATA_L		)
#define MCVX_MAKE_FCP_COMP_PICINFO1( DATA_L )				MCVX_PACK_FC_DATA(	0x02000001u,	DATA_L		)
#define MCVX_MAKE_FCP_COMP_FCLCMP_CTRL( DATA_L )			MCVX_PACK_FC_DATA(	0x02000002u,	DATA_L		)
#define MCVX_MAKE_FCP_COMP_PICINFO2( DATA_L )				MCVX_PACK_FC_DATA(	0x02000003u,	DATA_L		)
#define MCVX_MAKE_FCP_COMP_PICINFO3( DATA_L )				MCVX_PACK_FC_DATA(	0x02000004u,	DATA_L		)

/* ANC (COMP:W) */
#define MCVX_MAKE_FCP_BA_W_ANC_DEC_TOP_Y( ADDR )			MCVX_PACK_FC_DATA(	0x02000010u,	ADDR		)
#define MCVX_MAKE_FCP_BA_W_ANC_DEC_BOT_Y( ADDR )			MCVX_PACK_FC_DATA(	0x02000011u,	ADDR		)
#define MCVX_MAKE_FCP_BA_W_ANC_DEC_TOP_C( ADDR )			MCVX_PACK_FC_DATA(	0x02000012u,	ADDR		)
#define MCVX_MAKE_FCP_BA_W_ANC_DEC_BOT_C( ADDR )			MCVX_PACK_FC_DATA(	0x02000013u,	ADDR		)

#define MCVX_MAKE_FCP_BA_W_ANC_DISP_TOP_Y( ADDR )			MCVX_PACK_FC_DATA(	0x02000014u,	ADDR		)
#define MCVX_MAKE_FCP_BA_W_ANC_DISP_BOT_Y( ADDR )			MCVX_PACK_FC_DATA(	0x02000015u,	ADDR		)
#define MCVX_MAKE_FCP_BA_W_ANC_DISP_TOP_C( ADDR )			MCVX_PACK_FC_DATA(	0x02000016u,	ADDR		)
#define MCVX_MAKE_FCP_BA_W_ANC_DISP_BOT_C( ADDR )			MCVX_PACK_FC_DATA(	0x02000017u,	ADDR		)

#define MCVX_MAKE_FCP_DCMP_PICINFO0( DATA_L )				MCVX_PACK_FC_DATA(	0x04000000u,	DATA_L		)
#define MCVX_MAKE_FCP_DCMP_PICINFO1( DATA_L )				MCVX_PACK_FC_DATA(	0x04000001u,	DATA_L		)
#define MCVX_MAKE_FCP_DCMP_PICINFO2( DATA_L )				MCVX_PACK_FC_DATA(	0x04000003u,	DATA_L		)
#define MCVX_MAKE_FCP_DCMP_FCLDCM_CTRL( DATA_L )			MCVX_PACK_FC_DATA(	0x04000004u,	DATA_L		)
#define MCVX_MAKE_FCP_DCMP_CACHE_CTRL( DATA_L )				MCVX_PACK_FC_DATA(	0x04000005u,	DATA_L		)
#define MCVX_MAKE_FCP_DCMP_CACHE_ENTRY_OFFSET( DATA_L )		MCVX_PACK_FC_DATA(	0x04000006u,	DATA_L		)

/* ANC (DCMP:R) */
#define MCVX_MAKE_FCP_BA_R_ANC( ADDR, CNT )					MCVX_PACK_FC_DATA(	( 0x04000020u + ( CNT ) ),	ADDR		)
#define MCVX_MAKE_FCP_BA_R_REF( ADDR, CNT )					MCVX_PACK_FC_DATA(	( 0x04000060u + ( CNT ) ),	ADDR		)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* #ifndef MCVX_ARCH_FCPC_H */
