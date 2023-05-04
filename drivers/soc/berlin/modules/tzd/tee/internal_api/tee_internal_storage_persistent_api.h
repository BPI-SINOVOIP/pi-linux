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

#ifndef _TEE_INTERNAL_STORAGE_PERSISTENT_API_H_
#define _TEE_INTERNAL_STORAGE_PERSISTENT_API_H_

#include "tee_internal_common.h"
#include "tee_internal_storage_generic_api.h"

typedef enum
{
	TEE_DATA_SEEK_SET = 0,
	TEE_DATA_SEEK_CUR,
	TEE_DATA_SEEK_END
} TEE_Whence;

/** Handle on a persistent object enumerator */
typedef struct TEE_ObjectEnum* TEE_ObjectEnumHandle;

enum TEE_StorageType {
	TEE_STORAGE_PRIVATE = 0x00000001,
};

enum TEE_DataFlag {
	TEE_DATA_FLAG_ACCESS_READ = 0x00000001,
	TEE_DATA_FLAG_ACCESS_WRITE = 0x00000002,
	TEE_DATA_FLAG_ACCESS_WRITE_META = 0x00000004,
	TEE_DATA_FLAG_SHARE_READ = 0x00000010,
	TEE_DATA_FLAG_SHARE_WRITE = 0x00000020,
	TEE_DATA_FLAG_CREATE = 0x00000200,
	TEE_DATA_FLAG_EXCLUSIVE = 0x00000400,
};

enum TEE_Miscellaneous {
	TEE_DATA_MAX_POSITION = 0xFFFFFFFF,
	TEE_OBJECT_ID_MAX_LEN = 64,
};

typedef int (*GenTSKeyFunc)(
		const uint8_t *seedBuf,
		size_t seedLen,
		uint8_t* uniqueKey,
		size_t uniqueKeyLen);

TEE_Result TEE_OpenPersistentObject(
	uint32_t storageID,
	void* objectID,
	size_t objectIDLen,
	uint32_t flags,
	TEE_ObjectHandle* object);

TEE_Result TEE_CreatePersistentObject(
	uint32_t storageID,
	void* objectID,
	size_t objectIDLen,
	uint32_t flags,
	TEE_ObjectHandle attributes,
	void* initialData,
	size_t initialDataLen,
	TEE_ObjectHandle* object);

void TEE_FreePersistentObject(TEE_ObjectHandle object);

void TEE_CloseAndDeletePersistentObject(TEE_ObjectHandle object);

TEE_Result TEE_AllocatePersistentObjectEnumerator(
	TEE_ObjectEnumHandle* objectEnumerator);

void TEE_FreePersistentObjectEnumerator(TEE_ObjectEnumHandle
	objectEnumerator);

void TEE_ResetPersistentObjectEnumerator(TEE_ObjectEnumHandle
	objectEnumerator);

TEE_Result TEE_StartPersistentObjectEnumerator(
	TEE_ObjectEnumHandle objectEnumerator,
	uint32_t storageID);

TEE_Result TEE_GetNextPersistentObject(
	TEE_ObjectEnumHandle objectEnumerator,
	TEE_ObjectInfo* objectInfo,
	void* objectID,
	size_t* objectIDLen);

TEE_Result TEE_ReadObjectData(
	TEE_ObjectHandle object,
	void* buffer,
	size_t size,
	uint32_t* count);

TEE_Result TEE_WriteObjectData(
	TEE_ObjectHandle object,
	void* buffer,
	size_t size);

TEE_Result TEE_SeekObjectData(
	TEE_ObjectHandle object,
	int32_t offset,
	TEE_Whence whence);

void TEE_RegisterGenKey(GenTSKeyFunc GenFigoTSKey);

#endif /* _TEE_INTERNAL_STORAGE_PERSISTENT_API_H_ */
