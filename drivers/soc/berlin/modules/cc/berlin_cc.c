// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Dejin Zheng <Dejin.Zheng@synaptics.com>
 *
 */

#define pr_fmt(fmt) "[berlin_cc kernel driver] " fmt

#include <linux/compat.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/sched/task.h>
#include <linux/miscdevice.h>
#include "uapi/berlin_cc.h"

#define BERLIN_CC_DEVICE_NAME "amp-cc"

struct berlin_gid_node {
	unsigned int gid;
	pid_t threadid;
	char threadname[TASK_COMM_LEN];
	pid_t pid;
	char task_comm[TASK_COMM_LEN];
	void *data;
	struct list_head list;
};

struct berlin_cc_device {
	char *dev_name;
	struct miscdevice dev;
	struct rb_root clients;
	struct rw_semaphore client_lock;
	struct dentry *debug_root;
	struct dentry *clients_debug_root;
};

struct berlin_cc_client {
	struct rb_node node;
	struct berlin_cc_device *dev;
	struct mutex lock;
	const char *name;
	char *display_name;
	int display_serial;
	struct task_struct *task;
	pid_t pid;
	struct dentry *debug_root;
	struct list_head cc_list;
};

struct berlin_gid_root {
	struct rw_semaphore rwsem;
	struct idr idr;
	struct dentry *debug_root;
};

static struct berlin_gid_root berlin_cc;
static struct berlin_cc_device cc_dev = {
	.dev_name = BERLIN_CC_DEVICE_NAME,
};

static int debug_cc_show(struct seq_file *s, void *unused)
{
	struct berlin_gid_node *tmp;
	struct berlin_gid_root *debug_gid = s->private;
	int i = 0;
	int id = 0;

	seq_puts(s, "|No  |  sid   |  pid   |  process_name  |   tid  |   thread_name  |\n");
	seq_puts(s, "-------------------------------------------------------------------\n");

	down_read(&debug_gid->rwsem);
	idr_for_each_entry(&debug_gid->idr, tmp, id) {
		seq_printf(s, "|%4d|%08x|%8d|%16s|%8d|%16s|\n", i,
			   tmp->gid, tmp->pid, tmp->task_comm,
			   tmp->threadid, tmp->threadname);
		i++;
	}
	up_read(&debug_gid->rwsem);
	seq_puts(s, "-------------------------------------------------------------------\n");

	return 0;
}

static int debug_cc_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_cc_show, inode->i_private);
}

static const struct file_operations debug_cc_fops = {
	.open = debug_cc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int berlin_cc_reg(struct berlin_cc_client *client,
			 struct berlin_cc_info *info)
{
	struct berlin_cc_info *tmp;
	struct berlin_gid_node *node;
	unsigned int new_gid = 0;

	pr_debug("%s start sid[%08x]\n", __func__, info->m_ServiceID);
	if (!info)
		return -EBADF;

	tmp = kzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node) {
		kfree(tmp);
		return -ENOMEM;
	}

	node->data = (void *)(tmp);
	node->pid = task_tgid_vnr(current);
	get_task_comm(node->task_comm, current->group_leader);
	node->threadid = task_pid_vnr(current);
	strncpy(node->threadname, current->comm, TASK_COMM_LEN);

	down_write(&berlin_cc.rwsem);
	if (info->m_ServiceID == BERLIN_CC_DYNAMIC_ID) {
		new_gid = idr_alloc(&berlin_cc.idr, node,
				    1, 0, GFP_KERNEL);
		if (new_gid < 0) {
			up_write(&berlin_cc.rwsem);
			pr_err("%s alloc gid fail: %d\n",
				 __func__, new_gid);
			kfree(tmp);
			kfree(node);
			return new_gid;
		}

		info->m_ServiceID = BERLIN_CC_START_ID + new_gid;
	}

	memcpy(tmp, info, sizeof(struct berlin_cc_info));
	node->gid = info->m_ServiceID;

	list_add(&node->list, &client->cc_list);
	up_write(&berlin_cc.rwsem);

	pr_debug("%s end sid[%08x]\n", __func__, info->m_ServiceID);

	return 0;
}

static int berlin_cc_free(struct berlin_cc_info *info)
{
	struct berlin_gid_node *node;

	pr_debug("%s sid[%08x]\n", __func__, info->m_ServiceID);

	down_write(&berlin_cc.rwsem);
	node = idr_find(&berlin_cc.idr,
			info->m_ServiceID - BERLIN_CC_START_ID);
	if (!node) {
		up_write(&berlin_cc.rwsem);
		pr_err("%s: can't found gid[%u]\n",
		       __func__, info->m_ServiceID);
		return -1;
	}

	list_del(&node->list);
	idr_remove(&berlin_cc.idr,
		   info->m_ServiceID - BERLIN_CC_START_ID);
	up_write(&berlin_cc.rwsem);

	kfree(node->data);
	kfree(node);
	return 0;
}

static int berlin_cc_inquiry(struct berlin_cc_info *info)
{
	struct berlin_gid_node *node;

	pr_debug("%s sid[%08x]\n", __func__, info->m_ServiceID);

	down_read(&berlin_cc.rwsem);
	node = idr_find(&berlin_cc.idr,
			info->m_ServiceID - BERLIN_CC_START_ID);
	if (!node) {
		up_read(&berlin_cc.rwsem);
		if (info->m_ServiceID > BERLIN_CC_DYNAMIC_ID)
			pr_err("%s: can't found sid[%08x]\n",
					__func__, info->m_ServiceID);
		return -1;
	}

	memcpy(info, node->data, sizeof(struct berlin_cc_info));
	up_read(&berlin_cc.rwsem);

	return 0;
}

static int berlin_cc_update(struct berlin_cc_info *info)
{
	struct berlin_gid_node *node;

	pr_debug("%s sid[%08x]\n", __func__, info->m_ServiceID);

	if (info->m_ServiceID == BERLIN_CC_DYNAMIC_ID) {
		pr_err("%s can't update for sid[%08x]\n",
		       __func__, info->m_ServiceID);
	}

	down_write(&berlin_cc.rwsem);
	node = idr_find(&berlin_cc.idr,
			info->m_ServiceID - BERLIN_CC_START_ID);
	if (!node) {
		up_write(&berlin_cc.rwsem);
		pr_err("%s: can't found gid[%08x]\n", __func__,
			info->m_ServiceID);
		return -1;
	}
	memcpy(node->data, info, sizeof(struct berlin_cc_info));
	up_write(&berlin_cc.rwsem);

	return 0;
}

static int berlin_cc_cmd(struct berlin_cc_client *client,
			 struct berlin_cc_data *data)
{
	int ret = -1;

	switch (data->cmd) {
	case CC_REG:
		ret = berlin_cc_reg(client, &data->cc);
		break;
	case CC_FREE:
		ret = berlin_cc_free(&data->cc);
		break;
	case CC_INQUIRY:
		ret = berlin_cc_inquiry(&data->cc);
		break;
	case CC_UPDATE:
		ret = berlin_cc_update(&data->cc);
		break;
	default:
		pr_err("%s: Unknown cc command %d\n",
		       __func__, data->cmd);
		return -EINVAL;
	}

	return ret;
}

static int berlin_cc_release_by_taskid(struct berlin_cc_client *client)
{
	struct list_head *node, *next;
	struct berlin_gid_node *tmp;
	pid_t pid = task_tgid_vnr(current);

	pr_debug("%s pid[%d] threadname[%s] threadid[%d]\n",
		     __func__, pid,  current->comm, task_pid_vnr(current));

	down_write(&berlin_cc.rwsem);
	list_for_each_safe(node, next, &client->cc_list) {
		tmp = list_entry(node, struct berlin_gid_node, list);
		if (tmp->pid == pid) {
			pr_debug("%s sid[%08x]\n",
					__func__, tmp->gid);
			list_del(&tmp->list);
			idr_remove(&berlin_cc.idr,
				   tmp->gid - BERLIN_CC_START_ID);
			kfree(tmp->data);
			kfree(tmp);
		}
	}
	up_write(&berlin_cc.rwsem);

	return 0;
}

static int berlin_cc_get_client_serial(const struct rb_root *root,
				 const unsigned char *name)
{
	int serial = -1;
	struct rb_node *node;

	for (node = rb_first(root); node; node = rb_next(node)) {
		struct berlin_cc_client *client = rb_entry(node,
						  struct berlin_cc_client,
						  node);

		if (strcmp(client->name, name))
			continue;
		serial = max(serial, client->display_serial);
	}
	return serial + 1;
}

static struct berlin_cc_client *
berlin_cc_client_create(struct berlin_cc_device *dev, const char *name)
{
	struct berlin_cc_client *client;
	struct task_struct *task;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct berlin_cc_client *entry;
	pid_t pid;

	if (!name) {
		pr_err("%s: Name cannot be null\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);
	pid = task_pid_nr(current->group_leader);
	if (current->group_leader->flags & PF_KTHREAD) {
		put_task_struct(current->group_leader);
		task = NULL;
	} else {
		task = current->group_leader;
	}
	task_unlock(current->group_leader);

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		goto err_put_task_struct;

	client->dev = dev;
	mutex_init(&client->lock);
	client->task = task;
	client->pid = pid;
	client->name = kstrdup(name, GFP_KERNEL);
	if (!client->name)
		goto err_free_client;

	INIT_LIST_HEAD(&client->cc_list);

	down_write(&dev->client_lock);
	client->display_serial = berlin_cc_get_client_serial(&dev->clients,
							     name);
	client->display_name = kasprintf(GFP_KERNEL, "%s-%d",
					 name, client->display_serial);
	if (!client->display_name) {
		up_write(&dev->client_lock);
		goto err_free_client_name;
	}
	p = &dev->clients.rb_node;
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct berlin_cc_client, node);

		if (client < entry)
			p = &(*p)->rb_left;
		else if (client > entry)
			p = &(*p)->rb_right;
	}
	rb_link_node(&client->node, parent, p);
	rb_insert_color(&client->node, &dev->clients);

	client->debug_root = debugfs_create_file(client->display_name, 0664,
						 dev->clients_debug_root,
						 client, NULL);
	up_write(&dev->client_lock);

	return client;

err_free_client_name:
	kfree(client->name);
err_free_client:
	kfree(client);
err_put_task_struct:
	if (task)
		put_task_struct(current->group_leader);
	return ERR_PTR(-ENOMEM);
}

static void berlin_cc_client_destroy(struct berlin_cc_client *client)
{
	struct berlin_cc_device *dev = client->dev;

	down_write(&dev->client_lock);
	if (client->task)
		put_task_struct(client->task);
	rb_erase(&client->node, &dev->clients);
	debugfs_remove_recursive(client->debug_root);
	up_write(&dev->client_lock);

	kfree(client->display_name);
	kfree(client->name);
	kfree(client);
}

static int berlin_cc_release(struct inode *inode, struct file *file)
{
	struct berlin_cc_client *client = file->private_data;

	berlin_cc_release_by_taskid(client);
	berlin_cc_client_destroy(client);
	return 0;
}

static int berlin_cc_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct berlin_cc_device *dev = container_of(miscdev,
						    struct berlin_cc_device,
						    dev);
	struct berlin_cc_client *client;
	char debug_name[64];

	snprintf(debug_name, 64, "%u", task_pid_nr(current->group_leader));
	client = berlin_cc_client_create(dev, debug_name);
	if (IS_ERR(client))
		return PTR_ERR(client);
	file->private_data = client;

	return 0;
}

static long berlin_cc_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	int ret = 0;
	unsigned int dir;
	struct berlin_cc_client *client = filp->private_data;
	struct berlin_cc_data data;

	dir = _IOC_DIR(cmd);
	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (!(dir & _IOC_WRITE)) {
		memset(&data, 0, sizeof(data));
	} else {
		if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;
	}

	switch (cmd) {
	case BERLIN_IOC_CC:
		ret = berlin_cc_cmd(client, &data);
		break;
	default:
		pr_err("%s: Unknown cmd[%x]\n", __func__, cmd);
		return -EINVAL;
	}

	if (dir & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
			return -EFAULT;
	}
	return ret;
}

static long compat_berlin_cc_ioctl(struct file *filp, unsigned int cmd,
				   unsigned long arg)
{
	return berlin_cc_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}

static const struct file_operations berlin_cc_ops = {
	.open = berlin_cc_open,
	.release = berlin_cc_release,
	.unlocked_ioctl = berlin_cc_ioctl,
	.compat_ioctl = compat_berlin_cc_ioctl,
	.owner = THIS_MODULE,
};

static int __init berlin_cc_init(void)
{
	int ret;

	pr_info("berlin cc init\n");
	cc_dev.dev.minor = MISC_DYNAMIC_MINOR;
	cc_dev.dev.name = BERLIN_CC_DEVICE_NAME;
	cc_dev.dev.fops = &berlin_cc_ops;
	cc_dev.dev.parent = NULL;
	ret = misc_register(&cc_dev.dev);
	if (ret) {
		pr_err("error in register berlin cc device: %d\n", ret);
		return ret;
	}
	init_rwsem(&cc_dev.client_lock);
	cc_dev.clients = RB_ROOT;
	init_rwsem(&berlin_cc.rwsem);
	idr_init(&berlin_cc.idr);

	cc_dev.debug_root = debugfs_create_dir("berlin_cc", NULL);
	cc_dev.clients_debug_root = debugfs_create_dir("clients",
						 cc_dev.debug_root);
	berlin_cc.debug_root = debugfs_create_file("cc", 0664,
				cc_dev.debug_root, (void *)&berlin_cc,
				&debug_cc_fops);

	return 0;
}

static void __exit berlin_cc_exit(void)
{
	pr_info("berlin cc exit\n");
	idr_destroy(&berlin_cc.idr);
	debugfs_remove_recursive(berlin_cc.debug_root);
	debugfs_remove_recursive(cc_dev.clients_debug_root);
	debugfs_remove_recursive(cc_dev.debug_root);
	misc_deregister(&cc_dev.dev);
}

module_init(berlin_cc_init);
module_exit(berlin_cc_exit);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("berlin cc module");
