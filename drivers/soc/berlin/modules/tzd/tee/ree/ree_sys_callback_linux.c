/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ******************************************************************************/

#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/export.h>
#include <linux/namei.h>
#include <linux/delay.h>
#include <asm-generic/errno-base.h>
#include "config.h"
#include "ree_sys_callback_cmd.h"
#include "ree_sys_callback.h"
#include "ree_sys_callback_logger.h"
#ifdef CONFIG_REE_SYS_MUTEX
#include "ree_sys_callback_mutex.h"
#endif
#ifdef CONFIG_TEE_SYS_MUTEX
#include "ree_sys_callback_mutex_ext.h"
#endif
#include "tee_comm.h"
#include "log.h"

/*
 * Command functions
 */
#ifdef CONFIG_64BIT

#define uint64_to_ptr(u, p)					\
	do {							\
		p = (void*)(u);					\
	} while (0)

#define ptr_to_uint32(p, h, l)					\
	do {							\
		h = (uint32_t)((uint64_t)(p) >> 32);		\
		l = (uint32_t)((uint64_t)(p) & 0xffffffff);	\
	} while (0)

#else /* CONFIG_64BIT */

#define uint64_to_ptr(u, p)					\
	do {							\
		p = (void*)((uint32_t)(u) & 0xffffffff);	\
	} while (0)

#define ptr_to_uint32(p, h, l)					\
	do {							\
		h = 0;						\
		l = (uint32_t)(p);				\
	} while (0)

#endif /* CONFIG_64BIT */


static TEE_Result REESysCmd_GetTime(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	struct timespec64 ts;
	

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_OUTPUT)
		return TEE_ERROR_BAD_PARAMETERS;

	ktime_get_real_ts64(&ts);

	params[0].value.a = ts.tv_sec;
	params[0].value.b = ts.tv_nsec/1000;

	return TEE_SUCCESS;
}

#define LOGGER_BUF_SIZE		512
#define TZD_TAG "tzd"
static DEFINE_PER_CPU(char [LOGGER_BUF_SIZE], logger_buf);

static int default_linux_log_print(struct ree_logger_param *param)
{
	static const char *__kernel_log_head[] = {
		KERN_WARNING TZD_TAG "(%s) ",
		KERN_NOTICE TZD_TAG "(%s) ",
		KERN_INFO TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
	};
	char *buf;
	size_t n;
	unsigned long flags;

	local_irq_save(flags);
	buf  = *this_cpu_ptr(&logger_buf);
	n = snprintf(buf, LOGGER_BUF_SIZE, __kernel_log_head[param->prio & 0x7], param->tag);
	if (likely(n < LOGGER_BUF_SIZE))
		strlcpy(buf+n, param->text, LOGGER_BUF_SIZE-n);
	buf[LOGGER_BUF_SIZE-1] = 0; // make sure null terminated
	printk((const char *)buf);
	local_irq_restore(flags);

	return 0;
}

static BLOCKING_NOTIFIER_HEAD(ree_logger_notifier_list);

int register_ree_logger_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&ree_logger_notifier_list, nb);
}
EXPORT_SYMBOL(register_ree_logger_notifier);

int unregister_ree_logger_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&ree_logger_notifier_list, nb);
}
EXPORT_SYMBOL(unregister_ree_logger_notifier);

static TEE_Result REESysCmd_LogWrite(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	REE_Logger_t *logger = (REE_Logger_t *) (param_ext);
	uint32_t inParamTypes = TEE_PARAM_TYPES(
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_VALUE_INPUT);
	struct ree_logger_param param;
	int nr_calls = 0;

	if (unlikely(paramTypes != inParamTypes))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[0].value.a != REE_LOGGER_PARAM_BUFID))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[1].value.a != REE_LOGGER_PARAM_PRIO))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[2].value.a != REE_LOGGER_PARAM_TAG)
		|| unlikely(params[2].value.b > REE_LOGGER_TAG_SIZE))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[3].value.a != REE_LOGGER_PARAM_TEXT)
		|| unlikely(params[3].value.b > REE_LOGGER_TEXT_SIZE))
		return TEE_ERROR_BAD_PARAMETERS;

	param.bufID = params[0].value.b;
	param.prio  = params[1].value.b;
	param.tag   = logger->buffer.tag;
	param.text  = logger->buffer.text;

	if (!in_interrupt())
		__blocking_notifier_call_chain(&ree_logger_notifier_list,
				0, (void *) (&param), -1, &nr_calls);

	if (!nr_calls)
		default_linux_log_print(&param);

	return TEE_SUCCESS;
}

#ifdef CONFIG_REE_SYS_WAIT
static TEE_Result REESysCmd_Wait(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	unsigned long delay_ms, delay_us;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
		return TEE_ERROR_BAD_PARAMETERS;

	delay_ms = params[0].value.a;
	delay_us = delay_ms * 1000;

	usleep_range(delay_us, delay_us);

	return TEE_SUCCESS;
}
#endif

TEE_Result REE_InvokeSysCommandEntryPoint(
		void		*sessionContext,
		uint32_t	commandID,
		uint32_t	paramTypes,
		TEE_Param	params[4],
		void		*param_ext,
		uint32_t	param_ext_size)
{
	TEE_Result res = TEE_ERROR_NOT_SUPPORTED;

	trace("commandID=%d, paramTypes=0x%08x\n", commandID, paramTypes);

	switch (commandID) {
	case REE_CMD_TIME_GET:
		res = REESysCmd_GetTime(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_LOG_WRITE:
		res = REESysCmd_LogWrite(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#ifdef CONFIG_REE_SYS_WAIT
	case REE_CMD_TIME_WAIT:
		res = REESysCmd_Wait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#endif

#ifdef CONFIG_TEE_SYS_MUTEX
	case REE_CMD_MUTEX_EXT_WAIT:
		res = REESysCmd_MutexWait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_EXT_WAKE:
		res = REESysCmd_MutexWake(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_EXT_DEL:
		res = REESysCmd_MutexDel(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#endif
	}

	return res;
}
