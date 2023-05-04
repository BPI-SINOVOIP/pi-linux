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

#ifndef _TEE_INTERNAL_CRYPTO_COMMON_H_
#define _TEE_INTERNAL_CRYPTO_COMMON_H_

#include "tee_internal_common.h"
#include "tee_internal_storage_api.h"

/*
 * Typedefs for various crypto operation constants
 */
typedef enum {
	TEE_OPERATION_CIPHER = 0x1,
	TEE_OPERATION_MAC,
	TEE_OPERATION_AE,
	TEE_OPERATION_DIGEST,
	TEE_OPERATION_ASYMMETRIC_CIPHER,
	TEE_OPERATION_ASYMMETRIC_SIGNATURE,
	TEE_OPERATION_KEY_DERIVATION
} TEE_Operation_Constants;

/*
 * Valid tag lengths for Asymmetric cipher encryption
 */
typedef enum {
	TEE_TAG_LEN_32 = 32,
	TEE_TAG_LEN_48 = 48,
	TEE_TAG_LEN_64 = 64,
	TEE_TAG_LEN_96 = 96,
	TEE_TAG_LEN_104 = 104,
	TEE_TAG_LEN_112 = 112,
	TEE_TAG_LEN_120 = 120,
	TEE_TAG_LEN_128 = 128,
} TEE_Valid_Tag_Lengths;

/*
 * Typedefs for various crypto operation modes
 */
typedef enum {
	TEE_MODE_ENCRYPT = 0x0,
	TEE_MODE_DECRYPT,
	TEE_MODE_SIGN,
	TEE_MODE_VERIFY,
	TEE_MODE_MAC,
	TEE_MODE_DIGEST,
	TEE_MODE_DERIVE
} TEE_OperationMode;

/*
 * Structure that gives information regarding the cryptographic operation
 */
typedef struct {
	uint32_t algorithm;
	uint32_t operationClass;
	uint32_t mode;
	uint32_t digestLength;
	uint32_t maxKeySize;
	uint32_t keySize;
	uint32_t requiredKeyUsage;
	uint32_t handleState;
} TEE_OperationInfo;

/*
 * opaque structure definition for an operation handle.
 */
typedef struct {
	TEE_OperationInfo operationInfo;
	void *keyData;
	void *cryptoContext;
	const void *cryptoCipher;
} TEE_Operation;

/* Handle on a cryptographic operation */
typedef TEE_Operation* TEE_OperationHandle;

#define TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA224 TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA1
#define TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA256 TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA1
#define TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA384 TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA1
#define TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA512 TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA1

/*
 * enumeration of all possible ID's for algorithm values.
 */
typedef enum {
	TEE_ALG_AES_ECB_NOPAD =  0x10000010,
	TEE_ALG_AES_CBC_NOPAD = 0x10000110,
	TEE_ALG_AES_CTR = 0x10000210,
	TEE_ALG_AES_CTS = 0x10000310,
	TEE_ALG_AES_XTS = 0x10000410,
	TEE_ALG_AES_CBC_MAC_NOPAD = 0x30000110,
	TEE_ALG_AES_CBC_MAC_PKCS5 = 0x30000510,
	TEE_ALG_AES_CMAC = 0x30000610,
	TEE_ALG_AES_CCM = 0x40000710,
	TEE_ALG_AES_GCM = 0x40000810,
	TEE_ALG_DES_ECB_NOPAD = 0x10000011,
	TEE_ALG_DES_CBC_NOPAD = 0x10000111,
	TEE_ALG_DES_CBC_MAC_NOPAD = 0x30000111,
	TEE_ALG_DES_CBC_MAC_PKCS5 = 0x30000511,
	TEE_ALG_DES3_ECB_NOPAD = 0x10000013,
	TEE_ALG_DES3_CBC_NOPAD = 0x10000113,
	TEE_ALG_DES3_CBC_MAC_NOPAD = 0x30000113,
	TEE_ALG_DES3_CBC_MAC_PKCS5 = 0x30000513,
	TEE_ALG_RSASSA_PKCS1_V1_5_MD5 = 0x70001830,
	TEE_ALG_RSASSA_PKCS1_V1_5_SHA1 = 0x70002830,
	TEE_ALG_RSASSA_PKCS1_V1_5_SHA224 = 0x70003830,
	TEE_ALG_RSASSA_PKCS1_V1_5_SHA256 = 0x70004830,
	TEE_ALG_RSASSA_PKCS1_V1_5_SHA384 = 0x70005830,
	TEE_ALG_RSASSA_PKCS1_V1_5_SHA512 = 0x70006830,
	TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA1 = 0x70212930,
	TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA224 = 0x70313930,
	TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA256 = 0x70414930,
	TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA384 = 0x70515930,
	TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA512 = 0x70616930,
	TEE_ALG_RSAES_PKCS1_V1_5 = 0x60000130,
	TEE_ALG_RSAES_PKCS1_OAEP_MGF1_SHA1 = 0x60210230,
	TEE_ALG_RSA_NOPAD = 0x60000030,
	TEE_ALG_DSA_SHA1 = 0x70002131,
	TEE_ALG_DH_DERIVE_SHARED_SECRET = 0x80000032,
	TEE_ALG_MD5 = 0x50000001,
	TEE_ALG_SHA1 = 0x50000002,
	TEE_ALG_SHA224 = 0x50000003,
	TEE_ALG_SHA256 = 0x50000004,
	TEE_ALG_SHA384 = 0x50000005,
	TEE_ALG_SHA512 = 0x50000006,
	TEE_ALG_HMAC_MD5 = 0x30000001,
	TEE_ALG_HMAC_SHA1 = 0x30000002,
	TEE_ALG_HMAC_SHA224 = 0x30000003,
	TEE_ALG_HMAC_SHA256 = 0x30000004,
	TEE_ALG_HMAC_SHA384 = 0x30000005,
	TEE_ALG_HMAC_SHA512 = 0x30000006
} TEE_CRYPTO_ALGORITHM_ID;

#endif /* _TEE_INTERNAL_CRYPTO_COMMON_H_ */
