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
#include "tee_ca_mgr_cmd.h"
#include "tee_mgr_cmd.h"
#include "tee_client_util.h"
#include "tz_client_api.h"
#include "log.h"
#ifndef __KERNEL__
#include "string.h"
#endif /* __KERNEL__ */

/*
 * params[0]: memref. TA encrypted code binary start.
 *            Currently, it's the TEE_TA structure.
 */
TEEC_Result TAMgr_Register(int tzc, struct tee_comm *comm,
	TEEC_Operation *operation)
{
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;
	struct tee_comm_param *cmd;
	int i, t[4];

	assert(comm);
	assert(operation);

	cmd = &comm->call_param;
	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_REGISTER;

	for (i = 0; i < 4; i++) {
		int paramType = TEEC_PARAM_TYPE_GET(operation->paramTypes, i);
		t[i] = TEEC_CallParamToComm(tzc, &cmd->params[i],
				&operation->params[i], paramType);
	}

	cmd->param_types = TEE_PARAM_TYPES(t[0], t[1], t[2], t[3]);

	/* invoke command */
	res = tzc_invoke_tee_sys_command(tzc, TZ_TASK_ID_MGR, comm,
			&origin, NULL, NULL);

	return res;
}

TEE_Result TAMgr_CreateInstance(
	int			tzc,
	struct tee_comm*	comm,
	const TEE_UUID*		destination,
	uint32_t*		returnOrigin,
	uint32_t*		taskId)
{
	struct tee_comm_param *cmd;
	TAMgrCmdCreateInstanceParamExt *p;
	uint32_t param;
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;

	assert(comm);

	cmd = &comm->call_param;
	p = (void *)cmd->param_ext;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);

	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_CREATE_INSTANCE;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	cmd->param_ext_size = sizeof(TAMgrCmdCreateInstanceParamExt);
	memcpy(&p->destination, destination, sizeof(p->destination));

	/* invoke command */
	res = tzc_create_instance(tzc, param, &origin);

	if (TEEC_SUCCESS == res)
		*taskId = p->taskId;

	if (returnOrigin)
		*returnOrigin = origin;

	return res;
}

TEE_Result TAMgr_DestroyInstance(
	int			tzc,
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t*		returnOrigin)
{
	struct tee_comm_param *cmd;
	uint32_t param;
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;

	assert(comm);

	cmd = &comm->call_param;
	param = (uint32_t)((unsigned long)comm->pa) +
			offsetof(struct tee_comm, call_param);
	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_DESTROY_INSTANCE;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	cmd->params[0].value.a = taskId;

	/* invoke command */
	res = tzc_destroy_instance(tzc, param, &origin);

	if (returnOrigin)
		*returnOrigin = origin;

	return res;
}

TEEC_Result TAMgr_ProvisionFeatureCert(int tzc, struct tee_comm *comm,
	TEEC_Operation *operation)
{
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;
	struct tee_comm_param *cmd;
	int i, t[4];

	assert(comm);
	assert(operation);

	cmd = &comm->call_param;
	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_PROVISION_FEATURE_CERT;

	for (i = 0; i < 4; i++) {
		int paramType = TEEC_PARAM_TYPE_GET(operation->paramTypes, i);
		t[i] = TEEC_CallParamToComm(tzc, &cmd->params[i],
				&operation->params[i], paramType);
	}

	cmd->param_types = TEE_PARAM_TYPES(t[0], t[1], t[2], t[3]);

	/* invoke command */
	res = tzc_invoke_tee_sys_command(tzc, TZ_TASK_ID_MGR, comm,
			&origin, NULL, NULL);

	return res;
}

TEEC_Result TAMgr_LoadFeatureCert(int tzc, struct tee_comm *comm)
{
	TEEC_Result res = TEEC_SUCCESS;
	uint32_t origin = TEEC_ORIGIN_API;
	struct tee_comm_param *cmd;

	assert(comm);

	cmd = &comm->call_param;
	/* pack the command */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TAMGR_CMD_LOAD_FEATURE_CERT;

	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	/* invoke command */
	res = tzc_invoke_tee_sys_command(tzc, TZ_TASK_ID_MGR, comm,
			&origin, NULL, NULL);

	return res;
}
