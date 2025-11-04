/*************************************************************************/ /*
 UVCS Driver common module

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

#ifndef UVCS_CMN_H
#define UVCS_CMN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************/
/*                      INCLUDE FILES                                        */
/*****************************************************************************/
#include "uvcs_types.h"

/*****************************************************************************/
/*                      MACROS/DEFINES                                       */
/*****************************************************************************/
#define UVCS_CMN_MAX_HW_NUM		(3u)
#define UVCS_CMN_PROC_REQ_MAX	(3u)
#define UVCS_CMN_HWP_CMD_NOEL	(512u)

#define UVCS_CMN_BASE_ADDR_VLC	(0u)
#define UVCS_CMN_BASE_ADDR_CE	(1u)
#define UVCS_CMN_BASE_ADDR_NOEL	(2u)

#define UVCS_CMN_MDL_VLC		(0u)
#define UVCS_CMN_MDL_CE			(1u)
#define UVCS_CMN_MDL_FCV		(2u)

#define UVCS_CMN_HWPIDX_REQ_TMG	(0u)	/* timing */
#define UVCS_CMN_HWPIDX_REQ_COD	(1u)	/* codec */
#define UVCS_CMN_HWPIDX_REQ_DAT	(2u)	/* data */
#define UVCS_CMN_HWPIDX_REQ_BAA	(256u)	/* BAA */

#define UVCS_CMN_HWPIDX_RES_STA	(0u)	/* status NML or ERR */
#define UVCS_CMN_HWPIDX_RES_CVL	(1u)	/* vlc execute counter */
#define UVCS_CMN_HWPIDX_RES_CCE	(2u)	/* ce execute counter */
#define UVCS_CMN_HWPIDX_RES_CFC	(3u)	/* fcv execute counter */
#define UVCS_CMN_HWPIDX_RES_EST	(4u)	/* execute start time */
#define UVCS_CMN_HWPIDX_RES_EET	(5u)	/* execute end time */
#define UVCS_CMN_HWPIDX_RES_DAT	(6u)	/* data */
#define UVCS_CMN_HWP_NOEL_BASE	(UVCS_CMN_HWPIDX_RES_EET)

#define UVCS_CMN_IPARCH_VCPLF	(0x00u)
#define UVCS_CMN_IPARCH_VCPL4	(0x11u)
#define UVCS_CMN_IPARCH_BELZ	(0x01u)
#define UVCS_CMN_IPARCH_FCPCS	(0x20u)
#define UVCS_CMN_IPARCH_FCPCI	(0x21u)

#define UVCS_CMN_RANGE_NONE	(0x00u)
#define UVCS_CMN_RANGE_1K	(0x01u)
#define UVCS_CMN_RANGE_2K	(0x02u)
#define UVCS_CMN_RANGE_4K	(0x04u)
#define UVCS_CMN_RANGE_8K	(0x08u)

/*****************************************************************************/
/*               TYPE DEFINITION                                             */
/*****************************************************************************/

typedef UVCS_PTR	 UVCS_CMN_LIB_INFO;

typedef UVCS_PTR	 UVCS_CMN_HANDLE;

typedef struct UVCS_CMN_OPEN_PARAM {
	UVCS_U32		 struct_size;
	UVCS_PTR		 hdl_udptr;
	UVCS_PTR		 hdl_work_0_virt;
	UVCS_U32		 hdl_work_0_size;
	UVCS_BOOL		 ip_binding_enable;
	UVCS_U32		 ip_binding_id;
} UVCS_CMN_OPEN_PARAM_T;

typedef struct UVCS_CMN_HW_PROC {
	UVCS_U32		 struct_size;
	UVCS_U32		 module_id;
	UVCS_U32		 req_serial;
	UVCS_U32		 cmd_param_noel;
	UVCS_U32		 cmd_param[UVCS_CMN_HWP_CMD_NOEL];
	UVCS_U32		 range;
	UVCS_U32		 encoder;
	UVCS_U32		 width;
	UVCS_U32		 height;
} UVCS_CMN_HW_PROC_T;

typedef struct UVCS_CMN_IP_INFO {
	UVCS_U32		 struct_size;
	UVCS_U32		 ip_version;
	UVCS_U32		 ip_option;
} UVCS_CMN_IP_INFO_T;

typedef struct UVCS_CMN_IP_CAPABILITY {
	UVCS_U32		 struct_size;
	UVCS_U32		 ip_capability[UVCS_CMN_MAX_HW_NUM];
} UVCS_CMN_IP_CAPABILITY_T;

typedef struct UVCS_CMN_LSI_INFO {
	UVCS_U32		 struct_size;
	UVCS_U32		 product_id;
	UVCS_U32		 cut_id;
} UVCS_CMN_LSI_INFO_T;

typedef void (*UVCS_CMN_CB_REG_READ)(
	UVCS_PTR			 udptr,
	volatile UVCS_U32	*reg_addr,
	UVCS_U32			*dst_addr,
	UVCS_U32			 num_reg
);

typedef void (*UVCS_CMN_CB_REG_WRITE)(
	UVCS_PTR			 udptr,
	volatile UVCS_U32	*reg_addr,
	UVCS_U32			*src_addr,
	UVCS_U32			 num_reg
);

typedef void (*UVCS_CMN_CB_HW_START)(
	UVCS_PTR			 udptr,
	UVCS_U32			 hw_ip_id,
	UVCS_U32			 hw_module_id,
	UVCS_U32			*baa
);

typedef void (*UVCS_CMN_CB_HW_STOP)(
	UVCS_PTR			 udptr,
	UVCS_U32			 hw_ip_id,
	UVCS_U32			 hw_module_id
);

typedef void (*UVCS_CMN_CB_HW_RESET)(
	UVCS_PTR			 udptr,
	UVCS_U32			 hw_ip_id
);

typedef void (*UVCS_CMN_CB_PROC_DONE)(
	UVCS_PTR			 udptr,
	UVCS_PTR			 hdl_udptr,
	UVCS_CMN_HANDLE		 handle,
	UVCS_CMN_HW_PROC_T	*res_info
);

typedef UVCS_BOOL (*UVCS_CMN_CB_SEM_LOCK)(
	UVCS_PTR			 udptr
);

typedef void (*UVCS_CMN_CB_SEM_UNLOCK)(
	UVCS_PTR			 udptr
);

typedef UVCS_BOOL (*UVCS_CMN_CB_SEM_CREATE)(
	UVCS_PTR			 udptr
);

typedef void (*UVCS_CMN_CB_SEM_DESTROY)(
	UVCS_PTR			 udptr
);

typedef void (*UVCS_CMN_CB_THREAD_EVENT)(
	UVCS_PTR			 udptr
);

typedef UVCS_BOOL (*UVCS_CMN_CB_THREAD_CREATE)(
	UVCS_PTR			 udptr
);

typedef void (*UVCS_CMN_CB_THREAD_DESTROY)(
	UVCS_PTR			 udptr
);

/* for uvcs_cmn_initialize().
 * Please see the specification (section 3.2.1) for more detail.
 */
typedef struct UVCS_CMN_INIT_PARAM {
	UVCS_U32	 struct_size;
	UVCS_U32	 hw_num;
	UVCS_PTR	 udptr;

	/* work memory */
	UVCS_PTR	 work_mem_0_virt;
	UVCS_U32	 work_mem_0_size;

	/* ip information */
	UVCS_PTR	 ip_base_addr[UVCS_CMN_MAX_HW_NUM][UVCS_CMN_BASE_ADDR_NOEL];
	UVCS_U32	 ip_arch[UVCS_CMN_MAX_HW_NUM];
	UVCS_U32	 ip_group_id[UVCS_CMN_MAX_HW_NUM][UVCS_CMN_BASE_ADDR_NOEL];
	UVCS_U32	 ip_option;

	/* FCP information */
	UVCS_U32	 fcpc_hw_num;
	UVCS_PTR	 fcpc_base_addr[UVCS_CMN_MAX_HW_NUM];
	UVCS_U32	 fcpc_arch[UVCS_CMN_MAX_HW_NUM];
	UVCS_BOOL	 fcpc_indep_mode;

	/* system callback */
	UVCS_CMN_CB_REG_READ		 cb_reg_read;
	UVCS_CMN_CB_REG_WRITE		 cb_reg_write;
	UVCS_CMN_CB_HW_START		 cb_hw_start;
	UVCS_CMN_CB_HW_STOP			 cb_hw_stop;
	UVCS_CMN_CB_HW_RESET		 cb_hw_reset;
	UVCS_CMN_CB_PROC_DONE		 cb_proc_done;
	UVCS_CMN_CB_SEM_LOCK		 cb_sem_lock;
	UVCS_CMN_CB_SEM_UNLOCK		 cb_sem_unlock;
	UVCS_CMN_CB_SEM_CREATE		 cb_sem_create;
	UVCS_CMN_CB_SEM_DESTROY		 cb_sem_destroy;
	UVCS_CMN_CB_THREAD_EVENT	 cb_thr_event;
	UVCS_CMN_CB_THREAD_CREATE	 cb_thr_create;
	UVCS_CMN_CB_THREAD_DESTROY	 cb_thr_destroy;

	/* debug information */
	UVCS_PTR	 debug_log_buff;
	UVCS_U32	 debug_log_size;

} UVCS_CMN_INIT_PARAM_T;


/*****************************************************************************/
/*               EXTERNAL FUNCTIONS                                          */
/*****************************************************************************/
extern UVCS_RESULT uvcs_cmn_vlc_interrupt(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_U32			 hw_ip_id,
	UVCS_U32			 cur_time,
	UVCS_BOOL			 timeout
);
extern UVCS_RESULT uvcs_cmn_ce_interrupt(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_U32			 hw_ip_id,
	UVCS_U32			 cur_time,
	UVCS_BOOL			 timeout
);
extern UVCS_RESULT uvcs_cmn_initialize(
	UVCS_CMN_INIT_PARAM_T	*init_param,
	UVCS_CMN_LIB_INFO		*lib_info
);
extern UVCS_RESULT uvcs_cmn_deinitialize(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_BOOL			 forced
);
extern UVCS_RESULT uvcs_cmn_open(
	UVCS_CMN_LIB_INFO		 lib_info,
	UVCS_CMN_OPEN_PARAM_T	*open_param,
	UVCS_CMN_HANDLE			*handle
);
extern UVCS_RESULT uvcs_cmn_close(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_CMN_HANDLE		 handle,
	UVCS_BOOL			 forced
);
extern UVCS_RESULT uvcs_cmn_request(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_CMN_HANDLE		 handle,
	UVCS_CMN_HW_PROC_T	*req_info
);
extern UVCS_RESULT uvcs_cmn_execute(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_U32			 cur_time
);
extern UVCS_RESULT uvcs_cmn_set_ip_binding(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_CMN_HANDLE		 handle,
	UVCS_BOOL			 mode,
	UVCS_U32			 ipid
);
extern UVCS_RESULT uvcs_cmn_get_work_size(
	UVCS_U32			*work_mem_0_size,
	UVCS_U32			*hdl_work_0_size
);
extern UVCS_RESULT uvcs_cmn_get_ip_info(
	UVCS_CMN_LIB_INFO	 lib_info,
	UVCS_CMN_IP_INFO_T	*ip_info
);

extern UVCS_RESULT uvcs_cmn_get_ip_capability(
	UVCS_CMN_LIB_INFO			 lib_info,
	UVCS_CMN_IP_CAPABILITY_T	*ip_cap
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UVCS_CMN_H */

