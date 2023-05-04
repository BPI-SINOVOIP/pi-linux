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

#ifndef _TZ_NW_TASK_CLIENT_H_
#define _TZ_NW_TASK_CLIENT_H_

#include "tz_comm.h"

#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#else
#include "spinlock.h"
#endif

enum tz_nw_task_state {
	TZ_NW_TASK_STATE_IDLE,
	TZ_NW_TASK_STATE_INVOKING,
	TZ_NW_TASK_STATE_CALLBACK,
	TZ_NW_TASK_STATE_MAX
};

struct tz_nw_task_client {
	uint32_t task_id;
	int state;
	uint32_t call_id;
	void *call_info;
	spinlock_t lock;	/* protect waitq */
#ifdef __KERNEL__
	wait_queue_head_t waitq;
#endif
	tz_cmd_handler callback;
	void *userdata;
	bool atomic;
	bool dead;
};

struct tz_nw_task_client *tz_nw_task_client_get(int task_id);

struct tz_nw_task_client *tz_nw_task_client_init(int task_id);
int tz_nw_task_client_init_all(void);

int tz_nw_task_client_register(int task_id,
		tz_cmd_handler callback, void *userdata);
int tz_nw_task_client_unregister(int task_id);

#endif /* _TZ_NW_TASK_CLIENT_H_ */
