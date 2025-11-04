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

#ifndef	MCVX_INNER_H
#define	MCVX_INNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mcvx_api.h"
#include "mcvx_register.h"

#if defined (MCVX_USE_MY_64BIT_PTR)
  typedef unsigned long long	MCVX_UINTPTR;	/* 64bit pointer */
#elif defined (MCVX_USE_MY_32BIT_PTR)
  typedef unsigned int 		MCVX_UINTPTR;		/* 32bit pointer */
#else
 #ifdef _MSC_FULL_VER	/* VC environment */
   #pragma warning( disable : 4001 )			/* disable // comment warnings  */
   #pragma warning( disable : 4127 )			/* disable do{}while(0) warnings */
 #endif
  #include <stdint.h>
  typedef uintptr_t			MCVX_UINTPTR;		/* compiler definition */
#endif

#if defined(MID_DEBUG) && !defined(__CMN_PRINTF_H__)
#define APP_LOG_L_ASSERT	( 0x00000001u )
void app_fprintf( MCVX_U32 log_level, const MCVX_CHAR *fmt, ... );
#ifndef APP_ASSERT
void app_assert( MCVX_U32 log_level, const MCVX_CHAR *file, const MCVX_S32 line, const MCVX_CHAR *cond );
#define APP_ASSERT(cond) if( !(cond)){ app_assert( APP_LOG_L_ASSERT, __FILE__, __LINE__, #cond );}
#define APP_FAIL(){ app_assert( APP_LOG_L_ASSERT, __FILE__, __LINE__, "failed" ); }
#endif
#endif

#define MCVX_CTX_VALID								( 0xA7EC0DECu )	/* Hexspeak "ate CODEC" */
#define MCVX_CTX_INVALID							( 0xBADC0DECu )	/* Hexspeak "bad CODEC" */

/* known ip identifier */
#define	MCVX_IP_VCP3C								( 0x60010000u )
#define	MCVX_IP_VCP3M								( 0x60020000u )
#define	MCVX_IP_ROTORUA								( 0x60200000u )
#define	MCVX_IP_BELZ_REV0							( 0x72000000u )
#define	MCVX_IP_BELZ_REV_C1							( 0x70200000u )

/* common use macro */
#define MCVX_DMAC_UNIT								( 256u )
#define MCVX_ALIGN_PO2( X, PO2 )					((( ((X) + (PO2)) - 1u) & ~((PO2) - 1u)))	/* power of 2 */
#define MCVX_DIV_ROUND_UP(V, D)						( ( (V) + ( (D) - 1u ) ) / (D) )
#define MCVX_ROUND_UP(V, D)							(( ( (V) + ( (D) - 1u ) ) / (D) ) * (D))
#define MCVX_DIV_ROUND_DOWN(V, D)					( ( (V) + ( (D) / 2u ) ) / (D) )
#define MCVX_CLIP3(V, MIN, MAX)						(( (V) < (MIN) )? (MIN) : (( (V) > (MAX) ) ? (MAX) : (V) ))
#define MCVX_MAX(A, B)								( ( (A) > (B) )? (A) : (B) )
#define MCVX_MIN(A, B)								( ( (A) < (B) )? (A) : (B) )
#define MCVX_IMD_OCP(R, B)							( ( (R) * 100u ) / (B) )	/* up to 100% */
#define MCVX_MOD_COUNT_UP(V, M)						( ( (V) + 1u ) % (M) )
#define MCVX_NUM_MB(P)								(( (P) + 15u ) >> 4 )
#define MCVX_NUM_CTB(P)								(( (P) + 31u ) >> 5 )
#define MCVX_BIT_TO_BYTE(BIT)						(( (BIT) + 7u ) >> 3 )
#define MCVX_BYTE_TO_BIT(BYTE)						(( (BYTE) * 8u ) )
#define MCVX_LOG2_TO_UINT( X )						( 1u << (X) )
#define MCVX_ROUND_UP_DMAC_UNIT( X ) 				( MCVX_DMAC_UNIT * ( ( (X) + ( MCVX_DMAC_UNIT - 1 ) ) / MCVX_DMAC_UNIT ) )
#define MCVX_ROUND_DOWN_DMAC_UNIT( X )				( MCVX_DMAC_UNIT * ( (X) / MCVX_DMAC_UNIT ) )
/* round up for IMS/IMC buffer  */
#define MCVX_ROUND_UP_IMD( X )						( MCVX_DMAC_UNIT * ( ( (X) + MCVX_DMAC_UNIT ) / MCVX_DMAC_UNIT ) )

/* swap */
#ifndef _BIG
/* Little endian */
#define MCVX_LONG_SWAP(X)	( (((X)) >> 24) | (((X) & 0x00ff0000u) >> 8) | (((X) & 0x0000ff00u) << 8) | (((X)) << 24) )
#else			
/* Big endian */
#define MCVX_LONG_SWAP(X)	( X )
#endif /* #ifndef _BIG */

/* for parameter check */
#define MCVX_IS_NOT_BOOL( X ) 						( (X) > MCVX_TRUE )
#define MCVX_IS_NOT_IN_RANGE( X, MIN, MAX )			( ( (X) < (MIN) ) || ( (X) > (MAX) ) )
#define MCVX_IS_OVER_MAX( X, MAX ) 					( (X) > (MAX) )
#define MCVX_IS_UNDER_MIN( X, MIN )					( (X) < (MIN) )
#define MCVX_IS_NEITHER_NOR( X, A, B ) 				( ( (X) != (A) ) && ( (X) != (B) ) )

/* get_version */
#define MCVX_MAKE_VERSION( MA, MI, BL )				( ( (MA) << 16 ) | ( (MI) << 8 ) | ( (BL) << 0 ) )

/* codec mode */
#define	MCVX_ENC									( 0u )
#define	MCVX_DEC									( 1u )

/* number of CE */
#define MCVX_MAX_NUM_CE								( 8u )
#define MCVX_1_CE									( 1u )
#define MCVX_2_CE									( 2u )

/* line memory size */
#define MCVX_LM_VLC_MBI_SIZE						(  24 * 1024u )		/*  VCP3 :  20Kbyte : actual 16896byte : 256MB(4096pixel) *  64byte + 512byte */
																		/*  BELZ :  24Kbyte : actual 22528byte : 320x(4096pixel/64) + 2048byte */		
#define MCVX_LM_VLC_TOTAL_SIZE						( MCVX_LM_VLC_MBI_SIZE )

#define MCVX_LM_CE_MBI_SIZE							(  36 * 1024u )		/*  VCP3 :  36Kbyte : actual  33280byte : 256MB(4096pixel) * 128byte + 512byte */
#define MCVX_LM_CE_PRD_SIZE							(  16 * 1024u )		/*  VCP3 :  16Kbyte : actual  16384byte : 256MB(4096pixel) *  64byte */
#define MCVX_LM_CE_OVT_SIZE							(  32 * 1024u )		/*  VCP3 :  32Kbyte : actual  32768byte : 256MB(4096pixel) * 128byte */
#define MCVX_LM_CE_DEB_SIZE							( 128 * 1024u )		/*  VCP3 : 128Kbyte : actual 131072byte : 256MB(4096pixel) * 512byte */
																		/*  BELZ :  none for up to 4096pixel */
#define MCVX_LM_CE_TOTAL_SIZE						( MCVX_LM_CE_MBI_SIZE + MCVX_LM_CE_PRD_SIZE + MCVX_LM_CE_OVT_SIZE + MCVX_LM_CE_DEB_SIZE )

/* irp parameter size for dec/enc */
//#define MCVX_IRP_VLC_SIZE							( 8192u )
//#define MCVX_IRP_VLC_LLI_SIZE						(  256u )
#define MCVX_IRP_VLC_SIZE							( 4096u )
#define MCVX_IRP_VLC_LLI_SIZE						(    0u )
#define MCVX_IRP_VLC_TOTAL_SIZE						( MCVX_IRP_VLC_SIZE + MCVX_IRP_VLC_LLI_SIZE )

#define MCVX_IRP_CE_SIZE							( 8192u )
//#define MCVX_IRP_CE_LLI_SIZE						(    0u )
//#define MCVX_IRP_CE_SIZE							( 16384u * 2u )
#define MCVX_IRP_CE_LLI_SIZE						(    0u )
#define MCVX_IRP_CE_TOTAL_SIZE						( MCVX_IRP_CE_SIZE + MCVX_IRP_CE_LLI_SIZE )

/* to be added hw_work_size */
#define MCVX_LM_IRP_TOTAL_SIZE						( MCVX_LM_VLC_TOTAL_SIZE + MCVX_LM_CE_TOTAL_SIZE + MCVX_IRP_VLC_TOTAL_SIZE + MCVX_IRP_CE_TOTAL_SIZE )

#define MCVX_DEC_IMAGE_STRIDE_ALIGN( STRIDE )		( MCVX_MAX( ( (STRIDE)*32u ) , 16384u ) )

#define MCVX_ENC_IMAGE_STRIDE_ALIGN					(   16u )		/* capture (enc) */
#define MCVX_ENC_IMAGE_STRIDE_MAX					( 4096u )		/* capture (enc) */

#define MCVX_ANC_BUFF_ALIGN							(  128u )

#define MCVX_IMD_BUFF_ALIGN							(  256u )
#define MCVX_MV_BUFF_ALIGN							(  256u )
#define MCVX_SDW_BUFF_ALIGN							(  256u )

/* range of imd_ratio (MCVX_DPAR_IDX_IMD_SIZE_RATIO) */
#define MCVX_IMD_MIN_RATIO							(   20u )
#define MCVX_IMD_MAX_RATIO							(  200u )

/* list item limits (es, ims) */
/* VCP3
#define MCVX_LIST_BUFF_MAX_SIZE						(  0x00fff000u )
#define MCVX_LIST_BUFF_ALIGN						(  256u )
*/
/* Belize */
#define MCVX_LIST_BUFF_MAX_SIZE						(  0x00ffff80u )
#define MCVX_LIST_BUFF_ALIGN						(  128u )

/* ec_mode */
#define	MCVX_EC_MODE_INTER							( 0u )
#define	MCVX_EC_MODE_INTRA							( 1u )

/* ec_rcv */
#define	MCVX_EC_RCV_NML								( 0u )
#define	MCVX_EC_RCV_NONE							( 1u )

/* error conceal pixel value */
#define MCVX_EC_FILL_Y								( 128u )
#define MCVX_EC_FILL_U								( 128u )
#define MCVX_EC_FILL_V								( 128u )

#define	MCVX_MAT_BELZ_INTRA_4X4_Y		(0u)
#define	MCVX_MAT_BELZ_INTRA_4X4_CB		(1u)
#define	MCVX_MAT_BELZ_INTRA_4X4_CR		(2u)
#define	MCVX_MAT_BELZ_INTER_4X4_Y		(3u)
#define	MCVX_MAT_BELZ_INTER_4X4_CB		(4u)
#define	MCVX_MAT_BELZ_INTER_4X4_CR		(5u)
#define MCVX_MAT_BELZ_4X4_ID_NOEL		(6u)	/* intra_4x4_y, intra_4x4_cb, intra_4x4_cr, inter_4x4_y, inter_4x4_cb, inter_4x4_cr */

#define	MCVX_MAT_BELZ_INTRA_8X8_Y		(0u)
#define	MCVX_MAT_BELZ_INTRA_8X8_CB		(1u)
#define	MCVX_MAT_BELZ_INTRA_8X8_CR		(2u)
#define	MCVX_MAT_BELZ_INTER_8X8_Y		(3u)
#define	MCVX_MAT_BELZ_INTER_8X8_CB		(4u)
#define	MCVX_MAT_BELZ_INTER_8X8_CR		(5u)
#define MCVX_MAT_BELZ_8X8_ID_NOEL		(6u)	/* intra_8x8_y, intra_8x8_cb, intra_8x8_cr, inter_8x8_y, inter_8x8_cb, inter_8x8_cr */

#define	MCVX_MAT_BELZ_INTRA_8X8_Y_AVC	(0u)
#define	MCVX_MAT_BELZ_INTER_8X8_Y_AVC	(1u)
#define MCVX_MAT_BELZ_8X8_ID_NOEL_AVC	(2u)	/* intra_8x8, inter_8x8 */

#define	MCVX_MAT_BELZ_INTRA_16X16_Y		(0u)
#define	MCVX_MAT_BELZ_INTRA_16X16_CB	(1u)
#define	MCVX_MAT_BELZ_INTRA_16X16_CR	(2u)
#define	MCVX_MAT_BELZ_INTER_16X16_Y		(3u)
#define	MCVX_MAT_BELZ_INTER_16X16_CB	(4u)
#define	MCVX_MAT_BELZ_INTER_16X16_CR	(5u)
#define MCVX_MAT_BELZ_16X16_ID_NOEL		(6u)	/* intra_16x16_y, intra_16x16_cb, intra_16x16_cr, inter_16x16_y, inter_16x16_cb, inter_16x16_cr */

#define	MCVX_MAT_BELZ_INTRA_32X32_Y		(0u)
#define	MCVX_MAT_BELZ_INTER_32X32_Y		(1u)
#define MCVX_MAT_BELZ_32X32_ID_NOEL		(2u)

/* for pic_width_m1, pic_height_m1 */
#define MCVX_PIC_WIDTH_M1_PIC_BOUND( X_PIC_SIZE )		(( X_PIC_SIZE ) - 1u )
#define MCVX_PIC_HEIGHT_M1_PIC_BOUND( Y_PIC_SIZE )		(( Y_PIC_SIZE ) - 1u )

#define MCVX_PIC_WIDTH_M1_MB_BOUND( X_PIC_SIZE )		( MCVX_ALIGN_PO2( ( X_PIC_SIZE ), 16u ) - 1u )
#define MCVX_PIC_HEIGHT_M1_MB_BOUND( Y_PIC_SIZE )		( MCVX_ALIGN_PO2( ( Y_PIC_SIZE ), 16u ) - 1u )

/* for blocl_pic_width_m1 (FCPC) */
#define MCVX_FCPC_BLOCK_PIC_WIDTH_M1( PIC_WIDTH_M1 )				( MCVX_ALIGN_PO2( ( ( PIC_WIDTH_M1 ) + 1u ),  16u    ) - 1u )
#define MCVX_FCPC_BLOCK_PIC_WIDTH_M1_CTB_BOUND( PIC_WIDTH_M1, CTB )	( MCVX_ALIGN_PO2( ( ( PIC_WIDTH_M1 ) + 1u ), ( CTB ) ) - 1u )

#define MCVX_NUM_ICS_16_UNIT( X_PIC_SIZE, Y_PIC_SIZE )	( ( ( ( X_PIC_SIZE ) + 15u ) >> 4 ) * ( ( ( Y_PIC_SIZE ) + 63u ) >> 4 ) )
#define MCVX_NUM_ICS_64_UNIT( X_PIC_SIZE, Y_PIC_SIZE )	( ( ( ( X_PIC_SIZE ) + 63u ) >> 6 ) * ( ( ( Y_PIC_SIZE ) + 63u ) >> 6 ) )

/* mv size / MB (Legacy Dec) */
#define MCVX_MV_SIZE_MB_DEC											(64u)
#define MCVX_MV_SIZE_PER_FRAME_LEGACY_DEC( X_PIC_SIZE, Y_PIC_SIZE )	( MCVX_ALIGN_PO2( ( ( MCVX_NUM_ICS_16_UNIT( X_PIC_SIZE, Y_PIC_SIZE ) * MCVX_MV_SIZE_MB_DEC ) + 256u ), 256u ) )
#define MCVX_MV_SIZE_PER_FRAME_DEC( FACTOR, X_PIC_SIZE, Y_PIC_SIZE )	( ( FACTOR * 256 * X_PIC_SIZE * Y_PIC_SIZE ) + 2048 )
#define MCVX_MV_SIZE_PER_FRAME_ENC( FACTOR, X_PIC_SIZE, Y_PIC_SIZE )    ( ( FACTOR * 256 * X_PIC_SIZE * Y_PIC_SIZE ) + 2048 )

/* mv size / CU (HEVC Dec) */
#define MCVX_MV_SIZE_CU_DEC											(256u * 4u)
#define MCVX_MV_SIZE_PER_FRAME_HEVC_DEC( X_PIC_SIZE, Y_PIC_SIZE )	( ( MCVX_NUM_ICS_64_UNIT( X_PIC_SIZE, Y_PIC_SIZE ) * MCVX_MV_SIZE_CU_DEC ) + 2048u )

/* mv size / CU (AVC Dec) */
#define MCVX_MV_SIZE_CU_AVC_DEC										(256u * 8u)
#define MCVX_MV_SIZE_PER_FRAME_AVC_DEC( X_PIC_SIZE, Y_PIC_SIZE )	( ( MCVX_NUM_ICS_64_UNIT( X_PIC_SIZE, Y_PIC_SIZE ) * MCVX_MV_SIZE_CU_AVC_DEC ) + 2048u )

/* mv size / MB (Legacy Enc) */
#define MCVX_MV_SIZE_MB_ENC											(16u)
#define MCVX_MV_SIZE_PER_FRAME_LEGACY_ENC( X_PIC_SIZE, Y_PIC_SIZE )	( MCVX_ALIGN_PO2( ( ( MCVX_NUM_ICS_16_UNIT( X_PIC_SIZE, Y_PIC_SIZE ) * MCVX_MV_SIZE_MB_ENC ) + 256u ), 256u ) )

/* seg size (VP9 ) */
#define MCVX_SEG_SIZE_MB_DEC										(64u)
#define MCVX_SEG_SIZE_PER_FRAME_DEC( X_PIC_SIZE, Y_PIC_SIZE )		( MCVX_ALIGN_PO2( ( ( MCVX_NUM_ICS_64_UNIT( X_PIC_SIZE, Y_PIC_SIZE ) * MCVX_SEG_SIZE_MB_DEC ) + 2048u ), 256u ) )

/* dp size / MB (MPEG-4/DivX) */
#define MCVX_DP_SIZE_MB_DEC											(16u)
#define MCVX_DP_SIZE_PER_STREAM_DEC( X_PIC_SIZE, Y_PIC_SIZE )		( ( MCVX_ALIGN_PO2( ( ( MCVX_NUM_ICS_16_UNIT( X_PIC_SIZE, Y_PIC_SIZE ) * MCVX_DP_SIZE_MB_DEC ) + 256u ), 256u ) ) * 2u ) /* 2: for R/W */
#define MCVX_DP_2ND_PART_OFFSET( DP_SIZE )							( (DP_SIZE) >> 1 )

/* IQMAT */
#define MCVX_MAKE_IQMAT_WORD( C0, C1, C2, C3 )			( ( (C0) << 24 ) | ( (C1) << 16 ) | ( (C2) << 8 ) | ( (C3) << 0 ) )

/* FCPC ANC/REF max entry */
#define MCVX_FCPC_ANC_REF_NOEL										(64u)

/* IRP cmn macro */
#define MCVX_IR_W0			p_buff[ tail + 0u ]
#define MCVX_IR_W1			p_buff[ tail + 1u ]
#define MCVX_IR_W2			p_buff[ tail + 2u ]
#define MCVX_IR_W3			p_buff[ tail + 3u ]
#define MCVX_IR_SIZEOF_4W	( 4u )

/**
 *  \struct MCVX_DEVICE_T
 *  \brief  device depends configurations
******************************************************/
typedef struct {
	MCVX_U32		dpar[ MCVX_DPAR_NOEL ];
} MCVX_DEVICE_T;

/**
 *  \struct MCVX_CMN_PAR_T
 *  \brief  basic decode/encode parameter
******************************************************/
typedef struct {
	MCVX_U32		tmp_placement;				/*!< memory allocation mode <br> MCVX_TRUE : field allocation <br> MCVX_FALSE : frame allocation <br> see HW manual to check more constraints */
	MCVX_U32		ref_placement;				/*!< memory allocation mode <br> MCVX_TRUE : field allocation <br> MCVX_FALSE : frame allocation <br> see HW manual to check more constraints */
	MCVX_U32		mode;						/*!< encode or decode <br> MCVX_DEC : decode <br> MCVX_ENC : encode */
	MCVX_U32		hw_stream_type;				/*!< stream_type <br> MCVX_H265 : H.265/HEVC <br> MCVX_H264 : H.264/AVC */
	MCVX_U32		padding_pic_edge;
	MCVX_U32		dec_out_disable;			/*!< disable YUV out <br> MCVX_TRUE : disable output <br> MCVX_FALSE : enable output <br> basically, this for encoder's non-ref pics */
	MCVX_U32		mv_info_out;				/*!< MV/Col-info output flag <br> MCVX_TRUE : enable Col-info output <br> MCVX_FALSE : disable Col-info output  */
	MCVX_U32		mv_info_in;					/*!< MV/Col-info input flag <br> MCVX_TRUE : enable Col-info input <br> MCVX_FALSE : disable Col-info input  */

	MCVX_U32		pic_width_m1;				/*!< picture width  - 1 <br>see HW manual to check CODEC depends definition */
	MCVX_U32		pic_height_m1;				/*!< picture height - 1 <br>see HW manual to check CODEC depends definition */
	MCVX_U32		interlace;					/*!< interlace flag  <br> MCVX_TRUE : interlace <br> MCVX_FALSE : progressive  */
	MCVX_U32		field_pic_flag;				/*!< field picture flag  <br> MCVX_TRUE : field picture <br> MCVX_FALSE : frame picture */
	MCVX_U32		bottom_field_flag;			/*!< bottom field flag  <br> MCVX_TRUE : bottom field <br> MCVX_FALSE : not bottom field */
	MCVX_U32		top_field_first;			/*!< top field first flag  <br> MCVX_TRUE : top field 1st <br> MCVX_FALSE : bottom field 1st */
} MCVX_CMN_PAR_T;

/**
 *  \struct MCVX_ENDIAN_T
 *  \brief  basic decode/encode parameter
******************************************************/
typedef struct {
	MCVX_U32		endian_irp;
	MCVX_U32		endian_cpu;
	MCVX_U32		endian_stream;
	MCVX_U32		endian_yuv_y;
	MCVX_U32		endian_yuv_c;
	MCVX_U32		endian_imd;
} MCVX_ENDIAN_T;

/**
 *  \struct MCVX_BELZ_IQMAT_T
 *  \brief  IQ-matrix(8bit) for Belize Decoder/Encoder
******************************************************/
typedef struct {
	MCVX_U8			*mat_4x4[ MCVX_MAT_BELZ_4X4_ID_NOEL ];		/*!< 4x4 Q-matrix <br> range : 1-255 */
	MCVX_U8			*mat_8x8[ MCVX_MAT_BELZ_8X8_ID_NOEL ];		/*!< 8x8 Q-matrix <br> range : 1-255 */
	MCVX_U8			*mat_16x16[ MCVX_MAT_BELZ_16X16_ID_NOEL ];	/*!< 16x16 Q-matrix <br> range : 1-255 */
	MCVX_U8			*mat_32x32[ MCVX_MAT_BELZ_32X32_ID_NOEL ];	/*!< 32x32 Q-matrix <br> range : 1-255 */
	MCVX_U8			*mat_16x16_DC;								/*!< 16x16 DC Q-matrix <br> range : -7 to 247 */
	MCVX_U8			*mat_32x32_DC;								/*!< 32x32 DC Q-matrix <br> range : -7 to 247 */
} MCVX_BELZ_IQMAT_T;

/**
 *  \struct MCVX_VLC_SW_T
 *  \brief  VP_VLC_SWITCH for Decoder
******************************************************/
typedef struct {
	MCVX_U32		imsb_mode;
	MCVX_U32		ec_mode;
	MCVX_U32		ec_rcv;
	MCVX_U32		ec_allpic;
	MCVX_U32		line_info_out;	/* belize only (vcp3 always use line memory at dram) */
	MCVX_U32		nal_switch;		/* vcp3 : as bit 26 of VPLC_SWITCH, belize :bit 0-31 of VP_VLC_SWITCH */
	MCVX_U32		seg_info_in;	/* belize: as bit 36 of VLC_SWITCH, MCVX_TRUE : enable SEG-info input, MCVX_FALSE : disable SEG-info input  */
	MCVX_U32		seg_info_out;	/* belize: as bit 35 of VLC_SWITCH, MCVX_TRUE : enable SEG-info output, MCVX_FALSE : disable SEG-info output  */
	MCVX_U32		col_info_in;	/* belize: as bit 34 of VLC_SWITCH, MCVX_TRUE : enable input. MCVX_FALSE : disable input */
	MCVX_U32		col_info_out;	/* belize: as bit 33 of VLC_SWITCH, MCVX_TRUE : enable output. MCVX_FALSE : disable output */
} MCVX_VLC_SW_T;

/**
 *  \struct MCVX_CE_SW_T
 *  \brief  VP_CE_SWITCH for Decoder
******************************************************/
typedef struct {
	MCVX_U32		imsb_mode;
	MCVX_U32		ec_allpic;
	MCVX_U32		ec_mode;
	MCVX_U32		deb_passthru;
} MCVX_CE_SW_T;

/**
 *  \struct MCVX_CE_RLC_CTRL_T
 *  \brief  VP_CE_SWITCH for Decoder
******************************************************/
typedef struct {
	MCVX_U32		ref_pic_cache_disable;
	MCVX_U32		cl_addr_trans_mode;
	MCVX_U32		shift_mode;
} MCVX_CE_RLC_CTRL_T;

/**
 *  \struct MCVX_FCPC_PAR_T
 *  \brief  basic decode/encode parameter
******************************************************/
typedef struct {
	MCVX_U32		rlv_rpr;
	MCVX_U32		rlv_version;
	MCVX_U32		rlv_deblock_passthru;
	MCVX_U32		hv3_annex_j;
	MCVX_U32		vc1_ranger;
	MCVX_U32		avc_mbaff;
	MCVX_U32		hev_ctb_size;

	MCVX_U32		block_pic_width_m1;
	MCVX_U32		bpp_luma;
	MCVX_U32		bpp_chroma;
	MCVX_U32		chroma_format_idc;
	MCVX_U32		imr_tile;
	MCVX_U32		fcpc_mode;
	MCVX_U32		cache_entry_offset;
} MCVX_FCPC_PAR_T;

/*===== functions =====*/
MCVX_U32 mcvx_get_cl_addr_trans_mode( MCVX_U32 x_pic_size, MCVX_U32 ctb_size );

MCVX_U32 mcvx_get_cl_addr_trans_mode_tile( MCVX_U32 x_pic_size, MCVX_U32 ctb_size );

#ifdef __cplusplus
}
#endif

#endif /* MCVX_INNER_H */
