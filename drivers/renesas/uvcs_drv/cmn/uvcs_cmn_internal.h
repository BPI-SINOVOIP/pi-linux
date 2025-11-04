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

#ifndef UVCS_CMN_INTERNAL_H
#define UVCS_CMN_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************/
/*                      INCLUDE FILE                                         */
/*****************************************************************************/
#include "uvcs_cmn.h"
#include "mcvx_api.h"

/*****************************************************************************/
/*                      MACROS/DEFINES                                       */
/*****************************************************************************/
#ifndef MINUNIT
	#define UVCS_STATIC		static
#else
	#define UVCS_STATIC
#endif

#define UVCS_C_YES				(1)
#define UVCS_C_NO				(0)

#define UVCS_C_DUMP_BUFF_MIN	(32772u)
#define UVCS_C_FCV_UNIFIED		(UVCS_C_YES)

#define UVCS_C_REQ_NONE			(0u)
#define UVCS_C_REQ_WAIT			(1u)
#define UVCS_C_REQ_EXEC			(2u)

#define UVCS_C_HWP_NONE			(0x00u)
#define UVCS_C_HWP_RUN			(0x01u)
#define UVCS_C_HWP_FINISH		(0xFFu)

#define UVCS_C_NOEL_EXCTRL		(UVCS_CMN_MAX_HW_NUM)
#define UVCS_C_NOEL_HW			(UVCS_CMN_MAX_HW_NUM)
#define UVCS_C_NOEL_REQ			(UVCS_CMN_PROC_REQ_MAX)
#define UVCS_C_NOEL_EXEMDL		(2u)

#define UVCS_C_ALIGN_MASK_8		(0x7u)

/*****************************************************************************/
/*                    LOCAL TYPES                                            */
/*****************************************************************************/
struct UVCS_C_REQ_ITEM {
	/* linked list */
	struct UVCS_C_REQ_ITEM		*next;
	struct UVCS_C_REQ_ITEM		*prev;
	struct UVCS_CMN_HW_PROC		*info;
	struct UVCS_C_HANDLE		*owner;
	UVCS_U32					 target_ipid;
	UVCS_U32					*ptr_hwp_state;
	UVCS_U32					 dbg_execnt;
	UVCS_U32					 dbg_s_time;
	UVCS_U32					 dbg_e_time;
};

struct UVCS_C_HANDLE {
	UVCS_PTR					 address;
	struct UVCS_CMN_OPEN_PARAM	 open_info;
	struct UVCS_C_REQ_ITEM		 request[UVCS_C_NOEL_EXEMDL];
};

struct UVCS_C_HW_EXCTRL {
	UVCS_U32					 hwp_state[UVCS_C_NOEL_EXEMDL];
	struct UVCS_C_REQ_ITEM		*cur_item[UVCS_C_NOEL_EXEMDL];
	UVCS_BOOL					 reset_req;
};


struct UVCS_C_REQ_LIST {
	struct UVCS_C_REQ_ITEM		*head;
	struct UVCS_C_REQ_ITEM		*tail;
};

struct UVCS_C_INR_INFO {
	UVCS_PTR					 address;	/* validator */
	struct UVCS_CMN_INIT_PARAM	 init_param;
	MCVX_IP_CTX_T				 ip_context[UVCS_C_NOEL_HW];
	struct UVCS_C_HW_EXCTRL		 hw_exctrl[UVCS_C_NOEL_EXCTRL];
	struct UVCS_C_REQ_LIST		 req_list[UVCS_C_NOEL_EXCTRL][UVCS_C_NOEL_EXEMDL];
	struct UVCS_C_REQ_LIST		 req_list_unbind[UVCS_C_NOEL_EXEMDL];
	UVCS_PTR					 dump_buff;
	UVCS_U32					 dump_max;
	UVCS_U32					 ip_version;
	UVCS_U32					 ip_capability[UVCS_C_NOEL_HW];
	MCVX_FCPC_CONFIG_T			 fcpc_config[UVCS_C_NOEL_HW];
	MCVX_FCPC_CTX_T				 fcpc_context[UVCS_C_NOEL_HW];
	UVCS_BOOL					 fcpc_reset_all;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UVCS_CMN_INTERNAL_H */

