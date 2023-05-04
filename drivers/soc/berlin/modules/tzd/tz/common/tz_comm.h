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

#ifndef _TZ_COMM_H_
#define _TZ_COMM_H_

#include "smc.h"

/** TrustZone Task ID.
 *
 * 0: CPU0 FIQ Trap
 * 1: CPU1 FIQ Trap
 * 2: CPU2 FIQ Trap
 * 3: CPU3 FIQ Trap
 * 4: TZ Manager, must NOT have callback
 * 5-123: auto asigned by TZ Manager
 * 124-126: reserved for world ID.
 * 127: Normal World
 *
 * note: TZ_TASK_ID_CALLER will be adjusted to right caller ID in scheduler.
 */
#define TZ_TASK_ID_SW_MAX		(CONFIG_TASK_NUM - 1)
#define TZ_TASK_ID_MAX			(CONFIG_TASK_NUM)

#define TZ_TASK_ID_CPU0_FIQ		(0)
#define TZ_TASK_ID_CPU1_FIQ		(1)
#define TZ_TASK_ID_CPU2_FIQ		(2)
#define TZ_TASK_ID_CPU3_FIQ		(3)
#define TZ_TASK_ID_MGR			(4)
#define TZ_TASK_ID_TEST			(5)
#define TZ_TASK_ID_USER_START		(6)
#define TZ_TASK_ID_USER_MAX		(TZ_TASK_ID_SW_MAX)
#define TZ_TASK_ID_CALLER		(0xfffffffe)
#define TZ_TASK_ID_NW			(CONFIG_TASK_NUM)

#define TZ_TASK_ID_FIQ(cpu_id)		((cpu_id) + TZ_TASK_ID_CPU0_FIQ)
#define TZ_TASK_ID_IS_FIQ(task_id)	((task_id) <= TZ_TASK_ID_CPU3_FIQ)

#define TZ_TASK_ID_IS_NW(task_id)	((task_id) > TZ_TASK_ID_SW_MAX)
#define TZ_TASK_ID_IS_SW(task_id)	((task_id) <= TZ_TASK_ID_SW_MAX)

#define TZ_TASK_ID_IS_USER(task_id)	\
	((task_id) > TZ_TASK_ID_USER_START && (task_id) <= TZ_TASK_ID_USER_MAX)

#define TZ_CMD_SYS			(0x00008000)	/* cmd is for system */
/* USER CMD and SYS CMD share the same command type */
#define TZ_CMD_RAW(cmd)			(cmd & (~TZ_CMD_SYS))

/** TrustZone command ID.
 *
 * it's used in 'status = __smc_tee(task_id, cmd_id, param, call_id, &callback)'
 *
 * note: for all other application commands, we should use Level 2 commands.
 */
/* 0x00 - 0x0f are for communication */
#define TZ_CMD_DONE			TOS_STDCALL(0)
#define TZ_CMD_ON_DONE			TOS_STDCALL(1)
#define TZ_CMD_RESUME_DONE		TOS_STDCALL(2)
#define TZ_CMD_OFF_DONE			TOS_STDCALL(3)
#define TZ_CMD_SUSPEND_DONE		TOS_STDCALL(4)
#define TZ_CMD_FIQ_DONE			TOS_STDCALL(5)
#define TZ_CMD_SYSTEM_OFF_DONE		TOS_STDCALL(6)
#define TZ_CMD_SYSTEM_RESET_DONE	TOS_STDCALL(7)
#define TZ_CMD_ENTRY_DONE		TOS_STDCALL(8)

/* 0x10 - 0x1f are for Level 2 command */
#define TZ_CMD_TEE			TOS_STDCALL(16)	/* param: tee_comm_param * */
#define TZ_CMD_TEE_USER			TZ_CMD_TEE

/* 0x80 - 0xef are for task private command */
#define TZ_CMD_PRIVATE_START		TOS_STDCALL(128)

/* system command */
#define TZ_CMD_TEE_SYS			(TZ_CMD_SYS | TZ_CMD_TEE)

#define TZ_CMD_IS_TEE(cmd_id)		(TZ_CMD_TEE == TZ_CMD_RAW(cmd_id))

/*
 * call id
 *
 * TODO: need let call_id to contain both pid and task_id info to support
 *       callback between the call id from NW.
 */
#define TZ_CALLID(pid, task_id)		(((pid) << 8) | ((task_id) & 0xff))
#define TZ_CALLID_PID(call_id)		((call_id) >> 8)
#define TZ_CALLID_TASKID(call_id)	((call_id) & 0xff)
#define TZ_SW_CALL_ID_IGNORED		(0)

#ifndef __ASSEMBLY__

#include "types.h"
#include "tz_errcode.h"

/*
 * Level 1 Command, transfer by regiter
 *
 * call:	status = __smc_tee(callee, cmd_id, param, call_id, &callback_result);
 * return:	__smc_tee(caller, TZ_CMD_DONE, result, origin, NULL);
 */
struct tz_cmd {
	unsigned long task_id;	/* for client: it's callee; for server, it's caller */
	unsigned long cmd_id;
	unsigned long param;
};

struct tz_cmd_result {
	unsigned long cmd_id;
	unsigned long result;
	unsigned long origin;
};

union tz_smc_return {
	struct tz_cmd		cb;
	struct tz_cmd_result	res;
};

enum tz_comm_req_flags {
	TZ_COMM_REQ_NONE		= 0x00000000,
	TZ_COMM_REQ_CANCELLED		= 0x00000001,
	TZ_COMM_REQ_MAX
};

enum tz_comm_rsp_flags {
	TZ_COMM_RSP_NONE		= 0x00000000,
	TZ_COMM_RSP_DONE		= 0x80000000,
	TZ_COMM_RSP_BUSY		= 0x40000000,
	TZ_COMM_RSP_IRQ_TRAP		= 0x20000000,
	TZ_COMM_RSP_CALLBACK		= 0x10000000,
	TZ_COMM_RSP_LOOPBACK		= 0x08000000,
	TZ_COMM_RSP_MAX
};

/*
 * return origin, sync with TEE_ORIGIN_*
 */
enum {
	TZ_ORIGIN_API			= 0x00000001,
	TZ_ORIGIN_COMMS			= 0x00000002,
	TZ_ORIGIN_TEE			= 0x00000003,
	TZ_ORIGIN_TRUSTED_APP		= 0x00000004,
	TZ_ORIGIN_UNTRUSTED_APP		= 0x00000005,
};

struct tz_cmd_verify_image_param {
	const void *src;
	unsigned int size;
	void *dst;
};

struct tz_cmd_get_mem_region_count_param {
	uint32_t attr_mask;
	uint32_t attr_val;
	uint32_t count;
};

struct tz_cmd_get_mem_region_list_param {
	struct mem_region *region;
	uint32_t max_num;
	uint32_t attr_mask;
	uint32_t attr_val;
	uint32_t count;
};

/** execute command.
 *
 * warning: if call to NW, then there may be a security risk. So, caller must
 *          consider the return results (including origin) as untrusted.
 */
typedef uint32_t (*tz_cmd_handler)(void *userdata,
		uint32_t cmd_id,
		uint32_t param,
		uint32_t *origin);

struct tz_cmd_handler_pair {
	uint32_t cmd_id;
	tz_cmd_handler handler;
	void *userdata;
};

tz_errcode_t tz_comm_register_cmd_handler(
			struct tz_cmd_handler_pair *handler_list,
			uint32_t max_count, uint32_t cmd_id,
			tz_cmd_handler handler, void *userdata);

tz_errcode_t tz_comm_unregister_cmd_handler(
			struct tz_cmd_handler_pair *handler_list,
			uint32_t max_count, uint32_t cmd_id);

struct tz_cmd_handler_pair *tz_comm_find_cmd_handler(
			struct tz_cmd_handler_pair *handler_list,
			uint32_t max_count, uint32_t cmd_id);

#endif /* __ASSEMBLY__ */

#endif /* _TZ_COMM_H_ */
