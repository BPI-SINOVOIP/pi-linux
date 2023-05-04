// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Jie Xi <Jie.Xi@synaptics.com>
 *
 */

#ifndef __SYNA_SHM_H__
#define __SYNA_SHM_H__

#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/debugfs.h>
#include <linux/rbtree.h>
#include <linux/dma-buf.h>
#include "uapi/shm.h"

#define SHM_DEVICE_NAME         "amp-shm"

struct berlin_gid_node {
	struct rb_node node;
	unsigned int gid;
	unsigned int ref_cnt;
	pid_t threadid;
	char threadname[TASK_COMM_LEN];
	pid_t pid;
	char task_comm[TASK_COMM_LEN];
	void *data;
};

struct shm_device {
	struct device *dev;
	struct miscdevice mdev;
	struct idr idr;
	struct rw_semaphore idr_lock;
	struct rb_root clients;
	struct rw_semaphore client_lock;
	struct dentry *debug_root;
	struct dentry *gid_debug_root;
};

struct shm_client {
	struct rb_node node;
	struct shm_device *dev;
	struct mutex lock;
	const char *name;
	struct task_struct *task;
	pid_t pid;
	struct rb_root gids;
};

struct berlin_buffer_node {
	struct kref ref;
	struct shm_device *dev;
	struct shm_client *owner;
	struct dma_buf *dmabuf;
	int gid;
	unsigned int buf_flags;
	size_t alloc_len;
};

union shm_ioctl_arg {
	struct shm_allocation_data allocation;
	struct shm_fd_data fd;
	struct shm_misc_data misc;
	struct shm_sync_data sync;
	int id;
};

struct shm_client *shm_client_create(const char *name);

void shm_client_destroy(struct shm_client *client);

struct dma_buf *shm_get_dma_buf(struct shm_client *client, int gid);

int shm_put(struct shm_client *client, int gid);

#endif
