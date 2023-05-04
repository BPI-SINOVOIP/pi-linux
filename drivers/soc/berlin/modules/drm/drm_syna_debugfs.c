// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#include <linux/debugfs.h>
#include "drm_syna_drv.h"
#include "syna_vpp.h"

#define SYNA_DEBUGFS_DISPLAY_HW_VSYNC "hw_vsync"
#define SYNA_DEBUGFS_DISPLAY_ROTATE   "display_rotate"

static long use_hw_vsync = 1;

#ifdef CONFIG_DEBUG_FS
static int display_rotate_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t display_rotate_read(struct file *file,
				   char __user *user_buffer,
				   size_t count, loff_t *position_ptr)
{
	char buffer[4];
	int ret;

	if (device_rotate == 0) {
		sprintf(buffer, "0");
		count = 2;
	} else if (device_rotate == 90) {
		sprintf(buffer, "90");
		count = 3;
	} else if (device_rotate == 180) {
		sprintf(buffer, "180");
		count = 4;
	} else if (device_rotate == 270) {
		sprintf(buffer, "270");
		count = 4;
	} else {
		sprintf(buffer, "-1");
		count = 3;
	}

	ret =
	    simple_read_from_buffer(user_buffer, count, position_ptr, buffer,
				    strlen(buffer));

	return ret;
}

static ssize_t display_rotate_write(struct file *file,
				    const char __user *user_buffer,
				    size_t count, loff_t *position)
{
	int ret;
	char val[5];
	int err;
	unsigned long p = *position;

	memset(val, 0, 5);

	if (p >= 5)
		return count ? -ENXIO : 0;

	if (count > 5 - p)
		count = 5 - p;

	if (copy_from_user(val, user_buffer, count)) {
		ret = -EFAULT;
	} else {
		*position += count;
		ret = count;
	}

	err = kstrtol(val, 10, &device_rotate);

	if (err != 0)
		DRM_WARN("\nSet a unknown rotate string\n");

	DRM_INFO("\nRotate change to %ld\n", device_rotate);

	return ret;
}

static int hw_vsync_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t hw_vsync_read(struct file *file,
			     char __user *user_buffer,
			     size_t count, loff_t *position_ptr)
{
	char buffer[4];
	int ret;

	if (use_hw_vsync == 0) {
		sprintf(buffer, "0");
		count = 2;
	} else if (use_hw_vsync == 1) {
		sprintf(buffer, "1");
		count = 3;
	} else if (use_hw_vsync == 2) {
		sprintf(buffer, "2");
		count = 3;
	} else {
		sprintf(buffer, "-1");
		count = 3;
	}

	ret =
	    simple_read_from_buffer(user_buffer, count, position_ptr, buffer,
				    strlen(buffer));

	return ret;
}

static ssize_t hw_vsync_write(struct file *file,
			      const char __user *user_buffer,
			      size_t count, loff_t *position)
{
	int ret;
	char val[5];
	int err;
	unsigned long p = *position;

	memset(val, 0, 5);

	if (p >= 5)
		return count ? -ENXIO : 0;

	if (count > 5 - p)
		count = 5 - p;

	if (copy_from_user(val, user_buffer, count)) {
		ret = -EFAULT;
	} else {
		*position += count;
		ret = count;
	}

	err = kstrtol(val, 10, &use_hw_vsync);

	if (err != 0)
		DRM_WARN("\nSet a unknown hw vsync string\n");

	DRM_INFO("\n Change hw_vsync to %ld\n", use_hw_vsync);

	return ret;
}

static const struct file_operations syna_display_rotate_fops = {
	.owner = THIS_MODULE,
	.open = display_rotate_open,
	.read = display_rotate_read,
	.write = display_rotate_write,
	.llseek = default_llseek,
};

static const struct file_operations syna_hw_vsync_fops = {
	.owner = THIS_MODULE,
	.open = hw_vsync_open,
	.read = hw_vsync_read,
	.write = hw_vsync_write,
	.llseek = default_llseek,
};

static int syna_debugfs_create(struct drm_minor *minor, const char *name,
			       umode_t mode, const struct file_operations *fops)
{
	struct drm_info_node *node;

	/*
	 * We can't get access to our driver private data when this function is
	 * called so we fake up a node so that we can clean up entries later on.
	 */
	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	node->dent = debugfs_create_file(name, mode, minor->debugfs_root,
					 minor->dev, fops);
	if (!node->dent) {
		kfree(node);
		return -ENOMEM;
	}

	node->minor = minor;
	node->info_ent = (void *)fops;

	mutex_lock(&minor->debugfs_lock);
	list_add(&node->list, &minor->debugfs_list);
	mutex_unlock(&minor->debugfs_lock);

	return 0;
}

int syna_debugfs_init(struct drm_minor *minor)
{
	int err;

	err = syna_debugfs_create(minor, SYNA_DEBUGFS_DISPLAY_ROTATE,
				  0100644, &syna_display_rotate_fops);
	if (err) {
		DRM_INFO("failed to create '%s' debugfs entry\n",
			 SYNA_DEBUGFS_DISPLAY_ROTATE);
	}

	err = syna_debugfs_create(minor, SYNA_DEBUGFS_DISPLAY_HW_VSYNC,
				  0100644, &syna_hw_vsync_fops);
	if (err) {
		DRM_INFO("failed to create '%s' debugfs entry\n",
			 SYNA_DEBUGFS_DISPLAY_HW_VSYNC);
	}

	return err;
}
#endif

long syna_debugfs_get_hw_vsync_val(void)
{
	return use_hw_vsync;
}
