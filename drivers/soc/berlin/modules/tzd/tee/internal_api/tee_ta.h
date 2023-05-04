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

#ifndef _TEE_TRUSTED_APPLICATION_H_
#define _TEE_TRUSTED_APPLICATION_H_

#include "tee_internal_core_api.h"

/** Trusted Application.
 *
 * @singleInstance	if it's singleton service, only 1 instance would
 * 			be created. scheduler will ensure calling is
 * 			synchronous.
 *
 * FIXME: for it's the interface between TZ kernel, need take care of the
 * 	future extension of it. a version number for the strucutre is
 * 	required, and need ensure the size of header is fixed for all
 * 	architectures, so TZ kernel (TAMgr) can easily check the changes
 * 	and compatible architectures.
 * 	a potential header is as bellow:
 *
 * 		uint32_t magic_num;	// 'T*TA'
 * 		uint32_t version;
 *		uint32_t machine_word;
 *		uint32_t reserved[5];
 *
 * FIXME: there is a bug in the standard bool definition between here and
 * 	standard C99. Here, we defined it as 'int', while C99 defines it
 * 	as 'char' (1 byte). in order to make it compatible with C99, we
 * 	use uint32_t to replace bool workaround the issue. this would be
 * 	fixed in 2.0 for BG2Q A1b.
 */
typedef struct {
	TEE_UUID uuid;
	char name[16];
	uint32_t singleInstance;	/* boolean */
	uint32_t multiSession;		/* boolean */
	uint32_t instanceKeepAlive;	/* boolean */

	TEE_Result (*Create)(void);
	void (*Destroy)(void);
	TEE_Result (*OpenSession)(
		uint32_t	paramTypes,
		TEE_Param	params[4],
		void**		sessionContext);
	void (*CloseSession)(void* sessionContext);
	TEE_Result (*InvokeCommand)(
		void*		sessionContext,
		uint32_t	commandID,
		uint32_t	paramTypes,
		TEE_Param	params[4]);
} TEE_TA;

#define TA_EXPORT

#define TA_DEFINE_BEGIN						\
	const TEE_TA __ThisTA = {


#define TA_DEFINE_END						\
		.Create		= TA_CreateEntryPoint,		\
		.Destroy	= TA_DestroyEntryPoint,		\
		.OpenSession	= TA_OpenSessionEntryPoint,	\
		.CloseSession	= TA_CloseSessionEntryPoint,	\
		.InvokeCommand	= TA_InvokeCommandEntryPoint,	\
	};							\
								\
void *__init(void)						\
{								\
        return (void *)&__ThisTA;				\
}								\
int __fini(void)						\
{								\
	return 0;						\
}

#endif /* _TEE_TRUSTED_APPLICATION_H_ */
