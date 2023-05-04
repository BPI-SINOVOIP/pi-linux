// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _VPP_VBUF_H_
#define _VPP_VBUF_H_

#define MAX_USER_DATA_TYPE	  10

#ifndef VPP_TZ_ENABLE
/* Note: VPP CA is in kernel and need to talk to kernel.
 * So enable this macro; otherwise correct VBUF info will not reach VPP TA
 */
#define VPP_TZ_ENABLE
#endif //VPP_TZ_ENABLE

typedef struct xvYCC_data_t {
	UINT16 m_format; /* 0: for black/red/green/blue vertice; 1: for red/green/blue range */
	UINT16 m_precision; /* see GBD Color Precision */
	UINT16 m_space; /* see GBD Color Space */
	UINT16 m_d0; /* black-Y or min red */
	UINT16 m_d1; /* black-Cb or max red */
	UINT16 m_d2; /* black-Cr or min green */
	UINT16 m_d3; /* red-Y or max green */
	UINT16 m_d4; /* red-Cb or min blue */
	UINT16 m_d5; /* red-Cr or max blue */
	UINT16 m_d6; /* green-Y or not used */
	UINT16 m_d7; /* green-Cb or not used */
	UINT16 m_d8; /* green -Cr or not used */
	UINT16 m_d9; /* blue-Y or not used */
	UINT16 m_d10; /* blue-Cb or not used */
	UINT16 m_d11; /* blue -Cr or not used */
} xvYCC_DATA;


typedef struct vpp_vbuf_dv_meta_t {
		/* pointer to  rpu_ext_config_fixpt_main_t  structure for composer config.
		 * The pointer shall be Virtual address for Clear World or physical for TZ
		 */
	UINT32 m_dv_com_cfg;
		/*pointer to dm_metadata_t structure for display manager config
		 * The pointer shall be Virtual address for Clear World or physical for TZ
		 */
	UINT32 m_dv_dm_seq;
} DV_META_INFO;

	/** Technicolor configure definition for m_thdr_present_mode other bits are reserved*/
#define AMP_THDR			(1 << 0)
#define AMP_THDR_METADATA   (1 << 1)

typedef struct vbuf_info_thdr_meta_t {
		/* pointer to tHDR metadatastructure. */
	UINT32  m_sl_hdr1_metadata;
} THDR_META_INFO;

/* Dolby Vision configure definition for m_dv_present_mode other bits are reserved*/

/* Bit mask for dolby vision configurature on Base layer present. it shall be on (1) when dolby vision present*/
#define AMP_DOLBY_VISION_BL  (1 << 0)
/* Bit mask for dolby vision configurature on Enhance layer present (1) or not (0) */
#define AMP_DOLBY_VISION_EL  (1 << 1)
/* Bit mask for dolby vision configurature on Meta data present. it shall be on (1) when dolby vision present */
#define AMP_DOLBY_VISION_RPU (1 << 2)
typedef struct vpp_vbuf_t {
	//INT32 m_strmID;			   // stream ID assigned for the buffer
	INT32   m_bufferID;			 // buffer Id in buffer pool
	UINT32  m_grp_alloc_flag;	   // is the discript in group alloc. bit 0: info grp alloc, bit 1: Descriptor grp alloc.
	UINT32 m_pbuf_start;		   // base address of buffer;
	UINT32  m_buf_size;			 // size in bytes;
	UINT32  m_allocate_type;		// 0, unallocated, 1, grp_alloc, 2: self-alloc
	UINT32  m_buf_type;			 // 0, unknown 1: disp buffer, 2 reference
	UINT32  m_buf_use_state;		// 0, free, 1, decoding, 2, ready_disp,
									//  3, in_display, 4, ready to use(displayed)
	UINT32  m_flags;				// bit 0: 0, following pts invalid, 1, following pts valid.
									// bit 1: 0, no err,1, have error/no displayable.
									// bit 2, 0, no clk change,  1, clk change, pts is new clk
									//
	UINT32  m_pts_hi;			   // bit 33 of pts.
	UINT32  m_pts_lo;			   // bit 0-32 of pts
	// data format in frame buffer
	UINT32  m_srcfmt;			   // ARGB, YCBCR, .. etc.
	UINT32  m_order;				// ARGB, ABGR, BGRA, ... etc.
	UINT32  m_bytes_per_pixel;	  // number of bytes per pixel
	// active video window in reference resolution
	UINT	m_content_offset;	   // offset (in bytes) of picture full content in frame buffer
	UINT32  m_content_width;		// picture full content width
	UINT32  m_content_height;	   //picture full content height
	INT32   m_pan_left[4];		  // pan scan top_left position(in 1/16 pixels) of the content;
	INT32   m_pan_top[4];		   // pan scan top_left position(in 1/16 pixels) of the content;
	INT32   m_pan_width[4];		 // pan scan window(in pixels) in the content;
	INT32   m_pan_height[4];		// pan scan window(in pixels) in the content;


	INT32   m_active_left;		  // x-coordination (in pixels) of active window top left in reference window
	INT32   m_active_top;		   // y-coordination (in pixels) of active window top left in reference window
	UINT32  m_active_width;		 // with of active in pixels.
	UINT32  m_active_height;		// height of active data in pixels.
	INT32   m_disp_offset;		   //Offset (in bytes) of active data to be displayed
									// in reference active window in frame buffer
	UINT32  m_buf_stride;		   // line stride (in bytes) of frame buffer
	UINT32  m_is_frame_seq;		 // non zero if it is frame-only sequence
	UINT32  m_is_top_field_first;   // only apply to interlaced frame, 1: top field first out
	UINT32  m_is_repeat_first_field; // only apply to interlaced frame, 1: repeat first field.
	UINT32  m_is_progressive_pic;   // non zero if this frame is a progressive picture.
	UINT32  m_pixel_aspect_ratio;   // pixel aspect ratio(PAR) 0: unknown, High[31:16] 16bits,Numerator(width), Low[15:0] 16bits, Denominator(heigh)
	UINT32  m_frame_rate_num;	   // Numerator of frame rate if available
	UINT32  m_frame_rate_den;	   // Denominator if frame rate if available

	UINT32   m_luma_left_ofst;		  // MTR mode, luma left cropping offset
	UINT32   m_luma_top_ofst;		   // MTR mode, luma top cropping offset
	UINT32   m_chroma_left_ofst;			// MTR mode, chroma left cropping offset
	UINT32   m_chroma_top_ofst;		 //MTR mode, chroma top cropping offset

	UINT32 m_user_data_block[MAX_USER_DATA_TYPE];

	// m_clut_... for color look-up table
	UINT32 m_clut_ptr;		 // color lookup table for CI8 (valid when element m_bytes_per_pixel==1)
	//UINT32	m_clut_order;   // use m_order and m_srcfmt elements to decide the type of the clut
	UINT32  m_clut_start_index; // start index in IO map to copy the clut
	UINT32  m_clut_num_items;   // length of the clut (number of clut entries)

	UINT32 m_is_right_eye_view_valid;		 // indicate whether right eye view content valid.
	UINT32 m_right_eye_view_buf_start;	   // the buffer start address for right view content.
	UINT32 m_right_eye_view_descriptor;	 // the descriptor of right eye view buffer,
	UINT32 m_number_of_offset_sequences;	  // present how many offset data is valid.
	UINT8  m_offset_meta_data[32];			// the offset meta data.

	UINT32 m_is_virtual_frame;			   // indicate whether this is a virtual frame
	UINT32 m_sec_field_start;			   // the buffer start address for the second field.
	UINT32 m_first_org_descriptor;		  // the original frame discriptor of first field
	UINT32 m_second_org_descriptor;		 // the second frame discritor of second field.
	UINT32  usage_count;					 // field used count for return the buffer.

	UINT32 m_surface[2];


	UINT32		m_is_xvYCC_valid;
	xvYCC_DATA	m_xvYCC_data;

	UINT32		m_is_UHD_frame;
	UINT32 m_cmpr_psample;
	UINT32 m_decmpr_psample;
	UINT32  m_is_preroll_frame;				//preroll frame will not be displayed. 1: prerool frame;  0:normal frame
	UINT32  m_display_stc_high;				//high 32-bit of STC clock when the frame is displayed currently not used
	UINT32  m_display_stc_low;				 //low 32-bit of STC clock when the frame is displayed

	UINT32 m_3D_source_fmt;
	UINT32 m_3D_interpretation_type;
	UINT32 m_3D_SBS_sampling_mode;
	UINT32 m_3D_convert_mode_from_SEI;

	UINT32 m_interlace_weave_mode;	  //indicate whether weave mode used for interlace
	UINT32 m_is_progressive_stream;	 //indicate whether it is a progressive stream, it's different with m_is_progressive_pic
										//because m_is_progressive_pic may be changed based on film detection
	UINT32 m_3D_SwitchLeftRightView;
	UINT32 m_hBD;
#if defined(VPP_TZ_ENABLE) || defined(OVP_TZ_ENABLE) || defined(OVP_IN_TRUST_ZONE) || defined(CONFIG_MV_AMP_TEE_ENABLE)
	UINT32 m_hDesc;
#endif
	UINT32 m_bits_per_pixel;
	UINT32 m_buf_pbuf_start_UV;		 //Start Address of UV data in 420SP format
	UINT32 m_buf_stride_UV;			 //Stride of UV data in 420SP format
	UINT32 m_colorprimaries;
	UINT32 m_OrgInputInterlaced;
	UINT32 m_pbuf_Out_start_0;
	UINT32 m_pbuf_Out_start_UV_0;
	UINT32 m_pbuf_Out_start_1;
	UINT32 m_pbuf_Out_start_UV_1;
	UINT32 m_Org_frameStatus;	   //Whether frame is originally progressive or generated from OVP
	UINT32 m_Out_buf1_valid;		//Whether m_pbuf_Out_start_1 is valid or not
	UINT32 m_Out_buf0_valid;		//Whether m_pbuf_Out_start_1 is valid or not
	UINT32 m_FrameDesc_RefStat;	 //Which frame is in use by any module/component or not.
	UINT32 m_hBuf;				  //For frame capture

	UINT32 m_is_compressed;

	//Tile&420SP Auto Mode support
	UINT32 m_pbuf_start_1;			 //Start Address of Y data in 420SP format
	UINT32 m_buf_stride_1;			  //Stride of Y data in 420SP format
	UINT32 m_buf_pbuf_start_UV_1;	   //Start Address of UV data in 420SP format
	UINT32 m_buf_stride_UV_1;		   //Stride of UV data in 420SP format
	UINT32 m_v_scaling;				 // 2 indicate 1/2 420sp downscale,4 indicate 1/4

	/**
	 * HDR information for HDMI spec
	 */
	UINT16 sei_display_primary_x[3];
	UINT16 sei_display_primary_y[3];
	UINT16 sei_white_point_x;
	UINT16 sei_white_point_y;
	UINT32 sei_max_display_mastering_lumi;
	UINT32 sei_min_display_mastering_lumi;
	UINT32 transfer_characteristics;
	UINT32 matrix_coeffs;
	UINT32 MaxCLL;
	UINT32 MaxFALL;
	UINT32 builtinFrame;
	UINT8 alt_transfer_characteristics;

	/**
	 * Extra HDR information for HLG
	 */
	UINT8	   m_primaries;
	UINT16	  m_bits_per_channel;
	UINT16	  m_chroma_subsampling_horz;
	UINT16	  m_Chroma_subsampling_vert;
	UINT16	  m_cb_Subsampling_horz;
	UINT16	  m_cb_subsampling_vert;
	UINT8	   m_chroma_siting_horz;
	UINT8	   m_chroma_siting_vert;
	UINT8  m_color_range; //for RGB & YUV

	//For Technicolor
	INT32 m_iDisplayOETF;
	INT32 m_iYuvRange;		//0 - limited range, 1 - full range
	INT32 m_iColorContainer;  //0 - 709, 1- BT 2020
	INT32 m_iHdrColorSpace;   //0 - 709, 1 - 2020, 2- P3
	INT32 m_iLdrColorSpace;
	INT32 m_iPeakLuminance;   //Peak Luminance
	UINT16  m_uiBa;		 //Modulation factor.
	INT32 m_iProcessMode;

	//For V-ITM
	INT m_vitmyuvRangeIn;
	INT m_vitmyuvRangeOut;
	INT m_vitmGaussFilterType;
	INT m_vitmYLutFilterType;
	INT m_vitmPQOutEnable;
	INT m_vitmPQLMax;
	INT m_vitmEnDebanding;
	INT m_vitmEnDenoising;

		//Technicolor Present Mode
	UINT32 m_thdr_present_mode;
	THDR_META_INFO m_thdr_meta;

	/**
	 * Whether the dolby vision is present and configuatations use bit-wise mask to specify the presence of each elements.
	 * current valid configures are 0(no present), 0x7(BL+EL+M) and 0x5(BL+M)
	 */
	UINT32 m_dv_present_mode;

	/** The pointer to the enhance layer VPP_VBUF descriptor. if zero, means match failed and not present. **/
	UINT32 m_dv_el_descriptor;

	/**
	 * Exceptions flag bits for dolby vision only BL descriptor was used.
	 * current definitions: bit 0: whether the meta data is invalid (error).
	 * all other bits are reserved.
	 */
	UINT32 m_dv_status;

	/** DV_META_INFO for parsed meta data information, only BL diescriptor was used. **/
	DV_META_INFO  m_dv_meta;
	UINT32 m_sar_width;
	UINT32 m_sar_height;
#if !(defined(VPP_TZ_ENABLE) || defined(OVP_TZ_ENABLE) || defined(OVP_IN_TRUST_ZONE))
	UINT32 DUMMY_m_hDesc;
#endif
}  VPP_VBUF;

#endif //_VPP_VBUF_H_
