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

#ifndef	MCVX_API_H
#define	MCVX_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mcvx_types.h"
#include "mcvx_register.h"

/* stream_type */
#define	MCVX_MPEG4							( 0u )
#define	MCVX_H263							( 1u )
#define	MCVX_H264							( 2u )
#define MCVX_SRS							( 3u )
#define	MCVX_VC1							( 4u )
#define	MCVX_DIVX							( 5u )
#define	MCVX_MPEG2							( 6u )
#define	MCVX_MPEG1							( 7u )
#define MCVX_VP6							( 8u )
#define MCVX_AVS							( 9u )
#define MCVX_RLV							( 10u )
#define MCVX_VP8							( 12u )
#define MCVX_VP9							( 14u )
#define	MCVX_H265							( 15u )

/* IP arch */
#define MCVX_ARCH_VCP3						( 0u )	/* VCP3 (a.k.a VCPLF, VCPL4) */
#define MCVX_ARCH_BELZ						( 1u )	/* Belize */

/* FCPC arch */
#define	MCVX_FCPC_S							( 0u )
#define	MCVX_FCPC_I							( 1u )

/* FCPC shared mode (M3W only) */
#define	MCVX_FCPC_SM_DEFAULT				( 0u )
#define	MCVX_FCPC_SM_1						( 1u )

/* FCPC cache size */
#define	MCVX_FCPC_CACHE_64KB				( 0u )
#define	MCVX_FCPC_CACHE_32KB				( 1u )

/* FCPC compress mode */
#define	MCVX_FCPC_MODE_CMP					( 0u )
#define	MCVX_FCPC_MODE_RAW					( 1u )
#define	MCVX_FCPC_MODE_DBG					( 2u )	/* for debug */

/* common use */
#define MCVX_FALSE							( 0u )
#define MCVX_TRUE							( 1u )

/* not applicable */
#define	MCVX_NA								( 0u )	

/* rtn_code */
#define MCVX_NML_END						(  0 )
#define MCVX_ERR_PARAM						( -1 )
#define MCVX_ERR_WORK						( -2 )
#define MCVX_ERR_STATE						( -3 )
#define MCVX_ERR_CONTEXT					( -4 )
#define MCVX_ERR_SEVERE						( -256 )

/* CE FSM */
#define MCVX_STATE_CE_IDLE					( 0u )
#define MCVX_STATE_CE_RUNNING				( 1u )

/* VLC FSM */
#define MCVX_STATE_VLC_IDLE					( 16u )
#define MCVX_STATE_VLC_RUNNING				( 17u )

/* FCV FSM */
#define MCVX_STATE_FCV_IDLE					( 32u )
#define MCVX_STATE_FCV_RUNNING				( 33u )

/* EVENT TYPES */
#define MCVX_EVENT_CE_START					( 0x00u )
#define MCVX_EVENT_REQ_CE_STOP				( 0x01u )

#define MCVX_EVENT_VLC_START				( 0x10u )
#define MCVX_EVENT_REQ_VLC_STOP				( 0x11u )

#define MCVX_EVENT_FCV_START				( 0x20u )
#define MCVX_EVENT_REQ_FCV_STOP				( 0x21u )

#define MCVX_CE								( 0u )
#define MCVX_VLC							( 1u )
#define MCVX_FCV							( 2u )

/* reset mode */
#define MCVX_RESET_NML						( 0u )
#define MCVX_RESET_FORCED					( 1u )

/* cmn_pic_type */
#define	MCVX_I_PIC							( 0u )	/* including IDR */
#define	MCVX_P_PIC							( 1u )
#define	MCVX_B_PIC							( 2u )
#define	MCVX_D_PIC							( 4u )

/* cmn_pic_struct */
#define MCVX_PS_TOP							( 0u )
#define MCVX_PS_BOT							( 1u )
#define MCVX_PS_FRAME						( 2u )

/* alloc_mode */
#define MCVX_ALLOC_FRAME					( 0u )
#define MCVX_ALLOC_FIELD					( 1u )

/* cmn_chroma_format */
#define	MCVX_CF_400							( 0u )
#define	MCVX_CF_420							( 1u )
#define	MCVX_CF_422							( 2u )
#define	MCVX_CF_444							( 3u )

#define	MCVX_MAX_NUM_REF_PIC				( 32u )
#define	MCVX_MAX_NUM_REF_PIC_HEVC			(  8u )

/* yuv_format */
#define MCVX_2PL_Y_CBCR						( 0u )
#define MCVX_2PL_Y_CRCB						( 1u )
#define MCVX_3PL							( 2u )

/* addressing mode */
#define	MCVX_AM_LINEAR						( 0u )
#define	MCVX_AM_TILE						( 2u )	/* reserved for possible future extensions */

/* dec_out_mode */
#define MCVX_DEC_OUT_MODE_TILE				( 0u )

/* dpar[] */
#define MCVX_DPAR_IDX_CAXI2_PORT_DISABLE	( 0u )
#define MCVX_DPAR_IDX_IMR_PORT_DISABLE		( 1u )
#define MCVX_DPAR_IDX_IMSB_ENABLE			( 2u )
#define MCVX_DPAR_IDX_AXI_OUTPUT_DISABLE	( 3u )
#define MCVX_DPAR_IDX_STB_INPUT_ENABLE		( 4u )
#define MCVX_DPAR_IDX_USE_REFBUF			( 5u )
#define MCVX_DPAR_IDX_WCMD_SEP				( 6u )
#define MCVX_DPAR_IDX_FCPC_CACHE_SIZE		( 10u )
#define MCVX_DPAR_IDX_FCPC_MODE				( 11u )

#define MCVX_DPAR_IDX_VLC_EDT_ENABLE		( 16u )
#define MCVX_DPAR_IDX_CE_EDT_ENABLE			( 17u )
#define MCVX_DPAR_IDX_USER_ID				( 24u ) 
#define MCVX_DPAR_IDX_NON_REF_LDEC_ENABLE	( 25u )
#define MCVX_DPAR_IDX_IMD_SIZE_RATIO		( 26u )

#define MCVX_DPAR_NOEL						( 32u )

/* VLC error_block */
#define MCVX_VLC_ERR_BLOCK_NONE				( 0u )
#define MCVX_VLC_ERR_BLOCK_VLCS				( 1u )
#define MCVX_VLC_ERR_BLOCK_STX				( 2u )
#define MCVX_VLC_ERR_BLOCK_MVD				( 3u )

/* VLC error_level */
#define MCVX_VLC_ERR_LEVEL_NONE				( 0u )
#define MCVX_VLC_ERR_LEVEL_WARNING			( 1u )
#define MCVX_VLC_ERR_LEVEL_ERROR			( 2u )
#define MCVX_VLC_ERR_LEVEL_FATAL			( 3u )
#define MCVX_VLC_ERR_LEVEL_HUNGUP			( 0xffffffffu )

/* VLC error_code */
#define MCVX_VLC_ERR_CODE_NONE				( 0u )

/* CE error_code */
#define MCVX_CE_ERR_CODE_NONE				( 0u )
#define MCVX_CE_ERR_CODE_ES_ROUND			( 0x20u )
#define MCVX_CE_ERR_CODE_HUNGUP				( 0xffffffffu )

/* number of elements */
#define MCVX_IMS_NOEL						( 2u )
#define MCVX_ES_NOEL						( 2u )
#define MCVX_DEC_MVR_NOEL					( 16u )
#define MCVX_ENC_CP_NOEL					( 2u )
#define MCVX_ENC_DP_COEF_NOEL				( 2u )
#define MCVX_SPB_NOEL						( 32u )

#define MCVX_U32_MAX						( 0xFFFFFFFFu )
#define MCVX_S32_MAX						( (MCVX_S32)0x7FFFFFFF )
#define MCVX_S32_MIN						( (MCVX_S32)0x80000000 )

/*===== call back functions =====*/
typedef MCVX_U32 (*MCVX_UDF_V_TO_P_T) (
	void			*udp,  
	void			*v_addr );

typedef void (*MCVX_UDF_SHOW_BAA_T) (
	void			*udp,
	MCVX_U32		module,
	void			*baa );

typedef void (*MCVX_UDF_VLC_EVENT_T) (	/* PRQA S 3448 *//* We confirmed declaration of typedef is correct */	
	void			*udp,  
	MCVX_U32		vlc_event,
	void			*vlc_event_data );

typedef void (*MCVX_UDF_CE_EVENT_T) (	/* PRQA S 3448 *//* We confirmed declaration of typedef is correct */
	void			*udp,
	MCVX_U32		ce_event,
	void			*ce_event_data );

typedef void (*MCVX_UDF_FCV_EVENT_T) (	/* PRQA S 3448 *//* We confirmed declaration of typedef is correct */
	void			*udp,
	MCVX_U32		fcv_event,
	void			*fcv_event_data );

typedef void (*MCVX_UDF_REG_READ_T)	(	/* PRQA S 3448 *//* We confirmed declaration of typedef is correct */
	MCVX_REG			*reg_addr, 
	MCVX_U32			*dst_addr, 
	MCVX_U32			num_reg );

typedef void (*MCVX_UDF_REG_WRITE_T) (	/* PRQA S 3448 *//* We confirmed declaration of typedef is correct */
	MCVX_REG			*reg_addr, 
	MCVX_U32			*src_addr, 
	MCVX_U32			num_reg );

/*===== structures =====*/
/**
 *  \struct MCVX_CONTEXT_T
 *  \brief  context
******************************************************/
typedef struct {
	MCVX_U32				context_status;
	void					*cmn_info;
	void					*codec_info;
	void					*codec_ext;
	void					*udp;
} MCVX_CONTEXT_T;

/**
 *  \struct MCVX_BUFF_PICTURE_T
 *  \brief  common picture address set
******************************************************/
typedef struct {
	MCVX_U32				Ypic_addr;
	MCVX_U32				Cpic_addr;
	MCVX_U32				Yanc_addr;
	MCVX_U32				Canc_addr;
} MCVX_BUFF_PICTURE_T;

/**
 *  \struct MCVX_BUFF_CAPTURE_T
 *  \brief  encode picture address set
******************************************************/
typedef struct {
	MCVX_U32				Ypic_addr;
	MCVX_U32				Cpic_addr[ MCVX_ENC_CP_NOEL ];
} MCVX_BUFF_CAPTURE_T;

/**
 *  @struct MCVX_FMEM_IS_REF_T
 *  @brief  set when fmem is reference
******************************************************/
typedef struct {
	MCVX_U32	top;
	MCVX_U32	bot;
} MCVX_FMEM_IS_REF_T;

/**
 *  \struct MCVX_CE_BAA_T
 *  \brief  CE base address array structure
******************************************************/
typedef struct {
	MCVX_U32				*irp_v_addr;
#if defined(__arm__)
    MCVX_U32                *lib32_padding1;
#endif
	MCVX_U32				irp_p_addr;

	MCVX_U32				imc_buff_addr;
	MCVX_UBYTES				imc_buff_size;
	MCVX_U32				its_buff_addr;
	MCVX_UBYTES				its_buff_size;
	MCVX_U32				ims_buff_addr[ MCVX_IMS_NOEL ];
	MCVX_UBYTES				ims_buff_size[ MCVX_IMS_NOEL ];

	MCVX_U32				lm_ce_mbi_addr;
	MCVX_U32				lm_ce_prd_addr;
	MCVX_U32				lm_ce_ovt_addr;
	MCVX_U32				lm_ce_deb_addr;

	MCVX_BUFF_PICTURE_T		img_dec_top;
	MCVX_BUFF_PICTURE_T		img_dec_bot;

	/* for deocoder */
	MCVX_BUFF_PICTURE_T		img_flt_top;
	MCVX_BUFF_PICTURE_T		img_flt_bot;

	/* for encoder */
	MCVX_BUFF_CAPTURE_T		img_capt;

	MCVX_U32				img_ref_num;
	MCVX_BUFF_PICTURE_T		img_ref[ MCVX_MAX_NUM_REF_PIC ];

	MCVX_U32				stride_dec;
	MCVX_U32				stride_ref;
	MCVX_U32				stride_flt;
	MCVX_U32				stride_enc_y;
	MCVX_U32				stride_enc_c;

	/* for encoder */
	MCVX_U32				enc_mv_w_addr;
	MCVX_U32				enc_mv_r_addr;
} MCVX_CE_BAA_T;

/**
 *  \struct MCVX_VLC_BAA_T
 *  \brief  VLC base address array structure
******************************************************/
typedef struct {
	MCVX_U32				*irp_v_addr;
#if defined(__arm__)
    MCVX_U32                *lib32_padding1;
#endif
	MCVX_U32				irp_p_addr;
#if defined(__arm__)
    MCVX_U32                *lib32_padding2;
#endif
	MCVX_U32				*list_item_v_addr;
#if defined(__arm__)
    MCVX_U32                *lib32_padding3;
#endif
	MCVX_U32				list_item_p_addr;

	MCVX_U32				imc_buff_addr;
	MCVX_UBYTES				imc_buff_size;
	MCVX_U32				ims_buff_addr[ MCVX_IMS_NOEL ];
	MCVX_UBYTES				ims_buff_size[ MCVX_IMS_NOEL ];

	MCVX_U32				its_buff_addr;

	MCVX_U32				lm_vlc_mbi_addr;

	MCVX_U32				str_es_addr[ MCVX_ES_NOEL ];
	MCVX_UBYTES				str_es_size[ MCVX_ES_NOEL ];
	MCVX_U32				str_es_bit_offset;

	/* for dec */
	MCVX_U32				dec_dp_addr;
	MCVX_U32				dec_bp_addr;
	MCVX_U32				dec_prob_r_addr;
	MCVX_U32				dec_prob_w_addr;
	MCVX_U32				dec_segm_r_addr;
	MCVX_U32				dec_segm_w_addr;
	MCVX_U32				dec_info_addr;
	MCVX_U32				dec_mai_addr;
	MCVX_U32				dec_mv_w_addr;
	MCVX_U32				dec_mv_r_addr[ MCVX_DEC_MVR_NOEL ];

	/* for enc */
	MCVX_U32				enc_dp_coef_addr[ MCVX_ENC_DP_COEF_NOEL ];
	MCVX_U32				enc_dp_coef_size[ MCVX_ENC_DP_COEF_NOEL ];
	MCVX_U32				enc_prob_base_addr;
	MCVX_U32				enc_prob_update_addr;
	MCVX_U32				enc_stat_addr;
} MCVX_VLC_BAA_T;

/**
 *  \struct MCVX_IP_CONFIG_T
 *  \brief  CODEC-IP configurations
******************************************************/
typedef struct {
	MCVX_U32				ip_id;
	MCVX_U32				ip_arch;
	void					*ip_vlc_base_addr;
	void					*ip_ce_base_addr;
	MCVX_UDF_REG_READ_T		udf_reg_read;
	MCVX_UDF_REG_WRITE_T	udf_reg_write;
} MCVX_IP_CONFIG_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_IP_CTX_T
 *  \brief  IP context
******************************************************/
typedef struct {
	MCVX_U32				ip_id;
	MCVX_U32				ip_arch;
	MCVX_UDF_REG_READ_T		udf_reg_read;
	MCVX_UDF_REG_WRITE_T	udf_reg_write;

	/* VCPLF/VCPL4 REG */
	MCVX_REG_VCP3_VLC_T		*REG_VCP3_VLC;
	MCVX_REG_VCP3_CE_T		*REG_VCP3_CE;

	/* Belize REG */
	MCVX_REG_BELZ_VLC_T		*REG_BELZ_VLC;
	MCVX_REG_BELZ_CE_T		*REG_BELZ_CE;

	/* VLC */
	MCVX_U32				hw_vlc_state;
	void					*vlc_handled_udp;
	MCVX_UDF_VLC_EVENT_T	udf_vlc_event;

	/* CE */
	MCVX_U32				hw_ce_state;
	void					*ce_handled_udp;
	MCVX_UDF_CE_EVENT_T		udf_ce_event;

	/* FCV */
	MCVX_U32				hw_fcv_state;
	void					*fcv_handled_udp;
	MCVX_UDF_FCV_EVENT_T	udf_fcv_event;

	MCVX_U32				vlc_hung_up;
	MCVX_U32				ce_hung_up;

	MCVX_U32				ip_version;
	MCVX_U32				ip_capability;
	MCVX_U32				vlc_stepping;
	MCVX_U32				ce_stepping;
} MCVX_IP_CTX_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_VLC_EXE_T
 *  \brief  VLC execution parameters
******************************************************/
typedef struct {
	MCVX_U32				ip_arch;

	MCVX_REG				vp_vlc_cmd;
	MCVX_REG				vp_vlc_ctrl;
	MCVX_REG				vp_vlc_edt;
	MCVX_REG				vp_vlc_irq_enb;
	MCVX_REG				vp_vlc_clk_stop;

	MCVX_REG				vp_vlc_list_init;
	MCVX_REG				vp_vlc_list_en;
	MCVX_REG				vp_vlc_list_lden;

	MCVX_REG				vp_vlc_pbah;
	MCVX_REG				vp_vlc_pbal;

	MCVX_UBYTES				irp_size;
} MCVX_VLC_EXE_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_VLC_RES_T
 *  \brief  VLC result parameters
******************************************************/
typedef struct {
	MCVX_U32				error_block;
	MCVX_U32				error_level;
	MCVX_U32				error_code;
	MCVX_U32				error_pos_x;
	MCVX_U32				error_pos_y;

	MCVX_UBYTES				pic_bytes;
	MCVX_UBYTES				is_byte;
	MCVX_U32				codec_info;
	MCVX_U32				vlc_cycle;
} MCVX_VLC_RES_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_CE_EXE_T
 *  \brief  CE execution parameters
******************************************************/
typedef struct {
	MCVX_U32				ip_arch;

	MCVX_REG				vp_ce_cmd;
	MCVX_REG				vp_ce_ctrl;
	MCVX_REG				vp_ce_edt;
	MCVX_REG				vp_ce_irq_enb;
	MCVX_REG				vp_ce_clk_stop;

	MCVX_REG				vp_ce_list_init;
	MCVX_REG				vp_ce_list_en;

	MCVX_REG				vp_ce_pbah;
	MCVX_REG				vp_ce_pbal;

	MCVX_UBYTES				irp_size;
} MCVX_CE_EXE_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_CE_RES_T
 *  \brief  CE result parameters
******************************************************/
typedef struct {
	MCVX_U32				error_code;
	MCVX_U32				error_pos_y;
	MCVX_U32				error_pos_x;

	MCVX_U32				ref_baa_log;
	MCVX_U32				is_byte;
	MCVX_U32				intra_mbs;
	MCVX_U32				sum_of_min_sad256;
	MCVX_U32				sum_of_intra_sad256;
	MCVX_U32				sum_of_inter_sad256;
	MCVX_U32				sum_of_act;
	MCVX_U32				act_min;
	MCVX_U32				act_max;
	MCVX_U32				sum_of_qp;
	MCVX_U32				ce_cycle;
	MCVX_S32				bocj;
} MCVX_CE_RES_T;

/**
 *  \struct MCVX_CE_TLC_T
 *  \brief  CE TLC configurations
******************************************************/
typedef struct {
	MCVX_BUFF_PICTURE_T     ba;
	MCVX_U32                buff_addr;
	MCVX_U32                stride;
	MCVX_U32                slice_height;
} MCVX_CE_TLC_T;

/**
 *  \struct MCVX_FCV_EXE_T
 *  \brief  FCV execution parameters
******************************************************/
typedef struct {
	MCVX_U32				ip_arch;

	MCVX_REG				vp_ce_cmd;
	MCVX_REG				vp_ce_ctrl;
	MCVX_REG				vp_ce_irq_enb;
	MCVX_REG				vp_ce_edt;

	MCVX_REG				vp_ce_list_init;
	MCVX_REG				vp_ce_list_en;
	MCVX_REG				vp_ce_pbah;
	MCVX_REG				vp_ce_pbal;
} MCVX_FCV_EXE_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_FCV_RES_T
 *  \brief  FCV result parameters
******************************************************/
typedef struct {
	MCVX_U32				reserved;
} MCVX_FCV_RES_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_FCPC_CONFIG_T
 *  \brief  FCPC configurations
******************************************************/
typedef struct {
	void					*fcpc_base_addr;
	MCVX_U32				fcpc_arch;
	MCVX_U32				shared_mode;
	MCVX_UDF_REG_READ_T		udf_reg_read;
	MCVX_UDF_REG_WRITE_T	udf_reg_write;
} MCVX_FCPC_CONFIG_T; /* PRQA S 3448 *//* We confirmed declaration of typedef is correct */

/**
 *  \struct MCVX_FCPC_CTX_T
 *  \brief  FCPC context
******************************************************/
typedef struct {
	MCVX_U32				fcpc_arch;
	MCVX_U32				shared_mode;
	MCVX_REG_FCPC_T			*REG_FCPC;
	MCVX_UDF_REG_READ_T		udf_reg_read;
	MCVX_UDF_REG_WRITE_T	udf_reg_write;
	MCVX_U32				revision;
} MCVX_FCPC_CTX_T;

/*===== functions =====*/
/* API function */
MCVX_RC mcvx_ip_init(
	const MCVX_IP_CONFIG_T	*config,
	MCVX_IP_CTX_T			*ip);

MCVX_RC mcvx_get_ip_version(
	const MCVX_IP_CTX_T	*ip,
	MCVX_U32			*ip_version);

MCVX_RC mcvx_get_ip_capability(
	const MCVX_IP_CTX_T	*ip,
	MCVX_U32			*ip_capability);

void mcvx_vlc_interrupt_handler(
	MCVX_IP_CTX_T	*ip,
	MCVX_U32		sw_timeout);

void mcvx_ce_interrupt_handler(
	MCVX_IP_CTX_T	*ip,
	MCVX_U32		sw_timeout);

MCVX_RC mcvx_vlc_reset( 
	MCVX_IP_CTX_T	*ip,
	MCVX_U32		reset_mode);

MCVX_RC mcvx_ce_reset( 
	MCVX_IP_CTX_T	*ip,
	MCVX_U32		reset_mode);

MCVX_RC mcvx_vlc_start(
	MCVX_IP_CTX_T			*ip,
	void					*udp,
	MCVX_UDF_VLC_EVENT_T	udf_vlc_event,
	MCVX_VLC_EXE_T			*vlc_exe);

MCVX_RC mcvx_vlc_stop( 
	MCVX_IP_CTX_T	*ip,
	MCVX_VLC_RES_T	*vlc_res);

MCVX_RC mcvx_ce_start( 
	MCVX_IP_CTX_T			*ip,
	void					*udp,
	MCVX_UDF_CE_EVENT_T		udf_ce_event,
	MCVX_CE_EXE_T			*ce_exe);

MCVX_RC mcvx_ce_stop(
	MCVX_IP_CTX_T	*ip,
	MCVX_CE_RES_T	*ce_res);

/* FCPC */
MCVX_RC mcvx_fcpc_init(
	const MCVX_FCPC_CONFIG_T	*config,
	MCVX_FCPC_CTX_T				*fcpc);

MCVX_RC mcvx_fcpc_reset(
	MCVX_FCPC_CTX_T	*fcpc,
	MCVX_U32		reset_mode);

#ifdef __cplusplus
}
#endif

#endif /* MCVX_API_H */
