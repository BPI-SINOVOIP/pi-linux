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

#ifndef _TEE_CA_SYS_H_
#define _TEE_CA_SYS_H_

#include "tee_client_api.h"

TEEC_Result TASysCmd_OpenSession(
	int			tzc,
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t		connectionMethod,
	const void*		connectionData,
	TEEC_Operation*		operation,
	uint32_t*		returnOrigin,
	uint32_t*		sessionId);

TEEC_Result TASysCmd_CloseSession(
	int			tzc,
	struct tee_comm*	comm,
	uint32_t		taskId,
	uint32_t		sessionId,
	uint32_t*		returnOrigin,
	bool*			instanceDead);

#endif /* _TEE_CA_SYS_H_ */