// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/jiffies.h>

#include "drv_hrx.h"
#include "hdmirx.h"
#include "avio_io.h"
#include "avio_common.h"

#define HDMIRX_CLEAR_ALL_INT 0xFFFFFFFF

#define AVPUNIT1_INTR   ( \
	HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP3 | \
	HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP2 | \
	HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP1 | \
	HDMI_AVPUNIT_1_INT_STATUS_AUD_CHSTATUS_SP0 | \
	HDMI_AVPUNIT_1_INT_STATUS_AUD_MUTE         | \
	HDMI_AVPUNIT_1_INT_STATUS_AFIFO_UNDERFLOW  | \
	HDMI_AVPUNIT_1_INT_STATUS_AFIFO_OVERFLOW   | \
	HDMI_AVPUNIT_1_INT_STATUS_AFIFO_THR_PASS   )

#define MAINUNIT_0_INTR          ( \
	HDMI_MAINUNIT_0_INT_STATUS_TIMER_BASE_LOCKED | \
	HDMI_MAINUNIT_0_INT_STATUS_TMDSQP_CK_OFF    | \
	HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_OFF     | \
	HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_LOCKED )

#define MAINUNIT_2_INTR          (\
	HDMI_MAINUNIT_2_INT_STATUS_TMDSVALID_STABLE | \
	HDMI_MAINUNIT_2_INT_STATUS_AUDPLL_LOCK_STABLE )

#define HDCP_INTR           (\
	HDMI_HDCP_INT_STATUS_ENCDIS | \
	HDMI_HDCP_INT_STATUS_ENCEN  )

#define PKT_0_INTR          (\
	HDMI_PKT_0_INT_STATUS_DRMIF | \
	HDMI_PKT_0_INT_STATUS_SRCPDIF | \
	HDMI_PKT_0_INT_STATUS_AVIIF   | \
	HDMI_PKT_0_INT_STATUS_VSIF )

#define PKT_0_ACR           (\
	HDMI_PKT_0_INT_STATUS_ACR_N | \
	HDMI_PKT_0_INT_STATUS_ACR_CTS )


static inline struct hrx_priv *get_hrx_priv(struct device *dev)
{
	return (struct hrx_priv *)dev_get_drvdata(dev);
}

static void dw_hdmi_clear_all_ints(struct hrx_priv *hrx)
{
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_MAINUNIT_0_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_MAINUNIT_1_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_MAINUNIT_2_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_AVPUNIT_0_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_AVPUNIT_1_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_PKT_0_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_PKT_1_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_SCDC_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_HDCP_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
	GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_CEC_INT_CLEAR, HDMIRX_CLEAR_ALL_INT);
}

static unsigned int dw_hdmi_get_int_val(struct hrx_priv *hrx,
		unsigned int  stat_reg, unsigned int  mask_reg)
{
	unsigned int stat_val, mask_val;

	GA_REG_WORD32_READ(hrx->hrx_base + stat_reg, &stat_val);
	GA_REG_WORD32_READ(hrx->hrx_base + mask_reg, &mask_val);
	return stat_val & mask_val;
}

static int hdmirxHandle(struct hrx_priv *hrx)
{
	HRESULT rc = S_OK;

	int hrx_intr = 0xff;
	unsigned int irq = 0;
	unsigned int mu0_mask, mu2_mask;

	unsigned int mu0_stat = dw_hdmi_get_int_val(hrx, HDMI_MAINUNIT_0_INT_STATUS,
			HDMI_MAINUNIT_0_INT_MASK_N);
	unsigned int mu2_stat = dw_hdmi_get_int_val(hrx, HDMI_MAINUNIT_2_INT_STATUS,
			HDMI_MAINUNIT_2_INT_MASK_N);
	unsigned int pkt0_stat = dw_hdmi_get_int_val(hrx, HDMI_PKT_0_INT_STATUS,
			HDMI_PKT_0_INT_MASK_N);
	unsigned int avp1_stat = dw_hdmi_get_int_val(hrx, HDMI_AVPUNIT_1_INT_STATUS,
			HDMI_AVPUNIT_1_INT_MASK_N);
	unsigned int hdcp_stat = dw_hdmi_get_int_val(hrx, HDMI_HDCP_INT_STATUS,
			HDMI_HDCP_INT_MASK_N);

	irq = (mu0_stat & MAINUNIT_0_INTR)|((mu2_stat & MAINUNIT_2_INTR) << 2)|
			(avp1_stat & AVPUNIT1_INTR)|(hdcp_stat & HDCP_INTR) |
			(pkt0_stat & PKT_0_INTR) | ((pkt0_stat & PKT_0_ACR) << 27) |
			((pkt0_stat & HDMI_PKT_0_INT_STATUS_GCP) << 25) |
			((pkt0_stat & HDMI_PKT_0_INT_STATUS_AUDIF) << 12);

	if ((mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_TMDSQP_CK_OFF) ||
		(mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_TMDSVALID_STABLE)) {
		hrx_intr = HDMIRX_INTR_SYNC;
		hrx_trace("HDMIRX_INTR_SYNC mu0_stat = 0x%x\n", mu0_stat);
		hrx_trace("HDMIRX_INTR_SYNC mu2_stat = 0x%x\n", mu2_stat);
		hrx_trace("HDMIRX_INTR_SYNC avp1_stat = 0x%x\n", avp1_stat);
		GA_REG_WORD32_READ(hrx->hrx_base + HDMI_MAINUNIT_0_INT_MASK_N, &mu0_mask);
		GA_REG_WORD32_READ(hrx->hrx_base + HDMI_MAINUNIT_2_INT_MASK_N, &mu2_mask);
		mu0_mask = (mu0_mask & (~mu0_stat));
		mu2_mask = (mu2_mask & (~mu2_stat));
		GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_MAINUNIT_0_INT_MASK_N, mu0_mask);
		GA_REG_WORD32_WRITE(hrx->hrx_base + HDMI_MAINUNIT_2_INT_MASK_N, mu2_mask);

	} else if ((pkt0_stat & HDMI_PKT_0_INT_STATUS_AVIIF) ||
			 (pkt0_stat & HDMI_PKT_0_INT_STATUS_VSIF)) {
		hrx_intr = HDMIRX_INTR_PKT;
		hrx_trace("HDMIRX_INTR_PKT pkt0_stat = 0x%x\n", pkt0_stat);

	} else if ((mu2_stat & HDMI_MAINUNIT_2_INT_STATUS_AUDPLL_LOCK_STABLE) ||
		(mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_OFF) ||
		(mu0_stat & HDMI_MAINUNIT_0_INT_STATUS_AUDIO_CK_LOCKED) ||
		(avp1_stat & HDMI_IRQ_AUDIO_AVP1_FLAG) ||
		(pkt0_stat & HDMI_IRQ_AUDIO_PKT0_FLAG) ||
		(pkt0_stat & HDMI_PKT_0_INT_STATUS_AUDIF)) {
		hrx_intr = HDMIRX_INTR_CHNL_STS;
		hrx_trace("HDMIRX_INTR_CHNL_STS pkt0_stat = 0x%x\n", pkt0_stat);
		hrx_trace("HDMIRX_INTR_CHNL_STS mu0_stat = 0x%x\n", mu0_stat);
		hrx_trace("HDMIRX_INTR_CHNL_STS mu2_stat = 0x%x\n", mu2_stat);
		hrx_trace("HDMIRX_INTR_CHNL_STS avp1_stat = 0x%x\n", avp1_stat);
	} else if ((hdcp_stat & HDMI_HDCP_INT_STATUS_ENCDIS) ||
			 (hdcp_stat & HDMI_HDCP_INT_STATUS_ENCEN)) {
		hrx_intr = HDMIRX_INTR_HDCP;
		hrx_trace("HDMIRX_INTR_HDCP hdcp_stat = 0x%x\n", hdcp_stat);
	}

	dw_hdmi_clear_all_ints(hrx);
	/* HDMI Rx interrupt */
	if (hrx_intr != 0xff) {
		MV_CC_MSG_t msg = {
			0,
			hrx_intr,
			irq
		};
		rc = AMPMsgQ_Add(&hrx->hHdmirxMsgQ, &msg);
		if (rc != S_OK) {
			hrx_error("Hdmirx MsgQ full\n");
			return -1;
		}
		hrx_trace("Pushed Hdmirx interrupt = %d", hrx_intr);
		up(&hrx->hdmirx_sem);
		return 1;
	}
	return 0;
}

static irqreturn_t devices_hrx_isr(int irq, void *dev_id)
{
	u32 intrId;
	irq_hw_number_t hw_irq;
	struct hrx_priv *hrx = (struct hrx_priv *) dev_id;
	HRESULT rc;
	MV_CC_MSG_t msg = { 0, 0, 0 };
	unsigned int avp1_stat, avp1_mask, avp1_mask_stat;
	int ret;
	u32 uiInstate;
	HDL_semaphore *pSemHandle = NULL;

	hw_irq = irqd_to_hwirq(irq_get_irq_data(irq));
	intrId = hw_irq;

	pSemHandle = dhub_semaphore(hrx->dhub);
	uiInstate = semaphore_chk_full(pSemHandle, intrId);
	if (uiInstate) {
		semaphore_pop(pSemHandle, intrId, 1);
		semaphore_clr_full(pSemHandle, intrId);
		hrx_error("semaphore_pop and clr_full for intrId = %d\n",intrId);
	}
	if ((intrId == avioDhubSemMap_aio64b_aio_intr7) ||
		(intrId == avioDhubSemMap_aio64b_aio_intr9) ||
		(intrId == avioDhubSemMap_aio64b_aio_intr10) ||
		(intrId == avioDhubSemMap_aio64b_aio_intr11)) {
		hrx_trace("%s: irq=%d intrId=%d ", __func__, irq, intrId);
		msg.m_MsgID = 1 << intrId;
		rc = AMPMsgQ_Add(&hrx->hVipMsgQ, &msg);
		if (rc == S_OK)
			up(&hrx->vip_sem);
		return IRQ_HANDLED;
	} else if (intrId == avioDhubSemMap_aio64b_aio_intr8) {

		hrx_trace("%s: irq=%d intrId=%d ", __func__, irq, intrId);
		GA_REG_WORD32_READ(hrx->hrx_base + HDMI_AVPUNIT_1_INT_STATUS, &avp1_stat);
		GA_REG_WORD32_READ(hrx->hrx_base + HDMI_AVPUNIT_1_INT_MASK_N, &avp1_mask);
		avp1_mask_stat = (avp1_stat & avp1_mask);
		if (avp1_mask_stat &
			HDMI_AVPUNIT_1_INT_MASK_N_DEFRAMER_VSYNC_MASK) {
			msg.m_MsgID = 1 << intrId;
			rc = AMPMsgQ_Add(&hrx->hVipMsgQ, &msg);
			if (rc == S_OK)
				up(&hrx->vip_sem);
			hrx_trace("Push VSYNC interrupt");
		}

		ret = hdmirxHandle(hrx);
		return IRQ_HANDLED;
	} else if (intrId == avioDhubChMap_aio64b_MIC3_CH_W) {
		hrx_trace("received avioDhubChMap_aio64b_MIC3_CH_W\n");
		msg.m_MsgID = 1 << intrId;
		aip_resume_cmd(hrx);

		rc = AMPMsgQ_Add(&hrx->hAipMsgQ, &msg);
		if (rc == S_OK)
			up(&hrx->aip_sem);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;

	return IRQ_NONE;
}

static int init_dhub(struct hrx_priv *hrx)
{
	hrx->dhub = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB);
	if (unlikely(hrx->dhub == NULL)) {
		hrx_error("hrx->dhub: get failed\n");
		return -1;
	}

	return 0;
}

static int drv_hrx_init(struct hrx_priv *hrx)
{
	int ret;

	spin_lock_init(&hrx->aip_spinlock);

	sema_init(&hrx->vip_sem, 0);
	sema_init(&hrx->hdmirx_sem, 0);
	sema_init(&hrx->aip_sem, 0);

	ret = AMPMsgQ_Init(&hrx->hVipMsgQ, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		hrx_error("hVipMsgQ init: failed, err:%8x\n", ret);
		return -1;
	}

	ret = AMPMsgQ_Init(&hrx->hHdmirxMsgQ, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		hrx_error("hHdmirxMsgQ init: failed, err:%8x\n", ret);
		return -1;
	}

	ret = AMPMsgQ_Init(&hrx->hAipMsgQ, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		hrx_error("hAipMsgQ init: failed, err:%8x\n", ret);
		return -1;
	}

	hrx_trace("%s ok\n", __func__);
	return 0;
}

static int drv_hrx_exit(struct hrx_priv *hrx)
{
	u32 err;

	err = AMPMsgQ_Destroy(&hrx->hVipMsgQ);
	if (unlikely(err != S_OK))
		hrx_error("hrx MsgQ Destroy: failed, err:%8x\n", err);
	err = AMPMsgQ_Destroy(&hrx->hHdmirxMsgQ);
	if (unlikely(err != S_OK))
		hrx_error("Hrx MsgQ Destroy: failed, err:%8x\n", err);
	err = AMPMsgQ_Destroy(&hrx->hAipMsgQ);
	if (unlikely(err != S_OK))
		hrx_error("Aip MsgQ Destroy: failed, err:%8x\n", err);
	return 0;
}

static long drv_hrx_ioctl_unlocked(struct file *filp, u32 cmd,
		unsigned long arg)
{
	struct hrx_priv *hrx = filp->private_data;
	struct dma_buf *buf = NULL;
	void *param = NULL;
	int aip_info[3];
	int gid = 0, ret;
	unsigned long aip_spinlock_flags;

	hrx_trace("%s: 0x%x", __func__, cmd);
	switch (cmd) {
	/**************************************
	 * VIP IOCTL
	 **************************************/
	case VIP_IOCTL_GET_MSG:
	{
		HRESULT rc = S_OK;
		MV_CC_MSG_t msg = { 0 };

		rc = down_interruptible(&hrx->vip_sem);
		if (rc < 0)
			return rc;
		rc = AMPMsgQ_ReadTry(&hrx->hVipMsgQ, &msg);
		if (unlikely(rc != S_OK)) {
			hrx_trace("VIP read message queue failed\n");
			return -EFAULT;
		}

		AMPMsgQ_ReadFinish(&hrx->hVipMsgQ);
		if (copy_to_user((void __user *) arg,
				 &msg, sizeof(MV_CC_MSG_t))) {
			return -EFAULT;
		}
		break;
	}
	case VIP_IOCTL_SEND_MSG:
	{
		/* get msg from VIP */
		int vip_msg = 0;

		if (copy_from_user(&vip_msg,
			(int __user *)arg, sizeof(int)))
			return -EFAULT;

		if (vip_msg == VIP_MSG_DESTROY_ISR_TASK) {
			//force one more INT to VIP to destroy ISR task
			hrx_trace("Destroy VIP ISR Task...\r\n");
			up(&hrx->vip_sem);
		}
		break;
	}

	/**************************************
	 * HDMIRX IOCTL
	 **************************************/
	case VIP_HRX_IOCTL_GET_MSG:
	{
		HRESULT rc = S_OK;
		MV_CC_MSG_t msg = { 0 };

		rc = down_interruptible(&hrx->hdmirx_sem);
		if (rc < 0)
			return rc;
		rc = AMPMsgQ_ReadTry(&hrx->hHdmirxMsgQ, &msg);
		if (unlikely(rc != S_OK)) {
			hrx_trace("HRX read message queue failed\n");
			return -EFAULT;
		}

		AMPMsgQ_ReadFinish(&hrx->hHdmirxMsgQ);
		if (copy_to_user((void __user *) arg,
				 &msg, sizeof(MV_CC_MSG_t))) {
			return -EFAULT;
		}
		break;
	}
	case VIP_HRX_IOCTL_SEND_MSG:
	{
		int hrx_msg = 0;

		if (copy_from_user
				(&hrx_msg,
				(int __user *)arg, sizeof(int)))
			return -EFAULT;

		if (hrx_msg == HRX_MSG_DESTROY_ISR_TASK) {
			//force one more INT to HRX to destroy ISR task
			hrx_trace("Destroy HRX ISR Task...\r\n");
			up(&hrx->hdmirx_sem);
		}
		break;
	}
	case VIP_HRX_IOCTL_GPIO_WRITE:
	{
		if (hrx->gpiod_hrxhpd)
			gpiod_set_value_cansleep(hrx->gpiod_hrxhpd, (int)arg);
		break;
	}
	case VIP_HRX_IOCTL_GPIO_5V_READ:
	{
		if (hrx->gpiod_hrx5v) {
			int value = gpiod_get_value(hrx->gpiod_hrx5v);
			if (copy_to_user((void __user *) arg,
						&value, sizeof(int))) {
				return -EFAULT;
			}
		}
		break;
	}

	/**************************************
	 * AIP IOCTL
	 **************************************/
	case AIP_IOCTL_GET_MSG_CMD:
	{
		HRESULT rc = S_OK;
		MV_CC_MSG_t msg = { 0 };

		rc = down_interruptible(&hrx->aip_sem);
		if (rc < 0)
			return rc;
		rc = AMPMsgQ_ReadTry(&hrx->hAipMsgQ, &msg);
		if (unlikely(rc != S_OK)) {
			hrx_trace("AIP read message queue failed\n");
			return -EFAULT;
		}

		AMPMsgQ_ReadFinish(&hrx->hAipMsgQ);
		if (copy_to_user((void __user *) arg,
				 &msg, sizeof(MV_CC_MSG_t))) {
			return -EFAULT;
		}
		break;
	}
	case AIP_IOCTL_START_CMD:
	{
		if (copy_from_user
			(aip_info, (int __user *) arg, 3 * sizeof(int))) {
			return -EFAULT;
		}
		gid = aip_info[1];
		if (gid) {
			buf = shm_get_dma_buf(hrx->client, gid);
			if (IS_ERR(buf)) {
				hrx->shm.buf = NULL;
				hrx_trace("gid = %d, dma_buf = %p\n",
						gid, buf);
				return -EFAULT;
			}
			hrx->shm.buf = buf;
		} else {
			hrx_error("invalid gid\n");
			return -EFAULT;
		}

		ret = dma_buf_begin_cpu_access(buf, DMA_BIDIRECTIONAL);
		if (ret) {
			hrx_error("error in beginning cpu access: %d\n", ret);
			goto error;
		}
		param = dma_buf_kmap(buf, 0);
		if (!param) {
			dma_buf_end_cpu_access(
			hrx->shm.buf, DMA_BIDIRECTIONAL);
			hrx_trace("ctl:%x Unable to map buf: %p\n",
					AIP_IOCTL_START_CMD, buf);
			return -EFAULT;
		}
		hrx_trace("buf(%p), mapping to %p\n", buf, param);
		hrx->shm.p = param;

		spin_lock_irqsave(&hrx->aip_spinlock, aip_spinlock_flags);
		aip_start_cmd(hrx, aip_info, param);
		spin_unlock_irqrestore(&hrx->aip_spinlock, aip_spinlock_flags);
		break;
error:
		shm_put(hrx->client, gid);
		break;
	}
	case AIP_IOCTL_STOP_CMD:
	{
		spin_lock_irqsave(&hrx->aip_spinlock, aip_spinlock_flags);
		aip_stop_cmd(hrx);
		spin_unlock_irqrestore(&hrx->aip_spinlock, aip_spinlock_flags);

		if (copy_from_user
			(aip_info, (int __user *) arg, 2 * sizeof(int))) {
			return -EFAULT;
		}
		gid = aip_info[1];
		if (!IS_ERR(hrx->shm.buf)) {
			dma_buf_end_cpu_access(
			hrx->shm.buf, DMA_BIDIRECTIONAL);
			dma_buf_kunmap(hrx->shm.buf, 0,
			hrx->shm.p);
			shm_put(hrx->client, gid);
		}
		break;
	}
	case AIP_IOCTL_SEMUP_CMD:
	{
		up(&hrx->aip_sem);
		break;
	}

	default:
		break;
	}
	return 0;
}

static atomic_t hrx_dev_refcnt = ATOMIC_INIT(0);

static int drv_hrx_open(struct inode *inode, struct file *file)
{
	struct hrx_priv *hrx;
	int err = 0;

	if (atomic_inc_return(&hrx_dev_refcnt) > 1) {
		hrx_trace("allowed single instance, hrx reference count %d!\n",
			atomic_read(&hrx_dev_refcnt));
		return 0;
	}

	hrx = container_of(inode->i_cdev, struct hrx_priv, cdev);

	err = init_dhub(hrx);
	if (unlikely(err != S_OK)) {
		hrx_error("init_dhub: failed, err:%8x\n", err);
		goto error;
	}

	file->private_data = (void *)hrx; /*for other methods*/

	hrx_enter_func();
	/* register and enable VIP ISR */
	err = request_irq(hrx->otg_intr,
					  devices_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP OutTG", hrx);
	if (unlikely(err < 0))
		goto error;

	err = request_irq(hrx->hdmirx_intr,
					  devices_hrx_isr,
					  IRQF_SHARED, "ADHUB HDMIRX", hrx);
	if (unlikely(err < 0))
		goto error2;

	err = request_irq(hrx->ytg_intr,
					  devices_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP YTG", hrx);
	if (unlikely(err < 0))
		goto error3;

	err = request_irq(hrx->uvtg_intr,
					  devices_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP UVTG", hrx);
	if (unlikely(err < 0))
		goto error4;

	err = request_irq(hrx->itg_intr,
					  devices_hrx_isr,
					  IRQF_SHARED, "ADHUB VIP InTG", hrx);
	if (unlikely(err < 0))
		goto error5;

	err = request_irq(hrx->mic3_intr,
					  devices_hrx_isr,
					  IRQF_SHARED, "ADHUB AIP MIC3", hrx);
	if (unlikely(err < 0))
		goto error6;

	hrx_trace("register hrx irq succcessfully");
	return err;

error6:
	free_irq(hrx->mic3_intr, hrx);
error5:
	free_irq(hrx->uvtg_intr, hrx);
error4:
	free_irq(hrx->ytg_intr, hrx);
error3:
	free_irq(hrx->hdmirx_intr, hrx);
error2:
	free_irq(hrx->otg_intr, hrx);
error:
	hrx_error("register hrx irq error");
	return err;
}

static int drv_hrx_release(struct inode *inode, struct file *filp)
{
	struct hrx_priv *hrx;

	if (atomic_read(&hrx_dev_refcnt) == 0) {
		hrx_trace("hrx driver is already released!\n");
		return 0;
	}

	if (atomic_dec_return(&hrx_dev_refcnt)) {
		hrx_trace("hrx dev ref cnt after this release: %d!\n",
				atomic_read(&hrx_dev_refcnt));
		return 0;
	}

	hrx = filp->private_data;

	/* unregister VIP interrupt */
	free_irq(hrx->otg_intr, hrx);
	free_irq(hrx->hdmirx_intr, hrx);
	free_irq(hrx->ytg_intr, hrx);
	free_irq(hrx->uvtg_intr, hrx);
	free_irq(hrx->itg_intr, hrx);
	free_irq(hrx->mic3_intr, hrx);

	return 0;
}

static int drv_hrx_config(struct platform_device *pdev)
{
	int irq;
	struct hrx_priv *hrx = get_hrx_priv(&pdev->dev);
	struct resource *edid_resource;
	struct resource *hrx_addr_resource;
	int ret = 0;

	hrx_enter_func();
	irq = platform_get_irq_byname(pdev, "otg");
	if (irq < 0) {
		hrx_error("fail to get irq for otg\n");
		ret = irq;
		goto error;
	} else {
		hrx->otg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "hdmirx");
	if (irq < 0) {
		hrx_error("fail to get irq for hdmirx\n");
		ret = irq;
		goto error;
	} else {
		hrx->hdmirx_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "ytg");
	if (irq < 0) {
		hrx_error("fail to get irq for ytg\n");
		ret = irq;
		goto error;
	} else {
		hrx->ytg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "uvtg");
	if (irq < 0) {
		hrx_error("fail to get irq for uvtg\n");
		ret = irq;
		goto error;
	} else {
		hrx->uvtg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "itg");
	if (irq < 0) {
		hrx_error("fail to get irq for itg\n");
		ret = irq;
		goto error;
	} else {
		hrx->itg_intr = irq;
	}

	irq = platform_get_irq_byname(pdev, "mic3");
	if (irq < 0) {
		hrx_error("fail to get irq for mic3\n");
		ret = irq;
		goto error;
	} else {
		hrx->mic3_intr = irq;
	}

	hrx_trace("otg_intr = %d\n", hrx->otg_intr);
	hrx_trace("hdmirx_intr = %d\n", hrx->hdmirx_intr);
	hrx_trace("ytg_intr = %d\n", hrx->ytg_intr);
	hrx_trace("uvtg_intr = %d\n", hrx->uvtg_intr);
	hrx_trace("itg_intr = %d\n", hrx->itg_intr);
	hrx_trace("mic3_intr = %d\n", hrx->mic3_intr);

	hrx_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
		hrx->otg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx->otg_intr)));
	hrx_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
		hrx->hdmirx_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx->hdmirx_intr)));
	hrx_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
		hrx->ytg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx->ytg_intr)));
	hrx_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
		hrx->uvtg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx->uvtg_intr)));
	hrx_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
		hrx->itg_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx->itg_intr)));
	hrx_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
		hrx->mic3_intr,
		(u32)irqd_to_hwirq(irq_get_irq_data(hrx->mic3_intr)));

	edid_resource = platform_get_resource_byname(pdev, IORESOURCE_MEM, "edid_base");
	if (edid_resource == NULL) {
		hrx_error("fail to get edid reg resource\n");
		ret = -EINVAL;
		goto error;
	} else {
		hrx->edid_resource = edid_resource;
	}

	hrx_addr_resource = platform_get_resource_byname(pdev, IORESOURCE_MEM, "hrx_base");
	if (hrx_addr_resource == NULL) {
		hrx_error("fail to get hrx base resource\n");
		ret = -EINVAL;
		goto error;
	} else {
		hrx->hrx_base = hrx_addr_resource->start;
	}

	hrx->gpiod_hrxhpd = devm_gpiod_get_optional(&pdev->dev,
				"hrxhpd", GPIOD_OUT_LOW);
	if (IS_ERR(hrx->gpiod_hrxhpd)) {
		hrx_error("could not get hdmirx hpd gpio, err : %ld\n",
				PTR_ERR(hrx->gpiod_hrxhpd));
		ret = PTR_ERR(hrx->gpiod_hrxhpd);
		goto error;
	}

	hrx->gpiod_hrx5v = devm_gpiod_get_optional(&pdev->dev,
				"hrx5v", GPIOD_IN);
	if (IS_ERR(hrx->gpiod_hrx5v)) {
		hrx_error("could not get hdmirx 5v gpio, err : %ld\n",
		PTR_ERR(hrx->gpiod_hrx5v));
		ret = -EINVAL;
		hrx->gpiod_hrx5v = NULL;
		goto error;
	}
	return 0;

error:
	return ret;
}

static int hrx_major;
static int hrx_driver_setup_cdev(struct cdev *dev, int major, int minor,
		const struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static int hrx_init(struct hrx_priv *hrx)
{
	int res;

	/* Now setup cdevs. */
	res = hrx_driver_setup_cdev(&hrx->cdev, hrx->major,
				  hrx->minor, hrx->fops);
	if (res) {
		hrx_error("hrx_driver_setup_cdev failed.\n");
		return -ENODEV;
	}
	hrx_trace("setup cdevs device minor [%d]\n", hrx->minor);

	/* add PE devices to sysfs */
	hrx->dev_class = class_create(THIS_MODULE, hrx->dev_name);
	if (IS_ERR(hrx->dev_class)) {
		hrx_error("class_create failed.\n");
		res = -ENODEV;
		goto err_class_create;
	}

	device_create(hrx->dev_class, NULL,
			  MKDEV(hrx->major, hrx->minor), NULL,
			  hrx->dev_name);
	hrx_trace("create device sysfs [%s]\n", hrx->dev_name);

	res = drv_hrx_init(hrx);
	if (res != 0) {
		hrx_error("failed !!! res = 0x%08X\n", res);
		res = -ENODEV;
		goto err_add_device;
	}

	hrx_trace("%s ok\n", __func__);

	return 0;
err_add_device:
	if (hrx->dev_class) {
		device_destroy(hrx->dev_class,
				   MKDEV(hrx->major, hrx->minor));
		class_destroy(hrx->dev_class);
	}
err_class_create:
	cdev_del(&hrx->cdev);

	return res;
}

static int hrx_exit(struct hrx_priv *hrx)
{
	int res;

	hrx_trace("%s[%s] enter\n", __func__,
			   hrx->dev_name);

	/* destroy kernel API */
	res = drv_hrx_exit(hrx);
	if (res < 0)
		hrx_error("%s failed\n", __func__);

	if (hrx->dev_procdir) {
		/* remove PE device proc file */
		remove_proc_entry(HRX_DEVICE_PROCFILE,
				  hrx->dev_procdir);
		remove_proc_entry(hrx->dev_name, NULL);
	}

	if (hrx->dev_class) {
		/* del sysfs entries */
		device_destroy(hrx->dev_class,
				   MKDEV(hrx->major, hrx->minor));
		hrx_trace("delete device sysfs [%s]\n",
				   hrx->dev_name);

		class_destroy(hrx->dev_class);
	}
	/* del cdev */
	cdev_del(&hrx->cdev);

	return 0;
}

static int hrx_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	struct hrx_priv *hrx = (struct hrx_priv *)file->private_data;
	size_t size = vma->vm_end - vma->vm_start;

	/* Not checking the valid range as the minimum size for mmap is
	 * 1 PAGE_SIZE. Hence, value of 'size' will be PAGE_SIZE, but the
	 * ioremap size is 0x404.
	 */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma, vma->vm_start,
		(hrx->edid_resource->start >> PAGE_SHIFT),
		size, vma->vm_page_prot);

	if (ret)
		hrx_error("%s failed, ret = %d\n", __func__, ret);
	return ret;
}

static const struct file_operations hrx_ops = {
	.open = drv_hrx_open,
	.release = drv_hrx_release,
	.unlocked_ioctl = drv_hrx_ioctl_unlocked,
	.compat_ioctl = drv_hrx_ioctl_unlocked,
	.mmap = hrx_driver_mmap,
	.owner = THIS_MODULE,
};

static int hrx_probe(struct platform_device *pdev)
{
	int ret;
	struct hrx_priv *hrx;
	struct device *dev = &pdev->dev;
	dev_t pedev;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	hrx_enter_func();
	hrx = devm_kzalloc(dev, sizeof(struct hrx_priv),
				  GFP_KERNEL);
	if (!hrx) {
		hrx_error("no memory for hrx\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, (void *)hrx);
	hrx->dev = dev;
	device_rename(dev, HRX_DEVICE_NAME);
	hrx->dev_name = dev_name(dev);
	hrx_trace("hrx device name is %s\n", dev_name(dev));
	hrx->fops = &hrx_ops;

	ret = drv_hrx_config(pdev);
	if (ret < 0)
		goto err_config;

	hrx->client = shm_client_create("hrx_client");
	if (IS_ERR(hrx->client)) {
		hrx_error("error in creating shm client: %ld\n",
				PTR_ERR(hrx->client));
		ret = PTR_ERR(hrx->client);
		goto err_shm_client;
	}

	ret = alloc_chrdev_region(&pedev, 0,
				HRX_MAX_DEVS, HRX_DEVICE_NAME);
	if (ret < 0) {
		hrx_error("alloc_chrdev_region() failed for hrx\n");
		goto err_alloc_chrdev;
	}
	hrx_major = MAJOR(pedev);
	hrx_trace("register cdev device major [%d]\n", hrx_major);
	hrx->major = hrx_major;
	hrx->minor = HRX_MINOR;

	ret = hrx_init(hrx);
	if (ret < 0) {
		hrx_error("failed to do hrx_init\n");
		goto err_init;
	}

	hrx_trace("%s ok\n", __func__);
	return 0;
err_init:
	unregister_chrdev_region(MKDEV(hrx_major, 0), HRX_MAX_DEVS);
err_alloc_chrdev:
	shm_client_destroy(hrx->client);
err_shm_client:
err_config:
	hrx_trace("%s failed\n", __func__);
	return ret;
}

static int hrx_remove(struct platform_device *pdev)
{
	struct hrx_priv *hrx = get_hrx_priv(&pdev->dev);

	hrx_exit(hrx);
	unregister_chrdev_region(MKDEV(hrx_major, 0), HRX_MAX_DEVS);
	hrx_trace("unregister cdev device major [%d]\n", hrx_major);
	hrx_major = 0;
	shm_client_destroy(hrx->client);
	hrx_trace("hrx removed OK\n");
	return 0;
}

static const struct of_device_id hrx_match[] = {
	{
		.compatible = "syna,berlin-hrx",
	},
	{},
};

static struct platform_driver hrx_driver = {
	.probe = hrx_probe,
	.remove = hrx_remove,
	.driver = {
		.name = HRX_DEVICE_NAME,
		.of_match_table = hrx_match,
	},
};
module_platform_driver(hrx_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HRX module driver");
