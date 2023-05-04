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

#ifndef _TEE_INTERNAL_CRYPTO_ASYMMETRIC_API_H_
#define _TEE_INTERNAL_CRYPTO_ASYMMETRIC_API_H_

#include "tee_internal_crypto_common.h"

/** Encrypt a message with an asymmetric operation.
 *
 * @param operation	operation handle
 * @param params	should be NULL in current version
 * @param paramCount	should be 0 in current version
 * @param srcData	data that is to be encrypted
 * @param srcLen	length of the input data encrypted
 * @param destData	output data into which the data needs to be stored
 * @param destLen	pointer to the length of output data
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_AsymmetricEncrypt(TEE_OperationHandle operation,
				  TEE_Attribute* params, uint32_t paramCount,
				  void* srcData, size_t srcLen,
				  void* destData, size_t *destLen);

/** Decrypt a message with an asymmetric operation.
 *
 * @param operation	operation handle
 * @param params	should be NULL in current version
 * @param paramCount	should be 0 in current version
 * @param srcData	data that is to be decrypted
 * @param srcLen	length of the input data decrypted
 * @param destData	output data into which the data needs to be stored
 * @param destLen	pointer to the length of output data
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_AsymmetricDecrypt(TEE_OperationHandle operation,
				  TEE_Attribute* params, uint32_t paramCount,
				  void* srcData, size_t srcLen,
				  void* destData, size_t *destLen);

/** Sign a message with an asymmetric operation, Note that only an already hashed
 *  message can be signed
 *
 * @param operation	operation handle
 * @param params	param for TEE_ATTR_RSA_PSS_SALT_LENGTH, only use for algorithm
 *			TEE_ALG_RSASSA_PKCS1_PSS_MGF1_XXX, This parameter is optional.
 *			If not present, the salt length is equal to the hash length.
 * @param paramCount	parameter count
 * @param digest	input data containing the input message digest
 * @param srcLen	length of the input data
 * @param signature	output buffer for signature of the digest
 * @param signatureLen	pointer to the length of signature
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_AsymmetricSignDigest(TEE_OperationHandle operation,
				    TEE_Attribute* params, uint32_t paramCount,
				    void* digest, size_t digestLen,
				    void* signature, size_t *signatureLen);

/** Verify a message digest signature with an asymmetric operation.
 *
 * @param operation	operation handle
 * @param params	param for TEE_ATTR_RSA_PSS_SALT_LENGTH, only use for algorithm
 *			TEE_ALG_RSASSA_PKCS1_PSS_MGF1_XXX, This parameter is optional.
 *			If not present, the salt length is equal to the hash length.
 * @param paramCount	parameter count
 * @param digest	input data containing the input message digest
 * @param srcLen	length of the input data
 * @param signature	input buffer for signature of the digest
 * @param signatureLen	length of signature
 * @retval TEE_SUCCESS			on success
 * @retval TEE_ERROR_SIGNATURE_INVALID	if the signature is invalid
 */
TEE_Result TEE_AsymmetricVerifyDigest(TEE_OperationHandle operation,
				      TEE_Attribute* params, uint32_t paramCount,
				      void* digest, size_t digestLen,
				      void* signature, size_t signatureLen);

#endif /* _TEE_INTERNAL_CRYPTO_ASYMMETRIC_API_H_ */
