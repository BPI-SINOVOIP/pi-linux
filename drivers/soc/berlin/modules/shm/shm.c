// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Dejin Zheng <Dejin.Zheng@synaptics.com>
 *         Jie Xi <Jie.Xi@synaptics.com>
 *
 */

#define pr_fmt(fmt) "[shm kernel driver]" fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include <linux/sched/task.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/ion.h>

#include "shm.h"

struct gid_node_data {
	int gid;
	bool alloc;
	struct berlin_buffer_node *bn;
};

enum shm_dmabuf_src {
	SHM_REG,
	SHM_ALLOC
};

static struct shm_device shm_dev;

static void berlin_buffer_node_destroy(struct kref *kref)
{
	struct berlin_buffer_node *bn = container_of(kref,
					 struct berlin_buffer_node, ref);

	pr_debug("free buffer node[%08x]\n", bn->gid);
	down_write(&bn->dev->idr_lock);
	idr_remove(&bn->dev->idr, bn->gid);
	up_write(&bn->dev->idr_lock);

	pr_debug("dma buffer refcount before put: %ld\n",
		atomic_long_read(&bn->dmabuf->file->f_count));
	dma_buf_put(bn->dmabuf);
	kfree(bn);
}

static inline int berlin_buffer_node_put(struct berlin_buffer_node *bn)
{
	pr_debug("ref count before putting buffer node[%08x]: %d\n",
		bn->gid, refcount_read(&bn->ref.refcount));
	return kref_put(&bn->ref, berlin_buffer_node_destroy);
}

static inline int berlin_buffer_node_get(struct berlin_buffer_node *bn)
{
	int ret;

	pr_debug("ref count before getting buffer node[%08x]: %d\n",
		bn->gid, refcount_read(&bn->ref.refcount));
	ret = kref_get_unless_zero(&bn->ref);
	if (!ret)
		pr_err("%s: fail to get buffer node\n", __func__);
	return ret ? 0 : -EINVAL;
}

static inline int buffer_dec_ref(struct berlin_buffer_node *bn)
{
	dma_buf_put(bn->dmabuf);
	return berlin_buffer_node_put(bn);
}

static inline int buffer_inc_ref(struct berlin_buffer_node *bn)
{
	int ret;

	ret = berlin_buffer_node_get(bn);
	if (!ret)
		get_dma_buf(bn->dmabuf);
	return ret;
}

static int shm_debug_gid_show(struct seq_file *s, void *unused)
{
	struct shm_device *dev = s->private;
	struct berlin_buffer_node *bn;
	phys_addr_t phys;
	size_t size;
	int i = 0;
	int id = 0;

	seq_puts(s, "|No  |Gid  |Ref |  pid   |  process_name  |   tid  |   thread_name  |physaddr|  size  |alloclen|heap_id|\n");
	seq_puts(s, "--------------------------------------------------------------------------------------------------------\n");

	down_read(&dev->idr_lock);
	idr_for_each_entry(&dev->idr, bn, id) {
		struct ion_buffer *buffer = bn->dmabuf->priv;
		struct sg_table *table = buffer->sg_table;
		struct page *page = sg_page(table->sgl);

		phys = PFN_PHYS(page_to_pfn(page));
		size = buffer->size;
		seq_printf(s, "|%4d|%5u|%4ld|%8d|%16s|%8d|%16s|%8llx|%8zx|%8zx|%7d|\n",
			   i, id,
			   atomic_long_read(&bn->dmabuf->file->f_count),
			   buffer->pid, buffer->task_comm,
			   buffer->tid, buffer->thread_name,
			   phys, size, bn->alloc_len, buffer->heap->id);
		i++;
	}
	up_read(&dev->idr_lock);
	seq_puts(s, "--------------------------------------------------------------------------------------------------------\n");

	return 0;
}

static int shm_debug_gid_open(struct inode *inode, struct file *file)
{
	return single_open(file, shm_debug_gid_show, inode->i_private);
}

static const struct file_operations debug_gid_fops = {
	.open = shm_debug_gid_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int gid_add_client(struct shm_client *client,
			  struct gid_node_data *node_data)
{
	struct rb_node **p;
	struct rb_node *parent = NULL;
	int found = 0;

	mutex_lock(&client->lock);
	p = &client->gids.rb_node;
	while (*p) {
		struct berlin_gid_node *tmp = rb_entry(*p,
						struct berlin_gid_node, node);
		parent = *p;
		if (tmp->gid < node_data->gid)
			p = &(*p)->rb_left;
		else if (tmp->gid > node_data->gid)
			p = &(*p)->rb_right;
		else {
			pr_debug("duplicated gid[%d] on client[%s]\n",
					tmp->gid, client->name);
			found = 1;
			if (!node_data->alloc)
				tmp->ref_cnt++;
			else
				pr_err("same gid[%d] allocated\n",
					  tmp->gid);
			break;
		}
	}
	if (!found) {
		struct berlin_gid_node *data;

		pr_debug("add gid node[%d] on client[%s]\n",
			node_data->gid, client->name);
		data = kzalloc(sizeof(*data), GFP_KERNEL);
		if (!data) {
			mutex_unlock(&client->lock);
			return -ENOMEM;
		}
		data->gid = node_data->gid;
		data->data = node_data->bn;
		data->ref_cnt = 1;

		rb_link_node(&data->node, parent, p);
		rb_insert_color(&data->node, &client->gids);
	}
	mutex_unlock(&client->lock);

	return 0;
}

static void gid_remove_all(struct shm_client *client)
{
	struct rb_node *p = NULL;
	struct berlin_gid_node *data = NULL;
	struct berlin_buffer_node *bn = NULL;

	mutex_lock(&client->lock);
	pr_debug("remove all gid nodes from client[%s]\n",
			client->name);
	while ((p = rb_first(&client->gids))) {
		data = rb_entry(p, struct berlin_gid_node, node);
		rb_erase(&data->node, &client->gids);
		bn = (struct berlin_buffer_node *)data->data;
		while (data->ref_cnt) {
			berlin_buffer_node_put(bn);
			data->ref_cnt--;
		}
		kfree(data);
	}
	mutex_unlock(&client->lock);
}

static int gid_remove_client(struct shm_client *client, int gid)
{
	int ret = 0;
	struct rb_node *p = NULL;
	struct berlin_gid_node *data = NULL;
	struct berlin_buffer_node *bn = NULL;

	mutex_lock(&client->lock);
	p = client->gids.rb_node;
	while (p) {
		data = rb_entry(p, struct berlin_gid_node, node);
		if (data->gid < gid)
			p = p->rb_left;
		else if (data->gid > gid)
			p = p->rb_right;
		else
			break;
	}
	if (data && p) {
		bn = (struct berlin_buffer_node *)data->data;
		berlin_buffer_node_put(bn);
		data->ref_cnt--;
		pr_debug("%s: gid %d from client[%s], ref cnt %d\n", __func__,
			 gid, client->name, data->ref_cnt);
		if (!data->ref_cnt) {
			pr_debug("remove gid node[%d] from client[%s]\n",
				  data->gid, client->name);
			rb_erase(&data->node, &client->gids);
			kfree(data);
		}
	} else {
		pr_err("%s: no gid %d found from client[%s]\n", __func__, gid,
			client->name);
		ret = -EINVAL;
	}
	mutex_unlock(&client->lock);
	return ret;
}

static int buffer_node_create(struct shm_client *client,
			      struct dma_buf *dmabuf, int *gid, int flags,
			      size_t alloc_len)
{
	struct ion_buffer *buf;
	struct berlin_buffer_node *bn;
	struct gid_node_data node_data;
	int ret;

	bn = kzalloc(sizeof(*bn), GFP_KERNEL);
	if (!bn)
		return -ENOMEM;

	bn->dmabuf = dmabuf;
	bn->buf_flags = flags;
	bn->alloc_len = alloc_len;
	bn->dev = &shm_dev;
	bn->owner = client;
	kref_init(&bn->ref);

	down_write(&shm_dev.idr_lock);
	ret = idr_alloc(&shm_dev.idr, bn, 1, 0, GFP_KERNEL);
	up_write(&shm_dev.idr_lock);
	if (ret < 0) {
		pr_err("error in allocating gid: %x\n", ret);
		goto free1;
	}
	*gid = ret;
	bn->gid = *gid;

	node_data.gid = *gid;
	node_data.bn = bn;
	node_data.alloc = true;
	ret = gid_add_client(client, &node_data);
	if (ret)
		goto free2;

	buf = dmabuf->priv;
	/* alloc_len = 0 means the buffer is not directly allocated from shm,
	 * we don't know the actual alloc_len, just use the ion buffer size
	 * instead
	 */
	if (!bn->alloc_len)
		bn->alloc_len = buf->size;

	return ret;
free2:
	down_write(&shm_dev.idr_lock);
	idr_remove(&shm_dev.idr, bn->gid);
	up_write(&shm_dev.idr_lock);
free1:
	kfree(bn);
	return ret;
}

struct shm_client *shm_client_create(const char *name)
{
	struct shm_client *client;
	struct task_struct *task;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct shm_client *entry;
	pid_t pid;

	if (!name) {
		pr_err("%s: Name cannot be null\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);
	pid = task_pid_nr(current->group_leader);
	/*
	 * don't bother to store task struct for kernel threads,
	 * they can't be killed anyway
	 */
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

	client->dev = &shm_dev;
	mutex_init(&client->lock);
	client->task = task;
	client->pid = pid;
	client->name = kstrdup(name, GFP_KERNEL);
	if (!client->name)
		goto err_free_client;

	client->gids = RB_ROOT;

	down_write(&shm_dev.client_lock);
	p = &shm_dev.clients.rb_node;
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct shm_client, node);

		if (client < entry)
			p = &(*p)->rb_left;
		else if (client > entry)
			p = &(*p)->rb_right;
	}
	rb_link_node(&client->node, parent, p);
	rb_insert_color(&client->node, &shm_dev.clients);
	up_write(&shm_dev.client_lock);

	return client;

err_free_client:
	kfree(client);
err_put_task_struct:
	if (task)
		put_task_struct(current->group_leader);
	return ERR_PTR(-ENOMEM);
}
EXPORT_SYMBOL(shm_client_create);

void shm_client_destroy(struct shm_client *client)
{
	struct shm_device *dev = client->dev;

	gid_remove_all(client);

	down_write(&dev->client_lock);
	if (client->task)
		put_task_struct(client->task);
	rb_erase(&client->node, &dev->clients);
	up_write(&dev->client_lock);

	kfree(client->name);
	kfree(client);
}
EXPORT_SYMBOL(shm_client_destroy);

static int shm_release(struct inode *inode, struct file *file)
{
	struct shm_client *client = file->private_data;

	pr_debug("%s called\n", __func__);
	shm_client_destroy(client);
	return 0;
}

static int shm_open(struct inode *inode, struct file *file)
{
	struct shm_client *client;
	char debug_name[64];

	pr_debug("%s called\n", __func__);
	snprintf(debug_name, 64, "%u", task_pid_nr(current->group_leader));
	client = shm_client_create(debug_name);
	if (IS_ERR(client))
		return PTR_ERR(client);
	file->private_data = client;

	return 0;
}

static int shm_alloc(struct shm_client *client, size_t len,
		     unsigned int ion_heap_id_mask,
		     unsigned int flags, int *gid)
{
	int fd;
	int ret;
	struct dma_buf *dmabuf;
	unsigned int buf_flags = (SHM_ALLOC << 16) | flags;
	unsigned int ion_flags = flags & SHM_FLAG_NONCACHED ?
				 ION_NONCACHED : ION_CACHED;

	pr_debug("len: %lx heap_id_mask: %x flags: %x\n",
		  len, ion_heap_id_mask, flags);
	pr_debug("ion_flags: %x\n", ion_flags);
	dmabuf = ion_alloc(len, ion_heap_id_mask, ion_flags);
	if (IS_ERR(dmabuf)) {
		pr_debug("fail to allocate ion buffer:%ld\n", PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}
	/* one additional refcnt for the gid */
	get_dma_buf(dmabuf);

	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (fd < 0) {
		pr_err("error in ion_alloc: %x\n", fd);
		ret = fd;
		goto free_buf;
	}

	ret = buffer_node_create(client, dmabuf, gid, buf_flags, len);
	if (ret)
		goto free_buf;

	return fd;

free_buf:
	/*
	 * The refcnt of the dmabuf are increased by 2,
	 * one for fd and one for gid
	 * So if there is any error happened, the refcnt
	 * should be decreased by 2 accordingly
	 */
	dma_buf_put(dmabuf);
	dma_buf_put(dmabuf);
	return ret;
}

static int shm_detach(struct shm_client *client, int gid)
{
	if (gid <= 0) {
		pr_err("%s: invalid gid[%08x]", __func__, gid);
		return -EINVAL;
	}

	return gid_remove_client(client, gid);
}

static int shm_get_fd(struct shm_client *client, int gid, int *fd)
{
	struct berlin_buffer_node *bn;
	struct gid_node_data node_data;
	int ret;

	down_read(&shm_dev.idr_lock);
	bn = idr_find(&shm_dev.idr, gid);
	if (!bn) {
		pr_err("%s: fail to find the buf from gid[%x].\n",
				__func__, gid);
		up_read(&shm_dev.idr_lock);
		return -EINVAL;
	}
	ret = buffer_inc_ref(bn);
	if (ret) {
		up_read(&shm_dev.idr_lock);
		return ret;
	}
	up_read(&shm_dev.idr_lock);

	node_data.gid = gid;
	node_data.bn = bn;
	node_data.alloc = false;
	ret = gid_add_client(client, &node_data);
	if (ret) {
		pr_err("%s: fail to add gid[%08x].\n",
				__func__, gid);
		buffer_dec_ref(bn);
		return ret;
	}

	*fd = dma_buf_fd(bn->dmabuf, O_CLOEXEC);
	if (*fd < 0) {
		pr_err("%s: fail to get fd from gid[%08x]: %d.\n",
				__func__, gid, *fd);
		gid_remove_client(client, gid);
		dma_buf_put(bn->dmabuf);
		ret = *fd;
	}

	return ret;
}

int shm_put(struct shm_client *client, int gid)
{
	struct berlin_buffer_node *bn;
	int ret;

	down_read(&shm_dev.idr_lock);
	bn = idr_find(&shm_dev.idr, gid);
	if (!bn) {
		pr_err("%s: fail to find the buf from gid[%08x].\n",
				__func__, gid);
		up_read(&shm_dev.idr_lock);
		return PTR_ERR(bn);
	}
	ret = buffer_inc_ref(bn);
	if (ret) {
		up_read(&shm_dev.idr_lock);
		return ret;
	}
	up_read(&shm_dev.idr_lock);

	gid_remove_client(client, gid);
	buffer_dec_ref(bn);
	return 0;
}
EXPORT_SYMBOL(shm_put);

struct dma_buf *shm_get_dma_buf(struct shm_client *client, int gid)
{
	struct berlin_buffer_node *bn;
	struct gid_node_data node_data;
	int ret;

	down_read(&shm_dev.idr_lock);
	bn = idr_find(&shm_dev.idr, gid);
	if (!bn) {
		pr_err("%s: fail to find the buf from gid[%x].\n",
				__func__, gid);
		up_read(&shm_dev.idr_lock);
		return ERR_PTR(-EINVAL);
	}
	ret = buffer_inc_ref(bn);
	if (ret) {
		up_read(&shm_dev.idr_lock);
		return ERR_PTR(ret);
	}
	up_read(&shm_dev.idr_lock);

	node_data.gid = gid;
	node_data.bn = bn;
	node_data.alloc = false;
	ret = gid_add_client(client, &node_data);
	if (ret) {
		pr_err("%s: fail to add gid[%08x].\n",
				__func__, gid);
		buffer_dec_ref(bn);
		return ERR_PTR(ret);
	}

	return bn->dmabuf;
}
EXPORT_SYMBOL(shm_get_dma_buf);

static unsigned long ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (unsigned long)PFN_PHYS(page_to_pfn(page));
}

static int shm_get_pa(struct shm_misc_data *data)
{
	struct ion_buffer *buffer;
	struct berlin_buffer_node *bn;
	int ret;

	down_read(&shm_dev.idr_lock);
	bn = idr_find(&shm_dev.idr, data->gid);
	if (!bn) {
		pr_err("%s: fail to find the buf from gid[%08x].\n",
				__func__, data->gid);
		up_read(&shm_dev.idr_lock);
		return PTR_ERR(bn);
	}
	ret = buffer_inc_ref(bn);
	if (ret) {
		up_read(&shm_dev.idr_lock);
		return ret;
	}
	up_read(&shm_dev.idr_lock);

	buffer = bn->dmabuf->priv;
	if (buffer->heap->type == ION_HEAP_TYPE_SYSTEM) {
		/* in case buffer allocated from system heap,
		 * it's not proper to return any physical address
		 */
		data->phy_addr = 0;
		data->len = buffer->size;
	} else {
		data->phy_addr = ion_dmabuf_get_phy(bn->dmabuf);
		data->len = buffer->size;
	}
	pr_debug("%s: gid[%08x] type[%d] addr[%llx] len[%llx].\n",
		  __func__, data->gid, buffer->heap->type,
		  data->phy_addr, data->len);
	buffer_dec_ref(bn);

	return 0;
}

static int shm_attach(struct shm_client *client, int fd, int *gid)
{
	struct dma_buf *dmabuf;
	int ret;
	int buf_flags = SHM_REG << 16;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("fail to get dmabuf from fd:%ld\n", PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}

	ret = buffer_node_create(client, dmabuf, gid, buf_flags, 0);
	if (ret) {
		pr_err("fail to create buffer node: %d\n", ret);
		dma_buf_put(dmabuf);
	}

	return ret;
}

static int do_sync(struct device *dev, struct ion_buffer *buffer,
		   struct shm_sync_data *data)
{
	struct sg_table *table = buffer->sg_table;
	unsigned long offset, region_len, op_len;
	struct scatterlist *sg;
	phys_addr_t phys_addr;
	int i;

	if (!dev) {
		pr_err("no device found\n");
		return -EINVAL;
	}

	offset = data->offset;
	region_len = data->len;
	switch (data->op) {
	case SHM_INVALIDATE_CACHE:
		mutex_lock(&buffer->lock);
		for_each_sg(table->sgl, sg, table->nents, i) {
			if (sg->length > offset) {
				phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
				op_len = sg->length - offset;
				op_len = op_len > region_len ?
					 region_len : op_len;
				dma_sync_single_for_cpu(dev,
					phys_addr + offset,
					op_len, DMA_FROM_DEVICE);
				region_len -= op_len;
				if (!region_len)
					break;
				offset = 0;
			} else {
				offset -= sg->length;
			}
		}
		mutex_unlock(&buffer->lock);
		break;
	case SHM_CLEAN_CACHE:
		mutex_lock(&buffer->lock);
		for_each_sg(table->sgl, sg, table->nents, i) {
			if (sg->length > offset) {
				phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
				op_len = sg->length - offset;
				op_len = op_len > region_len ?
					 region_len : op_len;
				dma_sync_single_for_device(dev,
					phys_addr + offset,
					op_len, DMA_TO_DEVICE);
				region_len -= op_len;
				if (!region_len)
					break;
				offset = 0;
			} else {
				offset -= sg->length;
			}
		}
		mutex_unlock(&buffer->lock);
		break;
	case SHM_FLUSH_CACHE:
		mutex_lock(&buffer->lock);
		for_each_sg(table->sgl, sg, table->nents, i) {
			if (sg->length > offset) {
				phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
				op_len = sg->length - offset;
				op_len = op_len > region_len ?
					 region_len : op_len;
				dma_sync_single_for_device(dev,
					phys_addr + offset,
					op_len, DMA_TO_DEVICE);
				dma_sync_single_for_cpu(dev,
					phys_addr + offset,
					op_len, DMA_FROM_DEVICE);
				region_len -= op_len;
				if (!region_len)
					break;
				offset = 0;
			} else {
				offset -= sg->length;
			}
		}
		mutex_unlock(&buffer->lock);
		break;
	default:
		pr_err("%s: Unknown cache command %d\n",
			__func__, data->op);
		return -EINVAL;
	}

	return 0;
}

static int shm_sync(struct shm_sync_data *data)
{
	int ret = 0;
	struct berlin_buffer_node *bn;
	struct ion_buffer *buffer;

	if (data->op < SHM_INVALIDATE_CACHE
		|| data->op > SHM_FLUSH_CACHE)
		return -EINVAL;

	down_read(&shm_dev.idr_lock);
	bn = idr_find(&shm_dev.idr, data->gid);
	if (!bn) {
		pr_err("%s: fail to find the buf from gid[%08x].\n",
				__func__, data->gid);
		up_read(&shm_dev.idr_lock);
		return PTR_ERR(bn);
	}
	ret = buffer_inc_ref(bn);
	if (ret) {
		up_read(&shm_dev.idr_lock);
		return ret;
	}
	up_read(&shm_dev.idr_lock);

	buffer = bn->dmabuf->priv;
	pr_debug("heap type: %d\n", buffer->heap->type);
	pr_debug("cache op: %d offset: %llx len: %llx\n",
		data->op, data->offset, data->len);

	do_sync(shm_dev.dev, buffer, data);

	buffer_dec_ref(bn);
	return ret;
}

static long shm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int fd;
	unsigned int dir;
	union shm_ioctl_arg data;
	struct shm_client *client = filp->private_data;
	struct shm_allocation_data *alloc = &(data.allocation);

	dir = _IOC_DIR(cmd);
	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (cmd == SHM_IOC_DETACH)
		data.id = (int)arg;
	else if (!(dir & _IOC_WRITE))
		memset(&data, 0, sizeof(data));
	else if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	pr_debug("shm cmd[%d] from client[%s]\n", _IOC_NR(cmd), client->name);
	switch (cmd) {
	case SHM_IOC_ALLOC:
		fd = shm_alloc(client, alloc->len, alloc->ion_heap_id_mask,
				alloc->flags, &(alloc->gid));
		if (fd < 0)
			return fd;
		alloc->fd = fd;

		break;
	case SHM_IOC_GET_PA:
		ret = shm_get_pa(&data.misc);
		break;
	case SHM_IOC_GET_FD:
		ret = shm_get_fd(client, data.fd.gid, &data.fd.fd);
		break;
	case SHM_IOC_PUT:
		ret = shm_put(client, data.fd.gid);
		break;
	case SHM_IOC_ATTACH:
		ret = shm_attach(client, data.fd.fd, &data.fd.gid);
		break;
	case SHM_IOC_DETACH:
		ret = shm_detach(client, data.id);
		break;
	case SHM_IOC_SYNC:
		ret = shm_sync(&data.sync);
		break;
	default:
		return -EINVAL;
	}

	if (dir & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
			// shm_alloc called
			if (cmd == SHM_IOC_ALLOC)
				shm_detach(client, data.allocation.gid);
			return -EFAULT;
		}
	}
	return ret;
}

static const struct file_operations shm_ops = {
	.open = shm_open,
	.release = shm_release,
	.unlocked_ioctl = shm_ioctl,
	.compat_ioctl = shm_ioctl,
	.owner = THIS_MODULE,
};

static int shm_init(struct shm_device *dev)
{
	dev->debug_root = debugfs_create_dir("shm", NULL);
	dev->gid_debug_root = debugfs_create_file("gid", 0664,
					dev->debug_root, (void *)dev,
					&debug_gid_fops);

	init_rwsem(&dev->idr_lock);
	init_rwsem(&dev->client_lock);
	idr_init(&dev->idr);
	dev->clients = RB_ROOT;

	return 0;
}

static int shm_exit(struct shm_device *dev)
{
	pr_debug("%s\n", __func__);
	idr_destroy(&dev->idr);
	debugfs_remove_recursive(dev->gid_debug_root);
	debugfs_remove_recursive(dev->debug_root);

	return 0;
}

static int shm_probe(struct platform_device *pdev)
{
	int ret;

	ret = shm_init(&shm_dev);
	if (ret < 0) {
		pr_err("shm_init fail!\n");
		return ret;
	}
	pr_debug("%s OK\n", __func__);

	shm_dev.dev = &pdev->dev;
	shm_dev.mdev.minor = MISC_DYNAMIC_MINOR;
	shm_dev.mdev.name = SHM_DEVICE_NAME;
	shm_dev.mdev.fops = &shm_ops;
	shm_dev.mdev.parent = NULL;
	ret = misc_register(&shm_dev.mdev);
	if (ret) {
		pr_err("error in register shm devices: %d\n", ret);
		shm_exit(&shm_dev);
	}

	return ret;
}

static int shm_remove(struct platform_device *pdev)
{
	misc_deregister(&shm_dev.mdev);
	shm_exit(&shm_dev);
	pr_debug("%s OK\n", __func__);
	return 0;
}

static const struct of_device_id shm_match[] = {
	{.compatible = "syna,berlin-shm",},
	{},
};

static struct platform_driver shm_driver = {
	.probe = shm_probe,
	.remove = shm_remove,
	.driver = {
		.name = "amp-shm",
		.of_match_table = shm_match,
	},
};

module_platform_driver(shm_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("shm module");
