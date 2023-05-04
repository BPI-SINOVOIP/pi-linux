// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Jie Xi <jie.xi@synaptics.com>
 *
 */

#define pr_fmt(fmt) "[berlin_bm kernel driver] " fmt

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/sched/task.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/utsname.h>
#include <linux/ion.h>
#include <linux/mod_devicetable.h>
#include <linux/ion_heap_extra.h>
#include "uapi/bm.h"
#include "ptm_wrapper.h"
#include "tz_driver.h"

#define BERLIN_BM_DEVICE_NAME "amp-bm"

struct berlin_bm_device {
	struct device *dev;
	struct miscdevice mdev;
	struct rb_root ptes;
	struct rw_semaphore pte_lock;
	struct rb_root clients;
	struct rw_semaphore client_lock;
	struct ptm_context ptm;
	struct workqueue_struct *wq;
	struct dentry *debug_root;
	struct dentry *pte_debug_root;
};

struct berlin_bm_client {
	struct rb_node node;
	struct rb_root refs;
	struct mutex ref_lock;
	struct berlin_bm_device *dev;
	const char *name;
	struct task_struct *task;
	pid_t pid;
};

struct berlin_pte_node {
	struct rb_node node;
	struct dma_buf *dmabuf;
	struct delayed_work work_free_pt;
	unsigned int try_count;
	phys_addr_t phy_addr_pt;
	size_t len_pt;
	u64 mem_id;
	u32 flags;
};

struct berlin_client_ref {
	struct rb_node node;
	struct berlin_pte_node *ref;
	unsigned int ref_cnt;
};

union bm_ioctl_arg {
	struct bm_fb_data fb;
	struct bm_pt_data pt;
	int fd;
};

#define FREE_PT_TRY_COUNT     100
#define FREE_PT_TRY_INTERVAL  1

static struct berlin_bm_device bm_dev;

static void free_pt_work(struct work_struct *work)
{
	int ret = 0;
	struct berlin_pte_node *pn =
		container_of(to_delayed_work(work),
			     struct berlin_pte_node,
			     work_free_pt);

	pn->try_count++;
	ret = ptm_wrapper_free_pt(&bm_dev.ptm,
				  pn->phy_addr_pt,
				  pn->len_pt);

	if (ret) {
		pr_info("fail to free pt: %pa. error returned: %x\n",
			&pn->phy_addr_pt, ret);
		pr_info("requeue the free pt work\n");
		if (pn->try_count < FREE_PT_TRY_COUNT)
			queue_delayed_work(bm_dev.wq, &pn->work_free_pt,
				msecs_to_jiffies(FREE_PT_TRY_INTERVAL));
		else {
			pr_crit("still cannot free the pt, memory leaked\n");
			BUG_ON(1);
		}
	} else {
		pr_info("pt freed retrying %d times, addr %pa\n",
			pn->try_count, &pn->phy_addr_pt);
		ion_free((struct ion_buffer *)(pn->dmabuf->priv));
		kfree(pn);
	}
}

static void print_pte_node(struct seq_file *s, int i, struct berlin_pte_node *pn)
{
	struct ion_buffer *buffer;
	struct sg_table *table;
	struct page *page;
	phys_addr_t phys;
	size_t size;

	buffer = pn->dmabuf->priv;
	table = buffer->sg_table;
	page = sg_page(table->sgl);

	phys = PFN_PHYS(page_to_pfn(page));
	size = buffer->size;
	seq_printf(s, "|%4d|%4ld|%8d|%16s|%8d|%16s|%8llx|%8zx|%7d|%8llx|%8zx|\n",
		   i, atomic_long_read(&pn->dmabuf->file->f_count),
		   buffer->pid, buffer->task_comm,
		   buffer->tid, buffer->thread_name,
		   phys, size, buffer->heap->id,
		   pn->phy_addr_pt, pn->len_pt);
}

static int debug_pte_show(struct seq_file *s, void *unused)
{
	struct berlin_bm_device *dev = s->private;
	struct berlin_pte_node *pn;
	struct rb_node *p;
	int i = 0;

	seq_puts(s, "List of allocated PTE entries\n");
	seq_puts(s, "|No  |ref |  pid   |  process_name  |   tid  |   thread_name  |physaddr|  size  |heap_id|pte addr|pte size|\n");
	seq_puts(s, "-----------------------------------------------------------------------------------------------------------\n");

	down_read(&dev->pte_lock);
	for (p = rb_first(&dev->ptes); p; p = rb_next(p)) {
		pn = rb_entry(p, struct berlin_pte_node, node);
		print_pte_node(s, i, pn);
		i++;
	}
	up_read(&dev->pte_lock);
	seq_puts(s, "-----------------------------------------------------------------------------------------------------------\n");

	return 0;
}

static int debug_pte_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_pte_show, inode->i_private);
}

static const struct file_operations debug_pte_fops = {
	.open = debug_pte_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef BM_PAGE_DEBUG
static void print_buf_pages(struct ion_buffer *buf)
{
	phys_addr_t pphy = 0, cphy = 0;
	struct sg_page_iter iter;
	int i = 0;

	for_each_sg_page(buf->sg_table->sgl, &iter, buf->sg_table->nents, 0) {
		p = sg_page_iter_page(&iter);
		cphy = PFN_PHYS(page_to_pfn(p));
		if (pphy == 0 || cphy - pphy != PAGE_SIZE) {
			if (i != 0)
				pr_debug("total %d\n", i);
				pr_debug("phy addr: %pa\n", &cphy);
				i = 1;
			} else
				i++;
			pphy = cphy;
		}
	}
	pr_debug("total %d\n", i);
}
#endif

static int create_pte(struct dma_buf *dmabuf, u32 flags,
		      struct bm_fb_param *fb_param,
		      struct bm_pt_param *pt_param)
{
	int ret = 0;
	struct ion_buffer *buf;
	phys_addr_t *addr_array = NULL;
	struct scatterlist *sg;
	struct page *p;
	TEEC_SharedMemory *handle = NULL;
	size_t len;
	int i = 0;

	buf = dmabuf->priv;

	pr_debug("number of sg_table: %d, flags: %x\n",
		buf->sg_table->nents, flags);
	pr_debug("buffer size: 0x%lx\n", buf->size);
	len = buf->sg_table->nents * 2 * sizeof(phys_addr_t);

	if (len > PTM_MAX_SG_SHM) {
		pr_err("fail to allocate addr array, %zx exceeds %zx\n",
			len, PTM_MAX_SG_SHM);
		return -ENOMEM;
	}
	handle = bm_dev.ptm.sg_shm;
	addr_array = handle->buffer;

	for_each_sg(buf->sg_table->sgl, sg, buf->sg_table->nents, i) {
		p = sg_page(sg);
		addr_array[2 * i] = PFN_PHYS(page_to_pfn(p));
		addr_array[2 * i + 1] = sg->length / PAGE_SIZE;
		pr_debug("[%d]start phy addr: %pa\n",
			  i, &addr_array[2 * i]);
		pr_debug("page number: %pa\n", &addr_array[2 * i + 1]);
	}
#ifdef BM_PAGE_DEBUG
	print_buf_pages(buf);
#endif
	pr_debug("sg list copy");
	/* invalidate the cache first if this buffer
	 * is to be switched from non-secure to secure
	 * linux side must not invalid cache in secure carveout
	 */
	if (flags && (buf->heap->type != ION_HEAP_TYPE_BERLIN_SECURE))
		for_each_sg(buf->sg_table->sgl, sg,
			    buf->sg_table->nents, i) {
			phys_addr_t phys_addr;
			phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
			dma_sync_single_for_cpu(bm_dev.dev,
						phys_addr,
						sg->length,
						DMA_FROM_DEVICE);
		}

	ret = ptm_wrapper_construct_pt(&bm_dev.ptm, handle, len,
				       pt_param, flags, fb_param);
	if (ret) {
		pr_err("fail to construct pt: %x\n", ret);
		/*
		 * return normal kernel error code instead of
		 * TEEC error code directly
		 */
		ret = -EACCES;
	}
	pr_debug("pt constructed %llx %llx", pt_param->phy_addr, pt_param->mem_id);
	return ret;
}

static struct berlin_pte_node *create_pte_node(struct dma_buf *dmabuf,
					       u32 flags,
					       struct bm_fb_param *fb_param,
					       struct bm_pt_param *pt_param)
{
	int ret;
	struct berlin_pte_node *pn;

	ret = create_pte(dmabuf, flags, fb_param, pt_param);
	if (ret)
		return ERR_PTR(ret);

	pn = kzalloc(sizeof(*pn), GFP_KERNEL);
	if (!pn) {
		ptm_wrapper_free_pt(&bm_dev.ptm,
				    pn->phy_addr_pt,
				    pn->len_pt);
		return ERR_PTR(-ENOMEM);
	}
	pn->dmabuf = dmabuf;
	pn->flags = flags;
	pn->phy_addr_pt = pt_param->phy_addr;
	pn->len_pt = pt_param->len;
	pn->mem_id = pt_param->mem_id;

	return pn;
}

static int get_pte_node(struct berlin_bm_client *client,
			struct dma_buf *dmabuf,
			u32 flags,
			struct bm_fb_param *fb_param,
			struct bm_pt_param *pt_param)
{
	int ret = 0;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct berlin_pte_node *data = NULL;
	struct berlin_pte_node *pn;
	bool found = false;

	down_write(&bm_dev.pte_lock);
	p = &bm_dev.ptes.rb_node;
	while (*p) {
		data = rb_entry(*p, struct berlin_pte_node, node);
		parent = *p;
		if (data->dmabuf < dmabuf)
			p = &(*p)->rb_left;
		else if (data->dmabuf > dmabuf)
			p = &(*p)->rb_right;
		else {
			found = true;
			break;
		}
	}

	if (found) {
		pr_debug("pte already exist for the buffer %p, client %d\n",
			 dmabuf, client->pid);
		if (!ret) {
			pt_param->phy_addr = data->phy_addr_pt;
			pt_param->len = data->len_pt;
			pt_param->mem_id = data->mem_id;
		}
	} else {
		pn = create_pte_node(dmabuf, flags, fb_param, pt_param);
		if (!IS_ERR(pn)) {
			rb_link_node(&pn->node, parent, p);
			rb_insert_color(&pn->node, &bm_dev.ptes);
			ret = 0;
			pr_debug("pte allocated %p, PT %pa, pid %d, mem id %llx\n",
				dmabuf, &pn->phy_addr_pt, client->pid, pn->mem_id);
		} else
			ret = PTR_ERR(pn);
	}
	up_write(&bm_dev.pte_lock);

	return ret;
}

static int bm_alloc_pt(struct berlin_bm_client *client, int fd,
		       u32 flags,
		       struct bm_fb_param *fb_param,
		       struct bm_pt_param *pt_param)
{
	int ret = 0;
	struct dma_buf *dmabuf;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("failed to get dmabuf from fd: %ld\n", PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}
	pr_debug("%s enter, caller pid %d", __func__,  client->pid);
	ret = get_pte_node(client, dmabuf, flags, fb_param, pt_param);
	pr_debug("%s exit, caller pid %d, ret %d", __func__,  client->pid, ret);
	if (ret)
		dma_buf_put(dmabuf);
	return ret;
}

static int bm_get_pt(struct berlin_bm_client *client, int fd,
		     struct bm_pt_param *pt_param)
{
	int ret = 0;
	struct dma_buf *dmabuf;
	struct rb_node *p;
	struct berlin_pte_node *data = NULL;
	bool found = false;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("failed to get dmabuf from fd: %ld\n", PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}

	down_read(&bm_dev.pte_lock);
	p = bm_dev.ptes.rb_node;
	while (p) {
		data = rb_entry(p, struct berlin_pte_node, node);
		if (data->dmabuf < dmabuf)
			p = p->rb_left;
		else if (data->dmabuf > dmabuf)
			p = p->rb_right;
		else {
			found = true;
			break;
		}
	}

	if (found) {
		pt_param->phy_addr = data->phy_addr_pt;
		pt_param->len = data->len_pt;
		pt_param->mem_id = data->mem_id;
	} else {
		pr_err("no pte node found for this dmabuf: %p,"
			"client %d, buffer owner %d\n",
			dmabuf, client->pid, ((struct ion_buffer *)(dmabuf->priv))->pid);
		ret = -EINVAL;
	}
	up_read(&bm_dev.pte_lock);

	return ret;
}

static int bm_put_pt(struct berlin_bm_client *client, int fd)
{
	int ret = 0;
	struct dma_buf *dmabuf;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("failed to get dmabuf from fd: %ld\n", PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}
	dma_buf_put(dmabuf);
	dma_buf_put(dmabuf);
	return ret;
}

static int pte_node_destroy_by_callback(struct berlin_pte_node *pn)
{
	int ret = 0;

	pr_debug("start free pte node %pa, dma buf %p, ref %ld, owner pid %d\n",
		&pn->phy_addr_pt, pn->dmabuf,
		atomic_long_read(&pn->dmabuf->file->f_count),
		((struct ion_buffer *)(pn->dmabuf->priv))->pid);
	rb_erase(&pn->node, &bm_dev.ptes);

	if (pn->phy_addr_pt)
		ret = ptm_wrapper_free_pt(&bm_dev.ptm,
					  pn->phy_addr_pt,
					  pn->len_pt);
	if (ret) {
		pr_err("fail to free pt: %pa. error returned: %x\n",
			&pn->phy_addr_pt, ret);
		pr_err("CPU: %d PID: %d Comm: %.20s %s %.*s\n",
			raw_smp_processor_id(), current->pid, current->comm,
			init_utsname()->release,
			(int)strcspn(init_utsname()->version, " "),
			init_utsname()->version);
		pr_err("queue the free pt work\n");
		pn->try_count = 0;
		INIT_DEFERRABLE_WORK(&pn->work_free_pt, free_pt_work);
		queue_delayed_work(bm_dev.wq, &pn->work_free_pt,
			usecs_to_jiffies(FREE_PT_TRY_INTERVAL));
	}

	return ret;
}

static int bm_callback(struct dma_buf *dmabuf)
{
	struct rb_node *p;
	struct berlin_pte_node *data = NULL;
	bool found = false;
	int ret = 0;

	down_write(&bm_dev.pte_lock);
	p = bm_dev.ptes.rb_node;
	while (p) {
		data = rb_entry(p, struct berlin_pte_node, node);
		if (data->dmabuf < dmabuf)
			p = p->rb_left;
		else if (data->dmabuf > dmabuf)
			p = p->rb_right;
		else {
			found = true;
			break;
		}
	}

	if (found) {
		ret = pte_node_destroy_by_callback(data);
	}
	up_write(&bm_dev.pte_lock);

	return ret;
}

static struct berlin_bm_client *berlin_bm_client_create(const char *name)
{
	struct berlin_bm_client *client;
	struct task_struct *task;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct berlin_bm_client *entry;
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

	client->dev = &bm_dev;
	mutex_init(&client->ref_lock);
	client->task = task;
	client->pid = pid;
	client->name = kstrdup(name, GFP_KERNEL);
	if (!client->name)
		goto err_free_client;

	client->refs = RB_ROOT;

	down_write(&bm_dev.client_lock);
	p = &bm_dev.clients.rb_node;
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct berlin_bm_client, node);

		if (client < entry)
			p = &(*p)->rb_left;
		else if (client > entry)
			p = &(*p)->rb_right;
	}
	rb_link_node(&client->node, parent, p);
	rb_insert_color(&client->node, &bm_dev.clients);
	up_write(&bm_dev.client_lock);

	return client;

err_free_client:
	kfree(client);
err_put_task_struct:
	if (task)
		put_task_struct(current->group_leader);
	return ERR_PTR(-ENOMEM);
}

static void berlin_bm_client_destroy(struct berlin_bm_client *client)
{
	down_write(&bm_dev.client_lock);
	if (client->task)
		put_task_struct(client->task);
	rb_erase(&client->node, &bm_dev.clients);
	up_write(&bm_dev.client_lock);

	kfree(client->name);
	kfree(client);
}

static int bm_release(struct inode *inode, struct file *file)
{
	struct berlin_bm_client *client = file->private_data;

	pr_info("client %s closed\n", client->name);
	berlin_bm_client_destroy(client);
	return 0;
}

static int bm_open(struct inode *inode, struct file *file)
{
	struct berlin_bm_client *client;
	char debug_name[64];

	snprintf(debug_name, 64, "%u", task_pid_nr(current->group_leader));
	client = berlin_bm_client_create(debug_name);
	if (IS_ERR(client))
		return PTR_ERR(client);
	pr_info("client %s opened\n", client->name);
	file->private_data = client;

	return 0;
}

static long bm_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	int ret = 0;
	unsigned int dir;
	struct berlin_bm_client *client = filp->private_data;
	union bm_ioctl_arg data;

	dir = _IOC_DIR(cmd);
	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (cmd == BM_IOC_PUT_PT)
		data.fd = (int)arg;
	else if (!(dir & _IOC_WRITE))
		memset(&data, 0, sizeof(data));
	else if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	pr_debug("ioctl cmd: %x fd: %d\n", cmd, data.fb.fd);
	switch (cmd) {
	case BM_IOC_ALLOC_PT:
		ret = bm_alloc_pt(client, data.fb.fd,
				  data.fb.flags,
				  &data.fb.fb_param,
				  &data.fb.pt_param);
		break;
	case BM_IOC_GET_PT:
		ret = bm_get_pt(client, data.pt.fd, &data.pt.pt_param);
		break;
	case BM_IOC_PUT_PT:
		ret = bm_put_pt(client, data.fd);
		break;
	default:
		return -EINVAL;
	}

	if (dir & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
			return -EFAULT;
	}

	return ret;
}

static const struct file_operations berlin_bm_ops = {
	.open = bm_open,
	.release = bm_release,
	.unlocked_ioctl = bm_ioctl,
	.compat_ioctl = bm_ioctl,
	.owner = THIS_MODULE,
};

static int bm_init(void)
{
	int ret;

	pr_info("berlin bm init\n");
	bm_dev.mdev.minor = MISC_DYNAMIC_MINOR;
	bm_dev.mdev.name = BERLIN_BM_DEVICE_NAME;
	bm_dev.mdev.fops = &berlin_bm_ops;
	bm_dev.mdev.parent = NULL;
	ret = misc_register(&bm_dev.mdev);
	if (ret) {
		pr_err("error in register berlin bm device: %d\n", ret);
		return ret;
	}

	bm_dev.ptes = RB_ROOT;
	init_rwsem(&bm_dev.pte_lock);
	bm_dev.clients = RB_ROOT;
	init_rwsem(&bm_dev.client_lock);
	ret = ptm_wrapper_init(&bm_dev.ptm);
	if (ret) {
		pr_err("error in ptm init: %d\n", ret);
		goto error_ptm_init;
	}

	bm_dev.wq = alloc_ordered_workqueue("berlin_bm", WQ_MEM_RECLAIM);
	if (!bm_dev.wq) {
		pr_err("error in allocing workqueue\n");
		goto error_wq;
	}

	bm_dev.debug_root = debugfs_create_dir("berlin_bm", NULL);
	bm_dev.pte_debug_root = debugfs_create_file("pte", 0664,
				bm_dev.debug_root, (void *)&bm_dev,
				&debug_pte_fops);

	ion_heap_extra_reg_free_cb(ION_HEAP_TYPE_MAX, bm_callback);
	return 0;

error_wq:
	ptm_wrapper_exit(&bm_dev.ptm);

error_ptm_init:
	misc_deregister(&bm_dev.mdev);
	return ret;
}

static void bm_exit(void)
{
	pr_info("berlin bm exit\n");
	misc_deregister(&bm_dev.mdev);
	ptm_wrapper_exit(&bm_dev.ptm);
	flush_workqueue(bm_dev.wq);
	destroy_workqueue(bm_dev.wq);
	debugfs_remove_recursive(bm_dev.pte_debug_root);
	debugfs_remove_recursive(bm_dev.debug_root);
}

static int bm_probe(struct platform_device *pdev)
{
	int ret;

	/* Defer probe since there is dependency of tzd */
	if (!tzd_get_kernel_dev_file())
		return -EPROBE_DEFER;

	ret = bm_init();
	if (ret) {
		pr_err("bm_init failed\n");
		return ret;
	}
	pr_debug("%s OK\n", __func__);

	bm_dev.dev = &pdev->dev;
	return ret;
}

static int bm_remove(struct platform_device *pdev)
{
	bm_exit();
	pr_debug("%s OK\n", __func__);
	return 0;
}

static const struct of_device_id bm_match[] = {
	{.compatible = "syna,berlin-bm",},
	{},
};

static struct platform_driver bm_driver = {
	.probe = bm_probe,
	.remove = bm_remove,
	.driver = {
		.name = "amp-bm",
		.of_match_table = bm_match,
	},
};

module_platform_driver(bm_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("berlin bm module");
