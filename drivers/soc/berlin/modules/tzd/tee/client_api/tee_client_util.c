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
#include "tee_client_util.h"
#include "tz_client_api.h"
#include "log.h"
#ifndef __KERNEL__
#include <string.h>
#else
#include <linux/string.h>
#endif

uint32_t TEEC_CallParamToComm(int tzc,
			union tee_param *commParam,
			TEEC_Parameter *param, uint32_t paramType)
{
	uint32_t t = paramType & 0x7;	/* covented paramType for communication */
	uint32_t attr;

	switch (paramType) {
	case TEEC_VALUE_INPUT:
	case TEEC_VALUE_OUTPUT:
	case TEEC_VALUE_INOUT:
		commParam->value.a = param->value.a;
		commParam->value.b = param->value.b;
		break;
	case TEEC_MEMREF_TEMP_INPUT:
	case TEEC_MEMREF_TEMP_OUTPUT:
	case TEEC_MEMREF_TEMP_INOUT:
		commParam->memref.buffer = (uint32_t)((unsigned long)
				tzc_get_mem_info(tzc, param->tmpref.buffer, &attr));
		commParam->memref.size   = param->tmpref.size;
		break;
	case TEEC_MEMREF_WHOLE:
		t = TEE_PARAM_TYPE_MEMREF_INOUT;
		commParam->memref.buffer = (uint32_t)((unsigned long)
				param->memref.parent->phyAddr);
		commParam->memref.size   = param->memref.parent->size;
		break;
	case TEEC_MEMREF_PARTIAL_INPUT:
	case TEEC_MEMREF_PARTIAL_OUTPUT:
	case TEEC_MEMREF_PARTIAL_INOUT:
		commParam->memref.buffer = (uint32_t)((unsigned long)
				(param->memref.parent->phyAddr +
				param->memref.offset));
		commParam->memref.size   = param->memref.size;
		break;
	}

	return t;
}

uint32_t TEEC_CallCommToParam(TEEC_Parameter *param,
			union tee_param *commParam,
			uint32_t paramType)
{
	uint32_t t = paramType & 0x7;	/* covented paramType for communication */
	switch (paramType) {
	case TEEC_VALUE_INPUT:
	case TEEC_VALUE_OUTPUT:
	case TEEC_VALUE_INOUT:
		param->value.a = commParam->value.a;
		param->value.b = commParam->value.b;
		break;
	case TEEC_MEMREF_TEMP_OUTPUT:
	case TEEC_MEMREF_TEMP_INOUT:
		param->tmpref.size = commParam->memref.size;
		break;
	case TEEC_MEMREF_WHOLE:
	case TEEC_MEMREF_PARTIAL_OUTPUT:
	case TEEC_MEMREF_PARTIAL_INOUT:
		param->memref.size = commParam->memref.size;
		break;
	}

	return t;
}

uint32_t TEEC_CallbackCommToParam(TEEC_Parameter *param,
			union tee_param *commParam,
			uint32_t paramType)
{
	uint32_t t = paramType & 0x7;	/* covented paramType for communication */

	switch (paramType) {
	case TEE_PARAM_TYPE_VALUE_INPUT:
	case TEE_PARAM_TYPE_VALUE_OUTPUT:
	case TEE_PARAM_TYPE_VALUE_INOUT:
		param->value.a = commParam->value.a;
		param->value.b = commParam->value.b;
		break;
	case TEE_PARAM_TYPE_MEMREF_INPUT:
	case TEE_PARAM_TYPE_MEMREF_OUTPUT:
	case TEE_PARAM_TYPE_MEMREF_INOUT:
		param->tmpref.buffer	= (void *)((unsigned long)
				commParam->memref.buffer);
		param->tmpref.size	= commParam->memref.size;
		break;
	}

	return t;
}

uint32_t TEEC_CallbackParamToComm(union tee_param *commParam,
			TEEC_Parameter *param, uint32_t paramType)
{
	uint32_t t = paramType & 0x7;	/* covented paramType for communication */

	switch (paramType) {
	case TEE_PARAM_TYPE_VALUE_INPUT:
	case TEE_PARAM_TYPE_VALUE_OUTPUT:
	case TEE_PARAM_TYPE_VALUE_INOUT:
		commParam->value.a = param->value.a;
		commParam->value.b = param->value.b;
		break;
	case TEE_PARAM_TYPE_MEMREF_INPUT:
	case TEE_PARAM_TYPE_MEMREF_OUTPUT:
	case TEE_PARAM_TYPE_MEMREF_INOUT:
		commParam->memref.buffer	= (uint32_t)((unsigned long)param->tmpref.buffer);
		commParam->memref.size		= param->tmpref.size;
		break;
	}

	return t;
}

/*
 * it will fill cmd->param_types and cmd->params[4]
 */
TEEC_Result TEEC_CallOperactionToCommand(int tzc,
				struct tee_comm_param *cmd,
				uint32_t cmdId, TEEC_Operation *operation)
{
	int i, t[4];

	if (!cmd) {
		error("illigle arguments\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);

	cmd->cmd_id = cmdId;

	if (!operation) {
		cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
		goto out;
	}

	/* travel all the 4 parameters */
	for (i = 0; i < 4; i++) {
		int paramType = TEEC_PARAM_TYPE_GET(operation->paramTypes, i);
		t[i] = TEEC_CallParamToComm(tzc,
					&cmd->params[i],
					&operation->params[i], paramType);
	}

	cmd->param_types = TEE_PARAM_TYPES(t[0], t[1], t[2], t[3]);

out:
	trace("invoke command 0x%08x\n", cmd->cmd_id);
	return TEEC_SUCCESS;
}

/*
 * it will copy data from cmd->params[4] to operation->params[4]
 */
TEEC_Result TEEC_CallCommandToOperaction(TEEC_Operation *operation,
				struct tee_comm_param *cmd)
{
	int i;

	if (!operation)
		return TEEC_SUCCESS;

	if (!cmd) {
		error("illigle arguments\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	/* travel all the 4 param */
	for (i = 0; i < 4; i++) {
		int paramType = TEEC_PARAM_TYPE_GET(operation->paramTypes, i);
		TEEC_CallCommToParam(&operation->params[i],
					&cmd->params[i],
					paramType);
	}

	return TEEC_SUCCESS;
}

/*
 * it will copy data from cmd->params[4] to operation->params[4]
 */
TEEC_Result TEEC_CallbackCommandToOperaction(TEEC_Operation *operation,
				struct tee_comm_param *cmd)
{
	int i, t[4];

	if (!operation || !cmd) {
		error("illigle arguments\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	memset(operation, 0, sizeof(*operation));

	/* travel all the 4 param */
	for (i = 0; i < 4; i++) {
		int paramType = TEE_PARAM_TYPE_GET(cmd->param_types, i);
		t[i] = TEEC_CallbackCommToParam(&operation->params[i],
					&cmd->params[i],
					paramType);
	}

	operation->paramTypes = TEEC_PARAM_TYPES(t[0], t[1], t[2], t[3]);

	return TEEC_SUCCESS;
}


/*
 * it will fill cmd->param_types and cmd->params[4]
 */
TEEC_Result TEEC_CallbackOperactionToCommand(struct tee_comm_param *cmd,
				TEEC_Operation *operation)
{
	int i;

	if (!cmd) {
		error("illigle arguments\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	/* travel all the 4 parameters */
	for (i = 0; i < 4; i++) {
		int paramType = TEE_PARAM_TYPE_GET(cmd->param_types, i);
		TEEC_CallbackParamToComm(&cmd->params[i],
					&operation->params[i], paramType);
	}

	trace("invoke command 0x%08x\n", cmd->cmd_id);
	return TEEC_SUCCESS;
}

#define TEEC_PROPERTY_VALUE_MAX		64

typedef struct TEEC_PropNode {
	char *name;
	char *value;
	struct TEEC_PropNode *next;
} TEEC_PropNode;

struct TEEC_Property {
	TEEC_PropNode *head;
};

/*
 * create the property handle, initialize the head to NULL,
 */
TEEC_Result TEEC_CreateProperty(TEEC_Property **property)
{
	if (NULL == property)
		return TEEC_ERROR_BAD_PARAMETERS;

	*property = (TEEC_Property *) tzc_malloc(sizeof(TEEC_Property));
	if (NULL == *property) {
		error("malloc memory for Prop_Handle failed\n");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	(*property)->head = NULL;

	return TEEC_SUCCESS;
}

static TEEC_PropNode **teec_find_property(TEEC_Property *property,
					const char *name)
{
	TEEC_PropNode **current = &property->head;

	while (NULL != *current) {
		if (!strcmp((*current)->name, name))
			return current;
		current = &(*current)->next;
	}

	return current;
}

TEEC_Result TEEC_AddPropertyString(TEEC_Property *property,
					const char *name, const char *value)
{
	TEEC_PropNode **current;

	if ((NULL == property) || (NULL == name))
		return TEEC_ERROR_BAD_PARAMETERS;

	if (NULL == value)
		value = ""; /* give a default value if NULL */

	current = teec_find_property(property, name);
	if (*current)
		return TEEC_ERROR_EXCESS_DATA; /* ITEM duplicated */

	*current = (TEEC_PropNode *) tzc_malloc(sizeof(TEEC_PropNode));
	if (!*current)
		return TEEC_ERROR_OUT_OF_MEMORY;
	(*current)->name  = tzc_strdup(name);
	if (NULL == (*current)->name) {
		tzc_free(*current);
		*current = NULL;
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	(*current)->value = tzc_strdup(value);
	if (NULL == (*current)->value) {
		tzc_free((*current)->name);
		tzc_free(*current);
		*current = NULL;
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	(*current)->next = NULL;

	return TEEC_SUCCESS;
}

TEEC_Result TEEC_AddPropertyUint32(TEEC_Property *property,
				const char *name, uint32_t value)
{
	char str[TEEC_PROPERTY_VALUE_MAX];

	if ((NULL == property) || (NULL == name))
		return TEEC_ERROR_BAD_PARAMETERS;

	sprintf(str, "%u", value);

	return TEEC_AddPropertyString(property, name, str);
}

TEEC_Result TEEC_GetPropertyString(TEEC_Property *property,
				char const *name, char *value, uint32_t valueLen)
{
	TEEC_PropNode **current;

	if ((NULL == property) || (NULL == name) || (NULL == value))
		return TEEC_ERROR_BAD_PARAMETERS;

	current = teec_find_property(property, name);
	if (NULL == *current)
		return TEEC_ERROR_ITEM_NOT_FOUND;

	if (strlen((*current)->value) + 1 < valueLen ) {
		strcpy(value, (*current)->value);
		return TEEC_SUCCESS;
	}

	return TEEC_ERROR_SHORT_BUFFER;
}

TEEC_Result TEEC_GetPropertyUint32(TEEC_Property *property, const char *name,
			uint32_t *value)
{
	char str[TEEC_PROPERTY_VALUE_MAX];
	TEEC_Result res = TEEC_SUCCESS;

	if ((NULL == property) || (NULL == name) || (NULL == value))
		return TEEC_ERROR_BAD_PARAMETERS;

	res = TEEC_GetPropertyString(property, name, str, TEEC_PROPERTY_VALUE_MAX);
	if (res == TEEC_SUCCESS)
		*value = tzc_strtoul(str, 0, 0);

	return res;
}

TEEC_Result TEEC_DeleteProperty(TEEC_Property *property, const char *name)
{
	TEEC_PropNode **ptr, *next;

	if ((NULL == property) || (NULL == name))
		return TEEC_ERROR_BAD_PARAMETERS;

	ptr = &property->head;
	while (NULL != *ptr) {
		if (!strcmp((*ptr)->name, name)) {
			next = (*ptr)->next;
			tzc_free((*ptr)->name);
			tzc_free((*ptr)->value);
			tzc_free(*ptr);
			*ptr = next;
			return TEEC_SUCCESS;
		}
	}

	return TEEC_ERROR_ITEM_NOT_FOUND;
}

TEEC_Result TEEC_DestroyProperty(TEEC_Property *property)
{
	TEEC_PropNode *current, *next;

	if (NULL == property)
		return TEEC_ERROR_BAD_PARAMETERS;

	current = property->head;

	while (NULL != current) {
		next = current->next;
		tzc_free(current->name);
		tzc_free(current->value);
		tzc_free(current);
		current = next;
	}

	tzc_free(property);

	return TEEC_SUCCESS;
}
