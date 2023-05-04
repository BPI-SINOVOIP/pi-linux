// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _DRV_VPP_H_
#define _DRV_VPP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/ion.h>

#include "drv_msg.h"
#include "avio_type.h"
#include "avio_ioctl.h"
#include "avio_debug.h"

#include "hal_dhub.h"

#include "avio_sub_module.h"

#define VPP_CLOCK_LIST_MAX		    16
#define VPP_INTERRUPT_LIST_MAX		32
#define VPP_ISR_MSGQ_SIZE			64
#define DEBUG_TIMER_VALUE			(0xFFFFFFFF)
#define VPP_CC_MSG_TYPE_VPP			0x00

typedef enum _VPP_INTR_TYPE_ {
	VPP_INTR_TYPE_NONE,
	VPP_INTR_TYPE_VBI,
	VPP_INTR_TYPE_VDE,
	VPP_INTR_TYPE_CPCB1_VBI,
	VPP_INTR_TYPE_CPCB1_VDE,
	VPP_INTR_TYPE_CPCB2_VBI,
	VPP_INTR_TYPE_CPCB2_VDE,
	VPP_INTR_TYPE_4KTG,
	VPP_INTR_TYPE_HDMI_TX,
	VPP_INTR_TYPE_HDMI_RX,
	VPP_INTR_TYPE_HPD,
	VPP_INTR_TYPE_HDCP,
	VPP_INTR_TYPE_HDMI_AUDIO,
	VPP_INTR_TYPE_HDMI_SPDIF,
	VPP_INTR_TYPE_OVP,
} VPP_INTR_TYPE;

typedef enum {
	AVIO_CONFIG_VSYNC_MEASURE = 0x01,
} AVIO_CONFIG;

typedef struct _VPP_INTERRUPT_ {
	int intr_num;
	int intr_type;
} VPP_INTR;

typedef struct avio_irq_profiler {
	unsigned long long vppCPCB0_intr_curr;
	unsigned long long vppCPCB0_intr_last;
	unsigned long long vpp_task_sched_last;
	unsigned long long vpp_isr_start;

	unsigned long long vpp_isr_end;
	unsigned long vpp_isr_time_last;

	unsigned long vpp_isr_time_max;
	unsigned long vpp_isr_instat_max;

	int vpp_isr_last_instat;

} avio_irq_profiler_t;

typedef struct _VPP_CONTEXT_ {
	unsigned int vpp_intr_status[MAX_INTR_NUM];
	atomic_t vpp_isr_msg_err_cnt;

	AMPMsgQ_t hVPPMsgQ;
	spinlock_t vpp_msg_spinlock;
	spinlock_t bcm_spinlock;
	struct semaphore vpp_sem;
	struct semaphore vsync_sem;
	struct semaphore vsync1_sem;
	atomic64_t vsynctime;
	atomic64_t vsync1time;
	int64_t lastvsynctime;
	int is_vsync_measure_enabled;
	unsigned int vsync_period_max;
	unsigned int vsync_period_min;
	int vpp_cpcb0_vbi_int_cnt;
	unsigned int vpp_intr_timestamp;
#ifdef CONFIG_IRQ_LATENCY_PROFILE
	avio_irq_profiler_t avio_irq_profiler;
#endif
	int is_spdifrx_enabled;

	VPP_INTR vpp_interrupt_list[VPP_INTERRUPT_LIST_MAX];
	int vpp_interrupt_list_count;
	int hpd_intr_num, vsync_intr_num;

	int instat;
	HDL_semaphore *pSemHandle;
	int vpp_intr;
	int instat_used;
	int hpd_debounce_delaycnt;

	void (*aout_resume_cb)(int path_id);
	void (*aip_resume_cb)(void);

	int is_bootup_quiescent;
	VPP_CLOCK clk_list[VPP_CLOCK_LIST_MAX];
	int clk_list_count;
	struct device *dev;
	struct extcon_dev *hdmitx_dev;
	struct gpio_desc *gpio_hdmitx_5v;
	struct gpio_desc *gpio_mipirst;
} VPP_CTX;


void drv_vpp_add_vpp_clock(VPP_CTX *hVppCtx, struct device_node *np,
		VPP_CLK_ID clk_id, char *clk_name);
void drv_vpp_add_vpp_interrupt_num(VPP_CTX *hVppCtx, int intr_num,
			int intr_type);
int avio_module_drv_vpp_probe(struct platform_device *dev);

HRESULT avio_devices_vpp_post_msg(VPP_CTX *hVppCtx, unsigned int msgId,
			unsigned int param1, unsigned int param2);
#endif //_DRV_VPP_H_

