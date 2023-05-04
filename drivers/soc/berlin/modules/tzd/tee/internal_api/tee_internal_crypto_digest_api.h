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

#ifndef _TEE_INTERNAL_CRYPTO_DIGEST_API_H_
#define _TEE_INTERNAL_CRYPTO_DIGEST_API_H_

#include "tee_internal_crypto_common.h"

/** Accumulate message data for hashing.
 *
 * @param operation	operation handle
 * @param chunk		buffer that contains the data
 * @param chunkSize	size of the input buffer.
 */
void TEE_DigestUpdate(TEE_OperationHandle operation,
			void* chunk, size_t chunkSize);

/** Finalize the message digest operation and produce the hash of the message.
 *
 * @param operation	operation handle
 * @param chunk		last buffer that needs to be used for producing the hash
 * @param chunkLen	size of the last buffer
 * @param hash		output buffer into which the hash is stored
 * @param hashlen	pointer to size of the output data
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to contain the output
 */
TEE_Result TEE_DigestDoFinal(TEE_OperationHandle operation,
		void* chunk, size_t chunkLen,void* hash, size_t *hashLen);

#endif /* _TEE_INTERNAL_CRYPTO_DIGEST_API_H_ */
