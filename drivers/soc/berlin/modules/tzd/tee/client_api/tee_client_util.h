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

#ifndef _TEE_CLIENT_UTIL_H_
#define _TEE_CLIENT_UTIL_H_

#include "tee_client_api.h"
#include "tee_comm.h"

uint32_t TEEC_CallParamToComm(int tzc,
			union tee_param *commParam,
			TEEC_Parameter *param, uint32_t paramType);

uint32_t TEEC_CallCommToParam(TEEC_Parameter *param,
			union tee_param *commParam,
			uint32_t paramType);

uint32_t TEEC_CallbackCommToParam(TEEC_Parameter *param,
			union tee_param *commParam,
			uint32_t paramType);

uint32_t TEEC_CallbackParamToComm(union tee_param *commParam,
			TEEC_Parameter *param, uint32_t paramType);

/*
 * it will fill cmdData except cmdData->param_ext
 */
TEEC_Result TEEC_CallOperactionToCommand(int tzc,
				struct tee_comm_param *cmd,
				uint32_t cmdId, TEEC_Operation *operation);

/*
 * it will copy data from cmdData->params[4] to operation->params[4]
 */
TEEC_Result TEEC_CallCommandToOperaction(TEEC_Operation *operation,
				struct tee_comm_param *cmd);

/*
 * it will copy data from cmd->params[4] to operation->params[4]
 */
TEEC_Result TEEC_CallbackCommandToOperaction(TEEC_Operation *operation,
				struct tee_comm_param *cmd);

/*
 * it will fill cmd->param_types and cmd->params[4]
 */
TEEC_Result TEEC_CallbackOperactionToCommand(struct tee_comm_param *cmd,
				TEEC_Operation *operation);

/*
 * it will create a TEEC property handle
 */
TEEC_Result TEEC_CreateProperty(TEEC_Property **property);

/*
 *  it will add a property node with the input name and string value
 */
TEEC_Result TEEC_AddPropertyString(TEEC_Property *property,
				const char *name, const char *value);

/*
 *  it will add a property node with the input name and uint32 value
 */
TEEC_Result TEEC_AddPropertyUint32(TEEC_Property *property,
				const char *name, uint32_t value);

/*
 * it will get the TEEC property, return in string
 */
TEEC_Result TEEC_GetPropertyString(TEEC_Property *property,
				const char *name, char *value, uint32_t valueLen);

/*
 * it will get the TEEC property, return in uint32_t
 */
TEEC_Result TEEC_GetPropertyUint32(TEEC_Property *property,
				const char *name, uint32_t *value);

/*
 *  it will delete a property node which matchs with the input name and value
 */
TEEC_Result TEEC_DeleteProperty(TEEC_Property *property, const char *name);

/*
 * it will delete the ta property link chain
 */
TEEC_Result TEEC_DestroyProperty(TEEC_Property *property);

#endif /* _TEE_CLIENT_UTIL_H_ */
