// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#include <linux/delay.h>

#include "drv_vpp.h"
#include "drv_vpp_cfg.h"
#include "avio_dhub_drv.h"
#include "hal_vpp_wrap.h"
#include "hal_dhub_fastcall_wrap.h"

#include "avio_sub_module.h"
#include "drv_hdmitx.h"
#include "avio_common.h"

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
//#include "tinker_mcu.h"

//#define CONFIG_RASPBERRY_PI_LCD_MCU 1

#ifdef CONFIG_RASPBERRY_PI_LCD_MCU

#define LOG_INFO(fmt,arg...) pr_info("tinker-mcu: %s: "fmt, __func__, ##arg);
#define LOG_ERR(fmt,arg...) pr_err("tinker-mcu: %s: "fmt, __func__, ##arg);

#define MAX_I2C_LEN 255

struct tinker_mcu_data {
	struct device *dev;
	struct i2c_client *client;
};
static struct tinker_mcu_data *g_mcu_data;
#endif

#define AVIO_DEVICE_PROCFILE_CONFIG		"config"
#define AVIO_DEVICE_PROCFILE_STATUS		"status"
#define AVIO_DEVICE_PROCFILE_DETAIL		"detail"

#define VPP_MODULE_NAME "avio_module_vpp"

#define AVIO_GET_VPP_CTX() \
	(VPP_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_VPP)

#define VPP_MODULE_RETURN_WITH_EFAULT() \
	AVIO_MODULE_STORE_AND_RETURN(retVal, -EFAULT, 1);

#define IS_DSI_SINGLE_DISPLAY_MODE()     (vpp_ctx.display_mode == VPP_VOUT_SINGLE_MODE_SEC)

#define IS_VALID_DISPLAY_MODE(MODE) ((MODE >= VPP_VOUT_SINGLE_MODE_PRI) &&\
									(MODE < VPP_VOUT_MODE_MAX))

#define VPP_VSYNC_SEM(VPP_TG_SEM, VPP_TG1_SEM) \
		IS_SINGLE_DISPLAY() ? VPP_TG_SEM : \
		(IS_DSI_SINGLE_DISPLAY_MODE() ? \
		VPP_TG1_SEM : VPP_TG_SEM)

#ifdef CONFIG_IRQ_LATENCY_PROFILE
#define VPP_IRQ_PROFILE_STORE_CLOCK_IN_PTR(ptr) \
	*ptr = cpu_clock(smp_processor_id())
#define VPP_IRQ_PROFILE_STORE_CLOCK(VAR) \
	hVppCtx->avio_irq_profiler.##VAR = cpu_clock(smp_processor_id())
#define VPP_IRQ_PROFILE_DIFF_CLOCK(VAR1, VAR2)     \
	((int64_t)hVppCtx->avio_irq_profiler.##VAR1 - \
			(int64_t)hVppCtx->avio_irq_profiler.##VAR2)
#define VPP_IRQ_PROFILE_SET_VAR(VAR1, VAR2)     \
	hVppCtx->avio_irq_profiler.##VAR1 = \
			hVppCtx->avio_irq_profiler.##VAR2
#endif //CONFIG_IRQ_LATENCY_PROFILE

static struct proc_dir_entry *avio_driver_config;
static struct proc_dir_entry *avio_driver_state;
static struct proc_dir_entry *avio_driver_detail;

static VPP_CTX vpp_ctx;
static bool bIsDisplayModeSwitchSignal;

void __weak drv_vpp_read_vpp_cfg(VPP_CTX *hVppCtx, void *dev) { }
static int drv_vpp_init(void *h_vpp_ctx);

HRESULT wait_vpp_primary_vsync(void) {
	struct semaphore *pSem;
	HRESULT hres;

	pSem = VPP_VSYNC_SEM(&vpp_ctx.vsync_sem, &vpp_ctx.vsync1_sem);

	hres = down_interruptible(pSem);

	if (bIsDisplayModeSwitchSignal) {
		bIsDisplayModeSwitchSignal = 0;
		hres = wait_vpp_primary_vsync();
	}

	return hres;
};

static void signal_vpp_primary_false_vsync(void) {
	struct semaphore *pSem;

	pSem = VPP_VSYNC_SEM(&vpp_ctx.vsync_sem, &vpp_ctx.vsync1_sem);

	/* Trigger False VSYNC only if there is a pending signal */
	if (!pSem->count) {
		bIsDisplayModeSwitchSignal = 1;
		up(pSem);
	}
};

//Various clocks controlled by the AVIO module
static void avio_devices_vpp_enable_clocks(VPP_CTX *hVppCtx)
{
	int i;
	int rate, ret;

	for (i = 0; i < hVppCtx->clk_list_count; i++) {
		if (!IS_ERR(hVppCtx->clk_list[i].vpp_clk)) {
			ret = clk_prepare_enable(hVppCtx->clk_list[i].vpp_clk);
			rate = 0;
			if (ret == 0) {
				rate = clk_get_rate(hVppCtx->clk_list[i].vpp_clk);
			}
			avio_trace("%s: num:%d, clk:%s, ret:%d, rate:%d \n", __func__,
					hVppCtx->clk_list[i].clk_id,
					hVppCtx->clk_list[i].clk_name,
					ret, rate);
		}
	}
}

static void avio_devices_vpp_disable_clocks(VPP_CTX *hVppCtx)
{
	int i;

	for (i = 0; i < hVppCtx->clk_list_count; i++) {
		if (!IS_ERR(hVppCtx->clk_list[i].vpp_clk)) {
			  avio_trace("%s: num:%d, clk:%s\n", __func__,
						hVppCtx->clk_list[i].clk_id,
						hVppCtx->clk_list[i].clk_name);
			  clk_disable_unprepare(hVppCtx->clk_list[i].vpp_clk);
		}
	}
}

static VPP_CLOCK *avio_devices_get_clk(VPP_CTX *hVppCtx, VPP_CLK_ID clk_id)
{
	VPP_CLOCK *vpp_clk = NULL;
	int i;

	for (i = 0; i < hVppCtx->clk_list_count; i++) {
		if (clk_id == hVppCtx->clk_list[i].clk_id &&
			  !IS_ERR(hVppCtx->clk_list[i].vpp_clk))
			vpp_clk = &hVppCtx->clk_list[clk_id];
	}

	return vpp_clk;
}

static void avio_devices_vpp_release_gpio(VPP_CTX *hVppCtx)
{
	/* Set the device to Reset state */
	gpiod_set_value_cansleep(hVppCtx->gpio_mipirst, 1);
}

static void avio_devices_vpp_process_msg(VPP_CTX *hVppCtx, MV_CC_MSG_t *pMsg)
{
	if (pMsg->m_MsgID == VPP_CC_MSG_TYPE_VPP) {
		if (bTST(pMsg->m_Param1, hVppCtx->vsync_intr_num)) {
			/* Drop redundant/duplicate CPCB0-VBI
			 * Send only latest CPCB0-VBI */
			if (atomic_dec_return(&hVppCtx->vsync_cnt)) {
				bCLR(pMsg->m_Param1, hVppCtx->vsync_intr_num);
				avio_trace("[vpp isr] Dropped CPCB0-VBI : %d\n",
					atomic_read(&hVppCtx->vsync_cnt));
			}
		}
		if (bTST(pMsg->m_Param1, hVppCtx->vde_intr_num)) {
			/* Drop redundant/duplicate CPCB0-VDE
			 * Send only latest CPCB0-VDE */
			if (atomic_dec_return(&hVppCtx->vde_cnt)) {
				bCLR(pMsg->m_Param1, hVppCtx->vde_intr_num);
				avio_trace("[vpp isr] Dropped CPCB0-VDE : %d\n",
					atomic_read(&hVppCtx->vde_cnt));
			}
		}
		if (bTST(pMsg->m_Param1, hVppCtx->vsync1_intr_num)) {
			/* Drop redundant/duplicate CPCB1-VBI
			 * Send only latest CPCB1-VBI */
			if (atomic_dec_return(&hVppCtx->vsync1_cnt)) {
				bCLR(pMsg->m_Param1, hVppCtx->vsync1_intr_num);
				avio_trace("[vpp isr] Dropped CPCB1-VBI : %d\n",
					atomic_read(&hVppCtx->vsync1_cnt));
			}
		}
	}
}

HRESULT avio_devices_vpp_post_msg(VPP_CTX *hVppCtx, unsigned int msgId,
	unsigned int param1, unsigned int param2)
{
	MV_CC_MSG_t msg;
	HRESULT ret = S_OK;

	msg.m_MsgID = msgId;
	msg.m_Param1 = param1;
	msg.m_Param2 = param2;

	spin_lock(&hVppCtx->vpp_msg_spinlock);
	ret = AMPMsgQ_Add(&hVppCtx->hVPPMsgQ, &msg);
	spin_unlock(&hVppCtx->vpp_msg_spinlock);

	if (ret == S_OK) {
		up(&hVppCtx->vpp_sem);
	} else {
#ifdef CONFIG_IRQ_LATENCY_PROFILE
		//Process(Decrement count) dropped message
		avio_devices_vpp_process_msg(hVppCtx, &msg);
#endif

		if (!atomic_read(&hVppCtx->vpp_isr_msg_err_cnt))
			avio_error("[vpp isr] MsgQ full\n");

		atomic_inc(&hVppCtx->vpp_isr_msg_err_cnt);
	}

	return ret;
}

static void vpp_drv_update_interrupt_time(VPP_CTX *hVppCtx, int intr_type)
{
	struct timespec nows;
	INT64 time;
	unsigned int period;

	ktime_get_ts(&nows);

	time = timespec_to_ns(&nows);

	if (intr_type == VPP_INTR_TYPE_VBI) {
		atomic64_set(&hVppCtx->vsynctime, (long long)time);
		if (hVppCtx->is_vsync_measure_enabled) {
			if (hVppCtx->lastvsynctime != 0) {

				period = (unsigned int)
					abs(time - hVppCtx->lastvsynctime);

				if (period > hVppCtx->vsync_period_max)
					hVppCtx->vsync_period_max = period;
				if (period < hVppCtx->vsync_period_min)
					hVppCtx->vsync_period_min = period;
			}
			hVppCtx->lastvsynctime = time;
		}
	}
	if (intr_type == VPP_INTR_TYPE_CPCB1_VBI) {
		atomic64_set(&hVppCtx->vsync1time, (long long)time);
	}
}

static void vpp_drv_record_intr(VPP_CTX *hVppCtx, int intr_type, int intr_num)
{
#ifdef CONFIG_IRQ_LATENCY_PROFILE
	unsigned long long *pintr_curr = NULL;
#endif
	struct semaphore *psem = NULL;
	atomic_t *pcounter = NULL;
	int record_intr = 1;

	if (intr_type == VPP_INTR_TYPE_VBI) {
		psem = &hVppCtx->vsync_sem;
		pcounter = &hVppCtx->vsync_cnt;
#ifdef CONFIG_IRQ_LATENCY_PROFILE
		pintr_curr = &hVppCtx->avio_irq_profiler.vppCPCB0_intr_curr;
#endif
	} else if (intr_type == VPP_INTR_TYPE_VDE) {
		pcounter = &hVppCtx->vde_cnt;
	} else if (intr_type == VPP_INTR_TYPE_CPCB1_VBI) {
		psem = &hVppCtx->vsync1_sem;
		pcounter = &hVppCtx->vsync1_cnt;
#ifdef CONFIG_IRQ_LATENCY_PROFILE
		pintr_curr = &hVppCtx->avio_irq_profiler.vppCPCB1_intr_curr;
#endif
	}

	//counter & semaphore operation for only periodic CPCB/TG interrupts
	if (pcounter) {
		vpp_drv_update_interrupt_time(hVppCtx, intr_type);

#ifdef CONFIG_IRQ_LATENCY_PROFILE
		if (pintr_curr)
			VPP_IRQ_PROFILE_STORE_CLOCK_IN_PTR(pintr_curr);
		//queue duplicate interrupt for profiling purposes
		atomic_inc(pcounter);
#else
		if (!atomic_read(pcounter)) {
			atomic_inc(pcounter);
		} else {
			//No need to queue duplicate interrupt, if no profiling
			record_intr = 0;
		}
#endif
		if (psem && !psem->count)
			up(psem);
	}

	if (record_intr)
		hVppCtx->vpp_intr |= bSETMASK(intr_num);
}

static int vpp_drv_check_and_clear_interrupt(VPP_CTX *hVppCtx, int intr_num_ndx)
{
	HRESULT ret = S_OK;
	int intr_num, intr_type;

	intr_num = hVppCtx->vpp_interrupt_list[intr_num_ndx].intr_num;
	intr_type = hVppCtx->vpp_interrupt_list[intr_num_ndx].intr_type;

	if (bTST(hVppCtx->instat, intr_num) &&
			(hVppCtx->vpp_intr_status[intr_num])) {
		bSET(hVppCtx->instat_used, intr_num);
		bCLR(hVppCtx->instat, intr_num);
		vpp_drv_record_intr(hVppCtx, intr_type, intr_num);
	}
	return ret;
}

int avio_devices_vpp_isr(UNSG32 intrMask, void *pArgs)
{
	VPP_CTX *hVppCtx = (VPP_CTX *) pArgs;
	int i = 0;

#ifdef CONFIG_IRQ_LATENCY_PROFILE
	VPP_IRQ_PROFILE_STORE_CLOCK(vpp_isr_start);
#endif

	/* VPP interrupt handling  */
	hVppCtx->instat = intrMask;

	hVppCtx->vpp_intr = hVppCtx->instat_used = 0;
	for (i = 0; i < hVppCtx->vpp_interrupt_list_count; i++)
		vpp_drv_check_and_clear_interrupt(hVppCtx,  i);


#ifdef CONFIG_IRQ_LATENCY_PROFILE
	if (hVppCtx->vpp_intr) {
		unsigned long isr_time;
		int32_t jitter = 0;

		VPP_IRQ_PROFILE_STORE_CLOCK(vpp_isr_end);
		isr_time = VPP_IRQ_PROFILE_DIFF_CLOCK(
					vpp_isr_end, vpp_isr_start);
		isr_time /= 1000;

		if (bTST(hVppCtx->vpp_intr, hVppCtx->vsync_intr_num)) {
			if (hVppCtx->avio_irq_profiler.vppCPCB0_intr_last) {
				jitter = VPP_IRQ_PROFILE_DIFF_CLOCK(
					vppCPCB0_intr_curr, vppCPCB0_intr_last);

				//nanosec_rem = do_div(interval, 1000000000);
				// transform to us unit
				jitter /= 1000;
				jitter -= 16667;
			}
			VPP_IRQ_PROFILE_SET_VAR(vppCPCB0_intr_last, vppCPCB0_intr_curr);
			hVppCtx->vpp_cpcb0_vbi_int_cnt++;
		}

		if ((jitter > 670) || (jitter < -670) || (isr_time > 1000)) {
			avio_trace
				(" W/[vpp isr] jitter:%6d > +-670 us, instat:0x%x last_instat:"
				 "0x%0x max_instat:0x%0x, isr_time:%d us last:%d max:%d \n",
				 jitter, instat_used,
				 hVppCtx->avio_irq_profiler.vpp_isr_last_instat,
				 hVppCtx->avio_irq_profiler.vpp_isr_instat_max,
				 isr_time,
				 hVppCtx->avio_irq_profiler.vpp_isr_time_last,
				 hVppCtx->avio_irq_profiler.vpp_isr_time_max);
		}

		hVppCtx->avio_irq_profiler.vpp_isr_last_instat = instat_used;
		hVppCtx->avio_irq_profiler.vpp_isr_time_last = isr_time;

		if (isr_time > hVppCtx->avio_irq_profiler.vpp_isr_time_max) {
			VPP_IRQ_PROFILE_SET_VAR(vpp_isr_time_max,
					vpp_isr_time_last);
			VPP_IRQ_PROFILE_SET_VAR(vpp_isr_instat_max,
					vpp_isr_last_instat);
		}
	} else {
		unsigned long isr_time;

		VPP_IRQ_PROFILE_STORE_CLOCK(vpp_isr_end);
		isr_time = VPP_IRQ_PROFILE_DIFF_CLOCK(
					vpp_isr_end, vpp_isr_start);
		isr_time /= 1000;

		if (isr_time > 1000) {
			avio_trace("###isr_time:%d us instat:%x last:%x\n",
			  isr_time, vpp_intr, instat,
			  hVppCtx->avio_irq_profiler.vpp_isr_last_instat);
		}

		hVppCtx->avio_irq_profiler.vpp_isr_time_last = isr_time;
	}
#endif

	if (hVppCtx->vpp_intr) {
		avio_devices_vpp_post_msg(hVppCtx, VPP_CC_MSG_TYPE_VPP,
				hVppCtx->vpp_intr, hVppCtx->vpp_intr_timestamp);
	}

	return IRQ_HANDLED;
}

static void drv_vpp_config(void *h_vpp_ctx, void *dev)
{
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;

	avio_trace("%s:%d:\n", __func__, __LINE__);

	//Read VPP cfg to hVppCtx
	drv_vpp_read_vpp_cfg(hVppCtx, dev);

	avio_devices_vpp_enable_clocks(hVppCtx);
}

#ifdef CONFIG_RASPBERRY_PI_LCD_MCU

static int send_c(struct i2c_client *client, unsigned char *buf, int size)
{
	int ret, retry = 5;

	while(retry-- > 0) {
	    ret = i2c_master_send(client, buf, size);
	    if (ret <= 0) {
	            printk("send command failed, ret = %d, retry again!\n", ret);
	    } else
	            break;
	    }

	if(ret <= 0) {
		printk("send command failed\n");
		return ret!=0 ? ret : -ECOMM;
	}

	msleep(20);
	return 0;
}

unsigned char rrbuf[] = {
    0x85 ,0x01,
    0x81 ,0x40,
    0x86, 0x80
};

static int init_cmd_check(struct tinker_mcu_data *mcu_data)
{
	int ret = 0, i = 0;

     for (i = 0; i < sizeof(rrbuf)/sizeof(rrbuf[0]); i++) {
	ret = send_c(mcu_data->client, &(rrbuf[i*2]), 2);
     }
     return 0;
}

static int tinker_mcu_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct tinker_mcu_data *mcu_data;
	int ret;

	printk("address = 0x%x\n", client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("I2C check functionality failed\n");
		return -ENODEV;
	}

	mcu_data = kzalloc(sizeof(struct tinker_mcu_data), GFP_KERNEL);
	if (mcu_data == NULL) {
		printk("no memory for device\n");
		return -ENOMEM;
	}

	mcu_data->client = client;
	i2c_set_clientdata(client, mcu_data);
	g_mcu_data = mcu_data;

	ret = init_cmd_check(mcu_data);
	if (ret < 0) {
		printk("init_cmd_check failed, %d\n", ret);
		goto error;
	}
	return 0;

error:
	kfree(mcu_data);
	return ret;
}

static int tinker_mcu_remove(struct i2c_client *client)
{
	struct tinker_mcu_data *mcu_data = i2c_get_clientdata(client);
	kfree(mcu_data);
	return 0;
}

static const struct i2c_device_id tinker_mcu_id[] = {
	{"tinker_mcu", 0},
	{},
};

#define TINKER_I2C_ADAPTER 0
#define I2C_ADDR_SLAVE 0x45

const struct i2c_board_info info = {
        I2C_BOARD_INFO("tinker_mcu", I2C_ADDR_SLAVE)
};

static int new_tinker_device(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	struct i2c_board_info info = {
		.type = "tinker_mcu",
		.addr = I2C_ADDR_SLAVE,
	};

	pr_debug("%s\n", __func__);

	adapter = i2c_get_adapter(TINKER_I2C_ADAPTER);
	if (!adapter) {
		pr_err("%s: i2c_get_adapter(%d) failed\n", __func__,
			TINKER_I2C_ADAPTER);
		return -EINVAL;
	}

	client = i2c_new_device(adapter, &info);
	if (!client) {
		pr_err("%s: i2c_new_device() failed\n", __func__);
		i2c_put_adapter(adapter);
		return -EINVAL;
	}

	return 0;
}

static struct i2c_driver tinker_mcu_driver = {
	.driver = {
		.name = "tinker_mcu",
	},
	.probe = tinker_mcu_probe,
	.remove = tinker_mcu_remove,
	.id_table = tinker_mcu_id,
};

static int icn6211_init(void)
{
	int ret;

	printk("enter %s\n", __FUNCTION__);
	ret = new_tinker_device();
	if (ret != 0) {
		pr_err("new tinker_mcu i2c device faield.");
		return ret;
	}
	ret = i2c_add_driver(&tinker_mcu_driver);
	if (ret < 0) {
		pr_err("register tinker_mcu i2c dirver faield.");
		return ret;
	}
	return 0;
}

#endif

static int drv_vpp_open(void *h_vpp_ctx)
{
	unsigned int i, intr_num;
	int err;
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;

	avio_trace("%s:%d:\n", __func__, __LINE__);

	err = drv_vpp_init(h_vpp_ctx);
	if (err) {
		avio_trace("%s: init failed: %x\n", __func__, err);
		return err;
	}

	//Register VPP interrupt with dhub module
	for (i = 0; i < hVppCtx->vpp_interrupt_list_count; i++) {
		intr_num = hVppCtx->vpp_interrupt_list[i].intr_num;

		Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB, intr_num,
			  hVppCtx, avio_devices_vpp_isr);
	}

	hVppCtx->display_mode = VPP_VOUT_SINGLE_MODE_PRI;

	/* Clear the Reset line to make the device to normal functional state */
	gpiod_set_value_cansleep(hVppCtx->gpio_mipirst, 0);
#ifdef CONFIG_RASPBERRY_PI_LCD_MCU
	msleep(120);
	icn6211_init();
#endif

	return 0;
}

static int drv_vpp_init(void *h_vpp_ctx)
{
	unsigned int err = 0;
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;
	void *hdl;

	avio_trace("%s:%d:\n", __func__, __LINE__);
	hVppCtx->is_vsync_measure_enabled = 0;
	hdl = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_VPP_DHUB);
	if (hdl)
		hVppCtx->pSemHandle = wrap_dhub_semaphore(hdl);

	err = AMPMsgQ_Init(&hVppCtx->hVPPMsgQ, VPP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		avio_error("%s: hVPPMsgQ init failed, err:%8x\n",
			  __func__, err);
		return err;
	}

	return err;
}

static void drv_vpp_exit(void *h_vpp_ctx)
{
	unsigned int err;
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;

	avio_devices_vpp_disable_clocks(hVppCtx);
	avio_devices_vpp_release_gpio(hVppCtx);

	err = AMPMsgQ_Destroy(&hVppCtx->hVPPMsgQ);
	if (unlikely(err != S_OK)) {
		avio_error("vpp MsgQ Destroy: failed, err:%8x\n", err);
		return;
	}
}

static int drv_vpp_ioctl_unlocked(void *h_vpp_ctx, unsigned int cmd,
			  unsigned long arg, long *retVal)
{
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;
	UINT64 aviointrtime;
	int processedFlag = 1;
	MV_CC_MSG_t msg;

	if (!retVal)
		return 0;

	*retVal = 0;

	switch (cmd) {
	case VPP_IOCTL_BCM_SCHE_CMD:
		{
#ifdef VPP_DRV_RETAIN_UNUSED_CODE
		unsigned int bcm_sche_cmd_info[3], q_id;

		if (copy_from_user(bcm_sche_cmd_info, (void __user *) arg,
				3 * sizeof(unsigned int)))
			VPP_MODULE_RETURN_WITH_EFAULT();

		q_id = bcm_sche_cmd_info[2];
		if (q_id > BCM_SCHED_Q5) {
			avio_trace("error BCM queue ID = %d\n", q_id);
			VPP_MODULE_RETURN_WITH_EFAULT();
		}
		hVppCtx->bcm_sche_cmd[q_id][0] = bcm_sche_cmd_info[0];
		hVppCtx->bcm_sche_cmd[q_id][1] = bcm_sche_cmd_info[1];
		avio_trace("vpp bcm shed qid:%d info:%x %x\n",
			  q_id, bcm_sche_cmd_info[0], bcm_sche_cmd_info[0]);
#endif //VPP_DRV_RETAIN_UNUSED_CODE
		break;
		}

	case VPP_IOCTL_GET_MSG:
		//Loop until valid interrupt message is retrieved from the queue
		do {
			unsigned long flags;
			HRESULT rc = S_OK;

			rc = down_interruptible(&hVppCtx->vpp_sem);
			if (rc < 0)
				VPP_MODULE_RETURN_WITH_EFAULT();

#ifdef CONFIG_IRQ_LATENCY_PROFILE
			VPP_IRQ_PROFILE_STORE_CLOCK(vpp_task_sched_last);
#endif
			memset(&msg, 0, sizeof(MV_CC_MSG_t));
			// check fullness, clear message queue once.
			// only send latest message to task.
			if (AMPMsgQ_Fullness(&hVppCtx->hVPPMsgQ) <= 0) {
				//avio_trace("E/[vpp isr task] msgq empty\n");
				VPP_MODULE_RETURN_WITH_EFAULT();
			}

			spin_lock_irqsave(&hVppCtx->vpp_msg_spinlock, flags);
			AMPMsgQ_DequeueRead(&hVppCtx->hVPPMsgQ, &msg);
			spin_unlock_irqrestore(&hVppCtx->vpp_msg_spinlock, flags);

			//Process(Decrement count) pop'ed message
			avio_devices_vpp_process_msg(hVppCtx, &msg);

			if (!atomic_read(&hVppCtx->vpp_isr_msg_err_cnt))
				atomic_set(&hVppCtx->vpp_isr_msg_err_cnt, 0);
		} while (!msg.m_Param1);

		if (copy_to_user((void __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
			VPP_MODULE_RETURN_WITH_EFAULT();
		break;

	case VPP_IOCTL_START_BCM_TRANSACTION:
		{
			break;
		}

	case VPP_IOCTL_INTR_MSG:
		//get VPP INTR MASK info
		{
			INTR_MSG intr_info = { 0, 0 };

			if (copy_from_user
				(&intr_info, (void __user *) arg,
				 sizeof(INTR_MSG)))
				VPP_MODULE_RETURN_WITH_EFAULT();

			if (intr_info.DhubSemMap < MAX_INTR_NUM)
				hVppCtx->vpp_intr_status[intr_info.DhubSemMap] =
					intr_info.Enable;
			else
				VPP_MODULE_RETURN_WITH_EFAULT();

			break;
		}

	case VPP_IOCTL_INIT:
		{
			unsigned int vpp_init_param[2];

			if (copy_from_user(vpp_init_param, (void __user *) arg,
				2 * sizeof(unsigned int)))
				VPP_MODULE_RETURN_WITH_EFAULT();

			wrap_MV_VPP_InitVPPS(TA_UUID_VPP, vpp_init_param);

			avio_trace("vpp init param info:%x %x\n",
				vpp_init_param[0], vpp_init_param[1]);
			break;
		}


	case VPP_IOCTL_GET_VSYNC:
		{
			HRESULT rc = S_OK;

			rc = down_interruptible(&hVppCtx->vsync_sem);
			if (rc < 0)
				VPP_MODULE_RETURN_WITH_EFAULT();
			if (!arg) {
				avio_error("IOCTL arg NULL!\n");
				VPP_MODULE_RETURN_WITH_EFAULT();
			}
			aviointrtime =
				(UINT64) atomic64_read(&hVppCtx->vsynctime);

			if (put_user(aviointrtime, (UINT64 __user *) arg))
				VPP_MODULE_RETURN_WITH_EFAULT();
			break;
		}
	case VPP_IOCTL_GET_VSYNC1:
		{
			HRESULT rc = S_OK;

			rc = down_interruptible(&hVppCtx->vsync1_sem);
			if (rc < 0)
				VPP_MODULE_RETURN_WITH_EFAULT();
			if (!arg) {
				avio_error("IOCTL arg NULL!\n");
				VPP_MODULE_RETURN_WITH_EFAULT();
			}
			aviointrtime =
				(UINT64) atomic64_read(&hVppCtx->vsync1time);

			if (put_user(aviointrtime, (UINT64 __user *) arg))
				VPP_MODULE_RETURN_WITH_EFAULT();
			break;
		}

	case VPP_IOCTL_GET_QUIESCENT_FLAG:
		{
			if (copy_to_user((void __user *) arg,
				&hVppCtx->is_bootup_quiescent, sizeof(int)))
				VPP_MODULE_RETURN_WITH_EFAULT();
			break;
		}

	case VPP_IOCTL_ENABLE_CLK:
		{
			VPP_CLK_ENABLE clk_en_info = { 0, 0 };
			VPP_CLOCK *vpp_clk;

			if (copy_from_user
				(&clk_en_info, (void __user *) arg,
				 sizeof(VPP_CLK_ENABLE)))
				VPP_MODULE_RETURN_WITH_EFAULT();

			vpp_clk = avio_devices_get_clk(hVppCtx, clk_en_info.clk_id);
			if (!IS_ERR(vpp_clk)) {
				avio_trace("vpp enable clk :%s -> %x\n",
					vpp_clk->clk_name, clk_en_info.enable);
				if (clk_en_info.enable)
					clk_prepare_enable(vpp_clk->vpp_clk);
				else
					clk_disable_unprepare(vpp_clk->vpp_clk);
			} else {
				avio_trace("vpp invalid clk-id : %x\n", clk_en_info.clk_id);
			}

			break;
		}

	case VPP_IOCTL_GET_CLK_RATE:
		{
			VPP_CLK_RATE clk_rate_info = { 0, 0 };
			VPP_CLOCK *vpp_clk;

			if (copy_from_user
				(&clk_rate_info, (void __user *) arg,
				 sizeof(VPP_CLK_RATE)))
				VPP_MODULE_RETURN_WITH_EFAULT();

			vpp_clk = avio_devices_get_clk(hVppCtx, clk_rate_info.clk_id);
			if (!IS_ERR(vpp_clk)) {
				clk_rate_info.clk_rate_in_hz = clk_get_rate(vpp_clk->vpp_clk);
				avio_trace("vpp get clk :%s -> %x\n",
					vpp_clk->clk_name, clk_rate_info.clk_rate_in_hz);
			} else {
				clk_rate_info.clk_rate_in_hz = 0;
				avio_trace("vpp invalid clk-id : %x\n", clk_rate_info.clk_id);
			}

			if (copy_to_user((void __user *) arg,
				&clk_rate_info, sizeof(VPP_CLK_RATE)))
				VPP_MODULE_RETURN_WITH_EFAULT();

			break;
		}

	case VPP_IOCTL_SET_CLK_RATE:
		{
			VPP_CLK_RATE clk_rate_info = { 0, 0 };
			VPP_CLOCK *vpp_clk;

			if (copy_from_user
				(&clk_rate_info, (void __user *) arg,
				 sizeof(VPP_CLK_RATE)))
				VPP_MODULE_RETURN_WITH_EFAULT();

			vpp_clk = avio_devices_get_clk(hVppCtx, clk_rate_info.clk_id);
			if (!IS_ERR(vpp_clk)) {
				avio_trace("vpp set clk :%s -> %x\n",
					vpp_clk->clk_name, clk_rate_info.clk_rate_in_hz);
				clk_set_rate(vpp_clk->vpp_clk, clk_rate_info.clk_rate_in_hz);
			} else {
				avio_trace("vpp invalid clk-id : %x\n", clk_rate_info.clk_id);
			}

			break;
		}

	case HDMITX_IOCTL_UPDATE_HPD:
		{
			unsigned int status = 0;

			if (get_user(status, (unsigned int *)arg)) {
				VPP_MODULE_RETURN_WITH_EFAULT();
			}

			drv_hdmitx_set_state(hVppCtx->hdmitx_dev, status);

			break;
		}

	case HDMITX_IOCTL_SET_5V_STATE:
		{
			avio_trace("setting HDMI 5V to %d\n", (int)arg);
			gpiod_set_value_cansleep(hVppCtx->gpio_hdmitx_5v, ((int)arg?1:0));
			break;
		}

	case VPP_IOCTL_SET_DISPLAY_MODE:
		{
			VOUT_DISP_MODE display_mode = (int)arg;

			if (!IS_VALID_DISPLAY_MODE(display_mode))
				VPP_MODULE_RETURN_WITH_EFAULT();

			if (hVppCtx->display_mode != display_mode) {
				signal_vpp_primary_false_vsync();
				hVppCtx->display_mode = display_mode;

				/* Apply MIPI reset if the current mode is Single primary to avoid
				* display residue in MIPI panels
				*/
				if (hVppCtx->display_mode == VPP_VOUT_SINGLE_MODE_PRI) {
					drv_mipi_reset(h_vpp_ctx, 1);
					msleep(VPP_MIPI_PANEL_RESET_DELAY_MS);
					drv_mipi_reset(h_vpp_ctx, 0);
				}
			}
			break;
		}

	default:
		processedFlag = 0;
		break;
	}
	return processedFlag;
}

static void drv_vpp_close(void *h_vpp_ctx)
{
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;
	unsigned int i, intr_num;

	avio_trace("%s:%d:\n", __func__, __LINE__);

	//un-Register VPP interrupt with dhub module
	for (i = 0; i < hVppCtx->vpp_interrupt_list_count; i++) {
		intr_num = hVppCtx->vpp_interrupt_list[i].intr_num;

		Dhub_IntrRegisterHandler(DHUB_ID_VPP_DHUB,
				intr_num, hVppCtx, NULL);
	}

	drv_vpp_exit(h_vpp_ctx);
}


static ssize_t avio_config_proc_write(struct file *file,
					 const char __user *buffer,
					 size_t count, loff_t *f_pos)
{
	VPP_CTX *hVppCtx = AVIO_GET_VPP_CTX();
	unsigned int config = 0;
	unsigned int size =
		count > sizeof(unsigned int) ? sizeof(unsigned int) : count;

	if (copy_from_user(&config, buffer, size))
		return 0;

	if (config & AVIO_CONFIG_VSYNC_MEASURE) {
		if (hVppCtx->is_vsync_measure_enabled == 0) {
			hVppCtx->lastvsynctime = 0;
			hVppCtx->vsync_period_max = 0;
			hVppCtx->vsync_period_min = 0xFFFFFFFF;
			hVppCtx->is_vsync_measure_enabled = 1;
		}
	} else {
		hVppCtx->is_vsync_measure_enabled = 0;
	}

	return size;
}

static int avio_config_seq_show(struct seq_file *m, void *v)
{
	unsigned int config = 0;
	VPP_CTX *hVppCtx = AVIO_GET_VPP_CTX();

	if (hVppCtx->is_vsync_measure_enabled)
		config |= AVIO_CONFIG_VSYNC_MEASURE;

	seq_puts(m, "--------------------------------------\n");
	seq_printf(m, "| CONFIG |0x%08x                     |\n", config);
	seq_puts(m, "|        |BIT0: vsync measure enable |\n");
	seq_puts(m, "--------------------------------------\n");

	return 0;
}

static int avio_config_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, avio_config_seq_show, NULL);
}

static int avio_status_seq_show(struct seq_file *m, void *v)
{
	VPP_CTX *hVppCtx = AVIO_GET_VPP_CTX();

	seq_puts(m, "-----------------------------------\n");
	seq_printf(m, "| VPP | IRQ:%-10d                 |\n",
		   hVppCtx->vpp_cpcb0_vbi_int_cnt);
	seq_puts(m, "-----------------------------------\n");


	return 0;
}

static int avio_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, avio_status_seq_show, NULL);
}

static int avio_detail_seq_show(struct seq_file *m, void *v)
{
	VPP_CTX *hVppCtx = AVIO_GET_VPP_CTX();

	seq_puts(m, "------------------------------------\n");

	if (hVppCtx->is_vsync_measure_enabled) {
		seq_printf(m, "|  VPP   | vsync | max:%-8d  ns |\n",
			   hVppCtx->vsync_period_max);
		seq_printf(m, "|        |       | min:%-8d  ns |\n",
			   hVppCtx->vsync_period_min);
		seq_puts(m, "------------------------------------\n");
	}

	return 0;
}

static int avio_detail_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, avio_detail_seq_show, NULL);
}


static const struct file_operations avio_driver_config_fops = {
	.owner = THIS_MODULE,
	.open = avio_config_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = avio_config_proc_write,
};

static const struct file_operations avio_driver_status_fops = {
	.owner = THIS_MODULE,
	.open = avio_status_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations avio_driver_detail_fops = {
	.owner = THIS_MODULE,
	.open = avio_detail_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void drv_vpp_create_proc_file(void *h_vpp_ctx,
					struct proc_dir_entry *dev_procdir)
{
	if (dev_procdir) {
		avio_driver_config =
			proc_create(AVIO_DEVICE_PROCFILE_CONFIG, 0644,
					dev_procdir, &avio_driver_config_fops);
		avio_driver_state =
			proc_create(AVIO_DEVICE_PROCFILE_STATUS, 0644,
					dev_procdir, &avio_driver_status_fops);
		avio_driver_detail =
			proc_create(AVIO_DEVICE_PROCFILE_DETAIL, 0644,
					dev_procdir, &avio_driver_detail_fops);
	}
}

static void drv_vpp_remove_proc_file(void *h_vpp_ctx,
		struct proc_dir_entry *dev_procdir)
{
	if (dev_procdir) {
		remove_proc_entry(AVIO_DEVICE_PROCFILE_DETAIL, dev_procdir);
		remove_proc_entry(AVIO_DEVICE_PROCFILE_STATUS, dev_procdir);
		remove_proc_entry(AVIO_DEVICE_PROCFILE_CONFIG, dev_procdir);
	}
}


void drv_vpp_add_vpp_clock(VPP_CTX *hVppCtx, struct device_node *np,
		VPP_CLK_ID clk_id, char *clk_name)
{
	avio_debug("%s:%d:VPP_CLK[%d] : %d = %s\n",
			__func__, __LINE__, hVppCtx->clk_list_count,
				clk_id, clk_name);

	if (hVppCtx->clk_list_count < VPP_CLOCK_LIST_MAX) {
		int ndx = hVppCtx->clk_list_count;

		hVppCtx->clk_list[ndx].clk_id = clk_id;
		hVppCtx->clk_list[ndx].clk_name = clk_name;
		hVppCtx->clk_list[ndx].vpp_clk =
			of_clk_get_by_name(np, hVppCtx->clk_list[ndx].clk_name);
		hVppCtx->clk_list_count++;
	}
}

void drv_vpp_add_vpp_interrupt_num(VPP_CTX *hVppCtx,
		int intr_num, int intr_type)
{
	avio_debug("%s:%d:VPP_DHUB_INTR[%d] : %d = %d\n",
			__func__, __LINE__, hVppCtx->vpp_interrupt_list_count,
				intr_num, intr_type);

	if (hVppCtx->vpp_interrupt_list_count < VPP_INTERRUPT_LIST_MAX) {
		int ndx = hVppCtx->vpp_interrupt_list_count;

		hVppCtx->vpp_interrupt_list[ndx].intr_num = intr_num;
		hVppCtx->vpp_interrupt_list[ndx].intr_type = intr_type;
		hVppCtx->vpp_interrupt_list_count++;

		if (intr_type == VPP_INTR_TYPE_VBI)
			hVppCtx->vsync_intr_num = intr_num;
		if (intr_type == VPP_INTR_TYPE_CPCB1_VBI)
			hVppCtx->vsync1_intr_num = intr_num;
		if (intr_type == VPP_INTR_TYPE_HPD)
			hVppCtx->hpd_intr_num = intr_num;
		if (intr_type == VPP_INTR_TYPE_VDE)
			hVppCtx->vde_intr_num = intr_num;
	}
}

static int drv_vpp_suspend(void *h_vpp_ctx)
{
	int ret;
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;

	/* During Suspend set the mipi reset line */
	gpiod_set_value_cansleep(hVppCtx->gpio_mipirst, 1);

	ret = wrap_MV_VPPOBJ_Suspend(1);
	if (ret != MV_VPP_OK) {
		avio_error("%s VPP Suspend failed\n", __func__);
		return -1;
	}

#ifndef CONFIG_IRQ_LATENCY_PROFILE
	/* Remove any stale intr messages */
	while (AMPMsgQ_Fullness(&hVppCtx->hVPPMsgQ) > 0) {
		MV_CC_MSG_t msg;

		AMPMsgQ_DequeueRead(&hVppCtx->hVPPMsgQ, &msg);

		avio_trace("%s VPP Suspend Dropped-intr : 0x%x\n", __func__, msg.m_Param1);

		//Process(Decrement count) pop'ed message
		avio_devices_vpp_process_msg(hVppCtx, &msg);
	}
#endif //CONFIG_IRQ_LATENCY_PROFILE

	return 0;
}

static int drv_vpp_resume(void *hVppCtx)
{
	int ret;
	VPP_CTX *h_vpp_ctx = (VPP_CTX *)hVppCtx;

	/* During Resume clear the mipi reset line */
	gpiod_set_value_cansleep(h_vpp_ctx->gpio_mipirst, 0);

	ret = wrap_MV_VPPOBJ_Suspend(0);
	if (ret != MV_VPP_OK) {
		avio_error("%s VPP Resume failed\n", __func__);
		return -1;
	}

	return 0;
}

static const AVIO_MODULE_FUNC_TABLE vpp_drv_fops = {
	.open = drv_vpp_open,
	.close = drv_vpp_close,
	.config = drv_vpp_config,
	.ioctl_unlocked = drv_vpp_ioctl_unlocked,
	.create_proc_file = drv_vpp_create_proc_file,
	.remove_proc_file = drv_vpp_remove_proc_file,
	.restore_state = drv_vpp_resume,
	.save_state = drv_vpp_suspend
};

int avio_module_drv_vpp_probe(struct platform_device *dev)
{
	VPP_CTX *hVppCtx = &vpp_ctx;

	avio_trace("%s:%d:\n", __func__, __LINE__);
	hVppCtx->dev = &dev->dev;
	hVppCtx->is_bootup_quiescent = avio_util_get_quiescent_flag();
	avio_sub_module_register(AVIO_MODULE_TYPE_VPP, VPP_MODULE_NAME,
			hVppCtx, &vpp_drv_fops);

	sema_init(&hVppCtx->vpp_sem, 0);
	sema_init(&hVppCtx->vsync_sem, 0);
	sema_init(&hVppCtx->vsync1_sem, 0);
	spin_lock_init(&hVppCtx->vpp_msg_spinlock);
	spin_lock_init(&hVppCtx->bcm_spinlock);

	atomic_set(&hVppCtx->vsync_cnt, 0);
	atomic_set(&hVppCtx->vsync1_cnt, 0);
	atomic_set(&hVppCtx->vde_cnt, 0);

	return 0;
}

void drv_mipi_reset(void *h_vpp_ctx, int enable)
{
	VPP_CTX *hVppCtx = (VPP_CTX *)h_vpp_ctx;

	gpiod_set_value_cansleep(hVppCtx->gpio_mipirst, enable);
}