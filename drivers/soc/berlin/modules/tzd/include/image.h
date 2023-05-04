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

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "types.h"

#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
	((unsigned int)(char)(ch0) | ((unsigned int)(char)(ch1) << 8) | \
	((unsigned int)(char)(ch2) << 16) | ((unsigned int)(char)(ch3) << 24))

/*
 * Generic Image Format:
 *
 * +----------------------------+
 * | image_header		|
 * +----------------------------+
 * | chunk 0			|
 * +----------------------------+
 * | ...			|
 * +----------------------------+
 * | chunk n			|
 * +----------------------------+
 */

#define IMAGE_HEADER_MAGIC_NUM		MAKE_FOURCC('I', 'M', '*', 'H')
#define IMAGE_HEADER_VERSION		(0)

struct image_header {
	uint32_t header_magic_num;	/* 'IM*H' */
	uint32_t header_size;
	uint32_t header_version;
	uint32_t header_reserved;
	char     image_name[32];
	uint32_t image_version;
	uint32_t reserved[2];
	uint32_t chunk_num;
	struct {
		uint32_t id;
		uint32_t offset;	/* start from header, 16bytes aligned */
		uint32_t size;
		uint32_t attr0;		/* bit24:31 - compress method; */
		/* data can be in (dest_start, dest_start + dest_size)
		 * if dest_size == 0, then chunk data must always place at
		 * dest_start.
		 */
		uint64_t dest_start;
		uint32_t dest_size;
		uint32_t attr1;		/* for compressed data, it's original_size */
	} chunk[0];
};

/*
 * predefined chunk ID.
 */

#define IMAGE_CHUNK_ID_BOOTLOADER		MAKE_FOURCC('B', 'T', 'L', 'R')
#define IMAGE_CHUNK_ID_SYSTEM_MANAGER		MAKE_FOURCC('S', 'M', '*', '*')
#define IMAGE_CHUNK_ID_UBOOT			MAKE_FOURCC('U', 'B', 'T', '*')

#define IMAGE_CHUNK_ID_LINUX_BOOTIMG		MAKE_FOURCC('L', 'N', 'X', 'B')
#define IMAGE_CHUNK_ID_LINUX_DTB		MAKE_FOURCC('L', 'D', 'T', 'B')

#define IMAGE_CHUNK_ID_TZ_KERNEL		MAKE_FOURCC('T', 'Z', 'K', '*')
#define IMAGE_CHUNK_ID_TZ_BOOT_PARAM		MAKE_FOURCC('T', 'Z', 'B', 'P')

#define IMAGE_CHUNK_ID_TZ_LOADABLE_TA		MAKE_FOURCC('T', 'Z', 'T', 'A')

#define IMAGE_CHUNK_ATTR_COMPRESS(attr)		((attr & 0xff000000) >> 24)

#endif /* _IMAGE_H_ */
