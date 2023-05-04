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

#ifndef _TEE_INTERNAL_STORAGE_TRANSIENT_API_H_
#define _TEE_INTERNAL_STORAGE_TRANSIENT_API_H_

#include "tee_internal_common.h"
#include "tee_internal_storage_generic_api.h"

TEE_Result TEE_AllocateTransientObject(
	uint32_t objectType,
	uint32_t maxObjectSize,
	TEE_ObjectHandle* object);

/* TEE_AllocateTransientObject and Object Sizes */
#define TEE_TYPE_AES		(0xA0000010) /* 128, 192, or 256 bits */
#define TEE_TYPE_DES		(0xA0000011) /* Always 56 bits */
#define TEE_TYPE_DES3		(0xA0000013) /* 112 or 168 bits */
#define TEE_TYPE_HMAC_MD5	(0xA0000001) /* Between 64 and 512 bits, multiple of 8 bits */
#define TEE_TYPE_HMAC_SHA1	(0xA0000002) /* Between 80 and 512 bits, multiple of 8 bits */
#define TEE_TYPE_HMAC_SHA224	(0xA0000003) /* Between 112 and 512 bits, multiple of 8 bits */
#define TEE_TYPE_HMAC_SHA256	(0xA0000004) /* Between 192 and 1024 bits, multiple of 8 bits */
#define TEE_TYPE_HMAC_SHA384	(0xA0000005) /* Between 256 and 1024 bits, multiple of 8 bits */
#define TEE_TYPE_HMAC_SHA512	(0xA0000006) /* Between 256 and 1024 bits, multiple of 8 bits */
#define TEE_TYPE_RSA_PUBLIC_KEY	(0xA0000030) /* Object size is the number of bits in the modulus.
						All key size up to 2048 bits must be supported.
						Support for bigger key 3 sizes is implementation-dependent.
						Minimum key size is 256 bits. */
#define TEE_TYPE_RSA_KEYPAIR	(0xA1000030) /* Same as for RSA public key size */
#define TEE_TYPE_DSA_PUBLIC_KEY	(0xA0000031) /* Between 512 and 1024 bits, multiple of 64 bits */
#define TEE_TYPE_DSA_KEYPAIR	(0xA1000031) /* Between 512 and 1024 bits, multiple of 64 bits */
#define TEE_TYPE_DH_KEYPAIR	(0xA1000032) /* From 256 to 2048 bits */
#define TEE_TYPE_GENERIC_SECRET	(0xA0000000) /* Multiple of 8 bits, up to 4096 bits. This type
						is intended for secret data that is not directly
						used as a key in a cryptographic operation, but
						participates in a key derivation. */

void TEE_FreeTransientObject(
	TEE_ObjectHandle object);

void TEE_ResetTransientObject(
	TEE_ObjectHandle object);

TEE_Result TEE_PopulateTransientObject(
	TEE_ObjectHandle object,
	TEE_Attribute* attrs,
	uint32_t attrCount);

void TEE_InitRefAttribute(
	TEE_Attribute* attr,
	uint32_t attributeID,
	void* buffer,
	size_t length);

void TEE_InitValueAttribute(
	TEE_Attribute* attr,
	uint32_t attributeID,
	uint32_t a, uint32_t b);

void TEE_CopyObjectAttributes(
	TEE_ObjectHandle destObject,
	TEE_ObjectHandle srcObject);

TEE_Result TEE_GenerateKey(
	TEE_ObjectHandle object,
	uint32_t keySize,
	TEE_Attribute* params,
	uint32_t paramCount);

#endif /* _TEE_INTERNAL_STORAGE_TRANSIENT_API_H_ */
