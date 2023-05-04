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

#ifndef _TEE_CLIENT_CONFIG_H_
#define _TEE_CLIENT_CONFIG_H_

/** The maximum size of a single Shared Memory block, in bytes, of both API
 * allocated and API registered memory. This version of the standard requires
 * that this maximum size is greater than or equal to 512kB.
 * In systems where there is no limit imposed by the Implementation then
 * this definition should be defined to be the size of the address space.
*/
#define TEEC_CONFIG_SHAREDMEM_MAX_SIZE		0x80000

#endif /* _TEE_CLIENT_CONFIG_H_ */