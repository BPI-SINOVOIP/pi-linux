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

#ifndef MCVX_ARCH_BELZ_H
#define MCVX_ARCH_BELZ_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mcvx_register.h"
#include "mcvx_arch_fcpc.h"

/* VP ADDRESS */
#define MCVX_BELZ_VP_VLC_ADDR_TOP		( 0x01000000u )
#define MCVX_BELZ_VP_CE_ADDR_TOP		( 0x0D000000u )

#define MCVX_BELZ_VP_CE_ADDR_IQMAT_4	( 0x0A000070u )
#define MCVX_BELZ_VP_CE_ADDR_IQMAT_8	( 0x0A000080u )
#define MCVX_BELZ_VP_CE_ADDR_IQMAT_16	( 0x0A0000B0u )
#define MCVX_BELZ_VP_CE_ADDR_IQMAT_32	( 0x0A0000E0u )
#define MCVX_BELZ_VP_CE_ADDR_IQMAT_DC	( 0x0A0000F0u )

/* for DCHR */
#define MCVX_BELZ_CH_R					( 0u )
#define MCVX_BELZ_CH_W					( 1u )

#define MCVX_BELZ_TL_V32				( 2u )	/* log2(32)  - 3 */
#define MCVX_BELZ_TL_H128				( 3u )	/* log2(128) - 4 */

#define MCVX_BELZ_AM_TILE16X1_2D		( 2u )	/* tile(16x1) : 2D burst access		*/
#define MCVX_BELZ_AM_TILE16X1_AN		( 4u )	/* tile(16x1) : word-by-word access	*/

#define MCVX_BELZ_IMG_DEC_AM			( MCVX_BELZ_AM_TILE16X1_2D )
#define MCVX_BELZ_IMG_REF_AM			( MCVX_BELZ_AM_TILE16X1_2D )

/* edt max */
/* PIC_CNT_MAX  = 600(VLC), 1000(CE) */
/* CNT_MAX      = 0 (ignore) */
/* SRST_CNT_MAX = 1			 */
#define	MCVX_BELZ_VLC_EDT_VAL	( 0x02580001u )
#define	MCVX_BELZ_CE_EDT_VAL	( 0x03E80001u )

/* DMA access unit size */
#define MCVX_BELZ_DMA_UNIT				( 256u )

/*----------------------------------------------------------------------------------------------------------*/
/* ES size for HW                                                                                           */
/*----------------------------------------------------------------------------------------------------------*/
#define MCVX_ES_SIZE_FOR_HW( ADDR, SIZE ) ( MCVX_BELZ_DMA_UNIT * ( ( ( (SIZE) + ((ADDR)&0xFF) ) + MCVX_BELZ_DMA_UNIT - 1 ) / MCVX_BELZ_DMA_UNIT ) )

/*----------------------------------------------------------------------------------------------------------*/
/* PAR0 macro (AVC)																							*/
/*----------------------------------------------------------------------------------------------------------*/
#define MCVX_PACK_BELZ_VLC_DEC_PAR0_H													\
	( ( ( MCVX_M_01BIT & vlc_dec->cmn_par->ref_placement ) 			<<  (60-32) )		\
	| ( ( MCVX_M_01BIT & vlc_dec->cmn_par->interlace ) 				<<  (59-32) )		\
	| ( ( MCVX_M_01BIT & vlc_dec->cmn_par->field_pic_flag )			<<  (58-32) )		\
	| ( ( MCVX_M_01BIT & vlc_dec->cmn_par->bottom_field_flag )		<<  (57-32) )		\
	| ( ( MCVX_M_01BIT & vlc_dec->cmn_par->top_field_first)			<<  (56-32) )		\
	| ( ( MCVX_M_01BIT & vlc_dec->cmn_par->mode )					<<	(48-32)	)		\
	| ( ( MCVX_M_04BIT & vlc_dec->cmn_par->hw_stream_type )			<<	(40-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(36-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(32-32)	) )

#define MCVX_PACK_BELZ_VLC_DEC_PAR0_L													\
	( ( ( MCVX_M_14BIT & vlc_dec->cmn_par->pic_width_m1 )			<<		16	)		\
	| ( ( MCVX_M_14BIT & vlc_dec->cmn_par->pic_height_m1 )			<<		 0	) )

#define MCVX_PACK_BELZ_CE_DEC_PAR0_H													\
	( ( ( MCVX_M_01BIT & ce_dec->cmn_par->ref_placement )			<<	(60-32)	)		\
	| ( ( MCVX_M_01BIT & ce_dec->cmn_par->interlace )				<<	(59-32)	)		\
	| ( ( MCVX_M_01BIT & ce_dec->cmn_par->field_pic_flag )			<<	(58-32)	)		\
	| ( ( MCVX_M_01BIT & ce_dec->cmn_par->bottom_field_flag )		<<	(57-32)	)		\
	| ( ( MCVX_M_01BIT & ce_dec->cmn_par->top_field_first )			<<	(56-32)	)		\
	| ( ( MCVX_M_01BIT & ce_dec->cmn_par->mode )					<<	(48-32)	)		\
	| ( ( MCVX_M_04BIT & ce_dec->cmn_par->hw_stream_type )			<<	(40-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(36-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(32-32)	) )

#define MCVX_PACK_BELZ_CE_DEC_PAR0_L													\
	( ( ( MCVX_M_14BIT & ce_dec->cmn_par->pic_width_m1 )			<<		16	)		\
	| ( ( MCVX_M_14BIT & ce_dec->cmn_par->pic_height_m1 )			<<		 0	) )

#define MCVX_PACK_BELZ_VLC_ENC_PAR0_H													\
	( ( ( MCVX_M_01BIT & vlc_enc->cmn_par->mode )					<<	(48-32)	)		\
	| ( ( MCVX_M_04BIT & vlc_enc->cmn_par->hw_stream_type )			<<	(40-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(36-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(32-32)	) )

#define MCVX_PACK_BELZ_VLC_ENC_PAR0_L													\
	( ( ( MCVX_M_14BIT & vlc_enc->cmn_par->pic_width_m1 )			<<		16	)		\
	| ( ( MCVX_M_14BIT & vlc_enc->cmn_par->pic_height_m1 )			<<		 0	) )

#define MCVX_PACK_BELZ_CE_ENC_PAR0_H													\
	( ( ( MCVX_M_01BIT & ce_enc->cmn_par->ref_placement )			<<	(60-32)	)		\
	| ( ( MCVX_M_01BIT & ce_enc->cmn_par->interlace )				<<	(59-32)	)		\
	| ( ( MCVX_M_01BIT & ce_enc->cmn_par->field_pic_flag )			<<	(58-32)	)		\
	| ( ( MCVX_M_01BIT & ce_enc->cmn_par->bottom_field_flag )		<<	(57-32)	)		\
	| ( ( MCVX_M_01BIT & ce_enc->cmn_par->top_field_first )			<<	(56-32)	)		\
	| ( ( MCVX_M_01BIT & ce_enc->cmn_par->mode )					<<	(48-32)	)		\
	| ( ( MCVX_M_04BIT & ce_enc->cmn_par->hw_stream_type )			<<	(40-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(36-32)	)		\
	| ( ( MCVX_M_02BIT & 0u )										<<	(32-32)	) )		\

#define MCVX_PACK_BELZ_CE_ENC_PAR0_L													\
	( ( ( MCVX_M_14BIT & ce_enc->cmn_par->pic_width_m1 )			<<		16	)		\
	| ( ( MCVX_M_14BIT & ce_enc->cmn_par->pic_height_m1 )			<<		 0	) )		\

/*----------------------------------------------------------------------------------------------------------*/
/* NOP IRP template																							*/
/*----------------------------------------------------------------------------------------------------------*/
#define MCVX_BELZ_NOP_PACKET		\
	do{ 							\
		MCVX_IR_W0 = 0x00000010u;	\
		MCVX_IR_W1 = 0x00000000u;	\
		MCVX_IR_W2 = 0x00000000u;	\
		MCVX_IR_W3 = 0x00000000u;	\
		tail += MCVX_IR_SIZEOF_4W; 	\
	}while(0)

/*----------------------------------------------------------------------------------------------------------*/
/* DMA BA template																							*/
/*----------------------------------------------------------------------------------------------------------*/
#define MCVX_BELZ_FMT_VLC_BA( VA_OFFSET, ADDR, SIZE )							\
	do{ 																		\
		MCVX_IR_W0 = 0u;														\
		MCVX_IR_W1 = ( MCVX_BELZ_VP_VLC_ADDR_TOP + ( VA_OFFSET ) );				\
		MCVX_IR_W2 = ( MCVX_M_20BIT & ( ( ( (SIZE) + ((ADDR)&0xFF) ) + MCVX_BELZ_DMA_UNIT - 1 ) / MCVX_BELZ_DMA_UNIT ) );	\
		MCVX_IR_W3 = ( ADDR );	 												\
		tail += MCVX_IR_SIZEOF_4W; 												\
	}while(0)

#define MCVX_BELZ_FMT_CE_BA( CID, VA_OFFSET, ADDR, SIZE )						\
	do{ 																		\
		MCVX_IR_W0 = ( ( ( MCVX_M_02BIT & ( CID ) )		<<	24 ) );				\
		MCVX_IR_W1 = ( MCVX_BELZ_VP_CE_ADDR_TOP + ( VA_OFFSET ) );				\
		MCVX_IR_W2 = ( MCVX_M_20BIT & ( ( ( (SIZE) + ( (ADDR)&0xFF) ) + MCVX_BELZ_DMA_UNIT - 1u ) / MCVX_BELZ_DMA_UNIT ) );	\
		MCVX_IR_W3 = ( ADDR );	 												\
		tail += MCVX_IR_SIZEOF_4W; 												\
	}while(0)

#define MCVX_BELZ_FMT_CE_BA_IMG( VA_OFFSET, STRIDE, ADDR )						\
	do{ 																		\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<<	28 ) );				\
		MCVX_IR_W1 = ( MCVX_BELZ_VP_CE_ADDR_TOP + ( VA_OFFSET ) );				\
		MCVX_IR_W2 = ( ( MCVX_M_10BIT & ( ( STRIDE ) >> 4u ) ) );				\
		MCVX_IR_W3 = ( ADDR );	 												\
		tail += MCVX_IR_SIZEOF_4W; 												\
	}while(0)

/* Double buffer */
#define MCVX_BELZ_FMT_VLC_BA_DOUBLE( VA_OFFSET, ADDR0, SIZE0, ADDR1, SIZE1 )	\
	do{ 																		\
		MCVX_BELZ_FMT_VLC_BA( ( (VA_OFFSET) +0u ),	(ADDR0), (SIZE0) );			\
		MCVX_BELZ_FMT_VLC_BA( ( (VA_OFFSET) +1u ),	(ADDR1), (SIZE1) );			\
	}while(0)

#define MCVX_BELZ_FMT_CE_BA_DOUBLE( CID, VA_OFFSET, ADDR0, SIZE0, ADDR1, SIZE1 )	\
	do{ 																			\
		MCVX_BELZ_FMT_CE_BA( ( CID ), ( (VA_OFFSET) +0u),	(ADDR0), (SIZE0) );		\
		MCVX_BELZ_FMT_CE_BA( ( CID ), ( (VA_OFFSET) +1u),	(ADDR1), (SIZE1) );		\
	}while(0)

/*----------------------------------------------------------------------------------------------------------*/
/* DMA CHANNEL template			                                                                            */
/*----------------------------------------------------------------------------------------------------------*/
#define MCVX_BELZ_FMT_VLC_DCHR_X( VP_OFFSET, ENDIAN )											\
	do{ 																						\
		MCVX_IR_W0 = 0u;																		\
		MCVX_IR_W1 = MCVX_BELZ_VP_VLC_ADDR_TOP + ( VP_OFFSET );									\
		MCVX_IR_W2 = 0u;																		\
		MCVX_IR_W3 = ( ( MCVX_M_04BIT & ( ENDIAN ) )			<<  0 );	 					\
		tail += MCVX_IR_SIZEOF_4W; 																\
	}while(0)

#define MCVX_BELZ_FMT_CE_DCHR_X( VP_OFFSET, YC , DECPIC, L2TH, L2TV, L2STRIDE, AM, ACM, TM, WR, ENDIAN )	\
	do{ 																							\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )				<< 28 ) );							\
		MCVX_IR_W1 = MCVX_BELZ_VP_CE_ADDR_TOP + ( VP_OFFSET );										\
		MCVX_IR_W2 = 0u;																			\
		MCVX_IR_W3 =																				\
					( ( ( MCVX_M_01BIT & ( ACM ) )				<< 25 )								\
					| ( ( MCVX_M_01BIT & ( 1u ) )				<< 24 )								\
					| ( ( MCVX_M_01BIT & ( YC ) )				<< 22 )								\
					| ( ( MCVX_M_01BIT & ( DECPIC ) )			<< 21 )								\
					| ( ( MCVX_M_03BIT & ( L2TH ) )				<< 18 )								\
					| ( ( MCVX_M_03BIT & ( L2TV ) )				<< 15 )								\
					| ( ( MCVX_M_04BIT & ( L2STRIDE-(L2TH)-4 ) )	<< 11 )							\
					| ( ( MCVX_M_03BIT & ( AM ) )				<<  8 )								\
					| ( ( MCVX_M_02BIT & ( TM ) )				<<  6 )								\
					| ( ( MCVX_M_01BIT & ( WR ) )				<<  5 )								\
					| ( ( MCVX_M_04BIT & ( ENDIAN ) )			<<  0 ) );	 						\
		tail += MCVX_IR_SIZEOF_4W; 																	\
	}while(0)

#define MCVX_BELZ_FMT_CE_DCHR_X_2( VP_OFFSET, YC , DECPIC, L2TH, L2TV, L2STRIDE, AM, ACM, TM, WR, ENDIAN )	\
	do{ 																							\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )				<< 28 ) );							\
		MCVX_IR_W1 = MCVX_BELZ_VP_CE_ADDR_TOP + ( VP_OFFSET );										\
		MCVX_IR_W2 = 0u;																			\
		MCVX_IR_W3 =																				\
					( ( ( MCVX_M_01BIT & ( ACM ) )				<< 25 )								\
					| ( ( MCVX_M_01BIT & ( 1u ) )				<< 24 )								\
					| ( ( MCVX_M_01BIT & ( YC ) )				<< 22 )								\
					| ( ( MCVX_M_01BIT & ( DECPIC ) )			<< 21 )								\
					| ( ( MCVX_M_03BIT & ( L2TH ) )				<< 18 )								\
					| ( ( MCVX_M_03BIT & ( L2TV ) )				<< 15 )								\
					| ( ( MCVX_M_04BIT & ( L2STRIDE-(L2TH) ) )	<< 11 )								\
					| ( ( MCVX_M_03BIT & ( AM ) )				<<  8 )								\
					| ( ( MCVX_M_02BIT & ( TM ) )				<<  6 )								\
					| ( ( MCVX_M_01BIT & ( WR ) )				<<  5 )								\
					| ( ( MCVX_M_04BIT & ( ENDIAN ) )			<<  0 ) );	 						\
		tail += MCVX_IR_SIZEOF_4W; 																	\
	}while(0)

#define MCVX_BELZ_FMT_CE_DCHR_BOT( VP_OFFSET, YC , DECPIC, L2TH, L2TV, L2STRIDE, AM, ACM, TM, WR, ENDIAN )	\
	do{ 																							\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )				<< 28 ) );							\
		MCVX_IR_W1 = MCVX_BELZ_VP_CE_ADDR_TOP + ( VP_OFFSET );										\
		MCVX_IR_W2 = 0u;																			\
		MCVX_IR_W3 =																				\
					( ( ( MCVX_M_01BIT & ( ACM ) )				<< 25 )								\
					| ( ( MCVX_M_01BIT & ( 1u ) )				<< 24 )								\
					| ( ( MCVX_M_01BIT & ( 1u ) )				<< 23 )								\
					| ( ( MCVX_M_01BIT & ( YC ) )				<< 22 )								\
					| ( ( MCVX_M_01BIT & ( DECPIC ) )			<< 21 )								\
					| ( ( MCVX_M_03BIT & ( L2TH ) )				<< 18 )								\
					| ( ( MCVX_M_03BIT & ( L2TV ) )				<< 15 )								\
					| ( ( MCVX_M_04BIT & ( L2STRIDE-(L2TH)-4 ) )	<< 11 )							\
					| ( ( MCVX_M_03BIT & ( AM ) )				<<  8 )								\
					| ( ( MCVX_M_02BIT & ( TM ) )				<<  6 )								\
					| ( ( MCVX_M_01BIT & ( WR ) )				<<  5 )								\
					| ( ( MCVX_M_04BIT & ( ENDIAN ) )			<<  0 ) );	 						\
		tail += MCVX_IR_SIZEOF_4W; 																	\
	}while(0)

#define MCVX_BELZ_FMT_DATA( MBC, CBC, VP_ADDR, DATA_H, DATA_L )				\
	do{ 																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( MBC ) ) << 29 )					\
					 | ( ( MCVX_M_01BIT & ( CBC ) ) << 28 ) );				\
		MCVX_IR_W1 = ( VP_ADDR );											\
		MCVX_IR_W2 = ( DATA_H );											\
		MCVX_IR_W3 = ( DATA_L );											\
		tail += MCVX_IR_SIZEOF_4W; 											\
	}while(0)


/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* DMA BASE ADDRESS			                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/

/* ES (VLC) */
#define MCVX_BELZ_VP_VLC_BA_ES( ADDR0, SIZE0, ADDR1, SIZE1 )			MCVX_BELZ_FMT_VLC_BA_DOUBLE( 0x0100, ADDR0, SIZE0, ADDR1, SIZE1 )

/* IMD (VLC) */
#define MCVX_BELZ_VP_VLC_BA_IMS( CID, ADDR0, SIZE0, ADDR1, SIZE1 )		MCVX_BELZ_FMT_VLC_BA_DOUBLE(	( 0x0110 + ((CID)*2u) ), ADDR0, SIZE0, ADDR1, SIZE1 )
#define MCVX_BELZ_VP_VLC_BA_IMC( CID, ADDR0, SIZE0, ADDR1, SIZE1 )		MCVX_BELZ_FMT_VLC_BA_DOUBLE(	( 0x0120 + ((CID)*2u) ), ADDR0, SIZE0, ADDR1, SIZE1 )
#define MCVX_BELZ_VP_VLC_BA_ITS( ADDR, SIZE )							MCVX_BELZ_FMT_VLC_BA( ( 0x0138 ),			ADDR, SIZE )
#define MCVX_BELZ_VP_VLC_BA_ITS_SEQ( ADDR, SIZE )						MCVX_BELZ_FMT_VLC_BA( ( 0x0100 + 0x38 ), ADDR, SIZE )

/* MISC (VLC) */
#define MCVX_BELZ_VP_VLC_BA_LD( ADDR )									MCVX_BELZ_FMT_VLC_BA( ( 0x0130			),	ADDR, 0u		)
#define MCVX_BELZ_VP_VLC_BA_W_COL( ADDR )								MCVX_BELZ_FMT_VLC_BA( ( 0x0140			),	ADDR, 0u		)
#define MCVX_BELZ_VP_VLC_BA_R_COL( ADDR, CNT )							MCVX_BELZ_FMT_VLC_BA( ( 0x0150 + (CNT)	),	ADDR, 0u		)
#define MCVX_BELZ_VP_VLC_BA_DI( ADDR )									MCVX_BELZ_FMT_VLC_BA( ( 0x0170			),	ADDR, 0u		)
#define MCVX_BELZ_VP_VLC_BA_W_SEG( ADDR )								MCVX_BELZ_FMT_VLC_BA( ( 0x0174			),	ADDR, 0u		)
#define MCVX_BELZ_VP_VLC_BA_PROB( ADDR )								MCVX_BELZ_FMT_VLC_BA( ( 0x0178			),	ADDR, 0u		)
#define MCVX_BELZ_VP_VLC_BA_R_SEG( ADDR )								MCVX_BELZ_FMT_VLC_BA( ( 0x017C			),	ADDR, 0u		)

/* IMD (CE) */
#define MCVX_BELZ_VP_CE_BA_IMS( CID, ADDR0, SIZE0, ADDR1, SIZE1 )		MCVX_BELZ_FMT_CE_BA_DOUBLE( CID, 0x0070, ADDR0, SIZE0, ADDR1, SIZE1 )
#define MCVX_BELZ_VP_CE_BA_IMC( CID, ADDR0, SIZE0, ADDR1, SIZE1 )		MCVX_BELZ_FMT_CE_BA_DOUBLE( CID, 0x0072, ADDR0, SIZE0, ADDR1, SIZE1 )

/* QMAP (CE) */
#define MCVX_BELZ_VP_CE_BA_QMAP( CID, ADDR )							MCVX_BELZ_FMT_CE_BA( CID, 0x0052, ADDR, 0u )

/* IMAGE/QMAP/LB (CE) */
#define MCVX_BELZ_VP_CE_BA_R_ENC_TOP_Y( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0040,	STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_R_ENC_TOP_C( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0041,	STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_R_ENC_TOP_CB( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0041,	STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_R_ENC_TOP_CR( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0042,	STRIDE, ADDR )

#define MCVX_BELZ_VP_CE_BA_W_DEC_TOP_Y( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0043,	STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_W_DEC_TOP_C( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0044,	STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_W_DEC_BOT_Y( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0045,	STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_W_DEC_BOT_C( STRIDE, ADDR )					MCVX_BELZ_FMT_CE_BA_IMG( 0x0046,	STRIDE, ADDR )

#define MCVX_BELZ_VP_CE_BA_R_REF_Y( STRIDE, ADDR, IDX )					MCVX_BELZ_FMT_CE_BA_IMG( ( 0x0060 + ((IDX)*2u) ), STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_R_REF_C( STRIDE, ADDR, IDX )					MCVX_BELZ_FMT_CE_BA_IMG( ( 0x0061 + ((IDX)*2u) ), STRIDE, ADDR )

#define MCVX_BELZ_VP_CE_BA_R_AREF_Y( STRIDE, ADDR, IDX )				MCVX_BELZ_FMT_CE_BA_IMG( ( 0x0080 + ((IDX)*2u) ), STRIDE, ADDR )
#define MCVX_BELZ_VP_CE_BA_R_AREF_C( STRIDE, ADDR, IDX )				MCVX_BELZ_FMT_CE_BA_IMG( ( 0x0081 + ((IDX)*2u) ), STRIDE, ADDR )

#define MCVX_BELZ_VP_CE_BA_R_LB( ADDR )									MCVX_BELZ_FMT_CE_BA_IMG( 0x0055, 0u, ADDR )
#define MCVX_BELZ_VP_CE_BA_W_LB( ADDR )									MCVX_BELZ_FMT_CE_BA_IMG( 0x0056, 0u, ADDR )

#define MCVX_BELZ_VP_CE_BA_DIR_REG_DUMP( ADDR )							MCVX_BELZ_FMT_CE_BA_IMG( 0x0024, 0u, ADDR )


/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* DMA CHANNEL				                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/
/* ES (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_ES( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x0180,	ENDIAN )

/* IMD (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_IMS( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x0190,	ENDIAN )

/* Coliocated info (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_COL( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x01C0,	ENDIAN )

/* Line data (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_LD( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x01B0,	ENDIAN )

/* Intermediate stream size for each tile (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_ITS( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x01B8,	ENDIAN )

/* Segmentation info (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_SEG( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x01C4,	ENDIAN )

/* Decoded info (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_DI( ENDIAN )							MCVX_BELZ_FMT_VLC_DCHR_X( 0x01C8,	ENDIAN )

/* Probability info (VLC) */
#define MCVX_BELZ_VP_VLC_DCHR_PROB( ENDIAN )						MCVX_BELZ_FMT_VLC_DCHR_X( 0x01D8,	ENDIAN )

/* IMD (CE) */																				/*	VP_ADDR,	YC,	DECPIC, L2TH,				L2TV,				L2STRIDE,	AM,						ACM,TM,	WR,			ENDIAN */
#define MCVX_BELZ_VP_CE_DCHR_R_IMS( ENDIAN )						MCVX_BELZ_FMT_CE_DCHR_X(	0x0018,		0u,	0u,		0u,					0u,					4u,			0u,						0u, 1u, MCVX_BELZ_CH_R, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_R_IMC( ENDIAN )						MCVX_BELZ_FMT_CE_DCHR_X(	0x0019,		0u,	0u,		0u,					0u,					4u,			0u,						0u, 1u, MCVX_BELZ_CH_R, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_W_IMS( ENDIAN )						MCVX_BELZ_FMT_CE_DCHR_X(	0x0018,		0u,	0u,		0u,					0u,					4u,			0u,						0u, 1u, MCVX_BELZ_CH_W, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_W_IMC( ENDIAN )						MCVX_BELZ_FMT_CE_DCHR_X(	0x0019,		0u,	0u,		0u,					0u,					4u,			0u,						0u, 1u, MCVX_BELZ_CH_W, ( ENDIAN ) )

/* IMAGE/LB (CE) */																			/*	VP_ADDR,	YC,	DECPIC, L2TH,				L2TV,				L2STRIDE,	AM,						ACM,TM,	WR,			ENDIAN */
#define MCVX_BELZ_VP_CE_DCHR_R_ENC_Y( L2STRIDE, ENDIAN )			MCVX_BELZ_FMT_CE_DCHR_X_2(	0x0005,		0u,	0u,		4u,					3u,					L2STRIDE,	0u,						0u, 0u, MCVX_BELZ_CH_R, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_R_ENC_C( L2STRIDE, ENDIAN )			MCVX_BELZ_FMT_CE_DCHR_X_2(	0x0006,		0u,	0u,		4u,					3u,					L2STRIDE,	0u,						0u, 0u, MCVX_BELZ_CH_R, ( ENDIAN ) )

#define MCVX_BELZ_VP_CE_DCHR_R_REF_Y( L2STRIDE, ENDIAN )			MCVX_BELZ_FMT_CE_DCHR_X(	0x0003,		0u,	0u,		MCVX_BELZ_TL_H128,	MCVX_BELZ_TL_V32,	L2STRIDE,	MCVX_BELZ_IMG_REF_AM,	1u, 0u, MCVX_BELZ_CH_R, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_R_REF_C( L2STRIDE, ENDIAN )			MCVX_BELZ_FMT_CE_DCHR_X(	0x0004,		0u,	0u,		MCVX_BELZ_TL_H128,	MCVX_BELZ_TL_V32,	L2STRIDE,	MCVX_BELZ_IMG_REF_AM,	1u, 0u, MCVX_BELZ_CH_R, ( ENDIAN ) )

#define MCVX_BELZ_VP_CE_DCHR_W_DEC_Y( L2STRIDE, ENDIAN )			MCVX_BELZ_FMT_CE_DCHR_X(	0x0007,		0u,	1u,		MCVX_BELZ_TL_H128,	MCVX_BELZ_TL_V32,	L2STRIDE,	MCVX_BELZ_IMG_DEC_AM,	0u, 0u, MCVX_BELZ_CH_W, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_W_DEC_C( L2STRIDE, ENDIAN )			MCVX_BELZ_FMT_CE_DCHR_X(	0x0008,		1u,	1u,		MCVX_BELZ_TL_H128,	MCVX_BELZ_TL_V32,	L2STRIDE,	MCVX_BELZ_IMG_DEC_AM,	0u, 0u, MCVX_BELZ_CH_W, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_W_DEC_BOT_Y( L2STRIDE, ENDIAN )		MCVX_BELZ_FMT_CE_DCHR_BOT(	0x001A,		0u,	1u,		MCVX_BELZ_TL_H128,	MCVX_BELZ_TL_V32,	L2STRIDE,	MCVX_BELZ_IMG_DEC_AM,	0u, 0u, MCVX_BELZ_CH_W, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_W_DEC_BOT_C( L2STRIDE, ENDIAN )		MCVX_BELZ_FMT_CE_DCHR_BOT(	0x001B,		1u,	1u,		MCVX_BELZ_TL_H128,	MCVX_BELZ_TL_V32,	L2STRIDE,	MCVX_BELZ_IMG_DEC_AM,	0u, 0u, MCVX_BELZ_CH_W, ( ENDIAN ) )

#define MCVX_BELZ_VP_CE_DCHR_R_LB( ENDIAN )							MCVX_BELZ_FMT_CE_DCHR_X(	0x0014,		0u, 0u,		0u,					0u,					0u,			0u,						0u, 0u, MCVX_BELZ_CH_R, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_W_LB( ENDIAN )							MCVX_BELZ_FMT_CE_DCHR_X(	0x0015,		0u, 0u,		0u,					0u,					0u,			0u,						0u, 0u, MCVX_BELZ_CH_W, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_QMAP( ENDIAN )							MCVX_BELZ_FMT_CE_DCHR_X(	0x0016,		0u, 0u,		0u,					0u,					0u,			0u,						0u, 0u, MCVX_BELZ_CH_R, ( ENDIAN ) )
#define MCVX_BELZ_VP_CE_DCHR_DIR_REG_DUMP( ENDIAN )					MCVX_BELZ_FMT_CE_DCHR_X(	0x001F,		0u, 0u,		0u,					0u,					0u,			0u,						0u, 0u, MCVX_BELZ_CH_W, ( ENDIAN ) )


/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* VLC PAR					                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/

/* VLC PAR */																	/*	MBC,	CBC,	VP_ADDRESS,					DATA[63:32],	DATA[31: 0] */
#define MCVX_BELZ_VP_VLC_PAR0( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000010,					DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_PAR1( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000011,					DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_PAR2( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000012,					DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_PAR3( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000013,					DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_PAR4( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000014,					DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_PAR5( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000015,					DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_PAR6( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000016,					DATA_H,			DATA_L		)

/* VLC_PIC_SIZE */																/*	MBC,	CBC,	VP_ADDRESS,					DATA[63:32],	DATA[31: 0] */
#define MCVX_BELZ_VP_VLC_PIC_SIZE( PIC_SIZE )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	0x01000001,					0u,				PIC_SIZE	)

/* HEVC, AVC */
#define MCVX_BELZ_VP_VLC_REF_POC( POC_H, POC_L, CNT )			MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000060 + ( CNT ) ),	POC_H,			POC_L		)
#define MCVX_BELZ_VP_VLC_CUR_POC( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000070           ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_SLICE_SIZE( DATA_H, DATA_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000200 + ( CNT ) ),	DATA_H,			DATA_L		)

/* AVC */
#define MCVX_BELZ_VP_VLC_MVRINFO( MVRINFO )						MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000074           ),	0u,				MVRINFO		)
#define MCVX_BELZ_VP_VLC_INITREF_B_L0( UID_H, UID_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x010000C0 + ( CNT ) ),	UID_H,			UID_L		)
#define MCVX_BELZ_VP_VLC_INITREF_B_L1( UID_H, UID_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x010000D0 + ( CNT ) ),	UID_H,			UID_L		)
#define MCVX_BELZ_VP_VLC_INITREF_P( UID_H, UID_L, CNT )			MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x010000B0 + ( CNT ) ),	UID_H,			UID_L		)

/* HEVC */
#define MCVX_BELZ_VP_VLC_TILE_COLW( DATA_H, DATA_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000080 + ( CNT ) ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_TILE_ROW( DATA_H, DATA_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000088 + ( CNT ) ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_INIT_REF_TBL( RID_H, RID_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x010000B0 + ( CNT ) ),	RID_H,			RID_L		)
#define MCVX_BELZ_VP_VLC_COL_REF_POC( POC_H, POC_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x010000C0 + ( CNT ) ),	POC_H,			POC_L		)

/* VP9 */
#define MCVX_MAKE_VP_VLC_Q_FEATURE( DATA_H, DATA_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000062 + ( CNT ) ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_LF_FEATURE( DATA_H, DATA_L )			MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000064           ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_REF_SKIP_FEATURE( DATA_H, DATA_L )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000065           ),	DATA_H,			DATA_L		)

#define MCVX_BELZ_VP_VLC_SH_DATA0_31( DATA_H, DATA_L, CNT )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000020 + ( CNT ) ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_SH_DATA32_47( DATA_H, DATA_L, CNT )	MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000040 + ( CNT ) ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_SH( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000050           ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_SEQ_CTRL( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000008           ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_SEQ_OUTPUT( DATA_H, DATA_L )			MCVX_BELZ_FMT_DATA(	0x00,	0x00,   ( 0x01000009           ),   DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_VLC_SIZE_INT_CTRL( DATA_H, DATA_L )		MCVX_BELZ_FMT_DATA(	0x00,	0x00,	( 0x01000004           ),	DATA_H,			DATA_L		)

#define MCVX_BELZ_VP_VLC_SWITCH( EC_ALL_PIC, EC_MODE, EC_RCV, IS_RW_MODE, COL_INFO_IN, COL_INFO_OUT, LINE_INFO_OUT, SKIP_NAL_BITS, SEG_INFO_IN, SEG_INFO_OUT )		\
	do{																							\
		MCVX_IR_W0 = 0x00000000u;																\
		MCVX_IR_W1 = 0x01000000u;																\
		MCVX_IR_W2 =( ( ( MCVX_M_01BIT & ( EC_ALL_PIC )	)			<<	(63-32) )				\
					| ( ( MCVX_M_01BIT & ( EC_MODE ) )				<<	(62-32) )				\
					| ( ( MCVX_M_01BIT & ( EC_RCV ) )				<<	(61-32) )				\
					| ( ( MCVX_M_01BIT & ( IS_RW_MODE )	)			<<	(48-32) )				\
					| ( ( MCVX_M_01BIT & ( SEG_INFO_IN ) )			<<	(36-32) )				\
					| ( ( MCVX_M_01BIT & ( SEG_INFO_OUT ) )			<<	(35-32) )				\
					| ( ( MCVX_M_01BIT & ( COL_INFO_IN ) )			<<	(34-32) )				\
					| ( ( MCVX_M_01BIT & ( COL_INFO_OUT ) )			<<	(33-32) )				\
					| ( ( MCVX_M_01BIT & ( LINE_INFO_OUT ) )		<<	(32-32) ) );			\
		MCVX_IR_W3 =( ( ( MCVX_M_32BIT & ( SKIP_NAL_BITS )	)		<<		 0  ) );			\
		tail += MCVX_IR_SIZEOF_4W; 																\
	}while(0)


/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* CE PAR					                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/
																					/*	MBC,	CBC,	VP_ADDRESS,		DATA[63:32],	DATA[31: 0] */
#define MCVX_BELZ_VP_CE_PAR0( DATA_H, DATA_L )						MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000010,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_PAR1( DATA_H, DATA_L )						MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000011,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_PAR2( DATA_H, DATA_L )						MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000012,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_PAR3( DATA_H, DATA_L )						MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000013,		DATA_H,			DATA_L		)

/* Encoder */
#define MCVX_BELZ_VP_CE_SLC_SIZE( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x00000028,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_ENC_CTRL1( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x0000001a,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_ENC_CTRL2( DATA_H, DATA_L ) 				MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x0000001b,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_SEQ_CTRL0( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x00000020,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_SEQ_CTRL1( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x00000021,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_RATE_CTRL0( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x00000050,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_RATE_CTRL1( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x00000051,		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_IS_BUFF_TH( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x00000058,		DATA_H,			DATA_L		)

#define MCVX_BELZ_VP_CE_TILE_COLW_MBC( DATA_H, DATA_L, CNT )		MCVX_BELZ_FMT_DATA(	0x01,	0x01,	(0x00000040 + ( CNT )),		DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_TILE_ROW_MBC( DATA_H, DATA_L, CNT )			MCVX_BELZ_FMT_DATA(	0x01,	0x01,	(0x00000048 + ( CNT )),		DATA_H,			DATA_L		)

#define MCVX_BELZ_VP_CE_TMP_CTRL( DATA_H, DATA_L )					MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x03000003,		DATA_H,			DATA_L		)

#define MCVX_BELZ_VP_CE_INTRA_REFRESH( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x04000002u,	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_INTRA_CTRL0( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000060u,	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_INTRA_CTRL1( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000061u,	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_INTRA_CTRL2( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000062u,	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_INTER_CTRL( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x01,	0x01,	0x00000068u,	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_INTER_OFFSET0( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x06000066u,	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_INTER_OFFSET1( DATA_H, DATA_L )				MCVX_BELZ_FMT_DATA(	0x00,	0x01,	0x06000067u,	DATA_H,			DATA_L		)

#define MCVX_BELZ_VP_CE_REF_CTRL( REF_AM )								\
	do{																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<< 28 ) );		\
		MCVX_IR_W1 = 0x03000004u;										\
		MCVX_IR_W2 = ( REF_AM );										\
		MCVX_IR_W3 = 0u;												\
		tail += MCVX_IR_SIZEOF_4W; 										\
	}while(0)

#define MCVX_BELZ_VP_CE_CLK_STOP( CLK_STOP )							\
	do{																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<< 28 ) );		\
		MCVX_IR_W1 = 0x000000C3u;										\
		MCVX_IR_W2 = 0x00000000u;										\
		MCVX_IR_W3 =( ( ( MCVX_M_32BIT & ( CLK_STOP ) )	<<	0 ) );		\
		tail += MCVX_IR_SIZEOF_4W; 										\
	}while(0)

#define MCVX_BELZ_VP_CE_USER_ID( USER_ID )								\
	do{																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<< 28 ) );		\
		MCVX_IR_W1 = 0x000000C4u;										\
		MCVX_IR_W2 = 0x00000000u;										\
		MCVX_IR_W3 =( ( ( MCVX_M_32BIT & ( USER_ID ) )	<<	0 ) );		\
		tail += MCVX_IR_SIZEOF_4W; 										\
	}while(0)

#define MCVX_BELZ_VP_CE_RLC_CTRL( THR_MODE, CL_ADDR_TRANS_MODE, SHIFT_MODE )			\
	do{																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<< 28 ) );		\
		MCVX_IR_W1 = 0x0D0000CCu;										\
		MCVX_IR_W2 =( ( ( MCVX_M_01BIT & ( THR_MODE )	)			<<	(40-32) )		\
					| ( ( MCVX_M_03BIT & ( CL_ADDR_TRANS_MODE ) )	<<	(32-32) ) );	\
		MCVX_IR_W3 =( ( ( MCVX_M_16BIT & ( SHIFT_MODE ) )	<<	0 ) );	\
		tail += MCVX_IR_SIZEOF_4W; 										\
	}while(0)

#define MCVX_BELZ_VP_CE_RLC_REF_OFS0()									\
	do{																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<< 28 ) );		\
		MCVX_IR_W1 = 0x0D0000CDu;										\
		MCVX_IR_W2 = 0x00000000u;										\
		MCVX_IR_W3 = 0x00000000u;										\
		tail += MCVX_IR_SIZEOF_4W; 										\
	}while(0)

#define MCVX_BELZ_VP_CE_RLC_REF_OFS1()									\
	do{																	\
		MCVX_IR_W0 = ( ( ( MCVX_M_01BIT & ( 1u ) )		<< 28 ) );		\
		MCVX_IR_W1 = 0x0D0000CEu;										\
		MCVX_IR_W2 = 0x00000000u;										\
		MCVX_IR_W3 = 0x00000000u;										\
		tail += MCVX_IR_SIZEOF_4W; 										\
	}while(0)

#define MCVX_BELZ_VP_CE_SWITCH( IMSB_MODE, LINE_INFO_OUT, DEC_OUT_MODE, DEC_OUT_DISABLE, EC_ALL_PIC, EC_MODE )	\
	do{																								\
		MCVX_IR_W0 =( ( ( MCVX_M_01BIT & ( 1u ) )					<< 29 )							\
					| ( ( MCVX_M_01BIT & ( 1u ) )					<< 28 ) );						\
		MCVX_IR_W1 = 0x00000018u;																	\
		MCVX_IR_W2 =( ( ( MCVX_M_01BIT & ( EC_ALL_PIC )	)			<<	(63-32) )					\
					| ( ( MCVX_M_02BIT & ( EC_MODE ) )				<<	(61-32) )					\
					| ( ( MCVX_M_01BIT & ( 0u ) )					<<	(60-32) )					\
					| ( ( MCVX_M_01BIT & ( IMSB_MODE ) )			<<	(48-32) )					\
					| ( ( MCVX_M_01BIT & ( 0u ) )					<<	(40-32) )					\
					| ( ( MCVX_M_02BIT & ( DEC_OUT_MODE ) )			<<	(34-32)	)					\
					| ( ( MCVX_M_01BIT & ( DEC_OUT_DISABLE ) )		<<	(33-32)	)					\
					| ( ( MCVX_M_01BIT & ( LINE_INFO_OUT ) )		<<	(32-32) ) );				\
		MCVX_IR_W3 = 0;																				\
		tail += MCVX_IR_SIZEOF_4W; 																	\
	}while(0)

/* VP9 */
#define MCVX_BELZ_VP_CE_SCALE_INFO( DATA_H, DATA_L, CNT )			MCVX_BELZ_FMT_DATA(	0x01,	0x01,	( 0x00000030 + ( CNT ) ),	DATA_H,			DATA_L		)
#define MCVX_BELZ_VP_CE_REF_DELTA( DATA_H, DATA_L)					MCVX_BELZ_FMT_DATA(	0x00,	0x01,	( 0x0B000070           ),	DATA_H,			DATA_L		)

/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* VLC REG					                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/	
#define MCVX_BELZ_REG_VLC_PBAH( IRP_SIZE, ENDIAN )									\
	( ( ( MCVX_M_08BIT & ( ( ( ( IRP_SIZE ) + 127u ) / 128u ) - 1u ) )	<< 16 )		\
	| ( ( MCVX_M_04BIT & ( ENDIAN ) )									<<  0 ) )	\

#define MCVX_BELZ_REG_VLC_EDT( PIC_CNT_MAX, CTB_CNT_MAX, SRST_CNT_MAX )	\
	( ( ( MCVX_M_16BIT & ( PIC_CNT_MAX ) )		<<  16 )				\
	| ( ( MCVX_M_08BIT & ( CTB_CNT_MAX ) )		<<   8 )				\
	| ( ( MCVX_M_08BIT & ( SRST_CNT_MAX ) )		<<   0 ) )				\


/*----------------------------------------------------------------------------------------------------------*/
/*							                                                                                */
/* CE REG					                                                                                */
/*							                                                                                */
/*----------------------------------------------------------------------------------------------------------*/	
#define MCVX_BELZ_REG_CE_PBAH( IRP_SIZE, ENDIAN )									\
	( ( ( MCVX_M_01BIT & 1u )											<< 24 )		\
	| ( ( MCVX_M_08BIT & ( ( ( ( IRP_SIZE ) + 127u ) / 128u ) - 1u ) )	<< 11 )		\
	| ( ( MCVX_M_03BIT & 1u )											<<  8 )		\
	| ( ( MCVX_M_02BIT & 3u )											<<  6 )		\
	| ( ( MCVX_M_04BIT & ( ENDIAN ) )									<<  0 ) )	\

#define MCVX_BELZ_REG_CE_EDT( PIC_CNT_MAX, CTB_CNT_MAX, SRST_CNT_MAX )	\
	( ( ( MCVX_M_16BIT & ( PIC_CNT_MAX ) )		<<  16 )				\
	| ( ( MCVX_M_08BIT & ( CTB_CNT_MAX ) )		<<   8 )				\
	| ( ( MCVX_M_08BIT & ( SRST_CNT_MAX ) )		<<   0 ) )				\

#define MCVX_BELZ_REG_CE_CTRL( MODE, LOG2OPCE, CSTSO )				\
	( ( ( MCVX_M_01BIT & ( MODE ) )				<<  31 )			\
	| ( ( MCVX_M_02BIT & ( LOG2OPCE ) )			<<   8 ) 			\
	| ( ( MCVX_M_01BIT & ( CSTSO ) )			<<   4 ) 			\
	| ( ( MCVX_M_03BIT & ( 0u ) )				<<   0 ) )			\

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* #ifndef MCVX_ARCH_BELZ_H */
