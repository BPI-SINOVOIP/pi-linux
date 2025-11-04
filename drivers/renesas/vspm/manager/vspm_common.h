/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2015-2024 Renesas Electronics Corporation
 *
 * License        Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * GPLv2:
 * If you wish to use this file under the terms of GPL, following terms are
 * effective.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */ /*************************************************************************/

#ifndef __VSPM_COMMON_H__
#define __VSPM_COMMON_H__

/* Function ID of VSP manager */
enum VSPM_FUNCTION_ID {
	FUNC_VSPM_ENTRY = FUNC_TASK_RESUME + 1,
	EVENT_VSPM_DRIVER_ON_COMPLETE,
	FUNC_VSPM_CANCEL,
	FUNC_VSPM_FORCED_CANCEL,
	FUNC_VSPM_SET_MODE,
	EVENT_VSPM_DISPATCH,
	EVENT_VSPM_MAX
};

/* Max job */
#define VSPM_MAX_ELEMENTS			32

/* job state */
#define VSPM_JOB_STATUS_EMPTY		0
#define VSPM_JOB_STATUS_ENTRY		1
#define VSPM_JOB_STATUS_EXECUTING	2

/* set job ID from array index */
#define VSPM_SET_JOB_ID(entry_cnt, index) \
	(((entry_cnt) << 8) | ((index) + 1))

/* get array index from job ID */
#define VSPM_GET_ARRAY_INDEX(job_id) \
	(0xFF & ((job_id) - 1))

/* convert channel number to bit */
#define VSPM_CH_TO_BIT(ch) \
	((unsigned int)(0x1 << (ch)))

/* convert channel number to bit with invert */
#define VSPM_CH_TO_BIT_INVERT(ch) \
	(~VSPM_CH_TO_BIT(ch))

/* job information structure */
struct vspm_job_info {
	unsigned long status;
	unsigned long job_id;
	unsigned long ch_num;
	long result;
	struct vspm_api_param_entry entry;
	void *next_job_info;
};

/* job management structure */
struct vspm_job_manager {
	unsigned long entry_count;
	unsigned long write_index;
	struct vspm_job_info job_info[VSPM_MAX_ELEMENTS];
};

/* queue information structure */
struct vspm_queue_info {
	unsigned short data_count;
	void *first_job_info;
};

/* execution information structure */
struct vspm_exec_info {
	unsigned int exec_ch_bits;
	struct vspm_job_info *p_exec_job_info[VSPM_CH_MAX];
};

/* VSP resource information structure */
struct vspm_usable_vsp_res_info {
	unsigned int module_bits;
	unsigned int rpf_bits;
	unsigned int rpf_clut_bits;
	unsigned int wpf_rot_bits;
};

/* usable resource information structure */
struct vspm_usable_res_info {
	unsigned int ch_bits;
	unsigned int occupy_bits;
	struct vspm_usable_vsp_res_info vsp_res[VSPM_VSP_IP_MAX];
};

/* control information structure */
struct vspm_ctrl_info {
	struct vspm_job_manager job_manager;
	struct vspm_queue_info queue_info;
	struct vspm_exec_info exec_info;
	struct vspm_usable_res_info usable_info;
};

/* control functions */
long vspm_ins_ctrl_initialize(struct vspm_drvdata *pdrv);
long vspm_ins_ctrl_quit(struct vspm_drvdata *pdrv);
long vspm_ins_ctrl_suspend(struct vspm_drvdata *pdrv);
long vspm_ins_ctrl_resume(struct vspm_drvdata *pdrv);
long vspm_ins_ctrl_set_mode(struct vspm_api_param_mode *mode);
long vspm_ins_ctrl_regist_entry(struct vspm_api_param_entry *entry);
long vspm_ins_ctrl_exec_entry(struct vspm_api_param_entry *entry);
long vspm_ins_ctrl_on_complete(unsigned short module_id, long result);
long vspm_ins_ctrl_get_status(unsigned long job_id);
long vspm_ins_ctrl_queue_cancel(unsigned long job_id);
long vspm_ins_ctrl_forced_cancel(struct vspm_api_param_forced_cancel *cancel);
long vspm_ins_ctrl_cancel_entry(struct vspm_privdata *priv);
long vspm_ins_ctrl_mode_param_check(
	unsigned int *use_bits, struct vspm_api_param_mode *mode);
long vspm_ins_ctrl_entry_param_check(struct vspm_api_param_entry *entry);
long vspm_ins_ctrl_assign_channel(
	struct vspm_job_t *ip_par,
	struct vspm_request_res_info *request,
	struct vspm_usable_res_info *usable,
	unsigned short *ch);
unsigned int vspm_ins_ctrl_get_usable_vsp_ch_bits(
	struct vsp_start_t *vsp_par, struct vspm_usable_res_info *usable);
void vspm_ins_ctrl_dispatch(void);
void vspm_inc_ctrl_on_driver_complete(unsigned short module_id, long result);

/* job manager functions */
struct vspm_job_info *vspm_ins_job_entry(
	struct vspm_job_manager *job_manager,
	struct vspm_api_param_entry *entry);
struct vspm_job_info *vspm_ins_job_find_job_info(
	struct vspm_job_manager *job_manager, unsigned long job_id);
unsigned long vspm_ins_job_get_status(struct vspm_job_info *job_info);
long vspm_ins_job_cancel(struct vspm_job_info *job_info);
long vspm_ins_job_execute_start(
	struct vspm_job_info *job_info, unsigned long exec_ch);
long vspm_ins_job_execute_complete(
	struct vspm_job_info *job_info, long result, unsigned long comp_ch);
void vspm_ins_job_remove(struct vspm_job_info *job_info);
unsigned long vspm_ins_job_get_job_id(struct vspm_job_info *job_info);
struct vspm_job_t *vspm_ins_job_get_ip_param(struct vspm_job_info *job_info);
struct vspm_request_res_info *vspm_ins_job_get_request_param
	(struct vspm_job_info *job_info);

/* executing manager functions */
long vspm_ins_exec_start(
	struct vspm_exec_info *exec_info,
	unsigned short module_id,
	struct vspm_job_t *ip_par,
	struct vspm_job_info *job_info);
long vspm_ins_exec_complete(
	struct vspm_exec_info *exec_info, unsigned short module_id);
struct vspm_job_info *vspm_ins_exec_get_current_job_info(
	struct vspm_exec_info *exec_info, unsigned short module_id);
void vspm_ins_exec_update_current_status(
	struct vspm_exec_info *exec_info, struct vspm_usable_res_info *usable);
long vspm_ins_exec_cancel(
	struct vspm_exec_info *exec_info, unsigned short module_id);

/* sort queue functions */
long vspm_inc_sort_queue_initialize(struct vspm_queue_info *queue_info);
long vspm_inc_sort_queue_entry(
	struct vspm_queue_info *queue_info, struct vspm_job_info *job_info);
long vspm_inc_sort_queue_refer(
	struct vspm_queue_info *queue_info,
	unsigned short index,
	struct vspm_job_info **p_job_info);
long vspm_inc_sort_queue_remove(
	struct vspm_queue_info *queue_info, unsigned short index);
unsigned short vspm_inc_sort_queue_get_count(
	struct vspm_queue_info *queue_info);
long vspm_inc_sort_queue_find_item(
	struct vspm_queue_info *queue_info,
	struct vspm_job_info *job_info,
	unsigned short *p_index);

/* VSP control functions */
long vspm_ins_vsp_ch(unsigned short module_id, unsigned char *ch);
long vspm_ins_vsp_initialize(
	struct vspm_usable_res_info *usable, struct vspm_drvdata *pdrv);
long vspm_ins_vsp_execute(
	unsigned short module_id, struct vsp_start_t *vsp_par);
long vspm_ins_vsp_exec_complete(unsigned short module_id);
long vspm_ins_vsp_cancel(unsigned short module_id);
long vspm_ins_vsp_quit(struct vspm_usable_res_info *usable);
long vspm_ins_vsp_execute_low_delay(
	unsigned short module_id,
	struct vspm_api_param_entry *entry);
long vspm_ins_vsp_suspend(void);
long vspm_ins_vsp_resume(void);

/* FDP control functions */
long vspm_ins_fdp_ch(unsigned short module_id, unsigned char *ch);
long vspm_ins_fdp_initialize(
	struct vspm_usable_res_info *usable, struct vspm_drvdata *pdrv);
long vspm_ins_fdp_execute(
	unsigned short module_id,
	struct fdp_start_t *fdp_par,
	struct vspm_request_res_info *request);
long vspm_ins_fdp_exec_complete(unsigned short module_id);
long vspm_ins_fdp_cancel(unsigned short module_id);
long vspm_ins_fdp_quit(struct vspm_usable_res_info *usable);
long vspm_ins_fdp_execute_low_delay(
	unsigned short module_id,
	struct vspm_api_param_entry *entry);
long vspm_ins_fdp_suspend(void);
long vspm_ins_fdp_resume(void);

/* ISU control functions */
long vspm_ins_isu_ch(unsigned short module_id, unsigned char *ch);
long vspm_ins_isu_initialize(
	struct vspm_usable_res_info *usable, struct vspm_drvdata *pdrv);
long vspm_ins_isu_exec_complete(unsigned short module_id);
long vspm_ins_isu_cancel(unsigned short module_id);
long vspm_ins_isu_quit(struct vspm_usable_res_info *usable);
long vspm_ins_isu_execute(
	unsigned short module_id,
	struct isu_start_t *isu_par);
long vspm_ins_isu_execute_low_delay(
	unsigned short module_id,
	struct vspm_api_param_entry *entry);
long vspm_ins_isu_suspend(void);
long vspm_ins_isu_resume(void);
#endif /* __VSPM_COMMON_H__ */
