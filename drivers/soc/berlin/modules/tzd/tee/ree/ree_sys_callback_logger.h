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

#ifndef _REE_SYS_CALLBACK_LOGGER_H_
#define _REE_SYS_CALLBACK_LOGGER_H_

#include "tee_comm.h"

#define REE_LOGGER_SIZE		TEE_COMM_PARAM_EXT_SIZE
#define REE_LOGGER_TAG_SIZE	16
#define REE_LOGGER_TEXT_SIZE	(REE_LOGGER_SIZE-REE_LOGGER_TAG_SIZE)

#define REE_LOGGER_PARAM_BUFID		1
#define REE_LOGGER_PARAM_PRIO		2
#define REE_LOGGER_PARAM_TAG		3
#define REE_LOGGER_PARAM_TEXT		4

typedef union {
	char raw[REE_LOGGER_SIZE];
	struct REE_Logger_Buffer {
		char tag[REE_LOGGER_TAG_SIZE];
		char text[REE_LOGGER_TEXT_SIZE];
	} buffer;
} REE_Logger_t;

#ifdef __KERNEL__
#include <linux/notifier.h>

struct ree_logger_param {
	int bufID;
	int prio;
	const char *tag;
	const char *text;
};

int register_ree_logger_notifier(struct notifier_block *nb);
int unregister_ree_logger_notifier(struct notifier_block *nb);
#endif

#endif
