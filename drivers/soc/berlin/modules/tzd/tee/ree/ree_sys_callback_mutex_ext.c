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
#include <linux/slab.h>
#include "config.h"
#include "ree_sys_callback_cmd.h"
#include "ree_sys_callback.h"
#include "ree_sys_callback_mutex_ext.h"

struct mutex_wait_list {
	struct mutex mu;
	struct list_head lh;
};

struct mutex_wait {
	struct list_head link;
	struct completion comp;
	struct mutex mu;
	uint32_t wait_after;
	uint32_t key;
};

static struct mutex_wait_list tee_wait_list;

static struct mutex_wait *mutex_wait_get(uint32_t key,
					struct mutex_wait_list *wait_list)
{
	struct mutex_wait *w = NULL;

	mutex_lock(&wait_list->mu);

	list_for_each_entry(w, &wait_list->lh, link)
		if (w->key == key)
			goto out;

	w = kmalloc(sizeof(struct mutex_wait), GFP_KERNEL);
	if (!w) {
		goto out;
	}

	init_completion(&w->comp);
	mutex_init(&w->mu);
	w->wait_after = 0;
	w->key = key;
	list_add_tail(&w->link, &wait_list->lh);
out:
	mutex_unlock(&wait_list->mu);
	return w;
}

TEE_Result REESysCmd_MutexWait(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	struct mutex_wait *w;
	int key = params[0].value.a;

	w = mutex_wait_get(key, &tee_wait_list);

	/*
	 * if interrupted, give app a chance to handle some signal
	 */
	if (-ERESTARTSYS == wait_for_completion_interruptible(&w->comp))
		return TEE_ERROR_NOT_SUPPORTED;

	return TEE_SUCCESS;
}

TEE_Result REESysCmd_MutexWake(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	struct mutex_wait *w;
	int key = params[0].value.a;

	w = mutex_wait_get(key, &tee_wait_list);

	complete(&w->comp);

	return TEE_SUCCESS;
}

TEE_Result REESysCmd_MutexDel(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	struct mutex_wait *w;
	int key = params[0].value.a;

	mutex_lock(&tee_wait_list.mu);

	list_for_each_entry(w, &tee_wait_list.lh, link) {
		if (w->key == key) {
			list_del(&w->link);
			mutex_destroy(&w->mu);
			kfree(w);
			break;
		}
	}

	mutex_unlock(&tee_wait_list.mu);

	return TEE_SUCCESS;
}

void REE_MutexInitExt(void)
{
	mutex_init(&tee_wait_list.mu);
	INIT_LIST_HEAD(&tee_wait_list.lh);
}
