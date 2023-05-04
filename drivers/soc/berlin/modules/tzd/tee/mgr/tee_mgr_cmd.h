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

#ifndef _TEE_MGR_CMD_H_
#define _TEE_MGR_CMD_H_

#define TAMGR_UUID	{0x9ef9656c, 0x8f49, 0x11e3,	\
			{0x90, 0x1c, 0x78, 0x2b, 0xcb, 0x5c, 0xf3, 0xe3}}

enum TAMgrCmd {
	TAMGR_CMD_SUSPEND,
	TAMGR_CMD_RESUME,
	TAMGR_CMD_REGISTER,
	/* Create Instance
	 * input: destination UUID
	 * output: created taskId
	 */
	TAMGR_CMD_CREATE_INSTANCE,
	/* Destroy Instance
	 * input: taskId to destroy in params[0].value.a
	 * note: should NOT need call it explictly
	 */
	TAMGR_CMD_DESTROY_INSTANCE,

	TAMGR_CMD_PROVISION_FEATURE_CERT,
	TAMGR_CMD_REVOKE_FEATURE_CERT,
	TAMGR_CMD_LOAD_FEATURE_CERT,

	TAMGR_CMD_MAX
};

typedef struct {
	TEE_UUID destination;	/* input: desitination TA uuid */
	uint32_t taskId;	/* output: created taskId */
} TAMgrCmdCreateInstanceParamExt;

#endif /* _TEE_MGR_CMD_H_ */
