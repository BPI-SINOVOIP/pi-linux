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

#ifndef _TEE_INTERNAL_STORAGE_GENERIC_API_H_
#define _TEE_INTERNAL_STORAGE_GENERIC_API_H_

#include "tee_internal_common.h"

typedef void* TEE_ObjectHandle;

#define TEE_ATTR_FLAG_VALUE			(0x20000000)
#define TEE_ATTR_FLAG_PUBLIC			(0x10000000)
#define TEE_ATTR_SECRET_VALUE			(0xC0000000)
#define TEE_ATTR_RSA_MODULUS			(0xD0000130)
#define TEE_ATTR_RSA_PUBLIC_EXPONENT		(0xD0000230)
#define TEE_ATTR_RSA_PRIVATE_EXPONENT		(0xC0000330)
#define TEE_ATTR_RSA_PRIME1			(0xC0000430)
#define TEE_ATTR_RSA_PRIME2			(0xC0000530)
#define TEE_ATTR_RSA_EXPONENT1			(0xC0000630)
#define TEE_ATTR_RSA_EXPONENT2			(0xC0000730)
#define TEE_ATTR_RSA_COEFFICIENT		(0xC0000830)
#define TEE_ATTR_RSA_PSS_SALT_LENGTH		(0xF0000A30)

typedef struct {
	uint32_t objectType;
	uint32_t objectSize;
	uint32_t maxObjectSize;
	uint32_t objectUsage;
	uint32_t dataSize;
	uint32_t dataPosition;
	uint32_t handleFlags;
} TEE_ObjectInfo;

typedef struct {
	uint32_t attributeID;
	union {
		struct {
			void* buffer;
			size_t length;
		} ref;
		struct {
			uint32_t a, b;
		} value;
	} content;
} TEE_Attribute;

enum TEE_UsageFlag {
	TEE_USAGE_EXTRACTABLE = 0x00000001,
	TEE_USAGE_ENCRYPT = 0x00000002,
	TEE_USAGE_DECRYPT = 0x00000004,
	TEE_USAGE_MAC = 0x00000008,
	TEE_USAGE_SIGN = 0x00000010,
	TEE_USAGE_VERIFY = 0x00000020,
	TEE_USAGE_DERIVE = 0x00000040,
};

enum TEE_HandleFlag {
	TEE_HANDLE_FLAG_PERSISTENT = 0x00010000,
	TEE_HANDLE_FLAG_INITIALIZED = 0x00020000,
	TEE_HANDLE_FLAG_KEY_SET = 0x00040000,
	TEE_HANDLE_FLAG_EXPECT_TWO_KEYS = 0x00080000,
};

void TEE_GetObjectInfo(
	TEE_ObjectHandle object,
	TEE_ObjectInfo* objectInfo);

void TEE_RestrictObjectUsage(
	TEE_ObjectHandle object,
	uint32_t objectUsage);

TEE_Result TEE_GetObjectBufferAttribute(
	TEE_ObjectHandle object,
	uint32_t attributeID,
	void* buffer, size_t* size);

TEE_Result TEE_GetObjectValueAttribute(
	TEE_ObjectHandle object,
	uint32_t attributeID,
	uint32_t* a,
	uint32_t* b);

void TEE_CloseObject(TEE_ObjectHandle object);

#endif /* _TEE_INTERNAL_STORAGE_GENERIC_API_H_ */
