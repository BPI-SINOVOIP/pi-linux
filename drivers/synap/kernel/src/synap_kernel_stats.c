#include "synap_kernel.h"
#include "synap_profile.h"
#include "synap_kernel_log.h"

#include <linux/vmalloc.h>

#define BIN_ATTR_BUF_LEN 8 * PAGE_SIZE
#define NETWORK_SHOW_LINES_MAX 20
#define NETWORK_STORE_BUF_LEN_MAX 32

static struct synap_device * synap_statistics_get_synap_device(struct device *dev) {
    struct miscdevice *md = (struct miscdevice *) dev_get_drvdata(dev);
    return container_of(md, struct synap_device, misc_dev);
}

static void synap_statistics_clear(struct synap_device *d) {
    d->inference_count = 0;
    d->inference_time_us = 0;
}

static ssize_t synap_inference_count_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct synap_device *d = synap_statistics_get_synap_device(dev);
    KLOGI("synap_inference_count_show for dev: %p, data: %p", dev, d);
    if (!d) {
        KLOGE("synap_inference_count_show for dev: %p, data: %p", dev, d);
        return snprintf(buf, PAGE_SIZE, "no drvdata\n");
    }
    return snprintf(buf, PAGE_SIZE, "%llu\n", d->inference_count);
}

static ssize_t synap_statistics_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct synap_device *d = synap_statistics_get_synap_device(dev);

    struct synap_file *inst;
    struct synap_network *n;
    s32 nid;

    KLOGI("synap_inference_count_store for dev: %p, data: %p, count: %zu : %.*s", dev, d, count, count, buf);

    mutex_lock(&d->files_mutex);

    synap_statistics_clear(d);

    list_for_each_entry(inst, &d->files, list) {

        mutex_lock(&inst->mutex);

        idr_for_each_entry(&inst->networks, n, nid) {
            n->inference_count = 0;
            n->inference_time_us = 0;
            n->last_inference_time_us = 0;
        }
        mutex_unlock(&inst->mutex);
    }

    mutex_unlock(&d->files_mutex);

    return count;
}

static ssize_t synap_inference_time_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct synap_device *d = synap_statistics_get_synap_device(dev);

    KLOGI("synap_inference_time_show for dev: %p, data: %p", dev, d);
    if (!d) {
        KLOGE("synap_inference_time_show for dev: %p, data: %p", dev, d);
        return snprintf(buf, PAGE_SIZE, "no drvdata\n");
    }
    return snprintf(buf, PAGE_SIZE, "%llu\n", d->inference_time_us);
}

static ssize_t synap_networks_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{

    char *p;
    unsigned long idx, lines;
    char* tmp;
    char strbuf[NETWORK_STORE_BUF_LEN_MAX] = {0};
    struct synap_device *d = synap_statistics_get_synap_device(dev);

    if (strlen(buf) >= NETWORK_STORE_BUF_LEN_MAX) {
        KLOGE("invalid input param");
        return count;
    }

    KLOGI("synap_inference_count_store for dev: %p, data: %p, count: %zu : %.*s", dev, d, count, buf);

    strncpy(strbuf, buf, strlen(buf));
    tmp = strbuf;

    p = strsep(&tmp, " ");
    if (!p || !*p || *p == '\n') {
        d->profile_request_idx = 0;
        d->profile_request_lines = 0;
        return count;
    }
    if (kstrtol(p, 10, &idx) < 0) {
        KLOGE("strtol idx failed");
        return count;
    }
    if (idx < 0) {
        KLOGE("idx < 0");
        return count;
    }

    p = strsep(&tmp, " ");
    if (!p || !*p) {
        KLOGE("strsep buf failed");
        return count;
    }
    if (kstrtol(p, 10, &lines) < 0) {
        KLOGE("strtol lines failed")
        return count;
    }
    if (lines< 0) {
        KLOGE("lines < 0");
        return count;
    }

    if (lines > NETWORK_SHOW_LINES_MAX) {
        lines = NETWORK_SHOW_LINES_MAX;
    }

    d->profile_request_idx = idx;
    d->profile_request_lines = lines;

    return count;
}

// sysfs `show` function buf size is limited to PAGE_SIZE
static ssize_t synap_networks_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct synap_file *inst;
    struct synap_network *n;
    s32 nid;

    struct synap_device *d = synap_statistics_get_synap_device(dev);

    size_t remaining = PAGE_SIZE;

    KLOGI("synap_networks_show for dev: %p, data: %p", dev, d);

    mutex_lock(&d->files_mutex);

    list_for_each_entry(inst, &d->files, list) {

        mutex_lock(&inst->mutex);

        idr_for_each_entry(&inst->networks, n, nid) {

            size_t len = snprintf(&buf[PAGE_SIZE - remaining], remaining,
                "pid: %lu, nid: %u, inference_count: %llu, inference_time: %llu, inference_last: %u, iobuf_count: %d, iobuf_size: %d, layers: %d\n",
                (unsigned long)n->inst->pid, n->nidr, n->inference_count, n->inference_time_us, n->last_inference_time_us,
                n->io_buff_count, n->io_buff_size, n->layer_count
            );

            remaining = remaining > len? remaining - len : 0;

            if (n->is_profile_mode && d->profile_request_lines > 0) {
                u32 j;
                u32 idx = d->profile_request_idx;

                struct synap_profile_layer *layers = (struct synap_profile_layer *)n->layer_profile;

                len = snprintf(&buf[PAGE_SIZE - remaining], remaining,
                               "| lyr |    cycle | time_us |  byte_rd |  byte_wr | ot | name\n");

                remaining = remaining > len? remaining - len : 0;

                for (j = idx; j < d->profile_request_lines + idx && j < n->layer_count; j++) {
                    const struct synap_profile_layer * lyr = &layers[j];
                    len = snprintf(&buf[PAGE_SIZE - remaining], remaining,
                                   "|%4d |%9d |%8d |%9d |%9d |%3s | %s\n",
                                   j, lyr->cycle, lyr->execution_time, lyr->byte_read, lyr->byte_write, 
                                   synap_operation_type2str(lyr->type), lyr->name);
                    remaining = remaining > len? remaining - len : 0;
                }
            }
        }

        mutex_unlock(&inst->mutex);
    }

    mutex_unlock(&d->files_mutex);

    return PAGE_SIZE - remaining;
}

static ssize_t synap_statistics_network_profile_read(struct file *fp, struct kobject *kobj,
                                                     struct bin_attribute *attr, char *buf,
                                                     loff_t off, size_t count)
{

    struct synap_file *inst;
    struct synap_network *n;
    s32 nid;
    struct device *dev;
    struct synap_device *d;
    const size_t buf_len = count + off;
    size_t remaining = buf_len;
    ssize_t used = 0;
    char *temp_buffer;

    // off can be negative, we don't support that
    if (off < 0) {
        return 0;
    }

    // allocate a big buffer that can contain the maximal size we support, use vzalloc to ensure
    // it doesn't fail if contiguous memory cannot be found
    temp_buffer = vzalloc(remaining);

    // in case the allocation fails we need to return immediately
    if (temp_buffer == NULL) {
        KLOGE("failed to create temp buffer");
        return 0;
    }


    dev = kobj_to_dev(kobj);
    d = synap_statistics_get_synap_device(dev);

    // when we get this lock no network can disappear / be addedd
    mutex_lock(&d->files_mutex);

    list_for_each_entry(inst, &d->files, list) {

        // when we get this lock no ioctl can be executed in parallel on this network
        mutex_lock(&inst->mutex);

        idr_for_each_entry(&inst->networks, n, nid) {

            size_t len = snprintf(&temp_buffer[buf_len - remaining], remaining,
                "pid: %lu, nid: %u, inference_count: %llu, inference_time: %llu, inference_last: %u, iobuf_count: %d, iobuf_size: %d, layers: %d\n",
                (unsigned long)n->inst->pid, n->nidr, n->inference_count, n->inference_time_us, n->last_inference_time_us,
                n->io_buff_count, n->io_buff_size, n->layer_count
            );
            remaining = remaining > len? remaining - len : 0;
            
            // print out the profile data in our maximal size buffer
            if (n->is_profile_mode) {
                u32 j;
                struct synap_profile_layer *layers;
                size_t len;

                layers = (struct synap_profile_layer *)n->layer_profile;

                len = snprintf(&temp_buffer[buf_len - remaining], remaining,
                               "| lyr |    cycle | time_us |  byte_rd |  byte_wr | ot | name\n");

                remaining = remaining > len? remaining - len : 0;

                for (j = 0; j < n->layer_count; j++) {
                    const struct synap_profile_layer * lyr = &layers[j];
                    len = snprintf(&temp_buffer[buf_len - remaining], remaining,
                                   "|%4d |%9d |%8d |%9d |%9d |%3s | %s\n",
                                   j, lyr->cycle, lyr->execution_time, lyr->byte_read, lyr->byte_write,
                                   synap_operation_type2str(lyr->type), lyr->name);
                    remaining = remaining > len? remaining - len : 0;
                }
            }
        }

        mutex_unlock(&inst->mutex);
    }

    mutex_unlock(&d->files_mutex);

    // copy the part of the buffer that the caller has requested in the buf variable
    // notice that the caller expects data from the start of buf
    used = (buf_len - remaining) - off;
    used =  used < 0 ? 0: used > count? count : used;
    memcpy(buf, temp_buffer + off, used);

    vfree(temp_buffer);

    return used;

}

static DEVICE_ATTR(inference_count, S_IRUGO | S_IWUSR, synap_inference_count_show, synap_statistics_store);
static DEVICE_ATTR(inference_time, S_IRUGO | S_IWUSR, synap_inference_time_show, synap_statistics_store);
static DEVICE_ATTR(networks, S_IRUGO | S_IWUSR, synap_networks_show, synap_networks_store);

// We specify a file-size of 0 so that it can be any lenght and it is not truncated by the kernel
// in https://elixir.bootlin.com/linux/latest/source/fs/sysfs/file.c#L77
static BIN_ATTR(network_profile, S_IRUGO, synap_statistics_network_profile_read, NULL, 0);

static struct attribute *synap_statistics_attrs[] = {
    &dev_attr_inference_count.attr,
    &dev_attr_inference_time.attr,
    &dev_attr_networks.attr,
    NULL
};

static struct bin_attribute *synap_bin_attrs[] = {
    &bin_attr_network_profile,
    NULL
};

static const struct attribute_group synap_statistics_group = {
    .name = "statistics",
    .attrs = synap_statistics_attrs,
    .bin_attrs = synap_bin_attrs,
};

const struct attribute_group *synap_device_groups[] = {
    &synap_statistics_group,
    NULL
};
