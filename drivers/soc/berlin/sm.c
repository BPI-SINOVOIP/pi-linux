// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Synaptics Incorporated
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>
#include <linux/reboot-mode.h>
#include <linux/semaphore.h>
#include <linux/sched/signal.h>
#include <soc/berlin/sm.h>

#define SM_MSG_SIZE		32
#define SM_MSG_BODY_SIZE	(SM_MSG_SIZE - sizeof(short) * 2)
#define SM_MSGQ_TOTAL_SIZE	512
#define SM_MSGQ_HEADER_SIZE	SM_MSG_SIZE
#define SM_MSGQ_SIZE		(SM_MSGQ_TOTAL_SIZE - SM_MSGQ_HEADER_SIZE )
#define SM_MSGQ_MSG_COUNT	(SM_MSGQ_SIZE/SM_MSG_SIZE)

#define SOC_MSGQ_START			(sm_base + msgq_offset)
#define SM_CPU1_OUTPUT_QUEUE_ADDR	(SOC_MSGQ_START + SM_MSGQ_TOTAL_SIZE * 0)
#define SM_CPU0_INPUT_QUEUE_ADDR	(SOC_MSGQ_START + SM_MSGQ_TOTAL_SIZE * 1)
#define SM_CPU1_INPUT_QUEUE_ADDR	(SOC_MSGQ_START + SM_MSGQ_TOTAL_SIZE * 2)
#define SM_CPU0_OUTPUT_QUEUE_ADDR	(SOC_MSGQ_START + SM_MSGQ_TOTAL_SIZE * 3)

#define REBOOT_MODE_ADDR		(sm_base + msgq_offset - 0x100)

#define BERLIN_MSGQ_OFFSET		0x1f000
#define VS640_MSGQ_OFFSET		0x17000

typedef struct
{
	short	m_iModuleID;
	short	m_iMsgLen;
	char	m_pucMsgBody[SM_MSG_BODY_SIZE];
} MV_SM_Message;

typedef struct
{
	int	m_iWrite;
	int	m_iRead;
	int	m_iWriteTotal;
	int	m_iReadTotal;
	char	m_Padding[SM_MSGQ_HEADER_SIZE - sizeof(int) * 4];
	char	m_Queue[SM_MSGQ_SIZE];
} MV_SM_MsgQ;

struct bsm_wakeup_event {
	MV_SM_WAKEUP_SOURCE_TYPE type;
	bool set;
	bool bypass;
	char *name;
	struct semaphore resume_sem;
};

static int sm_irq;
static void __iomem *sm_base;
static void __iomem *sm_ctrl;
static struct thermal_zone_device *bsm_thermal;
static u32 msgq_offset;
static struct reboot_mode_driver bsm_reboot;
static struct notifier_block bsm_reboot_nb;
static DEFINE_MUTEX(thermal_lock);
static struct bsm_wakeup_event wakeup_events[] = {
	{
		.type = MV_SM_WAKEUP_SOURCE_IR,
		.name = "IR",
		.bypass = true,
	},
	{
		.type = MV_SM_WAKEUP_SOURCE_WIFI_BT,
		.name = "WIFI_BT",
	},
	{
		.type = MV_SM_WAKEUP_SOURCE_WOL,
		.name = "WOL",
	},
	{
		.type = MV_SM_WAKEUP_SOURCE_VGA,
		.name = "VGA",
	},
	{
		.type = MV_SM_WAKEUP_SOURCE_CEC,
		.name = "CEC",
	},
	{
		.type = MV_SM_WAKEUP_SOURCE_TIMER,
		.name = "TIMER",
	},
	{
		.type = MV_SM_WAKEUP_SOURCE_BUTTON,
		.name = "BUTTON",
		.bypass = true,
	},
};

#define SM_Q_PUSH( pSM_Q ) {				\
	pSM_Q->m_iWrite += SM_MSG_SIZE;			\
	if( pSM_Q->m_iWrite >= SM_MSGQ_SIZE )		\
		pSM_Q->m_iWrite -= SM_MSGQ_SIZE;	\
	pSM_Q->m_iWriteTotal += SM_MSG_SIZE; }

#define SM_Q_POP( pSM_Q ) {				\
	pSM_Q->m_iRead += SM_MSG_SIZE;			\
	if( pSM_Q->m_iRead >= SM_MSGQ_SIZE )		\
		pSM_Q->m_iRead -= SM_MSGQ_SIZE;		\
	pSM_Q->m_iReadTotal += SM_MSG_SIZE; }

static int bsm_link_msg_nolock(MV_SM_MsgQ *q, MV_SM_Message *m)
{
	MV_SM_Message *p;

	if (q->m_iWrite < 0 || q->m_iWrite >= SM_MSGQ_SIZE)
		/* buggy ? */
		return -EIO;

	/* message queue full, ignore the newest message */
	if (q->m_iRead == q->m_iWrite && q->m_iReadTotal != q->m_iWriteTotal)
		return -EBUSY;

	p = (MV_SM_Message*)(&(q->m_Queue[q->m_iWrite]));
	memcpy(p, m, sizeof(*p));
	SM_Q_PUSH(q);

	return 0;
}

static int bsm_link_msg(MV_SM_MsgQ *q, MV_SM_Message *m, spinlock_t *lock)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	ret = bsm_link_msg_nolock(q, m);
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

static int bsm_unlink_msg_nolock(MV_SM_MsgQ *q, MV_SM_Message *m)
{
	MV_SM_Message *p;
	int ret = -EAGAIN; /* means no data */

	if (q->m_iRead < 0 || q->m_iRead >= SM_MSGQ_SIZE ||
			q->m_iReadTotal > q->m_iWriteTotal)
		/* buggy ? */
		return -EIO;

	/* if buffer was overflow written, only the last messages are
	 * saved in queue. move read pointer into the same position of
	 * write pointer and keep buffer full.
	 */
	if (q->m_iWriteTotal - q->m_iReadTotal > SM_MSGQ_SIZE) {
		int iTotalDataSize = q->m_iWriteTotal - q->m_iReadTotal;

		q->m_iReadTotal += iTotalDataSize - SM_MSGQ_SIZE;
		q->m_iRead += iTotalDataSize % SM_MSGQ_SIZE;
		if (q->m_iRead >= SM_MSGQ_SIZE)
			q->m_iRead -= SM_MSGQ_SIZE;
	}

	if (q->m_iReadTotal < q->m_iWriteTotal) {
		/* alright get one message */
		p = (MV_SM_Message*)(&(q->m_Queue[q->m_iRead]));
		memcpy(m, p, sizeof(*m));
		SM_Q_POP(q);
		ret = 0;
	}

	return ret;
}

static int bsm_unlink_msg(MV_SM_MsgQ *q, MV_SM_Message *m, spinlock_t *lock)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	ret = bsm_unlink_msg_nolock(q, m);
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

static DEFINE_SPINLOCK(sm_lock);

static inline int bsm_link_msg_to_sm(MV_SM_Message *m)
{
	MV_SM_MsgQ *q = (MV_SM_MsgQ *)SM_CPU0_INPUT_QUEUE_ADDR;

	return bsm_link_msg(q, m, &sm_lock);
}

static inline int bsm_unlink_msg_from_sm(MV_SM_Message *m)
{
	MV_SM_MsgQ *q = (MV_SM_MsgQ *)SM_CPU0_OUTPUT_QUEUE_ADDR;

	return bsm_unlink_msg(q, m, &sm_lock);
}

#define DEFINE_SM_MODULES(id)				\
	{						\
		.m_iModuleID  = id,			\
	}

typedef struct {
	int m_iModuleID;
	wait_queue_head_t m_wq;
	spinlock_t m_Lock;
	MV_SM_MsgQ m_MsgQ;
	struct mutex m_Mutex;
} MV_SM_Module;

static MV_SM_Module SMModules[MAX_MSG_TYPE] = {
	DEFINE_SM_MODULES(MV_SM_ID_SYS),
	DEFINE_SM_MODULES(MV_SM_ID_COMM),
	DEFINE_SM_MODULES(MV_SM_ID_IR),
	DEFINE_SM_MODULES(MV_SM_ID_KEY),
	DEFINE_SM_MODULES(MV_SM_ID_POWER),
	DEFINE_SM_MODULES(MV_SM_ID_WD),
	DEFINE_SM_MODULES(MV_SM_ID_TEMP),
	DEFINE_SM_MODULES(MV_SM_ID_VFD),
	DEFINE_SM_MODULES(MV_SM_ID_SPI),
	DEFINE_SM_MODULES(MV_SM_ID_I2C),
	DEFINE_SM_MODULES(MV_SM_ID_UART),
	DEFINE_SM_MODULES(MV_SM_ID_CEC),
	DEFINE_SM_MODULES(MV_SM_ID_WOL),
	DEFINE_SM_MODULES(MV_SM_ID_LED),
	DEFINE_SM_MODULES(MV_SM_ID_ETH),
	DEFINE_SM_MODULES(MV_SM_ID_DDR),
	DEFINE_SM_MODULES(MV_SM_ID_WIFIBT),
	DEFINE_SM_MODULES(MV_SM_ID_DEBUG),
	DEFINE_SM_MODULES(MV_SM_ID_CONSOLE),
	DEFINE_SM_MODULES(MV_SM_ID_PMIC),
	DEFINE_SM_MODULES(MV_SM_ID_AUDIO),
};

static inline MV_SM_Module *bsm_search_module(int id)
{
	if (id > 0 && id <= ARRAY_SIZE(SMModules))
		return &(SMModules[id-1]);
	else
		return NULL;
}

static int bsm_link_msg_to_module(MV_SM_Message *m)
{
	MV_SM_Module *module;
	int ret;

	module = bsm_search_module(m->m_iModuleID);
	if (!module)
		return -EINVAL;

	ret = bsm_link_msg(&(module->m_MsgQ), m, &(module->m_Lock));
	if (ret == 0) {
		/* wake up any process pending on wait-queue */
		wake_up_interruptible(&(module->m_wq));
	}

	return ret;
}

static int bsm_unlink_msg_from_module(MV_SM_Message *m)
{
	MV_SM_Module *module;
	DEFINE_WAIT(__wait);
	unsigned long flags;
	int ret;

	module = bsm_search_module(m->m_iModuleID);
	if (!module)
		return -EINVAL;

	for (;;) {
		prepare_to_wait(&(module->m_wq), &__wait, TASK_INTERRUPTIBLE);
		spin_lock_irqsave(&(module->m_Lock), flags);
		ret = bsm_unlink_msg_nolock(&(module->m_MsgQ), m);
		spin_unlock_irqrestore(&(module->m_Lock), flags);
		if (ret != -EAGAIN)
			break;

		schedule();

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}
	}
	finish_wait(&(module->m_wq), &__wait);

	return ret;
}

static LIST_HEAD(sm_ir_handler);
static DEFINE_SPINLOCK(sm_ir_lock);

void register_sm_ir_handler(struct sm_ir_handler *handler)
{
	unsigned long flags;

	spin_lock_irqsave(&sm_ir_lock, flags);
	list_add_rcu(&handler->node, &sm_ir_handler);
	spin_unlock_irqrestore(&sm_ir_lock, flags);
}
EXPORT_SYMBOL(register_sm_ir_handler);

void unregister_sm_ir_handler(struct sm_ir_handler *handler)
{
	unsigned long flags;

	spin_lock_irqsave(&sm_ir_lock, flags);
	list_del_rcu(&handler->node);
	spin_unlock_irqrestore(&sm_ir_lock, flags);
	synchronize_rcu();
}
EXPORT_SYMBOL(unregister_sm_ir_handler);

static void call_sm_ir_handler(int ir_key)
{
	struct sm_ir_handler *handler;

	rcu_read_lock();
	list_for_each_entry_rcu(handler, &sm_ir_handler, node) {
		if (!handler->key || handler->key == ir_key)
			handler->fn(ir_key);
	}
	rcu_read_unlock();
}

int bsm_msg_send(int id, void *msg, int len)
{
	MV_SM_Message m = {0};
	int ret;
	int cnt_timeout = 3 * 100;

	if (unlikely(len < 4) || unlikely(len > SM_MSG_BODY_SIZE))
		return -EINVAL;

	m.m_iModuleID = id;
	m.m_iMsgLen   = len;
	memcpy(m.m_pucMsgBody, msg, len);
	for (;;) {
		ret = bsm_link_msg_to_sm(&m);
		if (ret != -EBUSY)
			break;
		mdelay(10);

		cnt_timeout--;
		if (cnt_timeout < 0)
			break;
	}

	return ret;
}
EXPORT_SYMBOL(bsm_msg_send);

int bsm_msg_recv(int id, void *msg, int *len)
{
	MV_SM_Message m;
	int ret;

	m.m_iModuleID = id;
	ret = bsm_unlink_msg_from_module(&m);
	if (ret)
		return ret;

	if (msg)
		memcpy(msg, m.m_pucMsgBody, m.m_iMsgLen);

	if (len)
		*len = m.m_iMsgLen;

	return 0;
}
EXPORT_SYMBOL(bsm_msg_recv);

static void bsm_handle_wakeup_event(struct bsm_wakeup_event *wk)
{
	if (wk->set && !wk->bypass) {
		wk->set = false;
		up(&wk->resume_sem);
	}
}

static void bsm_msg_dispatch(void)
{
	MV_SM_Message m;
	int ret;

	/* read all messages from SM buffers and dispatch them */
	for (;;) {
		ret = bsm_unlink_msg_from_sm(&m);
		if (ret)
			break;

		if (m.m_iModuleID == MV_SM_ID_IR && m.m_iMsgLen == 4) {
			/* special case for IR events */
			int ir_key = *(int *)m.m_pucMsgBody;
			call_sm_ir_handler(ir_key);
		} else if (m.m_iModuleID == MV_SM_ID_POWER &&
				m.m_iMsgLen == 4 &&
				*(int *)m.m_pucMsgBody == 0) {
			struct bsm_wakeup_event *wk;
			wk = &wakeup_events[MV_SM_WAKEUP_SOURCE_TIMER];
			bsm_handle_wakeup_event(wk);
		} else {
			/* try best to dispatch received message */
			ret = bsm_link_msg_to_module(&m);
			if (ret != 0) {
				printk(KERN_ERR "Drop SM message\n");
				continue;
			}
		}
	}
}

static irqreturn_t bsm_intr(int irq, void *dev_id)
{
	u32 val;

	val = readl_relaxed(sm_ctrl);
	val &= ~(1 << 1);
	writel_relaxed(val, sm_ctrl);

	bsm_msg_dispatch();

	return IRQ_HANDLED;
}

static ssize_t bsm_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	MV_SM_Message m;
	int id = (int)(*ppos);
	int ret;

	if (count < SM_MSG_SIZE)
		return -EINVAL;

	m.m_iModuleID = id;
	ret = bsm_unlink_msg_from_module(&m);
	if (!ret) {
		if (copy_to_user(buf, (void *)&m, SM_MSG_SIZE))
			return -EFAULT;
		return SM_MSG_SIZE;
	}

	return 0;
}

static ssize_t bsm_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	MV_SM_Message SM_Msg;
	int ret;
	int id = (int)(*ppos);

	if (count < 4 || count > SM_MSG_BODY_SIZE)
		return -EINVAL;

	if (copy_from_user(SM_Msg.m_pucMsgBody, buf, count))
		return -EFAULT;

	ret = bsm_msg_send(id, SM_Msg.m_pucMsgBody, count);
	if (ret < 0)
		return -EFAULT;
	else
		return count;
}

static long bsm_unlocked_ioctl(struct file *file,
			unsigned int cmd, unsigned long arg)
{
	MV_SM_Module *module;
	MV_SM_Message m;
	int ret = 0, id;

	/* for legacy, will be removed */
	if (cmd == SM_Enable_WaitQueue || cmd == SM_Disable_WaitQueue)
		return 0;

	if (cmd == SM_WAIT_WAKEUP) {
		id = arg;
		if (id < 0 || id >= ARRAY_SIZE(wakeup_events) ||
			wakeup_events[id].bypass)
			return -EINVAL;

		wakeup_events[id].set = true;
		return down_interruptible(&wakeup_events[id].resume_sem);
	}

	if (copy_from_user(&m, (void __user *)arg, SM_MSG_SIZE))
		return -EFAULT;
	id = m.m_iModuleID;

	module = bsm_search_module(id);
	if (!module)
		return -EINVAL;

	mutex_lock(&(module->m_Mutex));

	switch (cmd) {
	case SM_READ:
		ret = bsm_unlink_msg_from_module(&m);
		if (!ret) {
			if (copy_to_user((void __user *)arg, &m, SM_MSG_SIZE))
				ret = -EFAULT;
		}
		break;
	case SM_WRITE:
		ret = bsm_msg_send(m.m_iModuleID, m.m_pucMsgBody, m.m_iMsgLen);
		break;
	case SM_RDWR:
		ret = bsm_msg_send(m.m_iModuleID, m.m_pucMsgBody, m.m_iMsgLen);
		if (ret)
			break;
		ret = bsm_unlink_msg_from_module(&m);
		if (!ret) {
			if (copy_to_user((void __user *)arg, &m, SM_MSG_SIZE))
				ret = -EFAULT;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&(module->m_Mutex));

	return ret;
}

static struct file_operations bsm_fops = {
	.owner		= THIS_MODULE,
	.write		= bsm_write,
	.read		= bsm_read,
	.unlocked_ioctl	= bsm_unlocked_ioctl,
	.compat_ioctl	= bsm_unlocked_ioctl,
	.llseek		= default_llseek,
};

static struct miscdevice sm_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "bsm",
	.fops	= &bsm_fops,
};

static int bsm_get_temp(struct thermal_zone_device *thermal, int *temp)
{
	int ret, msg, len, rcv[4];
	static int prev_temp = 0;

	mutex_lock(&thermal_lock);

	msg = MV_SM_TEMP_SAMPLE;
	ret = bsm_msg_send(MV_SM_ID_TEMP, &msg, sizeof(msg));
	if (ret < 0) {
		*temp = prev_temp;
		goto get_temp_error;
	}

	ret = bsm_msg_recv(MV_SM_ID_TEMP, rcv, &len);
	if (ret < 0) {
		*temp = prev_temp;
		goto get_temp_error;
	}

	if (len != 16) {
		ret = -EIO;
		*temp = prev_temp;
		goto get_temp_error;
	}

	*temp = rcv[3] * 1000;

get_temp_error:
	mutex_unlock(&thermal_lock);

	return ret;
}

static struct thermal_zone_device_ops ops = {
	.get_temp = bsm_get_temp,
};

static int bsm_reboot_mode_write(struct reboot_mode_driver *reboot,
				 unsigned int magic)
{
	writel_relaxed(magic, REBOOT_MODE_ADDR);

	return 0;
}

/*
 * Most reboot modes should be handled by reboot-mode driver, but we
 * do need an extra reboot nb to handle those modes which can't be
 * supported by reboot-mode driver, for example, there's a space in
 * the mode name: "foo bar".
 */
static int bsm_reboot_notify(struct notifier_block *this,
			     unsigned long mode, void *cmd)
{
	if (!cmd)
		return NOTIFY_DONE;

	if (!strcmp(cmd, "dm-verity device corrupted"))
		writel_relaxed(0x12513991, REBOOT_MODE_ADDR);

	return NOTIFY_DONE;
}

static int bsm_probe(struct platform_device *pdev)
{
	int i, ret;
	struct resource *r;
	struct resource *r_sm_ctrl;
	resource_size_t size;
	const char *name;

	msgq_offset = (uintptr_t)of_device_get_match_data(&pdev->dev);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!r || resource_type(r) != IORESOURCE_MEM) {
		dev_err(&pdev->dev, "invalid resource\n");
		return -EINVAL;
	}

	size = resource_size(r);
	name = r->name ?: dev_name(&pdev->dev);

	if (!devm_request_mem_region(&pdev->dev, r->start, size, name)) {
		dev_err(&pdev->dev, "can't request region for resource %pR\n", r);
		return -EBUSY;
	}

	if (of_property_read_bool(pdev->dev.of_node, "no-memory-wc"))
		sm_base = devm_ioremap(&pdev->dev, r->start, size);
	else
		sm_base = devm_ioremap_wc(&pdev->dev, r->start, size);
	if (IS_ERR(sm_base))
		return PTR_ERR(sm_base);

	r_sm_ctrl = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	sm_ctrl = devm_ioremap_resource(&pdev->dev, r_sm_ctrl);
	if (IS_ERR(sm_ctrl))
		return PTR_ERR(sm_ctrl);

	sm_irq = platform_get_irq(pdev, 0);
	if (sm_irq < 0)
		return -ENXIO;

	for (i = 0; i < ARRAY_SIZE(SMModules); i++) {
		init_waitqueue_head(&(SMModules[i].m_wq));
		spin_lock_init(&(SMModules[i].m_Lock));
		mutex_init(&(SMModules[i].m_Mutex));
		memset(&(SMModules[i].m_MsgQ), 0, sizeof(MV_SM_MsgQ));
	}

	ret = devm_request_irq(&pdev->dev, sm_irq, bsm_intr, 0, "bsm", &sm_dev);
	if (ret < 0)
		return ret;

	for (i = 0; i < ARRAY_SIZE(wakeup_events); i++)
		sema_init(&wakeup_events[i].resume_sem, 0);

	ret = misc_register(&sm_dev);
	if (ret < 0)
		return ret;

	bsm_thermal = thermal_zone_device_register("bsm_thermal", 0, 0,
						   NULL, &ops, NULL, 0, 0);
	if (IS_ERR(bsm_thermal)) {
		dev_warn(&pdev->dev,
			 "Failed to register thermal zone device\n");
		bsm_thermal = NULL;
	}

	bsm_reboot_nb.notifier_call = bsm_reboot_notify;
	register_reboot_notifier(&bsm_reboot_nb);

	bsm_reboot.dev = &pdev->dev;
	bsm_reboot.write = bsm_reboot_mode_write;
	/* clear reboot reason */
	writel_relaxed(0, REBOOT_MODE_ADDR);
	return devm_reboot_mode_register(&pdev->dev, &bsm_reboot);
}

static int bsm_remove(struct platform_device *pdev)
{
	misc_deregister(&sm_dev);

	if (bsm_thermal)
		thermal_zone_device_unregister(bsm_thermal);

	return 0;
}

#ifdef CONFIG_PM
static int bsm_resume(struct device *dev)
{
	int ret, msg, len, rcv[3];

	msg = MV_SM_POWER_WAKEUP_SOURCE_REQUEST;
	ret = bsm_msg_send(MV_SM_ID_POWER, &msg, sizeof(msg));
	if (ret < 0)
		return ret;

	ret = bsm_msg_recv(MV_SM_ID_POWER, rcv, &len);
	if (ret < 0)
		return ret;
	if (len != 12)
		return -EIO;

	ret = rcv[0];
	if (ret >= 0 && ret < ARRAY_SIZE(wakeup_events)) {
		struct bsm_wakeup_event *wk = &wakeup_events[ret];
		printk("wakeup from: %s\n", wk->name);
		bsm_handle_wakeup_event(wk);
	}

	return 0;
}

static struct dev_pm_ops bsm_pm_ops = {
	.resume		= bsm_resume,
};
#endif

static const struct of_device_id bsm_match[] = {
	{
		.compatible = "marvell,berlin-sm",
		.data = (void *)BERLIN_MSGQ_OFFSET
	},
	{
		.compatible = "syna,vs640-sm",
		.data = (void *)VS640_MSGQ_OFFSET
	},
	{},
};
MODULE_DEVICE_TABLE(of, bsm_match);

static struct platform_driver bsm_driver = {
	.probe		= bsm_probe,
	.remove		= bsm_remove,
	.driver = {
		.name	= "marvell,berlin-sm",
		.owner	= THIS_MODULE,
		.of_match_table = bsm_match,
#ifdef CONFIG_PM
		.pm	= &bsm_pm_ops,
#endif
	},
};
module_platform_driver(bsm_driver);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("System Manager Driver");
MODULE_LICENSE("GPL");
