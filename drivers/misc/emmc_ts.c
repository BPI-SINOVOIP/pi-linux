/*
 * eMMC-based transactional key-value store
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * Copyright (C) 2011, Marvell International Ltd.
 * Author: Qingwei Huang <huangqw@marvell.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 */
#include <linux/crc32.h>
#include <linux/flash_ts.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/notifier.h>

#include <linux/scatterlist.h>
#include <linux/syscalls.h>

#define DRV_NAME        	"fts"
#define DRV_VERSION     	"0.999"
#define DRV_DESC        	"eMMC-based transactional key-value storage"

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Qingwei Huang <huangqw@marvell.com>");
MODULE_LICENSE("GPL");

/* Keep in sync with 'struct flash_ts' */
#define FLASH_TS_HDR_SIZE	(4 * sizeof(u32))
#define FLASH_TS_MAX_SIZE	(16 * 1024)
#define FLASH_TS_MAX_DATA_SIZE	(FLASH_TS_MAX_SIZE - FLASH_TS_HDR_SIZE)

#define FLASH_TS_MAGIC		0x53542a46
static const char fts_dev_node[] = "/dev/block/by-name/fts";

/* Physical flash layout */
struct flash_ts {
	u32 magic;		/* "F*TS" */
	u32 crc;		/* doesn't include magic and crc fields */
	u32 len;		/* real size of data */
	u32 version;		/* generation counter, must be positive */

	/* data format is very similar to Unix environment:
	 *   key1=value1\0key2=value2\0\0
	 */
	char data[FLASH_TS_MAX_DATA_SIZE];
};

struct emmc_info {
	u32 erasesize;
	u32 writesize;
	u32 size;
};
static struct emmc_info default_emmc = {512*1024, 512, 0};

/* Internal state */
struct flash_ts_priv {
	struct mutex lock;
	struct emmc_info *emmc;

	/* chunk size, >= sizeof(struct flash_ts) */
	size_t chunk;

	/* current record offset within eMMC device */
	loff_t offset;

	/* in-memory copy of flash content */
	struct flash_ts cache;

	/* temporary buffers
	 *  - one backup for failure rollback
	 *  - another for read-after-write verification
	 */
	struct flash_ts cache_tmp_backup;
	struct flash_ts cache_tmp_verify;
};
static struct flash_ts_priv *__ts;

static int is_blank(const void *buf, size_t size)
{
	int i;
	const unsigned int *data = (const unsigned int *)buf;

	size >>= 2;
	if (data[0] != 0 && data[0] != 0xffffffff)
		return 0;
	for (i = 1; i < size; i++)
		if (data[i] != data[0])
			return 0;
	return 1;
}

static int erase_block(struct emmc_info *emmc, loff_t off)
{
	struct file *filp = NULL;
	mm_segment_t old_fs = get_fs();
	int ret, i, cnt;
	char buf[512] = {0};

	set_fs(KERNEL_DS);

	printk(KERN_INFO DRV_NAME ": erase_block off=0x%08llx size=0x%x\n", off, emmc->erasesize);

	filp = filp_open(fts_dev_node, O_WRONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}

	cnt = emmc->erasesize/sizeof(buf);
	for (i = 0; i < cnt; i++)
		if (vfs_write(filp, buf, sizeof(buf), &filp->f_pos) < sizeof(buf)) {
			printk(KERN_ERR DRV_NAME ": failed to write %s\n", fts_dev_node);
			ret = -3;
			goto err2;
		}

	ret = 0;
err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

/* erases the whole emmc fts device */
static int flash_erase_all(struct emmc_info *emmc)
{
	int res = 0;
	loff_t off = 0;

	do {
		printk(KERN_INFO DRV_NAME
			": erasing block @ 0x%08llx\n", off);
		res = erase_block(emmc, off);
		if (unlikely(res)) {
			printk(KERN_ERR DRV_NAME
				": flash_erase_all failed, errno %d\n", res);
			break;
		}
		off += emmc->erasesize;
	} while (off < emmc->size);

	return res;
}

static int flash_write(struct emmc_info *emmc, loff_t off,
		       const void *buf, size_t size)
{
	struct file *filp = NULL;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);

	printk(KERN_INFO DRV_NAME ": write off=0x%08llx size=0x%zx\n", off, size);

	filp = filp_open(fts_dev_node, O_WRONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}

	if (vfs_write(filp, buf, size, &filp->f_pos) < size) {
		printk(KERN_ERR DRV_NAME ": failed to write %s\n", fts_dev_node);
		ret = -3;
		goto err2;
	}

	/* force syncing file */
	vfs_fsync(filp, 0);

	ret = 0;
err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static int flash_read(struct emmc_info *emmc, loff_t off, void *buf, size_t size)
{
	struct file *filp = NULL;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);

	filp = filp_open(fts_dev_node, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}

	if (vfs_read(filp, buf, size, &filp->f_pos) < size) {
		printk(KERN_ERR DRV_NAME ": failed to read %s\n", fts_dev_node);
		ret = -3;
		goto err2;
	}

	ret = 0;
err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static int get_fts_dev_node(void)
{
	struct file *filp = NULL;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);

	filp = filp_open(fts_dev_node, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR DRV_NAME
		       ": failed to open %s\n", fts_dev_node);
		ret = -1;
		goto err1;
	}

	if (filp->f_op->llseek(filp, 0, SEEK_END) < 0) {
		printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
		ret = -2;
		goto err2;
	}
	ret = filp->f_pos;
	printk(KERN_INFO DRV_NAME ": fts partition is on %s, 0x%x bytes\n",
		fts_dev_node, ret);

err2:
	filp_close(filp, NULL);
err1:
	set_fs(old_fs);
	return ret;
}

static char *flash_ts_find(struct flash_ts_priv *ts, const char *key,
			   size_t key_len)
{
	char *s = ts->cache.data;
	while (*s) {
		if (!strncmp(s, key, key_len)) {
			if (s[key_len] == '=')
				return s;
		}

		s += strlen(s) + 1;
	}
	return NULL;
}

static inline u32 flash_ts_crc(const struct flash_ts *cache)
{
	/* skip magic and crc fields */
	return crc32(0, &cache->len, cache->len + 2 * sizeof(u32)) ^ ~0;
}

static void set_to_default_empty_state(struct flash_ts_priv *ts)
{
	ts->offset = ts->emmc->size - ts->chunk;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.data[0] = '\0';
	ts->cache.crc = flash_ts_crc(&ts->cache);
}

/* Verifies cache consistency and locks it */
static struct flash_ts_priv *__flash_ts_get(void)
{
	struct flash_ts_priv *ts = __ts;

	if (likely(ts)) {
		mutex_lock(&ts->lock);
	} else {
		printk(KERN_ERR DRV_NAME ": not initialized yet\n");
	}

	return ts;
}

static inline void __flash_ts_put(struct flash_ts_priv *ts)
{
	mutex_unlock(&ts->lock);
}

static int flash_ts_commit(struct flash_ts_priv *ts)
{
	struct emmc_info *emmc = ts->emmc;
	loff_t off = ts->offset + ts->chunk;
	/* we try to make two passes to handle non-erased blocks
	 * this should only matter for the inital pass over the whole device.
	 */
	int max_iterations = 10;
	size_t size = ALIGN(FLASH_TS_HDR_SIZE + ts->cache.len, emmc->writesize);

	/* fill unused part of data */
	memset(ts->cache.data + ts->cache.len, 0xff,
	       sizeof(ts->cache.data) - ts->cache.len);

	while (max_iterations--) {
		/* wrap around */
		if (off >= emmc->size)
			off = 0;
		/* write and read back to veryfy */
		if (flash_write(emmc, off, &ts->cache, size) ||
		    flash_read(emmc, off, &ts->cache_tmp_verify, size)) {
			/* next chunk */
			off += ts->chunk;
			continue;
		}

		/* compare */
		if (memcmp(&ts->cache, &ts->cache_tmp_verify, size)) {
			printk(KERN_WARNING DRV_NAME
			       ": record v%u read mismatch @ 0x%08llx\n",
				ts->cache.version, off);
			/* next chunk */
			off += ts->chunk;
			continue;
		}

		/* for new block, erase the previous block after write done,
		 * it's to speed up flash_ts_scan
		 */
		if (!(off & (emmc->erasesize - 1))) {
			loff_t pre_block_base = ts->offset & ~(emmc->erasesize - 1);
			loff_t cur_block_base = off & ~(emmc->erasesize - 1);
			if (cur_block_base != pre_block_base)
				erase_block(emmc, pre_block_base);
		}

		ts->offset = off;
		printk(KERN_INFO DRV_NAME ": record v%u commited @ 0x%08llx\n",
		       ts->cache.version, off);
		return 0;
	}

	printk(KERN_ERR DRV_NAME ": commit failure\n");
	return -EIO;
}

static int flash_ts_set(const char *key, const char *value)
{
	struct flash_ts_priv *ts;
	size_t klen = strlen(key);
	size_t vlen = strlen(value);
	int res;
	char *p;

	ts = __flash_ts_get();
	if (unlikely(!ts))
		return -EINVAL;

	/* save current cache contents so we can restore it on failure */
	memcpy(&ts->cache_tmp_backup, &ts->cache, sizeof(ts->cache_tmp_backup));

	p = flash_ts_find(ts, key, klen);
	if (p) {
		/* we are replacing existing entry,
		 * empty value (vlen == 0) removes entry completely.
		 */
		size_t cur_len = strlen(p) + 1;
		size_t new_len = vlen ? klen + 1 + vlen + 1 : 0;

		if (cur_len != new_len) {
			/* we need to move stuff around */

			if ((ts->cache.len - cur_len) + new_len >
			     sizeof(ts->cache.data))
				goto no_space;

			memmove(p + new_len, p + cur_len,
				ts->cache.len - (p - ts->cache.data + cur_len));

			ts->cache.len = (ts->cache.len - cur_len) + new_len;
		} else if (!strcmp(p + klen + 1, value)) {
			/* skip update if new value is the same as the old one */
			res = 0;
			goto out;
		}

		if (vlen) {
			p += klen + 1;
			memcpy(p, value, vlen);
			p[vlen] = '\0';
		}
	} else {
		size_t len = klen + 1 + vlen + 1;

		/* don't do anything if value is empty */
		if (!vlen) {
			res = 0;
			goto out;
		}

		if (ts->cache.len + len > sizeof(ts->cache.data))
			goto no_space;

		/* add new entry at the end */
		p = ts->cache.data + ts->cache.len - 1;
		memcpy(p, key, klen);
		p += klen;
		*p++ = '=';
		memcpy(p, value, vlen);
		p += vlen;
		*p++ = '\0';
		*p = '\0';
		ts->cache.len += len;
	}

	++ts->cache.version;
	ts->cache.crc = flash_ts_crc(&ts->cache);
	res = flash_ts_commit(ts);
	if (unlikely(res))
		memcpy(&ts->cache, &ts->cache_tmp_backup, sizeof(ts->cache));
	goto out;

no_space:
	printk(KERN_WARNING DRV_NAME ": no space left for '%s=%s'\n",
	       key, value);
	res = -ENOSPC;
out:
	__flash_ts_put(ts);

	return res;
}

static void flash_ts_get(const char *key, char *value, unsigned int size)
{
	size_t klen = strlen(key);
	struct flash_ts_priv *ts;
	const char *p;

	BUG_ON(!size);

	*value = '\0';

	ts = __flash_ts_get();
	if (unlikely(!ts))
		return;

	p = flash_ts_find(ts, key, klen);
	if (p)
		strlcpy(value, p + klen + 1, size);

	__flash_ts_put(ts);
}

/* erases the whole mmc device and re-initializes
 * the in-memory cache to default empty state
 */
static int flash_reinit(void)
{
	int res;
	struct flash_ts_priv *ts = __flash_ts_get();

	if (unlikely(!ts))
		return -EINVAL;

	/* erase the whole emmc device */
	res = flash_erase_all(ts->emmc);

	if (likely(!res)) {
		/* restore to default empty state */
		set_to_default_empty_state(ts);

		/* Fill the unused part of the cache.set_to_default_empty_state
		 * resets the cache by setting the first character to the null
		 * terminator and length to 1; this preserves that while wiping
		 * out any real data remaining in the cache.
		 */
		memset(ts->cache.data + ts->cache.len, 0xff,
		       sizeof(ts->cache.data) - ts->cache.len);
	}

	__flash_ts_put(ts);
	return res;
}

static inline u32 flash_ts_check_header(const struct flash_ts *cache)
{
	if (cache->magic == FLASH_TS_MAGIC &&
	    cache->version &&
	    cache->len && cache->len <= sizeof(cache->data) &&
	    cache->crc == flash_ts_crc(cache) &&
	    /* check correct null-termination */
	    !cache->data[cache->len - 1] &&
	    (cache->len == 1 || !cache->data[cache->len - 2])) {
		/* all is good */
		return cache->version;
	}

	return 0;
}

/* checks integrity of the emmc device and prints info about its contents
 * we don't need to check bad blocks for emmc.
 * There is eMMC controller inside eMMC storage.
 * The controller is in charge of mapping spare blocks.
*/
static int flash_ts_check(void)
{
	struct emmc_info *emmc;
	int res, bad_chunks = 0;
	loff_t off = 0;

	struct flash_ts_priv *ts = __flash_ts_get();

	if (unlikely(!ts))
		return -EINVAL;

	emmc = ts->emmc;

	do {
		u32 version;

		res = flash_read(emmc, off, &ts->cache_tmp_verify,
				 sizeof(ts->cache_tmp_verify));
		if (res) {
			printk(KERN_WARNING DRV_NAME
			       ": could not read flash @ 0x%08llx\n", off);
			off += ts->chunk;
			continue;
		}

		version = flash_ts_check_header(&ts->cache_tmp_verify);
		if (version == 0) {
			if (is_blank(&ts->cache_tmp_verify,
					   sizeof(ts->cache_tmp_verify))) {
				/* skip the whole block if chunk is blank */
				printk(KERN_INFO DRV_NAME
				       ": blank chunk @ 0x%08llx\n", off);
				off = (off + emmc->erasesize) & ~(emmc->erasesize - 1);
			} else {
				/* header didn't check out and flash is not blank */
				printk(KERN_ERR DRV_NAME
				       ": bad chunk @ 0x%08llx\n", off);
				++bad_chunks;
				off += ts->chunk;
			}
		} else {
			/* header checked out, so move on */
			printk(KERN_INFO DRV_NAME
			       ": record v%u @ 0x%08llx\n", version, off);
			off += ts->chunk;
		}
	} while (off < emmc->size);

	if (unlikely(bad_chunks)) {
		printk(KERN_ERR DRV_NAME ": %d bad chunks\n", bad_chunks);
		__flash_ts_put(ts);
		return -EIO;
	}

	__flash_ts_put(ts);
	return 0;
}

static int flash_ts_scan(struct flash_ts_priv *ts)
{
	struct emmc_info *emmc = ts->emmc;
	int skiped_blocks = 0;
	loff_t off = 0;
	struct file *filp = NULL;
	mm_segment_t old_fs = get_fs();
	u32 version;

	set_fs(KERNEL_DS);

	filp = filp_open(fts_dev_node, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		set_fs(old_fs);
		printk(KERN_ERR DRV_NAME
		       ": %s, failed to open %s\n", __FUNCTION__, fts_dev_node);
		return PTR_ERR(filp);
	}

	do {
		if (filp->f_op->llseek(filp, off, SEEK_SET) < 0) {
			printk(KERN_ERR DRV_NAME ": failed to llseek %s\n", fts_dev_node);
			off += ts->chunk;
			continue;
		}

		if (vfs_read(filp, (char *)&ts->cache_tmp_verify, sizeof(ts->cache_tmp_verify), &filp->f_pos)
						< sizeof(ts->cache_tmp_verify)) {
			printk(KERN_ERR DRV_NAME ": failed to read %s\n", fts_dev_node);
			off += ts->chunk;
			continue;
		}

		version = flash_ts_check_header(&ts->cache_tmp_verify);
		if (version > ts->cache.version) {
			memcpy(&ts->cache, &ts->cache_tmp_verify,
			       sizeof(ts->cache));
			ts->offset = off;
		}
		if (0 == version &&
		    is_blank(&ts->cache_tmp_verify,
			     sizeof(ts->cache_tmp_verify))) {
			/* skip the whole block if chunk is blank */
			off = (off + emmc->erasesize) & ~(emmc->erasesize - 1);
			skiped_blocks++;
		} else {
			off += ts->chunk;
		}
	} while (off < emmc->size);


	filp_close(filp, NULL);
	set_fs(old_fs);

	printk(KERN_INFO DRV_NAME ": flash_ts_scan skiped %d blocks\n", skiped_blocks);
	return 0;
}

/* User-space access */
struct flash_ts_dev {
	struct mutex lock;
	struct flash_ts_io_req req;
};

/* Round-up to the next power-of-2,
 * from "Hacker's Delight" by Henry S. Warren.
 */
static inline u32 clp2(u32 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

static int flash_ts_open(struct inode *inode, struct file *file)
{
	struct flash_ts_priv *ts;
	int res, size;
	struct flash_ts_dev *dev;

	size = get_fts_dev_node();
	if (size <= 0)
		return -ENOMEM;

	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (unlikely(!dev))
		return -ENOENT;

	mutex_init(&dev->lock);
	file->private_data = dev;

	ts = __flash_ts_get();
	if (unlikely(!ts)) {
		res = -EINVAL;
		goto err1;
	}

	/* determine chunk size so it doesn't cross block boundary,
	 * is multiple of page size and there is no wasted space in a block.
	 * We assume page and block sizes are power-of-2.
	 */
	ts->emmc->size = size;
	ts->chunk = clp2((sizeof(struct flash_ts) + ts->emmc->writesize - 1) &
			  ~(ts->emmc->writesize - 1));
	if (unlikely(ts->chunk > ts->emmc->erasesize)) {
		res = -ENODEV;
		printk(KERN_ERR DRV_NAME ": eMMC block size is too small\n");
		goto err2;
	}

	printk(KERN_INFO DRV_NAME ": chunk: 0x%zx\n", ts->chunk);

	/* default empty state */
	ts->offset = ts->emmc->size - ts->chunk;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.data[0] = '\0';
	ts->cache.crc = flash_ts_crc(&ts->cache);

	/* scan flash partition for the most recent record */
	res = flash_ts_scan(ts);
	if (ts->cache.version)
		printk(KERN_INFO DRV_NAME ": v%u loaded from 0x%08llx\n",
		       ts->cache.version, ts->offset);
	__flash_ts_put(ts);
	return 0;

err2:
	__flash_ts_put(ts);
err1:
	kfree(dev);
	return res;
}

static int flash_ts_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static long flash_ts_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	struct flash_ts_dev *dev = file->private_data;
	struct flash_ts_io_req *req = &dev->req;
	int res;

	if (unlikely(mutex_lock_interruptible(&dev->lock)))
		return -ERESTARTSYS;

	if (unlikely(copy_from_user(req, (const void* __user)arg,
				    sizeof(*req)))) {
		res = -EFAULT;
		goto out;
	}

	req->key[sizeof(req->key) - 1] = '\0';

	switch (cmd) {
	case FLASH_TS_IO_SET:
		req->val[sizeof(req->val) - 1] = '\0';
		res = flash_ts_set(req->key, req->val);
		break;

	case FLASH_TS_IO_GET:
		flash_ts_get(req->key, req->val, sizeof(req->val));
		res = copy_to_user((void* __user)arg, req,
				   sizeof(*req)) ? -EFAULT : 0;
		break;

	case FLASH_TS_IO_REINIT:
		res = flash_reinit();
		break;

	case FLASH_TS_IO_CHECK:
		res = flash_ts_check();
		break;

	default:
		res = -ENOTTY;
	}

out:
	mutex_unlock(&dev->lock);
	return res;
}

static struct file_operations flash_ts_fops = {
	.owner = THIS_MODULE,
	.open = flash_ts_open,
	.unlocked_ioctl = flash_ts_ioctl,
	.compat_ioctl = flash_ts_ioctl,
	.release = flash_ts_release,
};

static struct miscdevice flash_ts_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DRV_NAME,
	.fops = &flash_ts_fops,
};

/* Debugging (procfs) */
static void *flash_ts_proc_start(struct seq_file *m, loff_t *pos)
{
	if (*pos == 0) {
		struct flash_ts_priv *ts = __flash_ts_get();
		if (ts) {
			BUG_ON(m->private);
			m->private = ts;
			return ts->cache.data;
		}
	}

	*pos = 0;
	return NULL;
}

static void *flash_ts_proc_next(struct seq_file *m, void *v, loff_t *pos)
{
	char *s = (char *)v;
	s += strlen(s) + 1;
	++(*pos);
	return *s ? s : NULL;
}

static void flash_ts_proc_stop(struct seq_file *m, void *v)
{
	struct flash_ts_priv *ts = m->private;
	if (ts) {
		m->private = NULL;
		__flash_ts_put(ts);
	}
}

static int flash_ts_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", (char*)v);
	return 0;
}

static struct seq_operations flash_ts_seq_ops = {
	.start	= flash_ts_proc_start,
	.next	= flash_ts_proc_next,
	.stop	= flash_ts_proc_stop,
	.show	= flash_ts_proc_show,
};

static int flash_ts_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &flash_ts_seq_ops);
}

static const struct file_operations flash_ts_proc_fops = {
	.owner = THIS_MODULE,
	.open = flash_ts_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*
 * BCB (boot control block) support
 * Handle reboot command and set boot params for bootloader
 */
static int bcb_fts_reboot_hook(struct notifier_block *notifier,
                   unsigned long code, void *cmd)
{
	struct file file;
	char orig[128], *opts, *token;
	bool opt_quiescent = false;
	enum {
		NOTHING,
		RECOVERY,
		BACKUPSYS,
		BOOTLOADER,
	} flag = NOTHING;

	if (code == SYS_RESTART && cmd) {
		strncpy(orig, cmd, sizeof(orig)-1);
		opts = &orig[0];

		token = strsep(&opts, ",");
		if (!strcmp(token, "recovery"))
			flag = RECOVERY;
		else if (!strcmp(token, "backupsys"))
			flag = BACKUPSYS;
		else if (!strcmp(token, "bootloader"))
			flag = BOOTLOADER;

		do {
			if (!strcmp(token, "quiescent")) {
				opt_quiescent = true;
			}
		} while ((token = strsep(&opts, ",")) != NULL);
	}

	if (flag == NOTHING && !opt_quiescent)
		return NOTIFY_DONE;

	if (flash_ts_open(NULL, &file)) {
		printk(KERN_ERR "Failed to open fts device.\n");
		return NOTIFY_DONE;
	}

	switch (flag) {
	case RECOVERY:
		if (flash_ts_set("bootloader.command", "boot-recovery"))
			printk(KERN_ERR "Failed to set bootloader command\n");
		break;

	case BACKUPSYS:
		if (flash_ts_set("bootloader.command", "boot-backupsys") ||
				flash_ts_set("bootloader.status", "") ||
				flash_ts_set("bootloader.recovery", ""))
			printk(KERN_ERR "Failed to set bootloader command\n");
		break;

	case BOOTLOADER:
		if (flash_ts_set("bootloader.command", "bootonce-bootloader") ||
				flash_ts_set("bootloader.status", "") ||
				flash_ts_set("bootloader.recovery", ""))
			printk(KERN_ERR "Failed to set bootloader command\n");
		break;

	default:
		break;
	}

	if (opt_quiescent) {
		if (flash_ts_set("bootloader.opt.quiescent", "true"))
			printk(KERN_ERR "Failed to set bootloader.opt.quiescent command\n");
	}

	flash_ts_release(NULL, &file);

	return NOTIFY_DONE;
}

static struct notifier_block reboot_notifier = {
	.notifier_call	= bcb_fts_reboot_hook,
	.priority	= 128,
};

static int __init flash_ts_init(void)
{
	struct flash_ts_priv *ts;
	struct emmc_info *emmc = &default_emmc;
	int res;

	/* make sure both page and block sizes are power-of-2
	 * (this will make chunk size determination simpler).
	 */
	if (unlikely(!is_power_of_2(emmc->writesize) ||
		     !is_power_of_2(emmc->erasesize) ||
		     emmc->erasesize < sizeof(struct flash_ts))) {
		res = -ENODEV;
		printk(KERN_ERR DRV_NAME ": unsupported eMMC geometry\n");
		return res;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (unlikely(!ts)) {
		res = -ENOMEM;
		printk(KERN_ERR DRV_NAME ": failed to allocate memory\n");
		return res;
	}

	mutex_init(&ts->lock);
	ts->emmc = emmc;

	res = misc_register(&flash_ts_miscdev);
	if (unlikely(res))
		goto out_free;

	smp_mb();
	__ts = ts;

	proc_create(DRV_NAME, 0, NULL, &flash_ts_proc_fops);

	/* Register optional reboot hook */
	register_reboot_notifier(&reboot_notifier);

	return 0;

out_free:
	kfree(ts);

	return res;
}

/* Make sure eMMC subsystem is already initialized */
late_initcall(flash_ts_init);
