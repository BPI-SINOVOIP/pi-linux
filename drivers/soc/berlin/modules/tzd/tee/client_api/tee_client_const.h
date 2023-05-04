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

#ifndef _TEE_CLIENT_CONST_H_
#define _TEE_CLIENT_CONST_H_

/** TEE client return code.
 *
 * The following function return codes, of type TEEC_Result (see section 4.3.2),
 * are defined by the specification.
 */
enum {
/*! The operation was successful. */
	TEEC_SUCCESS			= 0x00000000,
/*! 0x00000001 - 0xFFFEFFFF are implementation defined */
/*! Non-specific cause. */
	TEEC_ERROR_GENERIC		= 0xFFFF0000,
/*! Access privileges are not sufficient. */
	TEEC_ERROR_ACCESS_DENIED	= 0xFFFF0001,
/*! The operation was cancelled. */
	TEEC_ERROR_CANCEL		= 0xFFFF0002,
/*! Concurrent accesses caused conflict. */
	TEEC_ERROR_ACCESS_CONFLICT	= 0xFFFF0003,
/*! Too much data for the requested operation was passed. */
	TEEC_ERROR_EXCESS_DATA		= 0xFFFF0004,
/*! Input data was of invalid format. */
	TEEC_ERROR_BAD_FORMAT		= 0xFFFF0005,
/*! Input parameters were invalid. */
	TEEC_ERROR_BAD_PARAMETERS	= 0xFFFF0006,
/*! Operation is not valid in the current state. */
	TEEC_ERROR_BAD_STATE		= 0xFFFF0007,
/*! The requested data item is not found. */
	TEEC_ERROR_ITEM_NOT_FOUND	= 0xFFFF0008,
/*! The requested operation should exist but is not yet implemented. */
	TEEC_ERROR_NOT_IMPLEMENTED	= 0xFFFF0009,
/*! The requested operation is valid but is not supported in this
 * Implementation. */
	TEEC_ERROR_NOT_SUPPORTED	= 0xFFFF000A,
/*! Expected data was missing. */
	TEEC_ERROR_NO_DATA		= 0xFFFF000B,
/*! System ran out of resources. */
	TEEC_ERROR_OUT_OF_MEMORY	= 0xFFFF000C,
/*! The system is busy working on something else. */
	TEEC_ERROR_BUSY			= 0xFFFF000D,
/*! Communication with a remote party failed. */
	TEEC_ERROR_COMMUNICATION	= 0xFFFF000E,
/*! A security fault was detected. */
	TEEC_ERROR_SECURITY		= 0xFFFF000F,
/*! The supplied buffer is too short for the generated output. */
	TEEC_ERROR_SHORT_BUFFER		= 0xFFFF0010,
/*! 0xFFFF0011 - 0xFFFFFFFF are reserved for future use */
/*! The MAC value supplied is different from the one calculated */
	TEEC_ERROR_MAC_INVALID		= 0xFFFF3071,
};

/** Return code origins
 *
 * The following function return code origins, of type  uint32_t, are defined
 * by the specification. These indicate where in the software stack the return
 * code was generated for an open -session operation or an invoke-command
 * operation.
 */
enum TEEC_Origin {
/*! The return code is an error that originated within the TEE Client API
 * implementation. */
	TEEC_ORIGIN_API			= 0x00000001,
/*! The return code is an error that originated within the underlying
 * communications stack linking the rich OS with the TEE. */
	TEEC_ORIGIN_COMMS		= 0x00000002,
/*! The return code is an error that originated within the common TEE code. */
	TEEC_ORIGIN_TEE			= 0x00000003,
/*! The return code is an error that originated within the Trusted application
 * code. This includes the case where the return code is a success. */
	TEEC_ORIGIN_TRUSTED_APP		= 0x00000004,
/* All other values Reserved for Future Use */
};

/** Shared memory flag constants
 *
 * The following flag constants, of type  uint32_t, are defined by the
 * specification. These are used to indicate the current status and
 * synchronization requirements of Share d Memory blocks.
 */
enum TEEC_SharedMemFlags {
/*! The Shared Memory can carry data from the Client Application
 * to the Trusted Application. */
	TEEC_MEM_INPUT			= 0x00000001,
/*! The Shared Memory can carry data from the Trusted Application
 * to the Client Application. */
	TEEC_MEM_OUTPUT			= 0x00000002,
};

/** Param type constants
 *
 * The following constants, of type  uint32_t, are defined by the specification.
 * These are used to indicate the type of Parameter encoded inside the operation
 * structure.
 */
enum TEEC_ParamType {
/*! The Parameter is not used. */
	TEEC_NONE			= 0x00000000,
/*! The Parameter is a TEEC_Value tagged as input. */
	TEEC_VALUE_INPUT		= 0x00000001,
/*! The Parameter is a TEEC_Value tagged as output. */
	TEEC_VALUE_OUTPUT		= 0x00000002,
/*! The Parameter is a TEEC_Value tagged as both as input and output,
 * i.e., for which both the behaviors of TEEC_VALUE_INPUT and
 * TEEC_VALUE_OUTPUT apply. */
	TEEC_VALUE_INOUT		= 0x00000003,
/*! The Parameter is a TEEC_TempMemoryReference describing a region of memory
 * which needs to be temporarily registered for the duration of the Operation
and is tagged as input. */
	TEEC_MEMREF_TEMP_INPUT		= 0x00000005,
/*! Same as TEEC_MEMREF_TEMP_INPUT, but the Memory Reference is tagged as
 * output. The Implementation may update the size field to reflect the
 * required output size in some use cases. */
	TEEC_MEMREF_TEMP_OUTPUT		= 0x00000006,
/*! A Temporary Memory Reference tagged as both input and output,
 * i.e., for which both the behaviors of TEEC_MEMREF_TEMP_INPUT and
 * TEEC_MEMREF_TEMP_OUTPUT apply. */
	TEEC_MEMREF_TEMP_INOUT		= 0x00000007,
/*! The Parameter is a Registered Memory Reference that refers to the
 * entirety of its parent Shared Memory block. The parameter structure is a
 * TEEC_MemoryReference. In this structure, the Implementation MUST read
 * only the parent field and MAY update the size field when the
 * operation completes. */
	TEEC_MEMREF_WHOLE		= 0x0000000C,
/*! A Registered Memory Reference structure that refers to a partial region
 * of its parent Shared Memory block and is tagged as input.
 */
	TEEC_MEMREF_PARTIAL_INPUT	= 0x0000000D,
/*! A Registered Memory Reference structure that refers to a partial region
 * of its parent Shared Memory block and is tagged as output.
 */
	TEEC_MEMREF_PARTIAL_OUTPUT	= 0x0000000E,
/*! A Registered Memory Reference structure that refers to a partial region
 * of its parent Shared Memory block and is tagged as both input and output.
 */
	TEEC_MEMREF_PARTIAL_INOUT	= 0x0000000F,

/* All other values Reserved for Future Use */
};


/** Login flag constants
 *
 * The following constants, of type  uint32_t, are defined by the specification.
 * These are used to indicate what identity credentials about the Client
 * Application are used by the Implementation to determine access control
 * permissions to functionality provided by, or data stored by, the Trusted
 * Application.
 */
enum TEEC_LoginFlags {
/*! No login is to be used.*/
	TEEC_LOGIN_PUBLIC		= 0x00000000,
/*! The user executing the application is provided.*/
	TEEC_LOGIN_USER			= 0x00000001,
/*! The user group executing the application is provided.*/
	TEEC_LOGIN_GROUP		= 0x00000002,
/*! Login data about the running Client Application itself is provided. */
	TEEC_LOGIN_APPLICATION		= 0x00000004,
/*! Login data about the user running the Client Application and about the
 * Client Application itself is provided. */
	TEEC_LOGIN_USER_APPLICATION	= 0x00000005,
/*! Login data about the group running the Client Application and about the
 * Client Application itself is provided. */
	TEEC_LOGIN_GROUP_APPLICATION	= 0x00000006,

/* All other constant values Reserved for Future Use */

/* 0x80000000 - 0xFFFFFFFF:
 * Reserved for implementation-defined connection methods.
 * Behavior is implementation-defined. */
};

#endif /* _TEE_CLIENT_CONST_H_ */
