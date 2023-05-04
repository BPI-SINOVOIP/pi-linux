/*
 * Sync File validation framework and debug infomation
 *
 * Copyright (C) 2012 Google, Inc.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SYNC_H
#define _LINUX_SYNC_H

#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/dma-fence.h>

#include <linux/sync_file.h>
#include <uapi/linux/sync_file.h>

/**
 * struct sync_timeline - sync object
 * @kref:		reference count on fence.
 * @name:		name of the sync_timeline. Useful for debugging
 * @lock:		lock protecting @pt_list and @value
 * @pt_list:		list of active (unsignaled/errored) sync_pts
 * @sync_timeline_list:	membership in global sync_timeline_list
 */
struct sync_timeline {
	struct kref		kref;
	char			name[32];

	/* protected by lock */
	u64			context;
	int			value;

	struct list_head	pt_list;
	spinlock_t		lock;

	struct list_head	sync_timeline_list;
};

static inline struct sync_timeline *dma_fence_parent(struct dma_fence *fence)
{
	return container_of(fence->lock, struct sync_timeline, lock);
}

/**
 * struct sync_pt - sync_pt object
 * @base: base fence object
 * @link: link on the sync timeline's list
 */
struct sync_pt {
	struct dma_fence base;
	struct list_head link;
};

#endif /* _LINUX_SYNC_H */
