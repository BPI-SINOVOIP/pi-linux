// SPDX-License-Identifier: GPL-2.0-only
/*
 * Sync File validation framework
 *
 * Copyright (C) 2012 Google, Inc.
 */

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sync_file.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include "berlin_sync.h"

/*
 * BERLIN SYNC validation framework
 *
 * A sync object driver that uses a 32bit counter to coordinate
 * synchronization.  Useful when there is no hardware primitive backing
 * the synchronization.
 *
 * To start the framework just open:
 *
 * /dev/berlin_sync
 *
 * That will create a sync timeline, all fences created under this timeline
 * file descriptor will belong to the this timeline.
 *
 * The 'berlin_sync' file can be opened many times as to create different
 * timelines.
 *
 * Fences can be created with BERLIN_SYNC_IOC_CREATE_FENCE ioctl with struct
 * berlin_sync_create_fence_data as parameter.
 *
 * To increment the timeline counter, BERLIN_SYNC_IOC_INC ioctl should be used
 * with the increment as u32. This will update the last signaled value
 * from the timeline and signal any fence that has a seqno smaller or equal
 * to it.
 *
 * struct berlin_sync_create_fence_data
 * @value:	the seqno to initialise the fence with
 * @name:	the name of the new sync point
 * @fence:	return the fd of the new sync_file with the created fence
 */
struct berlin_sync_create_fence_data {
	__u32	value;
	char	name[32];
	__s32	fence; /* fd of new fence */
};

#define BERLIN_SYNC_IOC_MAGIC	'W'

#define BERLIN_SYNC_IOC_CREATE_FENCE	_IOWR(BERLIN_SYNC_IOC_MAGIC, 0,\
		struct berlin_sync_create_fence_data)

#define BERLIN_SYNC_IOC_INC			_IOW(BERLIN_SYNC_IOC_MAGIC, 1, __u32)

static const struct dma_fence_ops timeline_fence_ops;

static inline struct sync_pt *dma_fence_to_sync_pt(struct dma_fence *fence)
{
	if (fence->ops != &timeline_fence_ops)
		return NULL;
	return container_of(fence, struct sync_pt, base);
}

/**
 * sync_timeline_create() - creates a sync object
 * @name:	sync_timeline name
 *
 * Creates a new sync_timeline. Returns the sync_timeline object or NULL in
 * case of error.
 */
static struct sync_timeline *sync_timeline_create(const char *name)
{
	struct sync_timeline *obj;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return NULL;

	kref_init(&obj->kref);
	obj->context = dma_fence_context_alloc(1);
	strlcpy(obj->name, name, sizeof(obj->name));

	INIT_LIST_HEAD(&obj->pt_list);
	spin_lock_init(&obj->lock);

	return obj;
}

static void sync_timeline_free(struct kref *kref)
{
	struct sync_timeline *obj =
		container_of(kref, struct sync_timeline, kref);

	kfree(obj);
}

static void sync_timeline_get(struct sync_timeline *obj)
{
	kref_get(&obj->kref);
}

static void sync_timeline_put(struct sync_timeline *obj)
{
	kref_put(&obj->kref, sync_timeline_free);
}

static const char *timeline_fence_get_driver_name(struct dma_fence *fence)
{
	return "berlin_sync";
}

static const char *timeline_fence_get_timeline_name(struct dma_fence *fence)
{
	struct sync_timeline *parent = dma_fence_parent(fence);

	return parent->name;
}

static void timeline_fence_release(struct dma_fence *fence)
{
	struct sync_pt *pt = dma_fence_to_sync_pt(fence);
	struct sync_timeline *parent = dma_fence_parent(fence);
	unsigned long flags;

	spin_lock_irqsave(fence->lock, flags);
	if (!list_empty(&pt->link)) {
		list_del(&pt->link);
	}
	spin_unlock_irqrestore(fence->lock, flags);

	sync_timeline_put(parent);
	dma_fence_free(fence);
}

static bool timeline_fence_signaled(struct dma_fence *fence)
{
	struct sync_timeline *parent = dma_fence_parent(fence);

	return !__dma_fence_is_later(fence->seqno, parent->value, fence->ops);
}

static bool timeline_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static void timeline_fence_value_str(struct dma_fence *fence,
				    char *str, int size)
{
	snprintf(str, size, "%lld", fence->seqno);
}

static void timeline_fence_timeline_value_str(struct dma_fence *fence,
					     char *str, int size)
{
	struct sync_timeline *parent = dma_fence_parent(fence);

	snprintf(str, size, "%d", parent->value);
}

static const struct dma_fence_ops timeline_fence_ops = {
	.get_driver_name = timeline_fence_get_driver_name,
	.get_timeline_name = timeline_fence_get_timeline_name,
	.enable_signaling = timeline_fence_enable_signaling,
	.signaled = timeline_fence_signaled,
	.release = timeline_fence_release,
	.fence_value_str = timeline_fence_value_str,
	.timeline_value_str = timeline_fence_timeline_value_str,
};

/**
 * sync_timeline_signal() - signal a status change on a sync_timeline
 * @obj:	sync_timeline to signal
 * @inc:	num to increment on timeline->value
 *
 * A sync implementation should call this any time one of it's fences
 * has signaled or has an error condition.
 */
static void sync_timeline_signal(struct sync_timeline *obj, unsigned int inc)
{
	struct sync_pt *pt, *next;

	spin_lock_irq(&obj->lock);

	obj->value += inc;

	list_for_each_entry_safe(pt, next, &obj->pt_list, link) {
		if (!timeline_fence_signaled(&pt->base))
			continue;

		list_del_init(&pt->link);
		/*
		 * A signal callback may release the last reference to this
		 * fence, causing it to be freed. That operation has to be
		 * last to avoid a use after free inside this loop, and must
		 * be after we remove the fence from the timeline in order to
		 * prevent deadlocking on timeline->lock inside
		 * timeline_fence_release().
		 */
		dma_fence_signal_locked(&pt->base);
	}

	spin_unlock_irq(&obj->lock);
}

/**
 * sync_pt_create() - creates a sync pt
 * @obj:	parent sync_timeline
 * @value:	value of the fence
 *
 * Creates a new sync_pt (fence) as a child of @parent.  @size bytes will be
 * allocated allowing for implementation specific data to be kept after
 * the generic sync_timeline struct. Returns the sync_pt object or
 * NULL in case of error.
 */
static struct sync_pt *sync_pt_create(struct sync_timeline *obj,
				      unsigned int value)
{
	struct sync_pt *pt;

	pt = kzalloc(sizeof(*pt), GFP_KERNEL);
	if (!pt)
		return NULL;

	sync_timeline_get(obj);
	dma_fence_init(&pt->base, &timeline_fence_ops, &obj->lock,
		       obj->context, value);
	INIT_LIST_HEAD(&pt->link);

	spin_lock_irq(&obj->lock);
	if (!dma_fence_is_signaled_locked(&pt->base))
		list_add_tail(&pt->link, &obj->pt_list);
	spin_unlock_irq(&obj->lock);

	return pt;
}

/*
 * *WARNING*
 *
 * improper use of this can result in deadlocking kernel drivers from userspace.
 */

/* opening berlin_sync create a new sync obj */
static int berlin_sync_open(struct inode *inode, struct file *file)
{
	struct sync_timeline *obj;
	char task_comm[TASK_COMM_LEN];

	get_task_comm(task_comm, current);

	obj = sync_timeline_create(task_comm);
	if (!obj)
		return -ENOMEM;

	file->private_data = obj;

	return 0;
}

static int berlin_sync_release(struct inode *inode, struct file *file)
{
	struct sync_timeline *obj = file->private_data;
	struct sync_pt *pt, *next;

	spin_lock_irq(&obj->lock);

	list_for_each_entry_safe(pt, next, &obj->pt_list, link) {
		dma_fence_set_error(&pt->base, -ENOENT);
		dma_fence_signal_locked(&pt->base);
	}

	spin_unlock_irq(&obj->lock);

	sync_timeline_put(obj);
	return 0;
}

static long berlin_sync_ioctl_create_fence(struct sync_timeline *obj,
				       unsigned long arg)
{
	int fd = get_unused_fd_flags(O_CLOEXEC);
	int err;
	struct sync_pt *pt;
	struct sync_file *sync_file;
	struct berlin_sync_create_fence_data data;

	if (fd < 0)
		return fd;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		err = -EFAULT;
		goto err;
	}

	pt = sync_pt_create(obj, data.value);
	if (!pt) {
		err = -ENOMEM;
		goto err;
	}

	sync_file = sync_file_create(&pt->base);
	dma_fence_put(&pt->base);
	if (!sync_file) {
		err = -ENOMEM;
		goto err;
	}

	data.fence = fd;
	if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
		fput(sync_file->file);
		err = -EFAULT;
		goto err;
	}

	fd_install(fd, sync_file->file);

	return 0;

err:
	put_unused_fd(fd);
	return err;
}

static long berlin_sync_ioctl_inc(struct sync_timeline *obj, unsigned long arg)
{
	u32 value;

	if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
		return -EFAULT;

	while (value > INT_MAX)  {
		sync_timeline_signal(obj, INT_MAX);
		value -= INT_MAX;
	}

	sync_timeline_signal(obj, value);

	return 0;
}

static long berlin_sync_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	struct sync_timeline *obj = file->private_data;

	switch (cmd) {
	case BERLIN_SYNC_IOC_CREATE_FENCE:
		return berlin_sync_ioctl_create_fence(obj, arg);

	case BERLIN_SYNC_IOC_INC:
		return berlin_sync_ioctl_inc(obj, arg);

	default:
		return -ENOTTY;
	}
}

static const struct file_operations berlin_sync_fops = {
	.open		= berlin_sync_open,
	.release	= berlin_sync_release,
	.unlocked_ioctl	= berlin_sync_ioctl,
	.compat_ioctl	= berlin_sync_ioctl,
	.owner		= THIS_MODULE,
};

static struct miscdevice berlin_sync_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "berlin_sync",
	.fops	= &berlin_sync_fops,
};

static int __init berlin_sync_device_init(void)
{
	return misc_register(&berlin_sync_dev);
}

static void __exit berlin_sync_device_remove(void)
{
	misc_deregister(&berlin_sync_dev);
}

module_init(berlin_sync_device_init);
module_exit(berlin_sync_device_remove);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("berlin vsync module");
