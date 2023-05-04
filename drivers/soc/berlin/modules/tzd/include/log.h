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

#ifndef _LOG_H_
#define _LOG_H_

#ifdef __KERNEL__
#include "asm/bug.h"
#define assert(x)	BUG_ON(!(x))
#else
#include "assert.h"
#endif

#if defined(__TRUSTZONE__)
#include "macros.h"
#include "printf.h"
#include "printk.h"
int get_cpu_id(void);
#define tee_printf	printf
#elif defined(CONFIG_AMP)
#include "isl/amp_logger.h"
#define tee_printf	AMPPRINTF
#undef CONFIG_LOG_TIME
#undef CONFIG_LOG_CPUID
#elif defined(__KERNEL__)
#include <linux/kernel.h>
#define tee_printf	printk
#undef CONFIG_LOG_TIME
#undef CONFIG_LOG_CPUID
#else
#define tee_printf	printf
#endif

#ifdef CONFIG_LOG_TIME
#include "clock_source.h"
#define log(info, fmt, args...)	do { \
		uint32_t s, us; \
		clocksource_read_timestamp(&s, &us); \
		tee_printf("[%d.%06d] TZ CPU%d " info "[%s:%d] " fmt, s, us, \
				get_cpu_id(), __func__, __LINE__, ##args); \
	} while (0)
#elif defined(CONFIG_LOG_CPUID)
#define log(info, fmt, args...)	do { \
		tee_printf("TZ CPU%d " info "[%s:%d] " fmt, get_cpu_id(), __func__, __LINE__, ##args); \
	} while (0)
#else
#define log(info, fmt, args...)	do { \
		tee_printf("TZ " info "[%s:%d] " fmt, __func__, __LINE__, ##args); \
	} while (0)
#endif /* CONFIG_LOG_TIME */

#ifdef CONFIG_TRACE
#	define trace(fmt, args...)	log("", fmt, ##args)
#else /* !CONFIG_TRACE */
#	define trace(fmt, ...)		do { } while (0)
#endif /* CONFIG_TRACE */

#ifdef CONFIG_ERROR
#	define error(fmt, args...)	log("ERROR", fmt, ##args)
#else /* !CONFIG_ERROR */
#	define error(fmt, ...)		do { } while (0)
#endif /* CONFIG_ERROR */

#ifdef CONFIG_WARN
#	define warn(fmt, args...)	log("WARNING", fmt, ##args)
#else /* !CONFIG_WARN */
#	define warn(fmt, ...)		do { } while (0)
#endif /* CONFIG_WARN */

#ifdef CONFIG_INFO
#	define info(fmt, args...)	log("", fmt, ##args)
#else /* !CONFIG_INFO */
#	define info(fmt, ...)		do { } while (0)
#endif /* CONFIG_INFO */

#endif /* _LOG_H_ */
