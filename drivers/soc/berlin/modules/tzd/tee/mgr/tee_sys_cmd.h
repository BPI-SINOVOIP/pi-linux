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

#ifndef _TEE_SYS_CMD_H_
#define _TEE_SYS_CMD_H_

#include "tee_internal_core_common.h"

/** TA System Command Protocol.
 *
 * All TA must support these System Commands.
 *
 * currently, we only support 2 system command in TA:
 * - OpenSession
 * - CloseSession
 */

enum TASysCmd {
	TASYS_CMD_OPEN_SESSION,
	TASYS_CMD_CLOSE_SESSION,	/* return instanceDead by param[0].value.a */
	TASYS_CMD_MAX
};

typedef struct {
	TEE_UUID client;	/* input: client TA uuid */
	uint32_t login;		/* input: login method */
	uint32_t group;		/* input: group to login for TEE_LOGIN_GROUP &
				 * TEE_LOGIN_APPLICATION_GROUP */
	uint32_t sessionId;	/* output: taskId (8bits) | index (24bits) */
} TASysCmdOpenSessionParamExt;

#endif /* _TEE_SYS_CMD_H_ */
