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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/moduleparam.h>
#include <linux/hardirq.h>

#include "config.h"
#include "tz_driver.h"
#include "tz_nw_ioctl.h"
#include "tz_nw_api.h"
#include "tz_nw_comm.h"
#include "tz_client_api.h"
#include "tz_boot_cmd.h"

#define TZC_DEV_ID	(1)

#ifdef CONFIG_DEBUG
#define tz_debug(fmt, args...) printk(KERN_DEBUG "TZ %s(%i, %s): " fmt "\n", \
         __func__, current->pid, current->comm, ## args)
#else
#define tz_debug(...)
#endif

#define tz_error(fmt, args...) printk(KERN_ERR "TZ ERROR %s(%i, %s): " fmt "\n", \
         __func__, current->pid, current->comm, ## args)

#define tz_info(fmt, args...) printk(KERN_INFO "TZ %s(%i, %s): " fmt "\n", \
         __func__, current->pid, current->comm, ## args)

void *tzc_malloc(uint32_t size)
{
	if (!size) return NULL;
	return kmalloc(size, GFP_KERNEL);
}

void tzc_free(void *ptr)
{
	if (ptr) kfree(ptr);
}

unsigned long tzc_strtoul(const char *nptr, char **endptr, int base)
{
	return simple_strtoul(nptr, endptr, base);
}

char *tzc_strdup(const char *str)
{
	if (!str)
		return NULL;

	return kstrdup(str, GFP_KERNEL);
}

int tzc_open(const char *dev)
{
	return TZC_DEV_ID;
}

void tzc_close(int tzc)
{
}

void *tzc_alloc_shm(int tzc, size_t size, void **pa)
{
	void *va;
	int ret;

	/*if in_interrupt or irqs_disabled,can't alloc shared memory*/
	if (in_interrupt() || irqs_disabled()) {
		*pa = NULL;
		return NULL;
	}

	ret = tzd_kernel_alloc_mem(&va, pa, size);
	if (ret) {
		*pa = NULL;
		return NULL;
	}
	return va;
}

void tzc_free_shm(int tzc, void *va, void *pa, size_t size)
{
	tzd_kernel_free_mem(pa);
}

int tz_invoke_command(int tzc, int task_id, uint32_t cmd_id,
			uint32_t param, uint32_t *origin,
			tz_cmd_handler callback, void *userdata, void *call_info)
{
	uint32_t call_id = param;
	struct tz_nw_comm cc;
	struct tz_nw_task_client *tc = tz_nw_task_client_get(task_id);
	int ret;

	cc.call.task_id = task_id;
	cc.call.cmd_id = cmd_id;
	cc.call.param = param;

	while (1) {
		ret = tz_nw_comm_invoke_command(tc, &cc, call_id, call_info);
		if (ret != TZ_PENDING)
			break;

		tz_debug("user callback cmd=%d, param=0x%08x\n",
			cc.callback.cmd_id, cc.callback.param);

		if (callback) {
			cc.callback.result = callback(userdata,
					cc.callback.cmd_id,
					cc.callback.param,
					&cc.callback.origin);
		} else {
			cc.callback.result = TZ_ERROR_NOT_SUPPORTED;
			cc.callback.origin = TZ_ORIGIN_UNTRUSTED_APP;
		}
	}

	if (ret != 0) {
		cc.call.origin = TZ_ORIGIN_API;
		cc.call.result = TZ_ERROR_COMMUNICATION;
	}

	if (origin)
		*origin = cc.call.origin;

	return cc.call.result;
}

int tzc_invoke_command(int tzc, int task_id, uint32_t cmd_id,
			uint32_t param, uint32_t *origin,
			tz_cmd_handler callback, void *userdata)
{
	void *kernel_dev_file = tzd_get_kernel_dev_file();

	return tz_invoke_command(tzc, task_id, cmd_id, param,
			origin, callback, userdata, kernel_dev_file);
}

/*
 * mutex function in kernel.
 *
 * need think about how to support different cases in kernel:
 * - call in normal cases
 * - call when irq is disabled
 * - call in isr
 */
void *tzc_mutex_create(void)
{
	struct mutex *lock;
	lock = kmalloc(sizeof(*lock), GFP_KERNEL);
	if (!lock)
		return NULL;
	mutex_init(lock);
	return lock;
}

int tzc_mutex_destroy(void *lock)
{
	if (lock)
		kfree(lock);
	return TZ_SUCCESS;
}

int tzc_mutex_lock(void *lock)
{
	if (lock && !in_atomic()) {
		/* if it's interrupted by signal, then try again */
		while (-EINTR == mutex_lock_interruptible(lock));
	}
	return TZ_SUCCESS;
}

int tzc_mutex_trylock(void *lock)
{
	if (lock && !in_atomic())
		mutex_trylock(lock);
	return TZ_SUCCESS;
}

int tzc_mutex_unlock(void *lock)
{
	if (lock && !in_atomic())
		mutex_unlock(lock);
	return TZ_SUCCESS;
}

#ifdef CONFIG_TEE

struct tee_comm *tzc_acquire_tee_comm_channel(int tzc)
{
	void *pa = NULL;

	struct tee_comm *tc = (struct tee_comm *)tzc_alloc_shm(tzc,
				sizeof(struct tee_comm), &pa);
	if (tc) {
		tc->pa = (uint32_t)((unsigned long)pa);
		tc->va = (uint32_t)((unsigned long)tc);
	}

	return tc;
}

void tzc_release_tee_comm_channel(int tzc, struct tee_comm *tc)
{
	tzc_free_shm(tzc, (void *)((unsigned long)tc->va),
				(void *)((unsigned long)tc->pa), sizeof(*tc));
}

#endif /* CONFIG_TEE */

void *tzc_get_mem_info(int tzc, void *va, uint32_t *attr)
{
	int ret;
	struct tz_mem_info info;

	info.va = va;

	ret = tz_get_meminfo(&info);
	if (ret != 0) {
		*attr = 0;
		return NULL;
	}
	*attr = info.attr;
	return info.pa;
}

int tzc_fast_memmove(int tzc, void *dst_phy_addr, void *src_phy_addr, size_t size)
{
	int ret = TZ_SUCCESS;
	unsigned long func_id = SMC_FUNC_TOS_MEM_MOVE;
	unsigned long result;

	result = __smc(func_id, dst_phy_addr, src_phy_addr, size);

	if (SMC_FUNC_ID_UNKNOWN == result) {
		tz_error("can't support fast memmove");
		ret = TZ_ERROR_NOT_SUPPORTED;
	} else if (0 != result) {
		tz_error("access denied");
		ret = TZ_ERROR_ACCESS_DENIED;
	}
	return ret;
}

static int tzc_fast_secure_cache_op(int tzc, void *phy_addr, size_t size,
		unsigned long func_id)
{
	int ret = TZ_SUCCESS;
	unsigned long result;

	result = __smc(func_id, phy_addr, size);

	if (SMC_FUNC_ID_UNKNOWN == result) {
		tz_error("can't support fast memmove");
		ret = TZ_ERROR_NOT_SUPPORTED;
	} else if (0 != result) {
		tz_error("access denied");
		ret = TZ_ERROR_ACCESS_DENIED;
	}
	return ret;
}


int tzc_fast_secure_cache_clean(int tzc, void *phy_addr, size_t size)
{
	return tzc_fast_secure_cache_op(tzc, phy_addr, size,
			SMC_FUNC_TOS_SECURE_CACHE_CLEAN);
}

int tzc_fast_secure_cache_invalidate(int tzc, void *phy_addr, size_t size)
{
	return tzc_fast_secure_cache_op(tzc, phy_addr, size,
			SMC_FUNC_TOS_SECURE_CACHE_INVALIDATE);
}

int tzc_fast_secure_cache_flush(int tzc, void *phy_addr, size_t size)
{
	return tzc_fast_secure_cache_op(tzc, phy_addr, size,
			SMC_FUNC_TOS_SECURE_CACHE_CLEAN_INVALIDATE);
}

int tzc_create_instance(int tzc, unsigned long param, uint32_t *origin)
{
	int ret;
	struct tz_instance_param create_param;

	create_param.param = param;
	ret = tzd_kernel_create_instance(&create_param);
	*origin = create_param.origin;
	return create_param.result;
}

int tzc_destroy_instance(int tzc, unsigned long param, uint32_t *origin)
{
	int ret;
	struct tz_instance_param destroy_param;

	destroy_param.param = param;
	ret = tzd_kernel_destroy_instance(&destroy_param);
	*origin = destroy_param.origin;
	return destroy_param.result;
}

int tzc_open_session(int tzc, unsigned long param, uint32_t taskId,
			uint32_t *origin)
{
	int ret;
	struct tz_session_param open_param;

	open_param.param = param;
	open_param.task_id = taskId;
	ret = tzd_kernel_open_session(&open_param);
	*origin = open_param.origin;
	return open_param.result;
}

int tzc_close_session(int tzc, unsigned long param, uint32_t taskId,
			uint32_t *origin)
{
	int ret;
	struct tz_session_param close_param;

	close_param.param = param;
	close_param.task_id = taskId;
	ret = tzd_kernel_close_session(&close_param);
	*origin = close_param.origin;
	return close_param.result;
}
