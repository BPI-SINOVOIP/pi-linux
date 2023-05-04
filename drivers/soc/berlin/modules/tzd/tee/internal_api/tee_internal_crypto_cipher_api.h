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

#ifndef _TEE_INTERNAL_CRYPTO_CIPHER_API_H_
#define _TEE_INTERNAL_CRYPTO_CIPHER_API_H_

#include "tee_internal_crypto_common.h"

/** Initialize a symmetric cipher operation.
 *
 * @param operation	operation handle
 * @param IV		initialization vector
 * @param IVLen		length of the initialization vector
 */
void TEE_CipherInit(TEE_OperationHandle operation,void* IV, size_t IVLen);

/** Encrypt or decrypt the input data.
 *
 * @param operation	operation handle
 * @param srcData	data that is to be encrypted or decrypted
 * @param srcLen	length of the input data encrypted or decrypted
 * @param destData	output data into which the data needs to be stored
 * @param destLen	pointer to the length of output data
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_CipherUpdate(TEE_OperationHandle operation,
				void* srcData, size_t srcLen,
				void* destData, size_t *destLen);

/** Finalize the cipher operation and encrypt or decrypt any remaining data.
 *
 * @param operation	operation handle
 * @param srcData	data that is to be encrypted or decrypted
 * @param srcLen	length of the input data encrypted or decrypted
 * @param destData	output data into which the data needs to be stored
 * @param destLen	pointer to the length of output data
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_CipherDoFinal(TEE_OperationHandle operation,
				void* srcData, size_t srcLen,
				void* destData, size_t *destLen);

#endif /* _TEE_INTERNAL_CRYPTO_CIPHER_API_H_ */
