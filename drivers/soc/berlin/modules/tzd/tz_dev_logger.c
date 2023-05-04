/*
 * tz/driver/tz_dev_logger.c
 *
 * A logging subsystem in trustzone driver
 *
 * Copyright (C) 2015 Marvell Ltd.
 * Contact: Jinghua Yu <yujh@marvell.com>
 *
 * The driver borrows from drivers/misc/logger.c which is:
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * Robert Love <rlove@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "tz_logger_dev: " fmt

#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#include "tz_driver_private.h"
#include "ree_sys_callback_logger.h"

/**
 * struct tzlogger_log - represents a specific log
 * @buffer:	The actual ring buffer
 * @misc:	The "misc" device representing the log
 * @wq:		The wait queue for @readers
 * @readers:	This log's readers
 * @mutex:	The mutex that protects the @buffer
 * @w_off:	The current write head offset
 * @head:	The head, or location that readers start reading at.
 * @size:	The size of the log
 *
 * This structure lives from module insertion until module removal, so it does
 * not need additional reference counting. The structure is protected by the
 * mutex 'mutex'.
 */
struct tzlogger_log {
	unsigned char		*buffer;
	struct miscdevice	misc;
	wait_queue_head_t	wq;
	struct list_head	readers;
	struct mutex		mutex;
	size_t			w_off;
	size_t			head;
	size_t			size;
};

static struct tzlogger_log *tzlogger;

/**
 * struct tzlogger_reader - a logging device open for reading
 * @log:	The associated log
 * @list:	The associated entry in @tzlogger_log's list
 * @r_off:	The current read head offset.
 *
 * This object lives from open to release, so we don't need additional
 * reference counting. The structure is protected by log->mutex.
 */
struct tzlogger_reader {
	struct tzlogger_log	*log;
	struct list_head	list;
	size_t			r_off;
};

/* logger_offset - returns index 'n' into the log via (optimized) modulus */
static size_t logger_offset(struct tzlogger_log *log, size_t n)
{
	return n & (log->size - 1);
}

/*
 * file_get_log - Given a file structure, return the associated log
 *
 * This isn't aesthetic. We have several goals:
 *
 *	1) Need to quickly obtain the associated log during an I/O operation
 *	2) Readers need to maintain state (tzlogger_reader)
 *	3) Writers need to be very fast (open() should be a near no-op)
 *
 * In the reader case, we can trivially go file->tzlogger_reader->tzlogger_log.
 * For a writer, we don't want to maintain a tzlogger_reader, so we just go
 * file->tzlogger_log. Thus what file->private_data points at depends on whether
 * or not the file was opened for reading. This function hides that dirtiness.
 */
static inline struct tzlogger_log *file_get_log(struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct tzlogger_reader *reader = file->private_data;
		return reader->log;
	} else
		return file->private_data;
}

/*
 * do_read_log_to_user - reads exactly 'count' bytes from 'log' into the
 * user-space buffer 'buf'. Returns 'count' on success.
 *
 * Caller must hold log->mutex.
 */
static ssize_t do_read_log_to_user(struct tzlogger_log *log,
				   struct tzlogger_reader *reader,
				   char __user *buf,
				   size_t count)
{
	size_t len;

	len = min(count, log->size - reader->r_off);
	if (copy_to_user(buf, log->buffer + reader->r_off, len))
		 return -EFAULT;
	/*
	 * Second, we read any remaining bytes, starting back at the head of
	 * the log.
	 */
	if (count != len) {
		if (copy_to_user(buf + len, log->buffer, count - len))
			return -EFAULT;
	}

	reader->r_off = logger_offset(log, (reader->r_off + count));

	return count;
}

/*
 * tzlogger_read - tzlogger's read() method
 *
 * Behavior:
 *	- O_NONBLOCK works
 *	- If there are no log messages to read, blocks until log is written to
 *
 * Will set errno to EINVAL if read
 * buffer is insufficient to hold next entry.
 */
static ssize_t tzlogger_read(struct file *file, char __user *buf,
			   size_t count, loff_t *pos)
{
	struct tzlogger_reader *reader = file->private_data;
	struct tzlogger_log *log = reader->log;
	ssize_t ret;
	DEFINE_WAIT(wait);

start:
	while (1) {
		mutex_lock(&log->mutex);

		prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

		ret = (log->w_off == reader->r_off);
		mutex_unlock(&log->mutex);
		if (!ret)
			break;

		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -EINTR;
			break;
		}

		schedule();
	}

	finish_wait(&log->wq, &wait);
	if (ret)
		return ret;

	mutex_lock(&log->mutex);

	/* is there still something to read or did we race? */
	if (unlikely(log->w_off == reader->r_off)) {
		mutex_unlock(&log->mutex);
		goto start;
	}

	if (log->w_off > reader->r_off)
		ret = log->w_off - reader->r_off;
	else
		ret = log->size - (reader->r_off - log->w_off);

	/* get exactly one entry from the log */
	ret = do_read_log_to_user(log, reader, buf, min_t(size_t, ret, count));

	mutex_unlock(&log->mutex);

	return ret;
}

/*
 * is_between - is a < c < b, accounting for wrapping of a, b, and c
 *    positions in the buffer
 *
 * That is, if a<b, check for c between a and b
 * and if a>b, check for c outside (not between) a and b
 *
 * |------- a xxxxxxxx b --------|
 *               c^
 *
 * |xxxxx b --------- a xxxxxxxxx|
 *    c^
 *  or                    c^
 */
static inline int is_between(size_t a, size_t b, size_t c)
{
	if (a < b) {
		/* is c between a and b? */
		if (a < c && c <= b)
			return 1;
	} else {
		/* is c outside of b through a? */
		if (c <= b || a < c)
			return 1;
	}

	return 0;
}

/*
 * fix_up_readers - walk the list of all readers and "fix up" any who were
 * lapped by the writer; also do the same for the default "start head".
 * We do this by "pulling forward" the readers and start head to the first
 * entry after the new write head.
 *
 * The caller needs to hold log->mutex.
 */
static void fix_up_readers(struct tzlogger_log *log, size_t len)
{
	size_t old = log->w_off;
	size_t new = logger_offset(log, old + len);
	struct tzlogger_reader *reader;

	if (is_between(old, new, log->head))
		log->head = (new + 1) & (log->size - 1);

	list_for_each_entry(reader, &log->readers, list)
		if (is_between(old, new, reader->r_off))
			reader->r_off = (new + 1) & (log->size - 1);
}

/*
 * do_write_log_user - writes 'len' bytes from the user-space buffer 'buf' to
 * tzlogger
 *
 * The caller needs to hold log->mutex.
 *
 * Returns 'count' on success, negative error code on failure.
 */
static ssize_t do_write_log_from_user(struct tzlogger_log *log,
				      const void __user *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	if (len && copy_from_user(log->buffer + log->w_off, buf, len))
		return -EFAULT;

	if (count != len)
		if (copy_from_user(log->buffer, buf + len, count - len))
			/*
			 * Note that by not updating w_off, this abandons the
			 * portion of the new entry that *was* successfully
			 * copied, just above.  This is intentional to avoid
			 * message corruption from missing fragments.
			 */
			return -EFAULT;

	log->w_off = logger_offset(log, log->w_off + count);

	return count;
}

/*
 * tzlogger_write - tzlogger' write method
 *
 * Returns 'count' on success, negative error code on failure.
 */
static ssize_t tzlogger_write(struct file *filp, const char __user *buf,
			    size_t count, loff_t *pos)
{
	struct tzlogger_log *log = file_get_log(filp);
	size_t orig;
	ssize_t nr;

	mutex_lock(&log->mutex);

	orig = log->w_off;

	/*
	 * Fix up any readers, pulling them forward to the first readable
	 * entry after (what will be) the new write offset. We do this now
	 * because if we partially fail, we can end up with clobbered log
	 * entries that encroach on readable buffer.
	 */
	fix_up_readers(log, count);

	/* write out this segment's payload */
	nr = do_write_log_from_user(log, buf, count);
	if (unlikely(nr < 0))
		log->w_off = orig;
	mutex_unlock(&log->mutex);

	/* wake up any blocked readers */
	wake_up_interruptible(&log->wq);

	return nr;
}

/*
 * get_tzlogger_from_minor - get tzlogger handler from minor
 */
static struct tzlogger_log *get_tzlogger_from_minor(int minor)
{
	BUG_ON(tzlogger->misc.minor != minor);
	return tzlogger;
}

/*
 * tzlogger_open - the tzlogger's open() file operation
 */
static int tzlogger_open(struct inode *inode, struct file *file)
{
	struct tzlogger_log *log;
	int ret;

	ret = nonseekable_open(inode, file);
	if (ret)
		return ret;

	log = get_tzlogger_from_minor(MINOR(inode->i_rdev));
	if (!log)
		return -ENODEV;

	if (file->f_mode & FMODE_READ) {
		struct tzlogger_reader *reader;

		reader = kmalloc(sizeof(struct tzlogger_reader), GFP_KERNEL);
		if (!reader)
			return -ENOMEM;

		reader->log = log;

		INIT_LIST_HEAD(&reader->list);

		mutex_lock(&log->mutex);
		reader->r_off = log->head;
		list_add_tail(&reader->list, &log->readers);
		mutex_unlock(&log->mutex);

		file->private_data = reader;
	} else
		file->private_data = log;

	return 0;
}

/*
 * tzlogger_release - the tzlogger's release file operation
 */
static int tzlogger_release(struct inode *ignored, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct tzlogger_reader *reader = file->private_data;
		struct tzlogger_log *log = reader->log;

		mutex_lock(&log->mutex);
		list_del(&reader->list);
		mutex_unlock(&log->mutex);

		kfree(reader);
	}

	return 0;
}

/*
 * tzlogger_poll - the tzlogger's poll file operation, for poll/select/epoll
 *
 * Note we always return POLLOUT, because you can always write() to the log.
 * Note also that, strictly speaking, a return value of POLLIN does not
 * guarantee that the log is readable without blocking, as there is a small
 * chance that the writer can lap the reader in the interim between poll()
 * returning and the read() request.
 */
static unsigned int tzlogger_poll(struct file *file, poll_table *wait)
{
	struct tzlogger_reader *reader;
	struct tzlogger_log *log;
	unsigned int ret = POLLOUT | POLLWRNORM;

	if (!(file->f_mode & FMODE_READ))
		return ret;

	reader = file->private_data;
	log = reader->log;

	poll_wait(file, &log->wq, wait);

	mutex_lock(&log->mutex);

	if (log->w_off != reader->r_off)
		ret |= POLLIN | POLLRDNORM;
	mutex_unlock(&log->mutex);

	return ret;
}

static const struct file_operations logger_fops = {
	.owner = THIS_MODULE,
	.read = tzlogger_read,
	.write = tzlogger_write,
	.poll = tzlogger_poll,
	.open = tzlogger_open,
	.release = tzlogger_release,
};

/*
 * Log size must must be a power of two.
 */
static struct tzlogger_log *create_log(char *log_name, int size)
{
	struct tzlogger_log *log;
	unsigned char *buffer;
	int ret;

	buffer = vmalloc(size);
	if (buffer == NULL)
		return NULL;

	log = kzalloc(sizeof(struct tzlogger_log), GFP_KERNEL);
	if (log == NULL)
		goto out_free_buffer;

	log->buffer = buffer;

	log->misc.minor = MISC_DYNAMIC_MINOR;
	log->misc.name = kstrdup(log_name, GFP_KERNEL);
	if (log->misc.name == NULL)
		goto out_free_log;

	log->misc.fops = &logger_fops;
	log->misc.parent = NULL;

	init_waitqueue_head(&log->wq);
	INIT_LIST_HEAD(&log->readers);
	mutex_init(&log->mutex);
	log->w_off = 0;
	log->head = 0;
	log->size = size;

	/* finally, initialize the misc device for this log */
	ret = misc_register(&log->misc);
	if (unlikely(ret)) {
		pr_err("failed to register misc device for log '%s'!\n",
				log->misc.name);
		goto out_free_log;
	}

	pr_info("created %luK log '%s'\n",
		(unsigned long) log->size >> 10, log->misc.name);

	return log;

out_free_log:
	kfree(log);

out_free_buffer:
	vfree(buffer);
	return NULL;
}

/*
 * tzlogger_write_log - writes 'len' bytes from the kernel-space buffer 'buf' to
 * tzlogger
 *
 * Return 'count' on success, negative error code on failure.
 */
static ssize_t tzlogger_write_log(const void *buf, size_t count)
{
	size_t len;

	mutex_lock(&tzlogger->mutex);

	/*
	 * Fix up any readers, pulling them forward to the first readable
	 * entry after (what will be) the new write offset. We do this now
	 * because if we partially fail, we can end up with clobbered log
	 * entries that encroach on readable buffer.
	 */
	fix_up_readers(tzlogger, count);

	/* write out this segment's payload */
	len = min(count, tzlogger->size - tzlogger->w_off);
	memcpy(tzlogger->buffer + tzlogger->w_off, buf, len);

	if (count != len){
		memcpy(tzlogger->buffer, buf + len, count - len);
	}

	tzlogger->w_off = logger_offset(tzlogger, tzlogger->w_off + count);

	mutex_unlock(&tzlogger->mutex);

	/* wake up any blocked readers */
	wake_up_interruptible(&tzlogger->wq);

	return count;
}

#define LOGGER_BUF_SIZE		512

static DEFINE_PER_CPU(char [LOGGER_BUF_SIZE], logger_buf);

/*
 * tzlogger_log_write - tzlogger's write() method for callback
 */
static ssize_t tzlogger_log_write(struct ree_logger_param *param)
{
	static const char *kernel_log_head[] = {
		"<0> (%s) ",
		"<1> (%s) ",
		"<2> (%s) ",
		"<3> (%s) ",
		"<4> (%s) ",
		"<5> (%s) ",
		"<6> (%s) ",
		"<7> (%s) ",
	};
	char *buf;
	size_t n;
	size_t count;
	unsigned long flags;

	local_irq_save(flags);
	buf  = *this_cpu_ptr(&logger_buf);
	n = snprintf(buf, LOGGER_BUF_SIZE,
			kernel_log_head[param->prio & 0x7], param->tag);
	if (likely(n < LOGGER_BUF_SIZE))
		strlcpy(buf+n, param->text, LOGGER_BUF_SIZE - n);
	buf[LOGGER_BUF_SIZE-1] = 0; // make sure null terminated
	local_irq_restore(flags);

	count = n + strlen(param->text);

	return tzlogger_write_log(buf, count);
}

static int ree_tzlogger_notify(struct notifier_block *self,
		unsigned long action, void *data)
{
	struct ree_logger_param *param = (struct ree_logger_param *) (data);

	tzlogger_log_write(param);

	return NOTIFY_DONE;
}

static struct notifier_block tzlogger_notifier = {
	.notifier_call = ree_tzlogger_notify,
};

int tzlogger_init(void)
{
	tzlogger = create_log("tzlogger", 128*1024);
	if (!tzlogger) {
		pr_err("Failed to create tzlogger\n");
		return -ENOMEM;
	}

	register_ree_logger_notifier(&tzlogger_notifier);

	return 0;
}

void tzlogger_exit(void)
{
	unregister_ree_logger_notifier(&tzlogger_notifier);

	if (tzlogger) {
		misc_deregister(&tzlogger->misc);
		vfree(tzlogger->buffer);
		kfree(tzlogger->misc.name);
		kfree(tzlogger);
	}
}
