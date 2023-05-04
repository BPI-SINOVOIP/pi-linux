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

#include "config.h"
#include "tee_sys_cmd.h"
#include "tee_ca_sys_cmd.h"
#include "tee_client_util.h"
#include "tz_client_api.h"
#include "log.h"
#ifndef __KERNEL__
#include "string.h"
#endif /* __KERNEL__ */

/*
 * shm/param_ext: TAMgrCmdOpenSessionParam
 * params[0-3]: params.
 */
TEEC_Result TASysCmd_OpenSession(
	int			tzc,
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t		connectionMethod,
	const void*		connectionData,
	TEEC_Operation*		operation,
	uint32_t*		returnOrigin,
	uint32_t*		sessionId)
{
	TEEC_Result result = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;
	struct tee_comm_param *cmd;
	uint32_t param, group = 0;
	TASysCmdOpenSessionParamExt *p;

	assert(comm);
	assert(taskId);

	if (!comm || !taskId) {
		error("invalid comm(0x%08x) or taskId(0x%08x)\n",
				(uint32_t)comm, taskId);
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	cmd = &comm->call_param;
	p = (TASysCmdOpenSessionParamExt *)cmd->param_ext;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);

	switch(connectionMethod) {
	case TEEC_LOGIN_PUBLIC:
	case TEEC_LOGIN_USER:
	case TEEC_LOGIN_APPLICATION:
	case TEEC_LOGIN_USER_APPLICATION:
		break;

	case TEEC_LOGIN_GROUP:
	case TEEC_LOGIN_GROUP_APPLICATION:
		if(connectionData == NULL) {
			error("connection method requires valid group\n");
			result = TEEC_ERROR_BAD_PARAMETERS;
			goto out;
		} else {
			group = *(uint32_t *)connectionData;
		}
		break;
	default:
		error("invalid connectionMethod (0x%08x)\n", connectionMethod);
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	/* pack the command */
	TEEC_CallOperactionToCommand(tzc, cmd, TASYS_CMD_OPEN_SESSION, operation);
	cmd->param_ext_size = sizeof(TASysCmdOpenSessionParamExt);
	/* for NW, it doesn't have a UUID */
	memset(&p->client, 0, sizeof(p->client));
	p->login = connectionMethod;
	p->group = group;

	trace("invoke command 0x%08x\n", cmd->cmd_id);

	/* invoke command */
	result = tzc_open_session(tzc, param, taskId, &origin);

	if (TEE_ORIGIN_TRUSTED_APP == origin) {
		/* copy back results,
		 * and free the communication channel after communication done
		 */
		TEEC_CallCommandToOperaction(operation, cmd);
		if (TEEC_SUCCESS == result)
			*sessionId = p->sessionId;
	}

out:
	if (returnOrigin)
		*returnOrigin = origin;
	return result;
}

/*
 * params[0]: value.a = sessionId.
 */
TEEC_Result TASysCmd_CloseSession(
	int			tzc,
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t		sessionId,
	uint32_t*		returnOrigin,
	bool*			instanceDead)
{

	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;
	struct tee_comm_param *cmd;
	uint32_t param;

	assert(comm);

	cmd = &comm->call_param;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);

	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TASYS_CMD_CLOSE_SESSION;
	cmd->session_id = sessionId;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	/* invoke command */
	res = tzc_close_session(tzc, param, taskId, &origin);

	if (TEEC_SUCCESS == res)
		*instanceDead = cmd->params[0].value.a;

	if (returnOrigin)
		*returnOrigin = origin;

	return res;
}
