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

#ifdef __RENESAS__
#pragma section UVCSCMN
#endif /* __RENESAS__ */

/*****************************************************************************/
/*                    INCLUDE FILES                                          */
/*****************************************************************************/
#include "uvcs_types.h"
#include "mcvx_types.h"
#include "mcvx_api.h"
#include "uvcs_cmn.h"		/* external header file */
#include "uvcs_cmn_internal.h"	/* internal common header file */
#include "uvcs_cmn_dump.h"

/*****************************************************************************/
/*                    MACROS/DEFINES                                         */
/*****************************************************************************/

#if (UVCS_DEBUG != 0)
#define UVCS_C_DUMP_ID			UVCS_C_API_C	/**< for debug */
#endif

/*****************************************************************************/
/*                    FORWARD DECLARATIONS                                   */
/*****************************************************************************/
/* util */
UVCS_STATIC struct UVCS_C_INR_INFO *uvcs_c_mng_inr_param(
					UVCS_BOOL mng_mode,
					struct UVCS_C_INR_INFO *new_ptr);
UVCS_STATIC void uvcs_c_memclr4(
					void *dst_mem,
					UVCS_U32 num_bytes);
UVCS_STATIC UVCS_U32 uvcs_c_get_executable_ipid(
					struct UVCS_C_INR_INFO *inr,
					UVCS_U32 grpid,
					UVCS_U32 exemdl,
					UVCS_U32 codec);
UVCS_STATIC UVCS_BOOL uvcs_c_check_codec_executable(
					struct UVCS_C_INR_INFO *inr,
					UVCS_U32 ipid,
					UVCS_U32 codec);

/* listctrl */
UVCS_STATIC void uvcs_c_execute(
					struct UVCS_C_INR_INFO *inr,
					UVCS_U32 grpid,
					UVCS_U32 exemdl,
					UVCS_U32 cur_time,
					UVCS_BOOL search_from_binded);
UVCS_STATIC struct UVCS_C_REQ_ITEM *uvcs_c_reqlist_get_candidate(
					struct UVCS_C_INR_INFO	*inr,
					struct UVCS_C_REQ_LIST *list,
					UVCS_U32 grpid,
					UVCS_U32 exemdl);

UVCS_STATIC struct UVCS_C_REQ_ITEM *uvcs_c_reqlist_peek(
					struct UVCS_C_REQ_LIST *list,
					UVCS_BOOL fifomode);
UVCS_STATIC void uvcs_c_reqlist_add_item(
					struct UVCS_C_REQ_LIST *list,
					struct UVCS_C_REQ_ITEM *item,
					UVCS_BOOL priority);
UVCS_STATIC void uvcs_c_reqlist_remove_item(
					struct UVCS_C_REQ_LIST *list,
					struct UVCS_C_REQ_ITEM *item);
UVCS_STATIC void uvcs_c_reqlist_remove_owner(
					struct UVCS_C_REQ_LIST *list,
					struct UVCS_C_HANDLE *handle);

/* hwctrl */
UVCS_STATIC void uvcs_c_hwctrl_start_processing(
					struct UVCS_C_INR_INFO *inr,
					struct UVCS_C_REQ_ITEM *item,
					UVCS_U32 *hwpstate,
					UVCS_U32 exemdl,
					UVCS_U32 dbg_curtime);
UVCS_STATIC void uvcs_c_hwctrl_stop_processing(
					struct UVCS_C_INR_INFO *inr,
					struct UVCS_C_REQ_ITEM *item,
					UVCS_U32 exemdl,
					UVCS_U32 dbg_curtime);
UVCS_STATIC void uvcs_c_hwctrl_fcpc_reset_one(
					struct UVCS_C_INR_INFO *inr,
					UVCS_U32 exemdl);
UVCS_STATIC void uvcs_c_hwctrl_fcpc_reset_all(
					struct UVCS_C_INR_INFO *inr);

/* event handler */
UVCS_STATIC void uvcs_c_event_vlc_state(
					void *udp,
					MCVX_U32 vlc_event,
					void *unused);
UVCS_STATIC void uvcs_c_event_ce_state(
					void *udp,
					MCVX_U32 ce_event,
					void *unused);
UVCS_STATIC void uvcs_c_event_reg_read(
					MCVX_REG *reg_addr,
					MCVX_U32 *dst_addr,
					MCVX_U32  num_reg);
UVCS_STATIC void uvcs_c_event_reg_write(
					MCVX_REG *reg_addr,
					MCVX_U32 *src_addr,
					MCVX_U32  num_reg);

/*****************************************************************************/
/*                    EXTERNAL FUNCTIONS                                     */
/*****************************************************************************/

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_vlc_interrupt
 *
 * Return
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_vlc_interrupt(
	UVCS_CMN_LIB_INFO	lib_info,
	UVCS_U32			hw_ip_id,
	UVCS_U32			cur_time,
	UVCS_BOOL			timeout
	)
{
	struct UVCS_C_INR_INFO		*inr = (struct UVCS_C_INR_INFO *)lib_info;
	struct UVCS_C_HW_EXCTRL		*exctrl;
	UVCS_RESULT result = UVCS_RTN_PARAMETER_ERROR;

	/* parameter check */
	if ((inr != NULL)
	&&	(inr->address == lib_info)
	&&	(inr->init_param.cb_thr_event != NULL)
	&&	(inr->init_param.hw_num > hw_ip_id)) {
		exctrl = &inr->hw_exctrl[(inr->init_param.ip_group_id[hw_ip_id][UVCS_CMN_MDL_VLC])];

		if (exctrl->cur_item[UVCS_CMN_MDL_VLC] != NULL) {
			exctrl->cur_item[UVCS_CMN_MDL_VLC]->dbg_e_time = cur_time;
		}
		if (timeout == UVCS_FALSE) {
			mcvx_vlc_interrupt_handler(
				&inr->ip_context[hw_ip_id], MCVX_FALSE);
		} else {
			mcvx_vlc_interrupt_handler(
				&inr->ip_context[hw_ip_id], MCVX_TRUE);
		}

		/* Function uvcs_cmn_execute() needs a calling from thread context.
		 * Requests to calling it to upper layer.
		 */
		inr->init_param.cb_thr_event(inr->init_param.udptr);
		result = UVCS_RTN_OK;
	}

	return result;
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_ce_interrupt
 *
 * Return
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_ce_interrupt(
	UVCS_CMN_LIB_INFO	lib_info,
	UVCS_U32			hw_ip_id,
	UVCS_U32			cur_time,
	UVCS_BOOL			timeout
	)
{
	struct UVCS_C_INR_INFO		*inr = (struct UVCS_C_INR_INFO *)lib_info;
	struct UVCS_C_HW_EXCTRL		*exctrl;
	UVCS_RESULT result = UVCS_RTN_PARAMETER_ERROR;

	/* parameter check */
	if ((inr != NULL)
	&&	(inr->address == lib_info)
	&&	(inr->init_param.cb_thr_event != NULL)
	&&	(inr->init_param.hw_num > hw_ip_id)) {
		exctrl = &inr->hw_exctrl[(inr->init_param.ip_group_id[hw_ip_id][UVCS_CMN_MDL_CE])];

		if (exctrl->cur_item[UVCS_CMN_MDL_CE] != NULL) {
			exctrl->cur_item[UVCS_CMN_MDL_CE]->dbg_e_time = cur_time;
		}
		if (timeout == UVCS_FALSE) {
			mcvx_ce_interrupt_handler(
				&inr->ip_context[hw_ip_id], MCVX_FALSE);
		} else {
			exctrl->reset_req = UVCS_TRUE;
			mcvx_ce_interrupt_handler(
				&inr->ip_context[hw_ip_id], MCVX_TRUE);
		}
		/* Function uvcs_cmn_execute() needs a calling from thread context.
		 * Requests to calling it to upper layer.
		 */
		inr->init_param.cb_thr_event(inr->init_param.udptr);
		result = UVCS_RTN_OK;
	}

	return result;
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_initialize
 *
 * Return
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_SYSTEM_ERROR
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_initialize(
	UVCS_CMN_INIT_PARAM_T	*init_param, /**< [in] init parameter */
	UVCS_CMN_LIB_INFO	*lib_info /**< [out] library information */
	)
{
	struct UVCS_C_INR_INFO *inr;
	UVCS_RESULT result;
	UVCS_U32 i;
	MCVX_IP_CONFIG_T ip_config;
	MCVX_FCPC_CONFIG_T *fcpc_config;

	/**
	 * \note QAC STMCC is over 10, but most items are parameter checks.
	 * This function is excluded.
	 */

	/** (1) parameter checking */
	if ((init_param == NULL)
	|| (lib_info == NULL)
	|| (init_param->struct_size < sizeof(struct UVCS_CMN_INIT_PARAM))) {
		result = UVCS_RTN_PARAMETER_ERROR;

	} else if ((init_param->hw_num > UVCS_C_NOEL_HW)
	|| (init_param->hw_num == 0u)
	|| (init_param->work_mem_0_virt == NULL)
	|| (init_param->work_mem_0_size < sizeof(struct UVCS_C_INR_INFO))) {
		result = UVCS_RTN_PARAMETER_ERROR;

	} else if ((init_param->cb_reg_read == NULL)
	|| (init_param->cb_reg_write == NULL)
	|| (init_param->cb_proc_done == NULL)
	|| (init_param->cb_sem_create == NULL)
	|| (init_param->cb_sem_destroy == NULL)
	|| (init_param->cb_sem_lock == NULL)
	|| (init_param->cb_sem_unlock == NULL)
	|| (init_param->cb_thr_create == NULL)
	|| (init_param->cb_thr_destroy == NULL)
	|| (init_param->cb_thr_event == NULL)) {
		result = UVCS_RTN_PARAMETER_ERROR;

	/** (2) request to create semaphore */
	} else if (init_param->cb_sem_create(init_param->udptr) == UVCS_FALSE) {
		result = UVCS_RTN_SYSTEM_ERROR;
	/** (3) request to create thread */
	} else if (init_param->cb_thr_create(init_param->udptr) == UVCS_FALSE) {
		init_param->cb_sem_destroy(init_param->udptr);
		result = UVCS_RTN_SYSTEM_ERROR;
	} else {
		result = UVCS_RTN_OK;

		uvcs_c_memclr4(init_param->work_mem_0_virt, init_param->work_mem_0_size);
		inr = (struct UVCS_C_INR_INFO *)init_param->work_mem_0_virt;
		inr->init_param = *init_param;

		/* initialise callback function for register access */
		(void)uvcs_c_mng_inr_param(UVCS_TRUE, inr);

		for (i = 0u; i < init_param->hw_num; i++) {
			if (result == UVCS_RTN_OK) {
				if ((init_param->ip_base_addr[i][UVCS_CMN_BASE_ADDR_VLC] == NULL)
					|| (init_param->ip_base_addr[i][UVCS_CMN_BASE_ADDR_CE] == NULL)
					|| (init_param->ip_group_id[i][UVCS_CMN_MDL_CE] >= init_param->fcpc_hw_num)) {
					result = UVCS_RTN_PARAMETER_ERROR;

				} else if ((init_param->ip_arch[i] != UVCS_CMN_IPARCH_VCPLF)
					&& (init_param->ip_arch[i] != UVCS_CMN_IPARCH_VCPL4)
					&& (init_param->ip_arch[i] != UVCS_CMN_IPARCH_BELZ)) {
					result = UVCS_RTN_PARAMETER_ERROR;

				} else {
					/* initialize VCP module */
					ip_config.ip_id = i;
					ip_config.ip_vlc_base_addr = init_param->ip_base_addr[i][UVCS_CMN_BASE_ADDR_VLC];
					ip_config.ip_ce_base_addr = init_param->ip_base_addr[i][UVCS_CMN_BASE_ADDR_CE];
					ip_config.udf_reg_read = &uvcs_c_event_reg_read;
					ip_config.udf_reg_write = &uvcs_c_event_reg_write;
					if (init_param->ip_arch[i] == UVCS_CMN_IPARCH_BELZ) {
						ip_config.ip_arch = MCVX_ARCH_BELZ;
					} else {
						result = UVCS_RTN_PARAMETER_ERROR;
					}
					if (mcvx_ip_init(&ip_config, &inr->ip_context[i]) != MCVX_NML_END) {
						result = UVCS_RTN_SYSTEM_ERROR;
					}
					if (mcvx_get_ip_capability(&inr->ip_context[i], &inr->ip_capability[i]) != MCVX_NML_END) {
						result = UVCS_RTN_SYSTEM_ERROR;
					}
				}
			}
		}

		for (i = 0u; i < init_param->fcpc_hw_num; i++) {
			if (result == UVCS_RTN_OK) {
				if (init_param->fcpc_base_addr[i] == NULL) {
					result = UVCS_RTN_PARAMETER_ERROR;

				} else if ((init_param->fcpc_arch[i] != UVCS_CMN_IPARCH_FCPCS)
					&& (init_param->fcpc_arch[i] != UVCS_CMN_IPARCH_FCPCI)) {
					result = UVCS_RTN_PARAMETER_ERROR;

				} else {
					/* initialize FCPC module */
					fcpc_config = &inr->fcpc_config[i];
					fcpc_config->fcpc_base_addr = init_param->fcpc_base_addr[i];
					if (init_param->fcpc_arch[i] == UVCS_CMN_IPARCH_FCPCS) {
						fcpc_config->fcpc_arch = MCVX_FCPC_S;
					} else {
						fcpc_config->fcpc_arch = MCVX_FCPC_I;
					}
					if (init_param->fcpc_indep_mode == UVCS_TRUE) {
						fcpc_config->shared_mode = MCVX_FCPC_SM_1;
					} else {
						fcpc_config->shared_mode = MCVX_FCPC_SM_DEFAULT;
					}
					fcpc_config->udf_reg_read = &uvcs_c_event_reg_read;
					fcpc_config->udf_reg_write = &uvcs_c_event_reg_write;
					if (mcvx_fcpc_init(fcpc_config, &inr->fcpc_context[i]) != MCVX_NML_END) {
						result = UVCS_RTN_SYSTEM_ERROR;
					}
				}
			}
		}

		if (result != UVCS_RTN_OK) {
			/* clear */
			(void)uvcs_c_mng_inr_param(UVCS_TRUE, NULL);
			init_param->cb_thr_destroy(init_param->udptr);
			init_param->cb_sem_destroy(init_param->udptr);
			uvcs_c_memclr4(init_param->work_mem_0_virt, sizeof(struct UVCS_C_INR_INFO));

		} else {
			(void)mcvx_get_ip_version(&inr->ip_context[0], &inr->ip_version);

			UVCS_CMN_DUMP_INIT(inr);
			inr->address = inr;
			*lib_info = (UVCS_CMN_LIB_INFO)inr;
		}
	}

	return result;
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_deinitialize
 *
 * Return
 *      \retval UVCS_RTN_NOT_INITIALISE
 *      \retval UVCS_RTN_CONTINUE
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_deinitialize(
	UVCS_CMN_LIB_INFO	 lib_info, /**< [in] library information */
	UVCS_BOOL		 forced
	)
{
	UVCS_RESULT		 result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO	*inr = (struct UVCS_C_INR_INFO *)lib_info;

	if ((inr != NULL)
	&& (inr->address == lib_info)
	&& (inr->init_param.cb_sem_lock != NULL)
	&& (inr->init_param.cb_sem_unlock != NULL)
	&& (inr->init_param.cb_sem_destroy != NULL)
	&& (inr->init_param.cb_thr_destroy != NULL)) {
		result = UVCS_RTN_OK;

		if (forced == UVCS_FALSE) {
			if (inr->init_param.cb_sem_lock(inr->init_param.udptr) == UVCS_FALSE) {
				result = UVCS_RTN_CONTINUE;
			}
		}

		UVCS_CMN_DUMP(inr, UVCS_CAPI_DEIN, UVCS_CP3((UVCS_U32)result), 0);

		if (result == UVCS_RTN_OK) {
			inr->address = NULL;

			/* destroy resources */
			if (forced == UVCS_FALSE) {
				inr->init_param.cb_sem_unlock(inr->init_param.udptr);
			}
			inr->init_param.cb_thr_destroy(inr->init_param.udptr);
			inr->init_param.cb_sem_destroy(inr->init_param.udptr);
			(void)uvcs_c_mng_inr_param(UVCS_TRUE, NULL);

			uvcs_c_memclr4(inr, sizeof(struct UVCS_C_INR_INFO));
		}
	}

	return result;
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_open
 *
 * Return
 *      \retval UVCS_RTN_NOT_INITIALISE
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_CONTINUE
 *      \retval UVCS_RTN_BUSY
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_open(
	UVCS_CMN_LIB_INFO		 lib_info,	/**< [in] library information */
	UVCS_CMN_OPEN_PARAM_T	*open_param,	/**< [in] configuration for handle */
	UVCS_CMN_HANDLE			*handle	/**< [out] virtual device handle */
)
{
	struct UVCS_C_INR_INFO	 *inr    = (struct UVCS_C_INR_INFO *)lib_info;
	UVCS_RESULT		  result = UVCS_RTN_NOT_INITIALISE;

	if ((inr != NULL)
	&& (inr->address == lib_info)
	&& (inr->init_param.cb_sem_lock != NULL)
	&& (inr->init_param.cb_sem_unlock != NULL)) {
		if ((handle == NULL)
		|| (open_param == NULL)
		|| (open_param->hdl_work_0_virt == NULL)
		|| (open_param->hdl_work_0_size < sizeof(struct UVCS_C_HANDLE))) {
			result = UVCS_RTN_PARAMETER_ERROR;

		} else if ((open_param->ip_binding_enable == UVCS_TRUE)
		&& (open_param->ip_binding_id >= inr->init_param.hw_num)) {
			result = UVCS_RTN_PARAMETER_ERROR;

		} else if (inr->init_param.cb_sem_lock(inr->init_param.udptr) == UVCS_FALSE) {
			result = UVCS_RTN_CONTINUE;

		} else {
			struct UVCS_C_HANDLE *new_hdl = (struct UVCS_C_HANDLE *)open_param->hdl_work_0_virt;

			uvcs_c_memclr4(open_param->hdl_work_0_virt, sizeof(struct UVCS_C_HANDLE));
			new_hdl->address = open_param->hdl_work_0_virt;
			new_hdl->open_info = *open_param;
			*handle = (UVCS_CMN_HANDLE)new_hdl;

			inr->init_param.cb_sem_unlock(inr->init_param.udptr);

			result = UVCS_RTN_OK;
		}

		UVCS_CMN_DUMP(inr, UVCS_CAPI_OPEN, UVCS_CP3((UVCS_U32)result), 0);
	}

	return result;
}


/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_close
 *
 * Return
 *      \retval UVCS_RTN_NOT_INITIALISE
 *      \retval UVCS_RTN_INVALID_HANDLE
 *      \retval UVCS_RTN_CONTINUE
 *      \retval UVCS_RTN_BUSY
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_close(
	UVCS_CMN_LIB_INFO	 lib_info,	/**< [in] library information */
	UVCS_CMN_HANDLE		 handle,	/**< [in] virtual device handle */
	UVCS_BOOL		 forced
)
{
	UVCS_RESULT		 result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO	*inr = (struct UVCS_C_INR_INFO *)lib_info;
	struct UVCS_C_HANDLE	*hdl = (struct UVCS_C_HANDLE *)handle;
	UVCS_U32		 exemdl;
	UVCS_U32		 grpid;

	if ((inr != NULL)
	&& (inr->address == lib_info)
	&& (inr->init_param.cb_sem_lock != NULL)
	&& (inr->init_param.cb_sem_unlock != NULL)) {

		if ((hdl == NULL)
		|| (hdl->address != handle)) {
			result = UVCS_RTN_INVALID_HANDLE;

		} else if (inr->init_param.cb_sem_lock(inr->init_param.udptr) == UVCS_FALSE) {
			result = UVCS_RTN_CONTINUE;

		} else {
			/* semaphore locked */

			/* remove from reqlist */
			for (grpid = 0; grpid < UVCS_C_NOEL_EXCTRL; grpid++) {
				uvcs_c_reqlist_remove_owner(&inr->req_list[grpid][UVCS_CMN_MDL_VLC], hdl);
				uvcs_c_reqlist_remove_owner(&inr->req_list[grpid][UVCS_CMN_MDL_CE], hdl);
			}
			uvcs_c_reqlist_remove_owner(&inr->req_list_unbind[UVCS_CMN_MDL_VLC], hdl);
			uvcs_c_reqlist_remove_owner(&inr->req_list_unbind[UVCS_CMN_MDL_CE], hdl);

			/* remove executed item (forced remove) */
			if (forced == UVCS_TRUE) {
				for (grpid = 0; grpid < UVCS_C_NOEL_EXCTRL; grpid++) {
					for (exemdl = 0; exemdl < UVCS_C_NOEL_EXEMDL; exemdl++) {
						if (inr->hw_exctrl[grpid].cur_item[exemdl] == &hdl->request[exemdl]) {
							hdl->request[exemdl].owner = NULL; /* disable callback */
							uvcs_c_hwctrl_stop_processing(inr, inr->hw_exctrl[grpid].cur_item[exemdl], exemdl, 0);
							inr->hw_exctrl[grpid].hwp_state[exemdl] = UVCS_C_HWP_NONE;
							inr->hw_exctrl[grpid].cur_item[exemdl] = NULL;
						}
						hdl->request[exemdl].info = NULL;
					}
				}
			}

			/* request exist */
			if ((hdl->request[UVCS_CMN_MDL_VLC].info != NULL)
			||	(hdl->request[UVCS_CMN_MDL_CE].info != NULL)) {
				result = UVCS_RTN_BUSY;

			} else {
				hdl->address = NULL;
				result = UVCS_RTN_OK;
			}

			inr->init_param.cb_sem_unlock(inr->init_param.udptr);
		}

		UVCS_CMN_DUMP(inr, UVCS_CAPI_CLOS, UVCS_CP3((UVCS_U32)result), 0);
	}

	return result;
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_request
 *
 * Return
 *      \retval UVCS_RTN_NOT_INITIALISE
 *      \retval UVCS_RTN_INVALID_HANDLE
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_CONTINUE
 *      \retval UVCS_RTN_BUSY
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_request(
	UVCS_CMN_LIB_INFO		 lib_info, /**< [in] library information */
	UVCS_CMN_HANDLE			 handle, /**< [in] virtual device handle */
	UVCS_CMN_HW_PROC_T		*req_info /**< [in] request information for hardware processing */
	)
{
	UVCS_RESULT result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO *inr = (struct UVCS_C_INR_INFO *)lib_info;
	struct UVCS_C_HANDLE *hdl = (struct UVCS_C_HANDLE *)handle;
	struct UVCS_C_REQ_LIST *reqlist = NULL;
	struct UVCS_C_REQ_ITEM *item = NULL;
	UVCS_U32 exemdl;
	UVCS_U32 ipid;

	if ((inr != NULL)
	&& (inr->address == lib_info)
	&& (inr->init_param.cb_sem_lock != NULL)
	&& (inr->init_param.cb_sem_unlock != NULL)
	&& (inr->init_param.cb_thr_event != NULL)) {

		if ((hdl == NULL)
		|| (hdl->address != handle)) {
			result = UVCS_RTN_INVALID_HANDLE;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_REQ0, UVCS_CP3((UVCS_U32)result), 0);

		} else if ((req_info == NULL)
		|| (req_info->struct_size < sizeof(UVCS_CMN_HW_PROC_T))
		|| (req_info->module_id >= UVCS_C_NOEL_REQ)) {
			result = UVCS_RTN_PARAMETER_ERROR;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_REQ0, UVCS_CP3((UVCS_U32)result), 0);

		} else if (inr->init_param.cb_sem_lock(inr->init_param.udptr) == UVCS_FALSE) {
			result = UVCS_RTN_CONTINUE;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_REQ0, UVCS_CP3((UVCS_U32)result), 0);

		} else {
			/* sempahore locked */
			result = UVCS_RTN_OK;

			exemdl = (req_info->module_id == UVCS_CMN_MDL_VLC) ? UVCS_CMN_MDL_VLC : UVCS_CMN_MDL_CE;
			if (hdl->open_info.ip_binding_enable == UVCS_TRUE) {
				ipid = hdl->open_info.ip_binding_id;
				if (uvcs_c_check_codec_executable(inr, ipid, req_info->cmd_param[UVCS_CMN_HWPIDX_REQ_COD]) == UVCS_FALSE) {
					reqlist = NULL;
				} else {
					reqlist = &inr->req_list[(inr->init_param.ip_group_id[ipid][exemdl])][exemdl];
				}
			} else {
				ipid = UVCS_C_NOEL_EXCTRL;
				reqlist = &inr->req_list_unbind[exemdl];
			}
			if (reqlist == NULL) {
				/* invalid codec setting */
				result = UVCS_RTN_PARAMETER_ERROR;
				UVCS_CMN_DUMP(inr, UVCS_CAPI_REQ0, UVCS_CP3((UVCS_U32)result), 0);
			}

			if (result == UVCS_RTN_OK) {
				item = &hdl->request[exemdl];
				if (item->info != NULL) {
					result = UVCS_RTN_BUSY;
					UVCS_CMN_DUMP(inr, UVCS_CAPI_REQ0, UVCS_CP3((UVCS_U32)result), 0);
				}
			}
			if (result == UVCS_RTN_OK) {
				UVCS_CMN_DUMP(inr, UVCS_CAPI_REQ1, UVCS_CP3(req_info->module_id), req_info->req_serial);

				/* store request info */
				item->info = req_info;
				item->owner = hdl;
				item->target_ipid = ipid;
				uvcs_c_reqlist_add_item(reqlist, item,
					(req_info->cmd_param[UVCS_CMN_HWPIDX_REQ_TMG] == 0) ? UVCS_TRUE : UVCS_FALSE);

				/* execute it */
				inr->init_param.cb_thr_event(inr->init_param.udptr);

				result = UVCS_RTN_OK;
			}

			inr->init_param.cb_sem_unlock(inr->init_param.udptr);
		}
	}

	return result;
}


/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_execute
 *
 * Return
 *      \retval UVCS_RTN_NOT_INITIALISE
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_CONTINUE
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_execute(
	UVCS_CMN_LIB_INFO	 lib_info, /**< [in] library information */
	UVCS_U32		 cur_time /**< [in] current time for debug */
	)
{
	UVCS_RESULT	 result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO		*inr = (struct UVCS_C_INR_INFO *)lib_info;
	struct UVCS_C_HW_EXCTRL		*exctrl;
	UVCS_U32					 gid;
	UVCS_U32					 exemdl;

	/** (1) initialize check */
	if ((inr != NULL)
	&& (inr->address == lib_info)
	&& (inr->init_param.cb_sem_lock != NULL)
	&& (inr->init_param.cb_sem_unlock != NULL)) {

		/** (2) semaphore lock */
		if (inr->init_param.cb_sem_lock(inr->init_param.udptr) == UVCS_FALSE) {
			result = UVCS_RTN_CONTINUE;

		} else {
			/** (3) change hw_state and output log */
			for (gid = 0u; gid < inr->init_param.hw_num; gid++) {
				exctrl = &inr->hw_exctrl[gid];
				for (exemdl = 0; exemdl < UVCS_C_NOEL_EXEMDL; exemdl++) {
					if (exctrl->hwp_state[exemdl] == UVCS_C_HWP_FINISH) {
						uvcs_c_hwctrl_stop_processing(inr, exctrl->cur_item[exemdl], exemdl, cur_time);
						exctrl->hwp_state[exemdl] = UVCS_C_HWP_NONE;
						exctrl->cur_item[exemdl] = NULL;
					}
				}
				if (exctrl->reset_req == UVCS_TRUE) {
					if (inr->init_param.fcpc_indep_mode == UVCS_TRUE) {
						inr->fcpc_reset_all = UVCS_TRUE;
						exctrl->reset_req = UVCS_FALSE;
					} else {
						uvcs_c_hwctrl_fcpc_reset_one(inr, gid);
					}
				}
			}
			if (inr->fcpc_reset_all == UVCS_TRUE) {
				uvcs_c_hwctrl_fcpc_reset_all(inr);
			}

			/* search requested item from reqlist and execute it */
			for (gid = 0u; gid < inr->init_param.hw_num; gid++) {
				exctrl = &inr->hw_exctrl[gid];
				for (exemdl = 0; exemdl < UVCS_C_NOEL_EXEMDL; exemdl++) {
					uvcs_c_execute(inr, gid, exemdl, cur_time, UVCS_TRUE);
					uvcs_c_execute(inr, gid, exemdl, cur_time, UVCS_FALSE);
				}
			}

			/** (4) semaphore unlock */
			inr->init_param.cb_sem_unlock(inr->init_param.udptr);
			result = UVCS_RTN_OK;
		}
	}

	return result;
}


/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_get_work_size
 *
 * Return
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_get_work_size(
	UVCS_U32	*work_mem_0_size,
	UVCS_U32	*hdl_work_0_size
	)
{
	UVCS_RESULT	 result;

	/** (1) parameter check */
	if ((work_mem_0_size == NULL)
	|| (hdl_work_0_size == NULL)) {
		result = UVCS_RTN_PARAMETER_ERROR;
	} else {
		*work_mem_0_size = sizeof(struct UVCS_C_INR_INFO);
		*hdl_work_0_size = sizeof(struct UVCS_C_HANDLE);
		result = UVCS_RTN_OK;
	}

	return result;
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_cmn_get_ip_info
 *
 * Return
 *		\retval UVCS_RTN_NOT_INITIALISE
 *      \retval UVCS_RTN_PARAMETER_ERROR
 *      \retval UVCS_RTN_OK
 *
 ****************************************************************/
UVCS_RESULT uvcs_cmn_get_ip_info(
	UVCS_CMN_LIB_INFO		 lib_info,
	UVCS_CMN_IP_INFO_T		*ip_info
)
{
	UVCS_RESULT result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO *inr = (struct UVCS_C_INR_INFO *)lib_info;

	/** (1) parameter check */
	if ((inr != NULL)
	&& (inr->address == lib_info)) {
		if ((ip_info == NULL)
		|| (ip_info->struct_size < sizeof(struct UVCS_CMN_IP_INFO))) {
			result = UVCS_RTN_PARAMETER_ERROR;
		} else {
			ip_info->struct_size = sizeof(struct UVCS_CMN_IP_INFO);
			ip_info->ip_version = inr->ip_version;
			ip_info->ip_option = inr->init_param.ip_option;
			result = UVCS_RTN_OK;
		}
	}

	return result;
}

UVCS_RESULT uvcs_cmn_get_ip_capability(
	UVCS_CMN_LIB_INFO			 lib_info,
	UVCS_CMN_IP_CAPABILITY_T	*ip_cap
)
{
	UVCS_RESULT result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO *inr = (struct UVCS_C_INR_INFO *)lib_info;
	UVCS_U32 i;

	if ((inr != NULL)
	&& (inr->address == lib_info)) {
		if ((ip_cap == NULL)
		|| (ip_cap->struct_size < sizeof(struct UVCS_CMN_IP_CAPABILITY))) {
			result = UVCS_RTN_PARAMETER_ERROR;

		} else {
			ip_cap->struct_size = sizeof(struct UVCS_CMN_IP_CAPABILITY);
			for (i = 0; i < UVCS_C_NOEL_HW; i++) {
				ip_cap->ip_capability[i] = inr->ip_capability[i];
			}
			result = UVCS_RTN_OK;
		}
	}

	return result;
}

UVCS_RESULT uvcs_cmn_set_ip_binding(
	UVCS_CMN_LIB_INFO	lib_info,
	UVCS_CMN_HANDLE		handle,
	UVCS_BOOL			mode,
	UVCS_U32			ipid
	)
{
	UVCS_RESULT result = UVCS_RTN_NOT_INITIALISE;
	struct UVCS_C_INR_INFO *inr = (struct UVCS_C_INR_INFO *)lib_info;
	struct UVCS_C_HANDLE *hdl = (struct UVCS_C_HANDLE *)handle;

	if ((inr != NULL)
	&&	(inr->address == lib_info)
	&&	(inr->init_param.cb_sem_lock != NULL)
	&&	(inr->init_param.cb_sem_unlock != NULL)) {

		if ((hdl == NULL)
		||	(hdl->address != handle)) {
			result = UVCS_RTN_INVALID_HANDLE;

		} else if (inr->init_param.cb_sem_lock(inr->init_param.udptr) == UVCS_FALSE) {
			result = UVCS_RTN_CONTINUE;

		} else {
			if (mode == UVCS_TRUE) {
				if (ipid >= inr->init_param.hw_num) {
					result = UVCS_RTN_PARAMETER_ERROR;

				} else if (hdl->open_info.ip_binding_enable == UVCS_TRUE) {
					result = UVCS_RTN_BUSY;

				} else {
					hdl->open_info.ip_binding_enable = UVCS_TRUE;
					hdl->open_info.ip_binding_id = ipid;
					result = UVCS_RTN_OK;
				}
			} else {
				hdl->open_info.ip_binding_enable = UVCS_FALSE;
				hdl->open_info.ip_binding_id = UVCS_C_NOEL_EXCTRL;
				result = UVCS_RTN_OK;
			}

			inr->init_param.cb_sem_unlock(inr->init_param.udptr);
		}
	}

	return result;
}

/******************************************************************************/
/*                    INTERNAL FUNCTIONS                                      */
/******************************************************************************/

/************************************************************//**
 *
 * Function Name
 *      uvcs_c_mng_inr_param
 *
 * Return
 *      \return pointer of local parameter
 *
 * Description
 *     \brief Set and Get a pointer of local parameter.
 *
 ****************************************************************/
UVCS_STATIC struct UVCS_C_INR_INFO *uvcs_c_mng_inr_param(
	UVCS_BOOL		 mng_mode,	/**< [in] TRUE: change to new pointer */
	struct UVCS_C_INR_INFO	*new_ptr	/**< [in] new pointer value */
)
{
	static struct UVCS_C_INR_INFO *inr_param;

	if (mng_mode == UVCS_TRUE) {
		inr_param = new_ptr;
	}

	return inr_param;
}



/************************************************************//**
 *
 * Function Name
 *      uvcs_c_reg_read
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief Read data from register. This function is callback function for driver.
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_event_reg_read(
	MCVX_REG		*reg_addr,	/**< [in] pointer of target register */
	MCVX_U32		*dst_addr,	/**< [out] register value */
	MCVX_U32		 num_reg	/**< [in] number of register to read */
)
{
	struct UVCS_C_INR_INFO *inr = uvcs_c_mng_inr_param(UVCS_FALSE, NULL);

	if ((inr != NULL)
	&& (inr->init_param.cb_reg_read != NULL)) {
		(*inr->init_param.cb_reg_read)(inr->init_param.udptr, reg_addr, dst_addr, num_reg);
	}
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_c_reg_write
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief Write data to register. This function is callback function for driver.
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_event_reg_write(
	MCVX_REG	*reg_addr,	/**< [in] pointer of target register */
	MCVX_U32	*src_addr,	/**< [in] source data */
	MCVX_U32	 num_reg	/**< [in] number of register to write */
)
{
	struct UVCS_C_INR_INFO *inr = uvcs_c_mng_inr_param(UVCS_FALSE, NULL);

	if ((inr != NULL)
	&& (inr->init_param.cb_reg_write != NULL)) {
		(*inr->init_param.cb_reg_write)(inr->init_param.udptr, reg_addr, src_addr, num_reg);
	}
}


UVCS_STATIC UVCS_U32 uvcs_c_get_executable_ipid(
		struct UVCS_C_INR_INFO	*inr,
		UVCS_U32				 grpid,
		UVCS_U32				 exemdl,
		UVCS_U32				 codec
		)
{
	UVCS_U32 i = UVCS_C_NOEL_HW;

	if (inr != NULL) {
		for (i = 0; i < inr->init_param.hw_num; i++) {
			if (inr->init_param.ip_group_id[i][exemdl] == grpid) {
				if (uvcs_c_check_codec_executable(inr, i, codec) == UVCS_TRUE) {
					break;
				}
			}
		}
	}

	return i;
}

UVCS_STATIC UVCS_BOOL uvcs_c_check_codec_executable(
		struct UVCS_C_INR_INFO	*inr,
		UVCS_U32				 ipid,
		UVCS_U32				 codec
		)
{
	UVCS_BOOL result = UVCS_FALSE;

	if (inr != NULL) {
		switch (inr->init_param.ip_arch[ipid]) {
		case UVCS_CMN_IPARCH_BELZ:
			if ((codec == MCVX_H265) || (codec == MCVX_H264)) {
				result = UVCS_TRUE;
			}
			break;
		default:
			break;
		}
	}
	return result;
}

UVCS_STATIC void uvcs_c_execute(
		struct UVCS_C_INR_INFO	*inr,
		UVCS_U32				 grpid,
		UVCS_U32				 exemdl,
		UVCS_U32				 cur_time,
		UVCS_BOOL				 search_from_binded
		)
{
	struct UVCS_C_REQ_LIST *reqlist;
	struct UVCS_C_REQ_ITEM *candidate;

	if ((inr != NULL)
	&&	(inr->fcpc_reset_all == UVCS_FALSE)
	&&	(inr->hw_exctrl[grpid].reset_req == UVCS_FALSE)
	&&	(inr->hw_exctrl[grpid].cur_item[exemdl] == NULL)) {

		if (search_from_binded == UVCS_TRUE) {
			/* search item from binded-reqlist */
			reqlist = &inr->req_list[grpid][exemdl];
		} else {
			/* search item from unbinded-reqlist */
			reqlist = &inr->req_list_unbind[exemdl];
		}

		/* search item from binded-reqlist */
		candidate = uvcs_c_reqlist_get_candidate(inr, reqlist, grpid, exemdl);
		/* starts a hardware processing for the acquired item */
		if (candidate != NULL) {
			inr->hw_exctrl[grpid].cur_item[exemdl] = candidate;
			uvcs_c_hwctrl_start_processing(inr, candidate,
					&inr->hw_exctrl[grpid].hwp_state[exemdl], exemdl, cur_time);
		}
	}
}

UVCS_STATIC struct UVCS_C_REQ_ITEM *uvcs_c_reqlist_get_candidate(
		struct UVCS_C_INR_INFO	*inr,
		struct UVCS_C_REQ_LIST	*list,
		UVCS_U32				 grpid,
		UVCS_U32				 exemdl
		)
{
	struct UVCS_C_REQ_ITEM *ret = NULL;

	if (inr != NULL) {
		if (exemdl == UVCS_CMN_MDL_VLC) {
			ret = uvcs_c_reqlist_peek(list, UVCS_TRUE);
		} else {
			ret = uvcs_c_reqlist_peek(list, UVCS_FALSE);
		}

		/* check executable */
		if (ret != NULL) {
			ret->target_ipid = uvcs_c_get_executable_ipid(inr, grpid, exemdl, ret->info->cmd_param[UVCS_CMN_HWPIDX_REQ_COD]);
			if (ret->target_ipid >= inr->init_param.hw_num) {
				/* not executable */
				ret = NULL;
			}
		}

		/* remove from list */
		if (ret != NULL) {
			uvcs_c_reqlist_remove_item(list, ret);
		}
	}

	return ret;
}


UVCS_STATIC struct UVCS_C_REQ_ITEM *uvcs_c_reqlist_peek(
		struct UVCS_C_REQ_LIST	*list,
		UVCS_BOOL				 fifomode
		)
{
	struct UVCS_C_REQ_ITEM *result = NULL;
	struct UVCS_C_REQ_ITEM *temp;
	UVCS_U32 min = 0xFFFFFFFFu;

	if (list != NULL) {
		if (fifomode == UVCS_TRUE) {
			result = list->head;
		} else {
			temp = list->head;
			while (temp != NULL) {
				if ((temp->info != NULL)
				&&	(temp->info->cmd_param[UVCS_CMN_HWPIDX_REQ_TMG] < min)) {
					result = temp;
					min = temp->info->cmd_param[UVCS_CMN_HWPIDX_REQ_TMG];
				}
				temp = temp->next;
			}
		}
	}

	return result;
}



UVCS_STATIC void uvcs_c_reqlist_add_item(
		struct UVCS_C_REQ_LIST	*list,
		struct UVCS_C_REQ_ITEM	*item,
		UVCS_BOOL				 priority
		)
{
	if ((list != NULL)
	&&	(item != NULL)) {
		if ((list->head == NULL)
		||	(list->tail == NULL)) {
			/* first */
			item->next = NULL;
			item->prev = NULL;
			list->head = item;
			list->tail = item;

		} else if (priority == UVCS_TRUE) {
			/* add to head */
			item->next = list->head;
			item->prev = NULL;
			list->head->prev = item;
			list->head = item;

		} else {
			/* add to tail */
			item->next = NULL;
			item->prev = list->tail;
			list->tail->next = item;
			list->tail = item;
		}
	}
}

UVCS_STATIC void uvcs_c_reqlist_remove_item(
		struct UVCS_C_REQ_LIST		*list,
		struct UVCS_C_REQ_ITEM		*item
		)
{
	struct UVCS_C_REQ_ITEM *temp;

	if ((list) && (item)) {
		temp = list->head;
		while (temp != NULL) {
			if (temp == item) {
				if (temp->prev != NULL) {
					temp->prev->next = temp->next;
				}
				if (temp->next != NULL) {
					temp->next->prev = temp->prev;
				}
				if (list->head == temp) {
					list->head = temp->next;
				}
				if (list->tail == temp) {
					list->tail = temp->prev;
				}
			}
			temp = temp->next;
		}
	}
}

UVCS_STATIC void uvcs_c_reqlist_remove_owner(
		struct UVCS_C_REQ_LIST		*list,
		struct UVCS_C_HANDLE		*handle
		)
{
	struct UVCS_C_REQ_ITEM *temp;

	if ((list) && (handle)) {
		temp = list->head;
		while (temp != NULL) {
			if (temp->owner == handle) {
				if (temp->prev != NULL) {
					temp->prev->next = temp->next;
				}
				if (temp->next != NULL) {
					temp->next->prev = temp->prev;
				}
				if (list->head == temp) {
					list->head = temp->next;
				}
				if (list->tail == temp) {
					list->tail = temp->prev;
				}
				temp->info = NULL;
			}
			temp = temp->next;
		}
	}
}


/************************************************************//**
 *
 * Function Name
 *      uvcs_c_hwctrl_start_processing
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief start hardware processing
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_hwctrl_start_processing(
	struct UVCS_C_INR_INFO			*inr,
	struct UVCS_C_REQ_ITEM			*item,
	UVCS_U32						*hwpstate,
	UVCS_U32						 exemdl,
	UVCS_U32						 dbg_curtime
	)
{
	if ((inr != NULL)
	&&	(item != NULL)
	&&	(item->info != NULL)) {
		void *baa_info_addr = &item->info->cmd_param[UVCS_CMN_HWPIDX_REQ_BAA];
		void *exe_info_addr = &item->info->cmd_param[UVCS_CMN_HWPIDX_REQ_DAT];

		if (inr->init_param.cb_hw_start != NULL) {
			inr->init_param.cb_hw_start(inr->init_param.udptr, item->target_ipid,
				exemdl, &item->info->cmd_param[UVCS_CMN_HWPIDX_REQ_BAA]);
		}
		item->dbg_execnt++;
		item->dbg_s_time = dbg_curtime;

		if (exemdl == UVCS_CMN_MDL_VLC) {
			MCVX_VLC_BAA_T *vlc_baa = (MCVX_VLC_BAA_T *)baa_info_addr;
			MCVX_VLC_EXE_T *vlc_exe = (MCVX_VLC_EXE_T *)exe_info_addr;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_RUNV, item->target_ipid,      0);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAV0, vlc_baa->irp_p_addr,     vlc_baa->list_item_p_addr);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAV1, vlc_baa->imc_buff_addr,  vlc_baa->ims_buff_addr[0]);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAV2, vlc_baa->str_es_addr[0], vlc_baa->str_es_size[0]);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAV3, vlc_baa->str_es_addr[1], vlc_baa->str_es_size[1]);
			/* start hardware */
			(void)mcvx_vlc_start(&inr->ip_context[item->target_ipid], hwpstate,
					(MCVX_UDF_VLC_EVENT_T)&uvcs_c_event_vlc_state, vlc_exe);

		} else if (item->info->module_id == UVCS_CMN_MDL_FCV) {
			/* do nothing */

		} else {
			MCVX_CE_BAA_T *ce_baa = (MCVX_CE_BAA_T *)baa_info_addr;
			MCVX_CE_EXE_T *ce_exe = (MCVX_CE_EXE_T *)exe_info_addr;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_RUNC, item->target_ipid,    0);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAC0, ce_baa->irp_p_addr,    ce_baa->img_ref_num);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAC1, ce_baa->imc_buff_addr, ce_baa->ims_buff_addr[0]);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAC2, ce_baa->stride_dec,    ce_baa->stride_ref);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_BAC3, ce_baa->stride_enc_y,  ce_baa->stride_enc_c);
			/* start hardware */
			(void)mcvx_ce_start(&inr->ip_context[item->target_ipid], hwpstate,
					(MCVX_UDF_CE_EVENT_T)&uvcs_c_event_ce_state, ce_exe);
		}
	}
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_c_hwctrl_stop_processing
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief stop hardware processing
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_hwctrl_stop_processing(
	struct UVCS_C_INR_INFO			*inr,
	struct UVCS_C_REQ_ITEM			*item,
	UVCS_U32						 exemdl,
	UVCS_U32						 dbg_curtime
	)
{
	if ((inr != NULL)
	&&	(item != NULL)
	&&	(item->info != NULL)) {
		void *res_info_addr = &item->info->cmd_param[UVCS_CMN_HWPIDX_RES_DAT];
		if (exemdl == UVCS_CMN_MDL_VLC) {
			MCVX_VLC_RES_T *vlc_res = (MCVX_VLC_RES_T *)res_info_addr;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_ENDV, item->target_ipid, 0);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_ENDT, item->dbg_s_time, dbg_curtime);
			(void)mcvx_vlc_stop(&inr->ip_context[item->target_ipid], vlc_res);
			/* write result */
			item->info->module_id      = UVCS_CMN_MDL_VLC; /* by way of safety */
			item->info->cmd_param_noel = ((sizeof(MCVX_VLC_RES_T) + 3u) >> 2u) + UVCS_CMN_HWP_NOEL_BASE;
			item->info->cmd_param[UVCS_CMN_HWPIDX_RES_STA] = vlc_res->error_level;
			item->info->cmd_param[UVCS_CMN_HWPIDX_RES_CVL] = item->dbg_execnt;
			item->info->cmd_param[UVCS_CMN_HWPIDX_RES_EST] = item->dbg_s_time;

		} else if (item->info->module_id == UVCS_CMN_MDL_FCV) {
			/* write result */
			item->info->module_id      = UVCS_CMN_MDL_FCV; /* by way of safety */

		} else {
			MCVX_CE_RES_T *ce_res = (MCVX_CE_RES_T *)res_info_addr;
			UVCS_CMN_DUMP(inr, UVCS_CAPI_ENDC, item->target_ipid, 0);
			UVCS_CMN_DUMP(inr, UVCS_CAPI_ENDT, item->dbg_s_time, dbg_curtime);
			(void)mcvx_ce_stop(&inr->ip_context[item->target_ipid], ce_res);
			/* write result */
			item->info->module_id      = UVCS_CMN_MDL_CE; /* by way of safety */
			item->info->cmd_param_noel = ((sizeof(MCVX_CE_RES_T) + 3u) >> 2u) + UVCS_CMN_HWP_NOEL_BASE;
			item->info->cmd_param[UVCS_CMN_HWPIDX_RES_STA] = ce_res->error_code;
			item->info->cmd_param[UVCS_CMN_HWPIDX_RES_CCE] = item->dbg_execnt;
			item->info->cmd_param[UVCS_CMN_HWPIDX_RES_EST] = item->dbg_s_time;
		}
		if (inr->init_param.cb_hw_stop != NULL) {
			inr->init_param.cb_hw_stop(inr->init_param.udptr, item->target_ipid, exemdl);
		}

		/* write remain result data */
		item->info->struct_size = sizeof(UVCS_CMN_HW_PROC_T);
		item->info->cmd_param[UVCS_CMN_HWPIDX_RES_EET] = item->dbg_e_time;

		if ((inr->init_param.cb_proc_done != NULL)
		&&	(item->owner != NULL)) {
			inr->init_param.cb_proc_done(inr->init_param.udptr, item->owner->open_info.hdl_udptr,
				(UVCS_CMN_HANDLE)item->owner, item->info);
			item->info = NULL;
		}
	}
}

/**
 * \brief reset target fcpc module
 *
 */
UVCS_STATIC void uvcs_c_hwctrl_fcpc_reset_one(
	struct UVCS_C_INR_INFO		*inr,
	UVCS_U32					 exemdl
)
{
	UVCS_U32 i;

	if ((inr != NULL)
	&&	(inr->hw_exctrl[exemdl].cur_item[UVCS_CMN_MDL_VLC] == NULL)
	&&	(inr->hw_exctrl[exemdl].cur_item[UVCS_CMN_MDL_CE] == NULL)) {
		/* reset VCP */
		for (i = 0; i < inr->init_param.hw_num; i++) {
			if ((inr->init_param.ip_group_id[i][exemdl] == exemdl)
			&&	(inr->init_param.cb_hw_reset != NULL)) {
				inr->init_param.cb_hw_reset(inr->init_param.udptr, i);
			}
		}
		/* reset FCPC */
		(void)mcvx_fcpc_init(&inr->fcpc_config[exemdl], &inr->fcpc_context[exemdl]);
		inr->hw_exctrl[exemdl].reset_req = UVCS_FALSE;
	}
}

/**
 * \brief reset all fcpc module for independet operation mode
 *
 */
UVCS_STATIC void uvcs_c_hwctrl_fcpc_reset_all(
	struct UVCS_C_INR_INFO		*inr
)
{
	UVCS_U32 id;
	UVCS_BOOL running = UVCS_FALSE;

	if (inr != NULL) {
		/* Checks an existence of hardware whose status is Running */
		for (id = 0; id < inr->init_param.hw_num; id++) {
			if ((inr->hw_exctrl[id].cur_item[UVCS_CMN_MDL_VLC] != NULL)
			||	(inr->hw_exctrl[id].cur_item[UVCS_CMN_MDL_CE] != NULL)) {
				running = UVCS_TRUE;
			}
		}
		if (running == UVCS_FALSE) {
			/* reset VCP */
			for (id = 0; id < inr->init_param.hw_num; id++) {
				if (inr->init_param.cb_hw_reset != NULL) {
					inr->init_param.cb_hw_reset(inr->init_param.udptr, id);
				}
			}
			/* reset FCP */
			for (id = 0; id < inr->init_param.fcpc_hw_num; id++) {
				(void)mcvx_fcpc_init(&inr->fcpc_config[id], &inr->fcpc_context[id]);
			}
			inr->fcpc_reset_all = UVCS_FALSE;
		}
	}
}


/************************************************************//**
 *
 * Function Name
 *      uvcs_c_event_vlc_state
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief Callback function for driver.
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_event_vlc_state(
		void		*udp,
		MCVX_U32	 vlc_event,
		void		*unused
		)
{
	UVCS_U32 *hwp_state = (UVCS_U32 *)udp;

	if (hwp_state != NULL) {
		switch (vlc_event) {
		case MCVX_EVENT_VLC_START:
			*hwp_state = UVCS_C_HWP_RUN;
			break;
		case MCVX_EVENT_REQ_VLC_STOP:
			*hwp_state = UVCS_C_HWP_FINISH;
			break;
		default: /* unknown event */
			/* do nothing */
			break;
		}
	}
}

/************************************************************//**
 *
 * Function Name
 *      uvcs_c_event_ce_state
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief Callback function for driver.
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_event_ce_state(
		void		*udp,
		MCVX_U32	 ce_event,
		void		*unused
		)
{
	UVCS_U32 *hwp_state = (UVCS_U32 *)udp;

	if (hwp_state != NULL) {
		switch (ce_event) {
		case MCVX_EVENT_CE_START:
			*hwp_state = UVCS_C_HWP_RUN;
			break;
		case MCVX_EVENT_REQ_CE_STOP:
			*hwp_state = UVCS_C_HWP_FINISH;
			break;
		default: /* unknown event */
			/* do nothing */
			break;
		}
	}
}


/************************************************************//**
 *
 * Function Name
 *      uvcs_c_memclr4
 *
 * Return
 *      \return none
 *
 * Description
 *     \brief Clear target memory area.
 *
 ****************************************************************/
UVCS_STATIC void uvcs_c_memclr4(
	void		*dst_mem, /**< [io] destination address */
	UVCS_U32	 num_bytes /**< [in] length of memory area */
	)
{
	UVCS_U32	 cnt;
	UVCS_U32	*dst_ptr = (UVCS_U32 *)dst_mem;

	if (dst_mem != NULL) {
		num_bytes >>= 2u;

		for (cnt = 0u; cnt < num_bytes; cnt++) {
			/* substitute of the pointer arithmetic. */
			dst_ptr[cnt] = 0u;
		}
	}
}
