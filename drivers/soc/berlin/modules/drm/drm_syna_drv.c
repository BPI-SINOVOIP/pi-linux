// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/component.h>
#include <linux/of_platform.h>
#include <linux/kthread.h>
#include <drm/drmP.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic_helper.h>

#include "drm_syna_drv.h"
#include "drm_syna_gem.h"
#include "syna_drm.h"
#include "syna_vpp.h"

#define DRIVER_NAME "synaptics"
#define DRIVER_DESC "Synaptics DRM Display Driver"
#define DRIVER_DATE "20200101"

static bool display_enable = true;
static struct task_struct *thread;

module_param(display_enable, bool, 0444);
MODULE_PARM_DESC(display_enable, "Enable all displays (default: Y)");

static void syna_irq_handler(void *data)
{
	struct drm_device *dev = data;
	struct drm_crtc *crtc;

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
		syna_crtc_irq_handler(crtc);
}

static int vblank_thread(void *parameter)
{
	DRM_DEBUG_VBL("%s %d\n", __func__, __LINE__);

	while (!kthread_should_stop()) {
		syna_vpp_wait_vsync();

		syna_irq_handler(parameter);
	}

	DRM_INFO("%s exit now\n", __func__);

	return 0;
}

static int syna_early_load(struct drm_device *dev)
{
	struct syna_drm_private *dev_priv;
	int err;

	DRM_DEBUG("loading %s device\n", to_platform_device(dev->dev)->name);

	platform_set_drvdata(to_platform_device(dev->dev), dev);

	dev_priv = kzalloc(sizeof(*dev_priv), GFP_KERNEL);
	if (!dev_priv)
		return -ENOMEM;

	dev->dev_private = dev_priv;
	dev_priv->dev = dev;
	dev_priv->display_enabled = display_enable;

	err = syna_modeset_early_init(dev_priv);
	if (err) {
		DRM_ERROR("early modeset initialisation failed (err=%d)\n",
			  err);
		goto err_gem_cleanup;
	}

	err = drm_vblank_init(dev_priv->dev, 1);
	if (err) {
		DRM_ERROR("failed to complete vblank init (err=%d)\n", err);
		goto err_modeset_late_cleanup;
	}

	thread = kthread_run(vblank_thread, dev, "Vblank_Thread");
	if (IS_ERR(thread)) {
		pr_err("Failed to vblank thread.\n");
		err = PTR_ERR(thread);
		goto err_modeset_late_cleanup;
	}

	dev->irq_enabled = true;
	err = syna_gem_init();
	if (err != 0) {
		goto err_thread_cleanup;
	}

	return 0;

err_thread_cleanup:
	kthread_stop(thread);
	thread = NULL;
err_modeset_late_cleanup:
	syna_modeset_late_cleanup(dev_priv);
err_gem_cleanup:
	kfree(dev_priv);
	return err;
}

static int syna_late_load(struct drm_device *dev)
{
	struct syna_drm_private *dev_priv = dev->dev_private;
	int err;

	if (!dev_priv) {
		DRM_ERROR("%s %d syna dev is NULL!!\n", __func__, __LINE__);
		return -1;
	}
	err = syna_modeset_late_init(dev_priv);
	if (err) {
		DRM_ERROR("late modeset initialisation failed (err=%d)\n", err);
		return err;
	}

	return 0;
}

static void syna_early_unload(struct drm_device *dev)
{
	if (thread)
		kthread_stop(thread);
}

static void syna_late_unload(struct drm_device *dev)
{
	struct syna_drm_private *dev_priv;

	DRM_INFO("unloading %s device.\n", to_platform_device(dev->dev)->name);

	dev_priv = dev->dev_private;
	syna_modeset_late_cleanup(dev_priv);

	syna_gem_deinit();
	kfree(dev_priv);
}

static void syna_lastclose(struct drm_device *dev)
{
	drm_atomic_helper_shutdown(dev);
}

static int syna_enable_vblank(struct drm_device *dev, unsigned int crtc)
{
	struct syna_drm_private *dev_priv;

	dev_priv = dev->dev_private;
	switch (crtc) {
	case 0:
		syna_crtc_set_vblank_enabled(dev_priv->crtc, true);
		break;
	default:
		DRM_ERROR("invalid crtc %u\n", crtc);
		return -EINVAL;
	}

	DRM_DEBUG("vblank interrupts enabled for crtc %d\n", crtc);

	return 0;
}

static void syna_disable_vblank(struct drm_device *dev, unsigned int crtc)
{
	struct syna_drm_private *dev_priv;

	dev_priv = dev->dev_private;
	switch (crtc) {
	case 0:
		syna_crtc_set_vblank_enabled(dev_priv->crtc, false);
		break;
	default:
		DRM_ERROR("invalid crtc %u\n", crtc);
		return;
	}

	DRM_DEBUG("vblank interrupts disabled for crtc %d\n", crtc);
}

static int syna_gem_object_create_ioctl(struct drm_device *dev,
					void *data, struct drm_file *file)
{
	return syna_gem_object_create_ioctl_priv(dev, data, file);
}

static void syna_gem_object_free(struct drm_gem_object *obj)
{
	syna_gem_object_free_priv(obj);
}

static const struct vm_operations_struct syna_gem_vm_ops = {
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static const struct drm_ioctl_desc syna_ioctls[] = {
	DRM_IOCTL_DEF_DRV(SYNA_GEM_CREATE, syna_gem_object_create_ioctl,
			  DRM_AUTH | DRM_UNLOCKED | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SYNA_GEM_MMAP, syna_gem_object_mmap_ioctl,
			  DRM_AUTH | DRM_UNLOCKED | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SYNA_GEM_CPU_PREP, syna_gem_object_cpu_prep_ioctl,
			  DRM_AUTH | DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(SYNA_GEM_CPU_FINI, syna_gem_object_cpu_fini_ioctl,
			  DRM_AUTH | DRM_UNLOCKED),
};

/* drm driver mmap file operations */
static int syna_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_gem_object *obj;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret)
		return ret;

	/*
	 * Set vm_pgoff (used as a fake buffer offset by DRM) to 0 and map the
	 * whole buffer from the start.
	 */
	vma->vm_pgoff = 0;

	obj = vma->vm_private_data;

	return syna_drm_gem_object_mmap(obj, vma);
}

static const struct file_operations syna_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = syna_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
	.llseek = noop_llseek,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
};

static struct drm_driver syna_drm_driver = {
	.lastclose = syna_lastclose,
	.enable_vblank = syna_enable_vblank,
	.disable_vblank = syna_disable_vblank,
#ifdef CONFIG_DEBUG_FS
	.debugfs_init = syna_debugfs_init,
#endif
	.gem_free_object = syna_gem_object_free,
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table = syna_gem_prime_get_sg_table,
	.gem_prime_mmap = syna_gem_mmap_buf,
	.dumb_create = syna_gem_dumb_create,
	.dumb_map_offset = syna_gem_dumb_map_offset,
	.gem_vm_ops = &syna_gem_vm_ops,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = SYNA_VERSION_MAJ,
	.minor = SYNA_VERSION_MIN,
	.patchlevel = SYNA_VERSION_BUILD,

	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,

	.ioctls = syna_ioctls,
	.num_ioctls = ARRAY_SIZE(syna_ioctls),
	.fops = &syna_driver_fops,
};

static int syna_probe(struct platform_device *pdev)
{
	struct drm_device *ddev;
	int ret;

	ddev = drm_dev_alloc(&syna_drm_driver, &pdev->dev);

	if (IS_ERR(ddev)) {
		DRM_ERROR("%s %d fail to alloc drm dev!!\n",
			  __func__, __LINE__);
		return PTR_ERR(ddev);
	}

	ret = syna_early_load(ddev);
	if (ret)
		goto err_drm_dev_put;

	ret = drm_dev_register(ddev, 0);
	if (ret)
		goto err_drm_dev_late_unload;

	ret = syna_late_load(ddev);
	if (ret)
		goto err_drm_dev_unregister;

	return 0;

err_drm_dev_unregister:
	drm_dev_unregister(ddev);
err_drm_dev_late_unload:
	syna_late_unload(ddev);
err_drm_dev_put:
	drm_dev_put(ddev);
	return ret;
}

static int syna_remove(struct platform_device *pdev)
{
	struct drm_device *ddev = platform_get_drvdata(pdev);

	syna_early_unload(ddev);

	drm_dev_unregister(ddev);

	syna_late_unload(ddev);

	drm_dev_put(ddev);

	return 0;
}

static void syna_shutdown(struct platform_device *pdev)
{
}

static const struct of_device_id drm_match[] = {
	{.compatible = "syna,berlin-drm",},
	{},
};

static struct platform_driver syna_platform_driver = {
	.probe = syna_probe,
	.remove = syna_remove,
	.shutdown = syna_shutdown,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = DRIVER_NAME,
		   .of_match_table = drm_match,
	},
};

static int __init syna_init(void)
{
	int err;

	err = platform_driver_register(&syna_platform_driver);

	if (err) {
		DRM_ERROR("%s:%d platform_driver_register fail\n",
			__func__, __LINE__);
		return err;
	}

	return 0;
}

static void __exit syna_exit(void)
{
	DRM_DEBUG_DRIVER("%s:%d\n", __func__, __LINE__);

	platform_driver_unregister(&syna_platform_driver);
	syna_vpp_exit();
}

late_initcall(syna_init);
module_exit(syna_exit);

MODULE_AUTHOR("Synaptics");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual MIT/GPL");
