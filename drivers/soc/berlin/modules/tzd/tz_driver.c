/*
 * TrustZone Driver
 *
 * Copyright (C) 2016 Marvell Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include "config.h"
#include "tz_log.h"
#include "tz_utils.h"
#include "tz_driver.h"
#include "tz_driver_private.h"
#include "tz_nw_ioctl.h"
#include "tz_nw_api.h"
#include "tz_nw_comm.h"
#include "tz_boot_cmd.h"
#include "tz_client_api.h"
#include "tee_sys_cmd.h"
#include "tee_mgr_cmd.h"
#include "tee_client_api.h"
#include "ree_sys_callback.h"

/* FIXME: copy from tee_ta_mgr.h, need to keep same */
#define TEE_SESSION_TASKID(sessionId)		((sessionId) >> 24)

/* Helper macro used for checking return value from smc */
#define SMC_RET(result, func_id) ({ \
	int __rc = 0; \
	if (result != 0) { \
		if (result == SMC_FUNC_ID_UNKNOWN) { \
			tz_error("not suppported func (%lu)", func_id); \
			__rc = -EINVAL; \
		} else { \
			tz_error("func(%lu) access denied", func_id); \
			__rc = -EACCES; \
		} \
	} \
	__rc; })

static struct tzd_dev_file *kernel_dev_file;

static int tzd_resource_free(void *private_data)
{
	struct tzd_dev_file *dev = (struct tzd_dev_file *) private_data;
	if (unlikely(!dev))
		return -ENODEV;
	tzd_shm_delete(dev);
	kfree(dev);
	return 0;
}

static int tzd_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct tzd_dev_file *dev = (struct tzd_dev_file *) filp->private_data;
	int ret = 0;

	if (unlikely(!dev))
		return -ENODEV;

	ret = tzd_shm_mmap(dev, vma);
	if (ret) {
		tz_error("tz mmap error at 0x%lx", vma->vm_pgoff << PAGE_SHIFT);
		return ret;
	}

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (ret) {
		tz_error("tz mmap remap_pfn_range error!");
		return ret;
	}

	return 0;
}

static int tzd_alloc_mem(struct tzd_dev_file *dev, unsigned long arg)
{
	unsigned long __user *argp = (unsigned long __user *) arg;
	unsigned long size;
	void *p;

	if (get_user(size, argp)) {
		tz_error("copy from user failed!");
		return -EFAULT;
	}

	tz_debug("alloc_len = %lu\n", size);

	if (unlikely(!size)) {
		tz_error("alloc mem length is 0!");
		return -EINVAL;
	}

	p = tzd_shm_alloc(dev, size, GFP_KERNEL);
	if (unlikely(!p)) {
		tz_error("alloc mem failed on size %lu!", size);
		return -ENOMEM;
	}

	/* copy back the phy addr */
	if (put_user((unsigned long)p, argp)) {
		tz_error("copy to user failed");
		tzd_shm_free(dev, p);
		return -EFAULT;
	}

	return 0;
}

static int tzd_free_mem(struct tzd_dev_file *dev, unsigned long arg)
{
	return tzd_shm_free(dev, (const void *)arg);
}

int tzd_kernel_alloc_mem(void **va, void **pa, uint32_t alloc_len)
{
	struct tzd_shm *mem_new;

	if (unlikely(!alloc_len)) {
		tz_error("alloc mem length is 0!");
		return -EINVAL;
	}

	mem_new = tzd_shm_new(kernel_dev_file, alloc_len, GFP_KERNEL);
	if (!mem_new) {
		tz_error("tzd_shm_new failed");
		return -ENOMEM;
	}

	if (va) *va = mem_new->k_addr;
	if (pa) *pa = mem_new->p_addr;

	return 0;
}

void *tzd_phys_to_virt(void *call_info, void *pa)
{
	struct tzd_dev_file *dev = call_info;
	return tzd_shm_phys_to_virt(dev, pa);
}

int tzd_kernel_free_mem(void *pa)
{
	return tzd_shm_free(kernel_dev_file, pa);
}

void *tzd_get_kernel_dev_file(void)
{
	return (void *)kernel_dev_file;
}
EXPORT_SYMBOL(tzd_get_kernel_dev_file);

static inline int __is_kernel_va(unsigned long va)
{
	return virt_addr_valid(va);
}

static inline int __is_vmalloc_va(unsigned long va)
{
	return (va <= VMALLOC_END && va >= VMALLOC_START);
}

static inline int __is_user_va(unsigned long va)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	return (va < MODULES_VADDR);
#else
#warning "FIXME: assume kernel va starts from PAGE_OFFSET"
	return (va < PAGE_OFFSET);
#endif
}

int tz_get_meminfo(struct tz_mem_info *info)
{
	int ret = 0;

	if (info->va == NULL) {
		tz_error("virtual address is NULL!");
		ret = -1;
		return ret;
	}

	if (__is_kernel_va((unsigned long)info->va)) {
		info->pa = (void *)virt_to_phys((void *)info->va);
		info->attr = 0;
	} else if (__is_vmalloc_va((unsigned long)info->va)) {
		tz_error("virtual address is vmalloc, not support");
		info->pa = NULL;
		info->attr = 0;
	} else {
		tz_error("virtual address is INVALID!");
		ret = -1;
		return ret;
	}

	return ret;
}

static int tzd_get_meminfo(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct tz_mem_info info;
	unsigned long pte;

	if (copy_from_user((void*)&info, argp, sizeof(struct tz_mem_info))) {
		tz_error("copy from user failed!");
		return -EFAULT;
	}

	if (info.va == NULL) {
		tz_error("user space address is NULL!");
		return -EFAULT;
	}

	pte = tz_user_virt_to_pte(current->mm, (unsigned long)info.va);
	if (pte) {
		info.pa = (void *)((pte & PAGE_MASK) +
				((unsigned long)info.va & ~PAGE_MASK));
		info.attr = pte & ~PAGE_MASK;
	} else {
		info.pa = NULL;
		info.attr = 0;
	}

	if (copy_to_user(argp, &info, sizeof(struct tz_mem_info))) {
		tz_error("copy to user failed");
		return -EFAULT;
	}

	return 0;
}

static int tzd_cmd_req(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int call_id, ret;
	struct tz_nw_comm cc;
	struct tz_nw_task_client *tc;

	/* copy the command to communication channel */
	if (copy_from_user((void *)&cc, argp, sizeof(struct tz_nw_comm))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}

	tc = tz_nw_task_client_get(cc.call.task_id);
	call_id = current->pid;
	ret = tz_nw_comm_invoke_command(tc, &cc, call_id, (void *)dev);

	/* copy back the communication data */
	if (copy_to_user(argp, &cc, sizeof(struct tz_nw_comm))) {
		tz_error("copy to user failed");
		return -EFAULT;
	}

	return ret;
}

static int tzd_fastcall_memmove(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct tz_memmove_param param;
	unsigned long func_id = SMC_FUNC_TOS_MEM_MOVE, result;
	/* copy the command to communication channel */
	if (copy_from_user((void *)&param, argp, sizeof(param))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}
	result = __smc(func_id, param.dst, param.src, param.size);
	return SMC_RET(result, func_id);
}

static int tzd_fastcall_secure_cache(struct tzd_dev_file *dev,
		unsigned long arg, unsigned long func_id)
{
	void __user *argp = (void __user *)arg;
	struct tz_cache_param param;
	unsigned long result;
	/* copy the command to communication channel */
	if (copy_from_user((void *)&param, argp, sizeof(param))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}
	result = __smc(func_id, param.start, param.size);
	return SMC_RET(result, func_id);
}

static int tzd_fastcall_generic_cmd(struct tzd_dev_file *dev,
		unsigned long arg, unsigned long func_id)
{
	struct fastcall_generic_param __user *argp;
	struct fastcall_generic_param param_header;
	struct fastcall_generic_param *param;
	struct tzd_shm *shm;
	int total_param_len;
	unsigned long result;

	argp = (struct fastcall_generic_param  __user *)arg;
	/* copy the command to communication channel */
	if (copy_from_user((void *)&param_header, argp,
				sizeof(param_header))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}

	total_param_len = param_header.param_len + sizeof(param_header);
	shm = tzd_shm_new(dev, total_param_len, GFP_KERNEL);
	if (NULL == shm) {
		tz_error("out of share memory");
		return -EFAULT;
	}
	param = (struct fastcall_generic_param *)shm->k_addr;
	if (copy_from_user((void *)param, (void __user *)argp,
				total_param_len)) {
		tzd_shm_free(dev, shm->p_addr);
		tz_error("copy from user failed");
		return -EFAULT;
	}
	result = __smc(func_id, shm->p_addr, total_param_len);
	if (copy_to_user((void __user *)argp->param, param->param,
			param->param_len)) {
		tzd_shm_free(dev, shm->p_addr);
		tz_error("copy to user failed");
		return -EFAULT;
	}
	tzd_shm_free(dev, shm->p_addr);

	return SMC_RET(result, func_id);
}

static int tzd_create_instance(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct tz_instance_param create_param;
	if (copy_from_user((void *)&create_param, argp,
				sizeof(create_param))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}
	create_param.result =
		tz_invoke_command(0, TZ_TASK_ID_MGR, TZ_CMD_TEE_SYS,
				create_param.param, &create_param.origin,
				NULL, NULL, dev);
	if (copy_to_user(argp, &create_param, sizeof(create_param))) {
		tz_error("copy to user failed");
		return -EFAULT;
	}
	return 0;
}

static int tzd_destroy_instance(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct tz_instance_param destroy_param;

	if (copy_from_user((void *)&destroy_param, argp,
				sizeof(destroy_param))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}

	destroy_param.result =
		tz_invoke_command(0, TZ_TASK_ID_MGR, TZ_CMD_TEE_SYS,
				destroy_param.param, &destroy_param.origin,
				NULL, NULL, dev);
	if (copy_to_user(argp, &destroy_param, sizeof(destroy_param))) {
		tz_error("copy to user failed");
		return -EFAULT;
	}
	return 0;
}

struct tzd_session_info {
	struct list_head head;
	uint32_t param;
	uint32_t session_id;
};

static int open_session(struct tzd_dev_file *dev,
			struct tz_session_param *open_param)
{
	if (unlikely(!dev))
		return -ENODEV;
	if (unlikely(!open_param))
		return -EINVAL;
	open_param->result =
		tz_invoke_command(0, open_param->task_id, TZ_CMD_TEE_SYS,
				open_param->param, &open_param->origin,
				NULL, NULL, dev);
	if (!open_param->result) {
		struct tzd_session_info *new_info = NULL;
		struct tee_comm_param *cmd;
		TASysCmdOpenSessionParamExt *p;
		cmd = tzd_shm_phys_to_virt(dev,
				(void *)((unsigned long)open_param->param));
		p = (void *)cmd->param_ext;

		new_info = kzalloc(sizeof(*new_info), GFP_KERNEL);
		if (!new_info) {
			tz_error("kzalloc failed");
			return -ENOMEM;
		}

		new_info->param = open_param->param;
		new_info->session_id = p->sessionId;

		mutex_lock(&dev->tz_mutex);
		list_add_tail(&new_info->head, &dev->session_list);
		mutex_unlock(&dev->tz_mutex);
	}

	return 0;
}

static int close_session(struct tzd_dev_file *dev,
				struct tz_session_param *close_param)
{
	close_param->result =
		tz_invoke_command(0, close_param->task_id, TZ_CMD_TEE_SYS,
				close_param->param, &close_param->origin,
				NULL, NULL, dev);
	if (!close_param->result) {
		struct tzd_session_info *temp_info;
		struct tzd_session_info *temp_pos;
		struct tee_comm_param *cmd;
		cmd = tzd_shm_phys_to_virt(dev,
				(void *)((unsigned long)close_param->param));

		mutex_lock(&dev->tz_mutex);
		list_for_each_entry_safe(temp_info, temp_pos,
				&dev->session_list, head) {
			if (temp_info->session_id == cmd->session_id) {
				list_del(&temp_info->head);
				if (temp_info)
					kfree(temp_info);
				break;
			}
		}
		mutex_unlock(&dev->tz_mutex);
	}

	return 0;
}

static int tzd_open_session(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct tz_session_param open_param;
	int ret;

	if (copy_from_user((void *)&open_param, argp, sizeof(open_param))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}

	ret = open_session(dev, &open_param);
	if (ret)
		return ret;

	if (copy_to_user(argp, &open_param, sizeof(open_param))) {
		tz_error("copy to user failed");
		return -EFAULT;
	}

	return 0;
}

static int tzd_close_session(struct tzd_dev_file *dev, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct tz_session_param close_param;

	if (copy_from_user((void *)&close_param, argp, sizeof(close_param))) {
		tz_error("copy from user failed");
		return -EFAULT;
	}

	close_session(dev, &close_param);

	if (copy_to_user(argp, &close_param, sizeof(close_param))) {
		tz_error("copy to user failed");
		return -EFAULT;
	}

	return 0;
}

int tzd_kernel_create_instance(struct tz_instance_param *create_param)
{
	create_param->result =
		tzc_invoke_command(0, TZ_TASK_ID_MGR, TZ_CMD_TEE_SYS,
			create_param->param, &create_param->origin,
			NULL, NULL);
	return create_param->result;
}

int tzd_kernel_destroy_instance(struct tz_instance_param *destroy_param)
{
	destroy_param->result =
		tzc_invoke_command(0, TZ_TASK_ID_MGR, TZ_CMD_TEE_SYS,
			destroy_param->param, &destroy_param->origin,
			NULL, NULL);
	return destroy_param->result;
}

int tzd_kernel_open_session(struct tz_session_param *open_param)
{
	return open_session(kernel_dev_file, open_param);
}

int tzd_kernel_close_session(struct tz_session_param *close_param)
{
	return close_session(kernel_dev_file, close_param);
}

long tzd_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	int ret = -EINVAL;
	struct tzd_dev_file *dev;

	dev = (struct tzd_dev_file *) file->private_data;

	switch (cmd) {
	case TZ_CLIENT_IOCTL_CMD:
		ret = tzd_cmd_req(dev, arg);
		break;

	case TZ_CLIENT_IOCTL_FASTCALL_GENERIC_CMD:
		ret = tzd_fastcall_generic_cmd(dev, arg,
				SMC_FUNC_TOS_FASTCALL_GENERIC);
		break;

	case TZ_CLIENT_IOCTL_FASTCALL_MEMMOVE:
		ret = tzd_fastcall_memmove(dev, arg);
		break;

	case TZ_CLIENT_IOCTL_FASTCALL_CACHE_CLEAN:
		ret = tzd_fastcall_secure_cache(dev, arg,
				SMC_FUNC_TOS_SECURE_CACHE_CLEAN);
		break;

	case TZ_CLIENT_IOCTL_FASTCALL_CACHE_INVALIDATE:
		ret = tzd_fastcall_secure_cache(dev, arg,
				SMC_FUNC_TOS_SECURE_CACHE_INVALIDATE);
		break;

	case TZ_CLIENT_IOCTL_FASTCALL_CACHE_FLUSH:
		ret = tzd_fastcall_secure_cache(dev, arg,
				SMC_FUNC_TOS_SECURE_CACHE_CLEAN_INVALIDATE);
		break;

	case TZ_CLIENT_IOCTL_ALLOC_MEM:
		ret = tzd_alloc_mem(dev, arg);
		if (ret)
			tz_error("failed tzd_alloc_mem: %d", ret);
		break;

	case TZ_CLIENT_IOCTL_FREE_MEM:
		ret = tzd_free_mem(dev, arg);
		if (ret)
			tz_error("failed tzd_free_mem: %d", ret);
		break;

	case TZ_CLIENT_IOCTL_GET_MEMINFO:
		ret = tzd_get_meminfo(dev, arg);
		break;

	case TZ_CLIENT_IOCTL_OPEN_SESSION:
		ret = tzd_open_session(dev, arg);
		break;

	case TZ_CLIENT_IOCTL_CLOSE_SESSION:
		ret = tzd_close_session(dev, arg);
		break;

	case TZ_CLIENT_IOCTL_CREATE_INSTANCE:
		ret = tzd_create_instance(dev, arg);
		break;

	case TZ_CLIENT_IOCTL_DESTROY_INSTANCE:
		ret = tzd_destroy_instance(dev, arg);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static int tzd_open(struct inode *inode, struct file *file)
{
	struct tzd_dev_file *new_dev;

	tz_debug("tzd_open");

	new_dev = (struct tzd_dev_file*)kzalloc(sizeof(struct tzd_dev_file), GFP_KERNEL);
	if (!new_dev) {
		tz_error("kzalloc failed for new dev file allocation");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&new_dev->dev_shm_head.shm_list);

	mutex_init(&new_dev->tz_mutex);

	INIT_LIST_HEAD(&new_dev->session_list);

	file->private_data = (void *)new_dev;

	return 0;
}

static int tzd_release_session(struct tzd_dev_file *temp_dev_file,
				struct tzd_session_info *info)
{
	struct tee_comm_param *cmd;
	uint32_t task_id;
	uint32_t origin;
	bool instance_dead = false;
	struct tz_nw_task_client *tc;
	int ret = 0;

	task_id = TEE_SESSION_TASKID(info->session_id);
	tc = tz_nw_task_client_get(task_id);

	WARN_ON(!tc);
	if (unlikely(!tc)) {
		return -EINVAL;
	}
	/* if in callback, first stop callback */
	if (tc->state == TZ_NW_TASK_STATE_CALLBACK) {
		struct tz_nw_comm cc;

		/* There may be a race-condition for the state. */
		cc.call.task_id = task_id;
		cc.call.cmd_id = TZ_CMD_TEE_USER;
		cc.call.param = info->param;
		cc.callback.result = TZ_ERROR_TARGET_DEAD;
		cc.callback.origin = TZ_ORIGIN_UNTRUSTED_APP;
		tc->dead = true;
		while (1) {
			ret = tz_nw_comm_invoke_command(tc, &cc, tc->call_id,
						temp_dev_file);
			if (ret != TZ_PENDING)
				break;
		}
	}

	cmd = tzd_shm_phys_to_virt_nolock(temp_dev_file,
			(void *)((unsigned long)info->param));

	/* FIXME: it introduce too much TEE code here */
	/* close session */
	memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
	cmd->cmd_id = TASYS_CMD_CLOSE_SESSION;
	cmd->session_id = info->session_id;
	cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	ret = tzc_invoke_command(0, task_id, TZ_CMD_TEE_SYS,
			info->param, &origin, NULL, NULL);

	if (ret) {
		tz_error("fail to close session, result=0x%08x\n", ret);
		return ret;
	}

	instance_dead = cmd->params[0].value.a;
	if (instance_dead) {
		/* destroy instance */
		memset(cmd, 0, TEE_COMM_PARAM_BASIC_SIZE);
		cmd->cmd_id = TAMGR_CMD_DESTROY_INSTANCE;
		cmd->param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

		cmd->params[0].value.a = task_id;

		tzc_invoke_command(0, TZ_TASK_ID_MGR, TZ_CMD_TEE_SYS,
				info->param, &origin, NULL, NULL);
	}

	/* It should clean task client somewhere when open session.
	 * However, this logic isn't there for now.
	 * So we just clean this flag here maunally.
	 */
	tc->dead = false;

	return ret;
}

static void tzd_release_all_session(void *private_data)
{
	struct tzd_dev_file *temp_dev_file = (struct tzd_dev_file*)private_data;
	struct tzd_session_info *temp_info;
	struct tzd_session_info *temp_pos;

	mutex_lock(&temp_dev_file->tz_mutex);
	list_for_each_entry_safe(temp_info, temp_pos, &temp_dev_file->
				session_list, head) {
		list_del(&temp_info->head);
		tzd_release_session(temp_dev_file, temp_info);
		if (temp_info)
			kfree(temp_info);
	}
	mutex_unlock(&temp_dev_file->tz_mutex);

}

static int tzd_release(struct inode *inode, struct file *file)
{
	tzd_release_all_session(file->private_data);
	tzd_resource_free(file->private_data);
	tz_debug("tzd_release");
	return 0;
}

static int kernel_tzd_open(void)
{
	struct tzd_dev_file *new_dev;

	tz_debug("kernel_tzd_open");

	new_dev = kzalloc(sizeof(*new_dev), GFP_KERNEL);
	if (!new_dev) {
		tz_error("kzalloc failed for new dev file allocation");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&new_dev->dev_shm_head.shm_list);

	mutex_init(&new_dev->tz_mutex);

	INIT_LIST_HEAD(&new_dev->session_list);

	BUG_ON(kernel_dev_file != NULL);
	kernel_dev_file = new_dev;

	return 0;
}

static int kernel_tzd_release(void)
{
	tzd_release_all_session(kernel_dev_file);
	tzd_resource_free(kernel_dev_file);
	kernel_dev_file = NULL;
	tz_debug("kernel_tzd_release");
	return 0;
}

static const struct file_operations tzd_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tzd_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzd_compat_ioctl,
#endif
	.open = tzd_open,
	.mmap = tzd_mmap,
	.release = tzd_release
};

static struct class *driver_class;
static dev_t tzd_device_num;
static struct cdev tzd_cdev;

static int tzd_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *class_dev;

	ret = tzd_shm_init();
	if (ret < 0)
		return ret;

	/* init task client */
	tz_nw_task_client_init_all();

	REE_RuntimeInit();

	ret = alloc_chrdev_region(&tzd_device_num, 0, 1,
			TZ_CLIENT_DEVICE_NAME);
	if (ret < 0) {
		tz_error("alloc_chrdev_region failed %d", ret);
		goto shm_exit;
	}

	driver_class = class_create(THIS_MODULE, TZ_CLIENT_DEVICE_NAME);
	if (IS_ERR(driver_class)) {
		ret = -ENOMEM;
		tz_error("class_create failed %d", ret);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, tzd_device_num,
			NULL, TZ_CLIENT_DEVICE_NAME);
	if (!class_dev) {
		tz_error("class_device_create failed %d", ret);
		ret = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&tzd_cdev, &tzd_fops);
	tzd_cdev.owner = THIS_MODULE;

	ret = cdev_add(&tzd_cdev,
			MKDEV(MAJOR(tzd_device_num), 0), 1);
	if (ret < 0) {
		tz_error("cdev_add failed %d", ret);
		goto class_device_destroy;
	}

	ret = kernel_tzd_open();
	if (ret < 0) {
		tz_error("kernel_tzd_open failed\n");
		goto cdev_destroy;
	}

	tzlogger_init();

	return ret;

cdev_destroy:
	cdev_del(&tzd_cdev);
class_device_destroy:
	device_destroy(driver_class, tzd_device_num);
class_destroy:
	class_destroy(driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(tzd_device_num, 1);
shm_exit:
	tzd_shm_exit();
	return ret;
}

static int tzd_remove(struct platform_device *pdev)
{
	tz_debug("tz driver exit\n");
	tzd_shm_exit();
	kernel_tzd_release();
	cdev_del(&tzd_cdev);
	device_destroy(driver_class, tzd_device_num);
	class_destroy(driver_class);
	unregister_chrdev_region(tzd_device_num, 1);

	tzlogger_exit();

	return 0;
}

static struct platform_driver tzd_driver = {
	.probe		= tzd_probe,
	.remove		= tzd_remove,
	.driver = {
		.name	= "berlin-tzd",
		.owner	= THIS_MODULE,
	},
};
module_platform_driver(tzd_driver);

static int __init tzd_platdev_init(void)
{
	return PTR_ERR_OR_ZERO(platform_device_register_simple("berlin-tzd",
				-1, NULL, 0));
}
device_initcall_sync(tzd_platdev_init);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Marvell");
MODULE_DESCRIPTION("Marvell TrustZone Driver");
MODULE_VERSION("1.00");
