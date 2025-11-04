/*************************************************************************/ /*
 UVCS Driver (kernel module)

 Copyright (C) 2015 - 2024 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

 The contents of this file are subject to the MIT license as set out below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 Alternatively, the contents of this file may be used under the terms of
 the GNU General Public License Version 2 ("GPL") in which case the provisions
 of GPL are applicable instead of those above.

 If you wish to allow use of your version of this file only under the terms of
 GPL, and not to allow others to use your version of this file under the terms
 of the MIT license, indicate your decision by deleting the provisions above
 and replace them with the notice and other provisions required by GPL as set
 out in the file called "GPL-COPYING" included in this distribution. If you do
 not delete the provisions above, a recipient may use your version of this file
 under the terms of either the MIT license or GPL.

 This License is also included in this distribution in the file called
 "MIT-COPYING".

 EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/

/******************************************************************************/
/*                    INCLUDE FILES                                           */
/******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include "uvcs_cmn.h"
#include "uvcs_lkm_internal.h"


/**
 * \brief worker thread of uvcs-driver.
 *
 * \return exit_code
 *
 * Executes uvcs_cmn_execute function when the event received.
 */
static int uvcs_thread(
	void *data	/**< driver information */
	)
{
	struct uvcs_driver_info *drv = (struct uvcs_driver_info *)data;
	struct uvcs_thr_ctrl *thr = &drv->thread_ctrl;
	struct timespec64 ts = {0u};
	int evt_ret;

	while (!kthread_should_stop()) {
		evt_ret = wait_event_interruptible(
			thr->evt_wait_q,
			(thr->evt_req == true) || kthread_should_stop()
			);
		if (!evt_ret) {
			if (thr->evt_req) {
				ktime_get_raw_ts64(&ts);
				thr->evt_req = false;
				uvcs_cmn_execute(drv->uvcs_info, ts.tv_nsec);
			}
		}
	}

	return 0;
}

/**
 * \brief sends event to worker thread
 */
void uvcs_thread_event(
	UVCS_PTR udptr	/**< driver information */
	)
{
	struct uvcs_driver_info *drv = (struct uvcs_driver_info *)udptr;

	if ((drv)
	&&	(drv->thread_ctrl.thread)) {
		drv->thread_ctrl.evt_req = true;
		wake_up_interruptible(&drv->thread_ctrl.evt_wait_q);
	}
}

/**
 * \brief creates worker thread.
 *
 * \retval UVCS_TRUE success
 * \retval UVCS_FALSE failed to create worker thread
 */
UVCS_BOOL uvcs_thread_create(
	UVCS_PTR udptr	/**< driver information */
	)
{
	struct uvcs_driver_info *drv = (struct uvcs_driver_info *)udptr;
	struct uvcs_thr_ctrl *thr;
	UVCS_BOOL result = UVCS_FALSE;

	if (drv) {
		thr = &drv->thread_ctrl;
		thr->evt_req  = false;
		init_waitqueue_head(&thr->evt_wait_q);
		thr->thread = kthread_run(uvcs_thread, drv, "uvcs");
		if (IS_ERR(drv->thread_ctrl.thread)) {
			thr->thread = NULL;
		} else {
			result = UVCS_TRUE;
		}
	}

	return result;
}

/**
 * \brief destroy worker thread
 */
void uvcs_thread_destroy(
	UVCS_PTR udptr	/**< driver information */
	)
{
	struct uvcs_driver_info *drv = (struct uvcs_driver_info *)udptr;

	if ((drv)
	&&	(drv->thread_ctrl.thread)) {
		kthread_stop(drv->thread_ctrl.thread);
		drv->thread_ctrl.thread = NULL;
	}
}

