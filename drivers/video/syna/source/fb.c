// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>

#include "vpp_api.h"
#include "vpp_mem.h"
#include "tz_driver.h"
#include "avio_common.h"

//Enable to push a black frame upon bootup-This is must when fastlogo.ta is used
#define SYNAFB_VPP_ENABLE_BLACKFRAME_ON_BOOTUP

static vpp_config_params vpp_config_param = { 0 };
static bool is_vpp_initialized;

static struct fb_var_screeninfo mrvl_fb_var = {
	.xres = MV_SYSTEM_WIDTH_DEFAULT,
	.yres =	MV_SYSTEM_HEIGHT_DEFAULT,
	.xres_virtual = MV_SYSTEM_WIDTH_DEFAULT,
	.yres_virtual = MV_SYSTEM_HEIGHT_DEFAULT,
	.bits_per_pixel = 32,
	.red = { 0, 8, 0 },
	.green = { 0, 8, 0 },
	.blue = { 0, 8, 0 },
	.activate = FB_ACTIVATE_NOW,
	.height = -1,
	.width = -1,
	.pixclock = 1487500,
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 64,
	.vsync_len = 2,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo mrvl_fb_fix = {
	.id = "SYNAFB",
	.smem_start = 0,
	.smem_len = 0,
	.type = FB_TYPE_PACKED_PIXELS,
	.type_aux = 0,
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 1,
	.ypanstep = 1,
	.ywrapstep = 1,
	.line_length = 32,
	.mmio_start = 0,
	.mmio_len = 0,
	.accel = FB_ACCEL_NONE,
};

static int mrvl_fb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info);
static int mrvl_fb_set_par(struct fb_info *info);
static int mrvl_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info);
static int mrvl_fb_blank(int blank, struct fb_info *info);
static int mrvl_fb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info);
static int mrvl_fb_mmap(struct fb_info *info,
			struct vm_area_struct *vma);
static int mrvl_fb_open(struct fb_info *info, int user);
static int mrvl_fb_release(struct fb_info *info, int user);

static struct fb_ops mrvl_fb_ops = {
	.owner          = THIS_MODULE,
	.fb_open        = mrvl_fb_open,
	.fb_release     = mrvl_fb_release,
	.fb_check_var   = mrvl_fb_check_var,
	.fb_set_par     = mrvl_fb_set_par,
	.fb_setcolreg	= mrvl_fb_setcolreg,
	.fb_blank       = mrvl_fb_blank,
	.fb_pan_display = mrvl_fb_pan_display,
	.fb_mmap        = mrvl_fb_mmap,
};

static atomic_t mrvl_fb_open_cnt = ATOMIC_INIT(0);

static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return length;
}

static int mrvl_fb_release(struct fb_info *info, int user)
{
	if (atomic_dec_return(&mrvl_fb_open_cnt) < 0)
		return -EBUSY; /* already released */
	return 0;
}

static int mrvl_fb_setup(struct fb_info *info)
{
	int ret;
	struct mrvl_vpp_dev *vdev = info->par;

	ret = MV_VPP_InitMemory(info->device);
	if (ret != 0) {
		pr_err("Failed to initialize memory\n");
		return -ENOMEM;
	}

	ret = MV_VPP_AllocateMemory(vdev, mrvl_fb_var.yres*vpp_config_param.fb_count,
		mrvl_fb_fix.line_length);
	if (ret != 0) {
		MV_VPP_DeinitMemory();
		pr_err("Failed to allocate screenbase in Open\n");
		return -ENOMEM;
	}

	ret = MV_VPP_Init();
	if (ret) {
		MV_VPP_FreeMemory(vdev);
		MV_VPP_DeinitMemory();
		pr_err("VPP Init failed\n");
		return -ENOMEM;
	}

	info->screen_base = (char *)vdev->ionvaddr;
	mrvl_fb_fix.smem_start = (unsigned long)vdev->ionvaddr;
	is_vpp_initialized = true;

	return 0;
}

static int mrvl_fb_open(struct fb_info *info, int user)
{
	int ret;

	if (atomic_inc_return(&mrvl_fb_open_cnt) > 1)
		return -EBUSY; /* already open */

	if (!is_vpp_initialized) {
		ret = mrvl_fb_setup(info);
		if (ret) {
			pr_err("FB setup FAILED\n");
			return ret;
		}
#ifdef SYNAFB_VPP_ENABLE_BLACKFRAME_ON_BOOTUP
		MV_VPP_DisplayFrame(info->par, 0, info->var.yres,
			info->var.xres, info->var.yres, info->fix.line_length);
#endif //SYNAFB_VPP_ENABLE_BLACKFRAME_ON_BOOTUP
	}

	ret = MV_VPP_Config();
	if (ret) {
		pr_err("VPP Config failed\n");
	}
	return ret;
}

static int mrvl_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info)
{
	return -EINVAL;
}

static int mrvl_fb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info)
{
	unsigned long line_length;

	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;
	if (var->bits_per_pixel < 32)
		return -EINVAL;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	line_length =
		get_line_length(var->xres_virtual, var->bits_per_pixel);
	switch (var->bits_per_pixel) {
	case 32:/* RGBA 8888 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 16;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	}
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

static int mrvl_fb_set_par(struct fb_info *info)
{
	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);
	return 0;
}

static int mrvl_fb_blank(int blank, struct fb_info *info)
{
	int ret = 0;

	pr_info("%s: blank %d\n", __func__, blank);
	switch (blank) {
	case FB_BLANK_UNBLANK:
		ret = MV_VPP_SetHdmiTxControl(1);
		break;
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_POWERDOWN:
		ret = MV_VPP_SetHdmiTxControl(0);
		break;
	}
	return 0;
}

static int mrvl_fb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info)
{
	struct mrvl_vpp_dev *vdev = info->par;

	MV_VPP_DisplayFrame(vdev, info->var.xoffset, info->var.yoffset,
			info->var.xres, info->var.yres, info->fix.line_length);
	return 0;
}

static int mrvl_fb_mmap(struct fb_info *info,
			struct vm_area_struct *vma)
{

	int result = 0;
	size_t req_size, fb_size;
	struct mrvl_vpp_dev *vdev = info->par;

	req_size = vma->vm_end - vma->vm_start;
	fb_size = info->fix.smem_len;
	if (req_size > fb_size) {
		pr_info("requested map is greater than framebuffer\n");
		return -EOVERFLOW;
	}

	if (info->screen_base == NULL) {
		pr_info("fb mmap failed!!!!\n");
		return -ENOMEM;
	}

	result = MV_VPP_MapMemory(vdev, vma);

	return result;
}

static int mrvl_fb_probe(struct platform_device *dev)
{
	struct fb_info *info;
	int retval = 0;
	struct mrvl_vpp_dev *vdev;
	int len = 0;
	int width, height;
	struct device_node *np = dev->dev.of_node;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_vpp_driver_initialized())
		return -EPROBE_DEFER;

	if (of_property_read_u32(np, "frame-rate", &vpp_config_param.frame_rate))
		vpp_config_param.frame_rate = 60; //default is 60

	if (of_property_read_u32(np, "frame-size-ndx", &vpp_config_param.frame_size_ndx)) {
		vpp_config_param.frame_size_ndx = 0; //default is 'MV_SYSTEM_WIDTH_DEFAULT * MV_SYSTEM_HEIGHT_DEFAULT'
	}
	if (of_property_read_u32(np, "disp-res-ndx", &vpp_config_param.disp_res_ndx)) {
		vpp_config_param.disp_res_ndx = 0; //default is 'MV_SYSTEM_WIDTH_DEFAULT * MV_SYSTEM_HEIGHT_DEFAULT'
	}

	of_node_put(np);

	vpp_config_param.enable_frame_buf_copy = 1;
#ifdef MRVL_FB_ENABLE_DOUBLE_BUFFERING
	vpp_config_param.fb_count = 2;
#else
	vpp_config_param.fb_count = 3;
#endif //MRVL_FB_ENABLE_DOUBLE_BUFFERING
	vpp_config_param.callback = NULL;
	vpp_config_param.data = NULL;

	MV_VPP_ConfigParams(&vpp_config_param);

	//Update the size of the frame buffer
	MV_VPP_GetFrameSize(&width, &height);
	mrvl_fb_var.xres = mrvl_fb_var.xres_virtual = width;
	mrvl_fb_var.yres = mrvl_fb_var.yres_virtual = height;
	pr_info("mrvl_fb_probe: frame-rate:%d, frame-size-ndx:%d, frame-size:%dx%d\n",
		vpp_config_param.frame_rate, vpp_config_param.frame_size_ndx, width, height);

	len = get_line_length(mrvl_fb_var.xres_virtual,
			mrvl_fb_var.bits_per_pixel);
	if (len <= 0) {
		pr_err("Invalid Line Length\n");
		return -EINVAL;
	}

	vdev = kzalloc(sizeof(struct mrvl_vpp_dev), GFP_KERNEL | __GFP_NOWARN);
	if (!vdev) {
		pr_err("Failed to allocate memory for device\n");
		return -ENOMEM;
	}

	mrvl_fb_fix.line_length = len;
	pr_info("mrvl_fb_fix.line_length:%x\n",
			mrvl_fb_fix.line_length);
	info = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
	if (!info) {
		retval = -ENOMEM;
		goto err_free_vdev;
	}

	info->fbops = &mrvl_fb_ops;
	info->var = mrvl_fb_var;
	info->screen_base = NULL;
	mrvl_fb_fix.smem_start =  0;
	mrvl_fb_fix.smem_len = 0;
	mrvl_fb_fix.smem_len = mrvl_fb_var.yres*vpp_config_param.fb_count*
			mrvl_fb_fix.line_length;
	info->fix = mrvl_fb_fix;
	info->flags = FBINFO_FLAG_DEFAULT;

	retval = register_framebuffer(info);
	if (retval < 0)
		goto err_free_fb;

	info->par = vdev;
	platform_set_drvdata(dev, info);

	return 0;

err_free_fb:
	framebuffer_release(info);
err_free_vdev:
	kfree(vdev);
	return retval;
}

static int mrvl_fb_remove(struct platform_device *dev)
{
	struct fb_info *info = platform_get_drvdata(dev);

	if (info) {
		struct mrvl_vpp_dev *vdev = info->par;
		unregister_framebuffer(info);
		if (is_vpp_initialized) {
			MV_VPP_Deinit();
			if (vdev && vdev->ionhandle)
				MV_VPP_FreeMemory(vdev);
		}
		MV_VPP_DeinitMemory();
		framebuffer_release(info);
		kfree(vdev);
	}
	return 0;
}

static const struct of_device_id fb_mrvl_match[] = {
	{
		.compatible = "marvell,berlin-fb",
	},
};

static struct platform_driver mrvl_fb_driver = {
	.probe	= mrvl_fb_probe,
	.remove = mrvl_fb_remove,
	.driver = {
		.name = "mrvl_fb",
		.of_match_table = fb_mrvl_match,
	},
};

static int __init mrvl_fb_init(void)
{
	int ret = 0;
	if (!avio_util_get_quiescent_flag()) {
		ret = platform_driver_register(&mrvl_fb_driver);
	}
	return ret;
}
late_initcall(mrvl_fb_init);

static void __exit mrvl_fb_exit(void)
{
	platform_driver_unregister(&mrvl_fb_driver);
}
module_exit(mrvl_fb_exit);

MODULE_LICENSE("GPL");
