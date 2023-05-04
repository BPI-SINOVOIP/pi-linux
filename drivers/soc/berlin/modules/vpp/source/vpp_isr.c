// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#define _VPP_ISR_C_
#include <linux/kthread.h>
#include "vpp_defines.h"
#include "vpp_api.h"
#include "hal_vpp_wrap.h"
#include "avio_dhub_cfg.h"
#include "avio_dhub_drv.h"

#include "vpp_intrs.h"

#define VPP_ISR_MSGQ_SIZE 64

static AMPMsgQ_t hVPPMsgQ;
static struct semaphore vpp_sem;
static struct task_struct *vpp_isr_task;

static int VPP_IRQ_Handler(unsigned int irq, void *dev_id)
{
	int rc;
	int intr_num;
	MV_CC_MSG_t msg;

	intr_num = ffs(irq) - 1;
	msg.m_MsgID = VPP_CC_MSG_TYPE_VPP;
	msg.m_Param2 = 0;
	msg.m_Param1 = bSETMASK(intr_num);
	rc = AMPMsgQ_Add(&hVPPMsgQ, &msg);
	if (likely(rc == S_OK))
		up(&vpp_sem);

	return IRQ_HANDLED;
}

static int VPP_ISR_Task(void *param)
{
	MV_CC_MSG_t msg;
	HRESULT rc = MV_VPP_OK;

	while (!kthread_should_stop()) {
		rc = down_interruptible(&vpp_sem);
		if (unlikely(rc < 0))
			return rc;
		rc = AMPMsgQ_ReadTry(&hVPPMsgQ, &msg);
		if (unlikely(rc != S_OK)) {
			pr_err("%s:%d Failed to read from msgQ\n", __func__, __LINE__);
			return -EFAULT;
		}
		AMPMsgQ_ReadFinish(&hVPPMsgQ);
		wrap_MV_VPPOBJ_IsrHandler(msg.m_MsgID, msg.m_Param1);
	}

	return 0;
}

void VPP_CreateISRTask(void)
{
	unsigned int err;

	sema_init(&vpp_sem, 0);
	err = AMPMsgQ_Init(&hVPPMsgQ, VPP_ISR_MSGQ_SIZE);
        if (unlikely(err != S_OK))
                pr_err("%s:%d: VPP MsgQ init FAILED, err:%8x\n", __func__, __LINE__, err);
	vpp_isr_task = kthread_run(VPP_ISR_Task, NULL, "VPP ISR Thread");
	if (IS_ERR(vpp_isr_task))
		return;
}

void VPP_StopISRTask(void)
{
	unsigned int err;
	MV_CC_MSG_t msg;

	kthread_stop(vpp_isr_task);
	do {
		err = AMPMsgQ_DequeueRead(&hVPPMsgQ, &msg);
	} while (likely(err == 1));
	sema_init(&vpp_sem, 0);
	err = AMPMsgQ_Destroy(&hVPPMsgQ);
	if (unlikely(err != S_OK))
		pr_err("%s:%d: VPP MsgQ Destroy FAILED, err:%8x\n", __func__, __LINE__, err);
}

void VPP_EnableDhubInterrupt(bool enable)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(vpp_intrs); i++) {
		Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB, vpp_intrs[i], NULL, (enable ? VPP_IRQ_Handler : NULL));
	}
}
