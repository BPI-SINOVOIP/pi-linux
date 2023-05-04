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

#ifndef __TZ_DRIVER_PRIVATE_H__
#define __TZ_DRIVER_PRIVATE_H__

struct tzd_shm_head {
	int shm_cnt;
	struct list_head shm_list;
};

struct tzd_dev_file {
	struct list_head head;
	uint32_t dev_file_id;
	struct tzd_shm_head dev_shm_head;
	struct mutex tz_mutex;
	struct list_head session_list;
};

struct tzd_shm {
	struct list_head head;
	uint32_t len;	/* size of the shm */
	void* u_addr;	/* user space address, set by mmap() */
	void* p_addr;	/* physical address, set by shm_ops->malloc() */
	void* k_addr;	/* kernel space address, set by shm_ops->malloc() */
	void* userdata;	/* user data for the shm memory allocator.
			for kmem, it's NULL; for ion, it's ion_handle */
};

/**
 * allocate share memory
 */
struct tzd_shm *tzd_shm_new(struct tzd_dev_file *dev, size_t size, gfp_t flags);

/**
 * allocate share memory and return physical address
 */
void *tzd_shm_alloc(struct tzd_dev_file *dev, size_t size, gfp_t flags);

/**
 * free allocated share memory
 */
int tzd_shm_free(struct tzd_dev_file *dev, const void *x);

/**
 * release all allocated share memory for a given
 * tzd device
 */
int tzd_shm_delete(struct tzd_dev_file *dev);

/**
 * associate a physical address with a user space
 * virtual address
 */
int tzd_shm_mmap(struct tzd_dev_file *dev, struct vm_area_struct *vma);

/**
 * return associated kernel space virtual address for
 * a given physical address. This is a nonlock version
 */
void *tzd_shm_phys_to_virt_nolock(struct tzd_dev_file *dev, void *pa);

/**
 * return associated kernel space virtual address for
 * a given physical address
 */
void *tzd_shm_phys_to_virt(struct tzd_dev_file *dev, void *pa);

/**
 * tzd shm initialzation
 */
int tzd_shm_init(void);

/**
 * tzd shm de-initialzation
 */
int tzd_shm_exit(void);

struct file;

/**
 *  * tzd logger initialzation
 *   */
int tzlogger_init(void);

/**
 *  * tzd logger de-initialzation
 *   */
void tzlogger_exit(void);

#ifdef CONFIG_COMPAT
/**
 * tzd device ioctl for 32-bit applications
 */
long tzd_compat_ioctl(struct file *file, unsigned cmd, unsigned long arg);
#endif

/**
 * tzd device ioctl
 */
long tzd_ioctl(struct file *file, unsigned cmd, unsigned long arg);

#endif
