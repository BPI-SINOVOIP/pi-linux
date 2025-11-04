/*************************************************************************/ /*
 UVCS Driver (kernel module)

 Copyright (C) 2015 - 2024 Renesas Electronics Corporation

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

#ifndef UVCS_LKM_INTERNAL_H
#define UVCS_LKM_INTERNAL_H

#define DEVNAME                 "uvcs"
#define DRVNAME                 DEVNAME
#define CLSNAME                 DEVNAME
#define DEVNUM                  1

/* base address */
#define UVCS_REG_VCPLF_VLC      (0xFE910000u)
#define UVCS_REG_VCPLF_CE       (0xFE910200u)
#define UVCS_REG_VCPL4_VLC      (0xFE8F0000u)
#define UVCS_REG_VCPL4_CE       (0xFE8F0200u)
#define UVCS_REG_VDPB_VLC       (0xFE900000u)
#define UVCS_REG_VDPB_CE        (0xFE900200u)
#define UVCS_REG_FCPCS          (0xFE90F000u)
#define UVCS_REG_FCPCI          (0xFE8FF000u)

/* interrupt */
#define UVCS_INT_OFFSET         (32u)
#define UVCS_INT_VCPLF_VLC      (260u + UVCS_INT_OFFSET)
#define UVCS_INT_VCPLF_CE       (261u + UVCS_INT_OFFSET)
#define UVCS_INT_VCPL4_VLC      (258u + UVCS_INT_OFFSET)
#define UVCS_INT_VCPL4_CE       (259u + UVCS_INT_OFFSET)
#define UVCS_INT_VDPB_VLC       (240u + UVCS_INT_OFFSET)
#define UVCS_INT_VDPB_CE        (241u + UVCS_INT_OFFSET)

#define UVCS_REG_SRCR           (0xE61500a8u)
#define UVCS_REG_SRSTCLR        (0xE6150944u)
#define UVCS_LSITYPE_H3         (0x4Fu)
#define UVCS_LSITYPE_M3W        (0x52u)
#define UVCS_LSITYPE_M3N        (0x55u)
#define UVCS_LSITYPE_E3         (0x57u)
#define UVCS_LSITYPE_V2MA       (0x5Au)
#define UVCS_LSITYPE_V2H        (0x5Bu)
#define UVCS_LSITYPE_V2N        (0x5Du)
#define UVCS_IPOPT_DEFAULT      (0x000000Au)

/* register size */
#define UVCS_REG_SIZE_SINGLE    (0x4u)
#define UVCS_REG_SIZE_VLC       (0x200u)
#define UVCS_REG_SIZE_CE        (0x200u)
#define UVCS_REG_SIZE_FCPC      (0x200u)
#define UVCS_REQ_NONE           (0x00000000u)
#define UVCS_REQ_USED           (0x00000001u)
#define UVCS_REQ_END            (0x80000000u)
#define UVCS_TIMEOUT_DEFAULT    (HZ*10u)
#define UVCS_TIMEOUT_TIMER      (HZ/5u) /* 200msec */
#define UVCS_TIMEOUT_RESET      (10u)
#define UVCS_DEBUG_BUFF_SIZE    (0x12C00u)
#define UVCS_DEBUG_PRINT_EXE    (0x4u)
#define UVCS_DEBUG_PRINT_VLC    (0x1u)
#define UVCS_DEBUG_PRINT_CE     (0x2u)

#define UVCS_CLOSE_WAIT_MAX     (10u)  /* timeout = n * CLOSE_WAIT_TIME */
#define UVCS_CLOSE_WAIT_TIME    (10u)  /* msec time (sleep wait) */
#define UVCS_VCPREG_IRQENB      (0x10u)

#define UVCS_FCP_DEVNUM         (3u)
#define UVCS_VCP_DEVNUM         (3u)
#define UVCS_MAX_DEVNUM         (6u)

#define UVCS_DT_COMPAT_BASE		"renesas,vcp4-"
#define UVCS_DT_VCPLF			"vcplf"
#define UVCS_DT_VCPL4			"vcpl4"
#define UVCS_DT_VDPB			"vdpb"
#define UVCS_DT_FCPCS			"fcpcs"
#define UVCS_DT_FCPCI0			"fcpci0"
#define UVCS_DT_FCPCI1			"fcpci1"
/******************************************************************************/
/*                    structures                                              */
/******************************************************************************/
struct uvcs_req_ctrl {
	UVCS_CMN_HW_PROC_T       data;
	ulong                    state;
	ulong                    time;
};

struct uvcs_hdl_info {
	UVCS_U32                 id; /* handle serial */
	struct uvcs_driver_info *drv;
	wait_queue_head_t        waitq;
	struct semaphore         sem;
	rwlock_t                 rwlock;
	UVCS_CMN_HANDLE          uvcs_hdl;
	void                    *uvcs_hdl_work;
	struct uvcs_req_ctrl     req_data[UVCS_CMN_PROC_REQ_MAX];
};

struct uvcs_thr_ctrl {
	struct task_struct      *thread;
	wait_queue_head_t        evt_wait_q;
	bool                     evt_req;
};

struct uvcs_vcp_hwinf {
	u64						 pa_vlc;
	u64						 pa_ce;
	int						 irq_vlc;
	int						 irq_ce;
	void					 *reg_vlc;
	void					 *reg_ce;
	char					 irq_name_vlc[64];
	char					 irq_name_ce[64];
	u32						 iparch;
	u32						 ipgroup;
	bool					 irq_enable;
	u64                      timer_vlc_data;
	u64                      timer_ce_data;

	struct platform_device	*pdev;
	struct timer_list       timer_vlc;
	struct timer_list       timer_ce;
	spinlock_t              slock_vlc;
	spinlock_t              slock_ce;
};

struct uvcs_fcp_hwinf {
	u64						 pa_fcp;
	void					*reg_fcp;
	u32						 iparch;
	bool					 irq_enable;

	struct platform_device	*pdev;
};

struct uvcs_driver_info {
	u32                     devnum;
	u32                     vcp_devnum;
	u32                     fcp_devnum;
	struct uvcs_vcp_hwinf   vcpinf[UVCS_VCP_DEVNUM];
	struct uvcs_fcp_hwinf   fcpinf[UVCS_FCP_DEVNUM];
	struct platform_device *pdev[UVCS_MAX_DEVNUM];

	struct semaphore        cdev_sem;
	struct semaphore        uvcs_sem;
	struct uvcs_thr_ctrl    thread_ctrl;

	u32                     counter;
	u32                     work_size_per_hdl;
	UVCS_CMN_INIT_PARAM_T   uvcs_init_param;
	UVCS_CMN_LIB_INFO       uvcs_info;
	UVCS_CMN_IP_INFO_T      ip_info;
	UVCS_CMN_LSI_INFO_T		lsi_info;
	UVCS_CMN_IP_CAPABILITY_T ip_cap;
};

struct uvcs_platform_data {
	u32					 device_id;
	bool				 device_vcp;
	void				*hwinf;
};

/******************************************************************************/
/*                    functions                                               */
/******************************************************************************/
int uvcs_io_init(struct uvcs_driver_info *drv);
void uvcs_io_deinit(struct uvcs_driver_info *drv);

void    uvcs_hw_processing_done(UVCS_PTR, UVCS_PTR,
			UVCS_CMN_HANDLE, UVCS_CMN_HW_PROC_T*);
UVCS_BOOL uvcs_semaphore_lock(UVCS_PTR);
void      uvcs_semaphore_unlock(UVCS_PTR);
UVCS_BOOL uvcs_semaphore_create(UVCS_PTR);
void      uvcs_semaphore_destroy(UVCS_PTR);
void      uvcs_thread_event(UVCS_PTR);
UVCS_BOOL uvcs_thread_create(UVCS_PTR);
void      uvcs_thread_destroy(UVCS_PTR);

/* resource */
int uvcs_get_vcp_resource(
				struct platform_device *pdev,
				struct uvcs_vcp_hwinf *vcpinf,
				u32 iparch,
				const char *name
				);
void uvcs_put_vcp_resource(
				struct uvcs_vcp_hwinf *vcpinf
				);
int uvcs_get_fcp_resource(
				struct platform_device *pdev,
				struct uvcs_fcp_hwinf *fcpinf,
				u32 iparch
				);
void uvcs_put_fcp_resource(
				struct uvcs_fcp_hwinf *fcpinf
				);

#endif /* UVCS_LKM_INTERNAL_H */
