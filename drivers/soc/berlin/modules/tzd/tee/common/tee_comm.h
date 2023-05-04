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

#ifndef _TZ_CMD_TEE_H_
#define _TZ_CMD_TEE_H_

#include "tee_internal_core_common.h"

#define tee_param	TEE_Param

/*
 * in order to make it simple, and sync with TEE,
 * we use fixed size command for communication.
 * each channel is 2kB, and it's readable/writable in nw.
 * here, shm is temporary shared memory between nw/sw, which
 * is used for some light communications.
 *
 */
#define TEE_COMM_CHANNEL_SIZE		(2048)
#define TEE_COMM_HEADER_SIZE		(32)
#define TEE_COMM_PARAM_SIZE		\
	((TEE_COMM_CHANNEL_SIZE - TEE_COMM_HEADER_SIZE) / 2)
#define TEE_COMM_PARAM_BASIC_SIZE	(52)
#define TEE_COMM_PARAM_EXT_SIZE		\
	(TEE_COMM_PARAM_SIZE - TEE_COMM_PARAM_BASIC_SIZE)

struct tee_comm_param {
	uint32_t session_id;
	uint32_t cmd_id;
	uint32_t flags;
	uint32_t param_types;
	union tee_param params[4];
	uint32_t param_ext_size;
	uint8_t param_ext[TEE_COMM_PARAM_EXT_SIZE];
};

struct tee_comm {
	uint32_t	used;
	/* FIXME: to support 32-bit user-land APP and
	 * 32-bit EL1 TZ kernel, change void * to uint32_t,
	 * needs to refine later if to support 64-bit EL1
	 * TZ kernel and 64-bit user-land app.
	 */
	uint32_t	pa;
	uint32_t	va;
	uint8_t		reserved[TEE_COMM_HEADER_SIZE -
				3 * sizeof(uint32_t)];
	struct tee_comm_param	call_param;
	struct tee_comm_param	callback_param;
};

#if 0
typedef int (*tee_cmd_handler_user)(void *userdata,
		uint32_t cmd_id,
		uint32_t param_types,
		union tee_param params[4]);

typedef int (*tee_cmd_handler_sys)(void *userdata,
		uint32_t cmd_id,
		uint32_t param_types,
		union tee_param params[4],
		void *param_ext,
		uint32_t param_ext_size);
#endif

#endif /* _TZ_CMD_TEE_H_ */
