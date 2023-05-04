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

#ifndef _TEE_INTERNAL_CRYPTO_AE_API_H_
#define _TEE_INTERNAL_CRYPTO_AE_API_H_

#include "tee_internal_crypto_common.h"

/** Initialize an Authentication Encryption operation.
 *
 * @param operation	operation handle
 * @param nonce		initialization vector
 * @param nonceLen	length of the initialization vector
 * @param tagLen	size (in bits) of the tag
 * @param AADLen	length of AAD (in bytes)
 * @param payloadLen	length of the payload (in bytes)
 *
 * @retval TEE_SUCCESS			on success
 * @retval TEE_ERROR_BAD_PARAMETERS	if the tag length is not supported by
 *					the algorithm
 */
TEE_Result TEE_AEInit(TEE_OperationHandle operation,
			void* nonce, size_t nonceLen, uint32_t tagLen,
			uint32_t AADLen, uint32_t payloadLen);


/** Feed a new chunk of Additional Authentication Data (AAD) to the AE operation.
 *
 * @param operation	operation handle
 * @param AAD input	buffer containing the chunk of AAD
 * @param AADdataLen	length of the input buffer
 */
void TEE_AEUpdateAAD(TEE_OperationHandle operation,
			void* AADdata, size_t AADdataLen);

/** Accumulate data for an Authentication Encryption operation.
 *
 * @param operation	operation handle
 * @param srcData	input buffer on which the AE operation needs to be performed
 * @param srcLen	length of the input buffer
 * @param destData	output buffer into which the result of the AE operation
 *			needs to be stored
 * @param destLen	pointer to length of the input buffer
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_AEUpdate(TEE_OperationHandle operation,void* srcData,
			size_t srcLen,void* destData, size_t *destLen);

/** Finalize the computation of the AE encryption.
 *
 * @param operation	operation handle
 * @param srcData	input buffer on which the AE encryption needs to be performed
 * @param srcLen	length of the input buffer
 * @param destData	output buffer against which the the result of the operation
 *			needs to be stored
 * @param destLen	pointer to length of the input buffer
 * @param tag		buffer into which the computed tag needs to be stored
 * @param tagLen	pointer to length of the buffer for tag
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_AEEncryptFinal(TEE_OperationHandle operation,
		void* srcData, size_t srcLen, void* destData, size_t* destLen,
		void* tag, size_t* tagLen);

/** Finalize the computation of the AE decryption.
 *
 * @param operation	operation handle
 * @param srcData	input buffer on which the AE decryption needs to be performed
 * @param srcLen	length of the input buffer
 * @param destData	output buffer against which the the result of the operation
 *			needs to be stored
 * @param destLen	pointer to length of the input buffer
 * @param tag		buffer into which the computed tag needs to be stored
 * @param tagLen	length of the buffer for tag
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 * @retval TEE_ERROR_MAC_INVALID	if the computed tag does not match the
 *					supplied tag
 */
TEE_Result TEE_AEDecryptFinal(TEE_OperationHandle operation, void* srcData,
			size_t srcLen, void* destData, size_t *destLen,
				void* tag, size_t tagLen);

#endif /* _TEE_INTERNAL_CRYPTO_AE_API_H_ */
