/*
 * TrustZone Driver
 *
 * Copyright (C) 2016 Marvell Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __TZ_LOG_H__
#define __TZ_LOG_H__

#include <linux/printk.h>
#include "config.h"

#ifdef CONFIG_DEBUG
#define tz_debug(fmt, args...)	\
	printk(KERN_DEBUG "TZ DEBUG %s(%i, %s): " fmt "\n", \
			__func__, current->pid, current->comm, ## args)
#else
#define tz_debug(...)
#endif

#define tz_info(fmt, args...)	\
	printk(KERN_INFO "TZ INFO %s(%i, %s): " fmt "\n", \
			__func__, current->pid, current->comm, ## args)

#define tz_error(fmt, args...)	\
	printk(KERN_ERR "TZ ERROR %s(%i, %s): " fmt "\n", \
			__func__, current->pid, current->comm, ## args)

#endif /* __TZ_LOG_H__ */
