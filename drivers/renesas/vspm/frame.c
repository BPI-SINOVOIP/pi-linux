/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
 *
 * License        Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * GPLv2:
 * If you wish to use this file under the terms of GPL, following terms are
 * effective.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */ /*************************************************************************/

#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "frame.h"

#include "vspm_public.h"
#include "vspm_ip_ctrl.h"
#include "vspm_main.h"
#include "vspm_log.h"

/* task message information structure */
struct fw_msg_info {
	struct list_head list;
	short func_id;
	short msg_id;
	struct {
		long ercd;
		struct completion comp;
	} reply;
	size_t size;
	void *para;
};

/* task information structure */
struct fw_task_info {
	struct list_head list;
	unsigned short tid;
	struct {
		struct list_head list;
		spinlock_t lock;	/* protects the task message list */
		wait_queue_head_t wait;
	} msg;
	struct completion suspend;
};

/* task control structure */
struct fw_task_ctl {
	struct list_head list;
	spinlock_t lock;	/* protects the task control list */
};

/* task control table*/
static struct fw_task_ctl task_ctl;

/******************************************************************************
 * Function:		get_task_info
 * Description:	get a task information.
 * Returns:		Pointer to a task information
 ******************************************************************************/
static struct fw_task_info *get_task_info(unsigned short tid)
{
	struct fw_task_info *task_info = NULL;
	unsigned long lock_flag;

	spin_lock_irqsave(&task_ctl.lock, lock_flag);
	list_for_each_entry(task_info, &task_ctl.list, list) {
		if (task_info->tid == tid)
			break;
	}
	spin_unlock_irqrestore(&task_ctl.lock, lock_flag);

	if (task_info) {
		if ((task_info == (struct fw_task_info *)&task_ctl.list) ||
		    task_info->tid != tid) {
			task_info = NULL;
		}
	}

	return task_info;
}

/******************************************************************************
 * Function:		send_message
 * Description:	send a message.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
static int send_message(
	unsigned short tid,
	short msg_id,
	short func_id,
	size_t size,
	void *para,
	struct fw_msg_info **p_snd_msg)
{
	struct fw_task_info *task_info;
	struct fw_msg_info *snd_msg = NULL;
	void *msg_para = NULL;

	unsigned long lock_flag;

	/* Search a task-information */
	task_info = get_task_info(tid);
	if (!task_info) {
		EPRINT("Didn't found task information!! tid=%d\n", tid);
		return FW_NG;
	}

	/* Allocate the send-message area */
	snd_msg = kzalloc(sizeof(*snd_msg), GFP_ATOMIC);
	if (!snd_msg) {
		EPRINT("failed to allocate memory!!\n");
		return FW_NG;
	}

	/* Set a send message information */
	snd_msg->func_id = func_id;
	snd_msg->msg_id = msg_id;
	init_completion(&snd_msg->reply.comp);
	snd_msg->size = size;
	if ((size) && (para)) {
		/* Allocate the parameter area */
		msg_para = kzalloc(size, GFP_ATOMIC);
		if (!msg_para) {
			EPRINT("failed to allocate memory!!\n");
			kfree(snd_msg);
			return FW_NG;
		}

		/* Copy the parameter */
		memcpy(msg_para, para, size);
	}
	snd_msg->para = msg_para;

	/* Send a message */
	spin_lock_irqsave(&task_info->msg.lock, lock_flag);
	if (func_id == FUNC_TASK_SUSPEND || func_id == FUNC_TASK_RESUME)
		list_add(&snd_msg->list, &task_info->msg.list);
	else
		list_add_tail(&snd_msg->list, &task_info->msg.list);
	spin_unlock_irqrestore(&task_info->msg.lock, lock_flag);

	/* Resume a framework */
	if (func_id == FUNC_TASK_RESUME)
		complete(&task_info->suspend);

	wake_up(&task_info->msg.wait);

	if (p_snd_msg)
		*p_snd_msg = snd_msg;

	return FW_OK;
}

/******************************************************************************
 * Function:		receive_message
 * Description:	wait to receive a message.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
static int receive_message(
	struct fw_task_info *task_info, struct fw_msg_info **p_rcv_msg)
{
	struct fw_msg_info *rcv_msg;
	unsigned long lock_flag;

	/* Wait to receive a message */
	if (wait_event_interruptible(
		task_info->msg.wait, !list_empty(&task_info->msg.list)))
		return FW_NG;

	/* Get a message */
	spin_lock_irqsave(&task_info->msg.lock, lock_flag);
	rcv_msg = list_first_entry(
		&task_info->msg.list, struct fw_msg_info, list);
	list_del(&rcv_msg->list);
	spin_unlock_irqrestore(&task_info->msg.lock, lock_flag);

	*p_rcv_msg = rcv_msg;

	return FW_OK;
}

/******************************************************************************
 * Function:		get_function
 * Description:	get a function address.
 * Returns:		Pointer to function address.
 ******************************************************************************/
static void *get_function(
	struct fw_func_tbl *func_tbl, short msg_id, short func_id)
{
	int index;
	void *func = NULL;

	/* Get index from func_id */
	index = (func_id & 0x00FF) - 1;

	/* Check the ID */
	if (func_tbl[index].msg_id == msg_id &&
	    func_tbl[index].func_id == func_id) {
		/* Get a function address */
		func = (void *)func_tbl[index].func;
	}

	return func;
}

/******************************************************************************
 * Function:		fw_initialize
 * Description:	the framework initialization.
 * Returns:		void
 ******************************************************************************/
void fw_initialize(void)
{
	INIT_LIST_HEAD(&task_ctl.list);
	spin_lock_init(&task_ctl.lock);
}

/******************************************************************************
 * Function:		fw_task_register
 * Description:	register a task information.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
int fw_task_register(unsigned short tid)
{
	struct fw_task_info *task_info;
	unsigned long lock_flag;

	/* Search a task-information */
	task_info = get_task_info(tid);
	if (task_info) {
		APRINT("Task is already registered!! tid=%d\n", tid);
		task_info = NULL;
		return FW_NG;
	}

	/* Allocate the task-information area */
	task_info = kzalloc(sizeof(*task_info), GFP_KERNEL);
	if (!task_info) {
		EPRINT("failed to allocate memory of task information!!\n");
		return FW_NG;
	}

	/* Set a task-information */
	task_info->tid = tid;

	/* Initialization for a receive messages */
	INIT_LIST_HEAD(&task_info->msg.list);
	spin_lock_init(&task_info->msg.lock);
	init_waitqueue_head(&task_info->msg.wait);

	/* Initialization for a suspend control */
	init_completion(&task_info->suspend);

	/* Register a task-information */
	spin_lock_irqsave(&task_ctl.lock, lock_flag);
	list_add_tail(&task_info->list, &task_ctl.list);
	spin_unlock_irqrestore(&task_ctl.lock, lock_flag);

	return FW_OK;
}

/******************************************************************************
 * Function:		fw_task_unregister
 * Description:	unregister a task information.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
int fw_task_unregister(unsigned short tid)
{
	struct fw_task_info *task_info;
	unsigned long lock_flag;

	/* Search a task-information */
	task_info = get_task_info(tid);
	if (!task_info) {
		APRINT("Task is not registered!! tid=%d\n", tid);
		return FW_NG;
	}

	/* Delete the task-information */
	spin_lock_irqsave(&task_ctl.lock, lock_flag);
	list_del(&task_info->list);
	kfree(task_info);
	spin_unlock_irqrestore(&task_ctl.lock, lock_flag);

	return FW_OK;
}

/******************************************************************************
 * Function:		fw_execute
 * Description:	start the processing framework.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
int fw_execute(unsigned short tid, struct fw_func_tbl *func_tbl)
{
	struct fw_task_info *task_info;
	struct fw_msg_info *rcv_msg;
	long (*func)(void *mesp, void *para);
	long ercd;
	short func_id;
	struct fw_msg fw_msg;

	/* Search a task-information */
	task_info = get_task_info(tid);
	if (!task_info) {
		APRINT("Task is not registered %d\n", tid);
		return FW_NG;
	}

	while (1) {
		/* Wait to receive a message */
		if (receive_message(task_info, &rcv_msg))
			continue;

		/* Check a message ID */
		if (rcv_msg->msg_id != MSG_EVENT &&
		    rcv_msg->msg_id != MSG_FUNCTION) {
			APRINT("Invalid message ID %d\n", rcv_msg->msg_id);
			continue;
		}

		/* Get a function address */
		func = get_function(
			func_tbl, rcv_msg->msg_id, rcv_msg->func_id);
		if (func) {
			/* execute the function */
			fw_msg.msg_id  = rcv_msg->msg_id;
			fw_msg.func_id = rcv_msg->func_id;
			fw_msg.size    = rcv_msg->size;
			fw_msg.para    = rcv_msg->para;

			ercd = func((void *)&fw_msg, rcv_msg->para);
		} else {
			APRINT("Invalid func!! tid=%d, msg_id=%d, func_id=%d\n",
			       tid, rcv_msg->msg_id, rcv_msg->func_id);
			ercd = 0;
		}

		func_id = rcv_msg->func_id;

		if (rcv_msg->msg_id == MSG_EVENT) {
			/* Release the received message */
			kfree(rcv_msg->para);
			kfree(rcv_msg);
		} else {	/* rcv_msg->msg_id == MSG_FUNCTION */
			/* Reply to the sender */
			rcv_msg->reply.ercd = ercd;

			complete(&rcv_msg->reply.comp);
		}

		if (func_id == FUNC_TASK_QUIT)
			break;
		else if (func_id == FUNC_TASK_SUSPEND)
			wait_for_completion(&task_info->suspend);
	}

	/* Do exit kthread */
	kthread_complete_and_exit(&rcv_msg->reply.comp, FW_OK);
	return FW_OK;
}

/******************************************************************************
 * Function:		fw_send_event
 * Description:	send a event message.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
long fw_send_event(unsigned short tid, short func_id, size_t size, void *para)
{
	/* Send a message */
	if (send_message(tid, MSG_EVENT, func_id, size, para, NULL))
		return FW_NG;

	return FW_OK;
}

/******************************************************************************
 * Function:		fw_send_function
 * Description:	send a function message.
 * Returns:		FW_OK/FW_NG
 ******************************************************************************/
long fw_send_function(
	unsigned short tid, short func_id, size_t size, void *para)
{
	struct fw_msg_info *snd_msg = NULL;
	long ercd;

	/* Send a message */
	if (send_message(tid, MSG_FUNCTION, func_id, size, para, &snd_msg))
		return FW_NG;

	/* Wait to receive a reply-message */
	wait_for_completion(&snd_msg->reply.comp);

	/* Set a reply-return-code */
	ercd = snd_msg->reply.ercd;

	/* Release the received reply-message */
	kfree(snd_msg->para);
	kfree(snd_msg);

	return ercd;
}

