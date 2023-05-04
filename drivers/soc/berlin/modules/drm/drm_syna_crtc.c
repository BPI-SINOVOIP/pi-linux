// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include <linux/version.h>
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic_helper.h>

#include "drm_syna_drv.h"
#include "drm_syna_gem.h"
#include "syna_vpp.h"

enum syna_crtc_flip_status {
	SYNA_CRTC_FLIP_STATUS_NONE = 0,
	SYNA_CRTC_FLIP_STATUS_PENDING,
	SYNA_CRTC_FLIP_STATUS_DONE,
};

struct syna_flip_data {
	struct dma_fence_cb base;
	struct drm_crtc *crtc;
	struct dma_fence *wait_fence;
};

/* returns true for ok, false for fail */
static bool syna_clocks_set(struct drm_crtc *crtc,
			    struct drm_display_mode *adjusted_mode)
{
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
	bool res;

	int clock_in_mhz = adjusted_mode->clock / 1000;

	if (!syna_crtc) {
		DRM_ERROR("%s %d syna crtc is NULL!!\n",
			  __func__, __LINE__);
		return false;
	}
	syna_vpp_set_updates_enabled(crtc->dev->dev,
				     syna_crtc->syna_reg, false);
	res = syna_vpp_clocks_set(crtc->dev->dev,
				  syna_crtc->syna_reg, clock_in_mhz,
				  adjusted_mode->hdisplay,
				  adjusted_mode->vdisplay);
	syna_vpp_set_updates_enabled(crtc->dev->dev, syna_crtc->syna_reg, true);

	DRM_DEBUG_DRIVER("syna clock set to %dMhz\n", clock_in_mhz);

	return res;
}

static void syna_crtc_set_plane_enabled(struct drm_crtc *crtc, bool enable)
{
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);

	if (!syna_crtc) {
		DRM_ERROR("%s %d syna crtc is NULL!!\n", __func__, __LINE__);
		return;
	}
	syna_vpp_set_plane_enabled(crtc->dev->dev,
				   syna_crtc->syna_reg, 0, enable);
}

static void syna_crtc_set_syncgen_enabled(struct drm_crtc *crtc, bool enable)
{
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);

	if (!syna_crtc) {
		DRM_ERROR("%s %d syna crtc is NULL!!\n", __func__, __LINE__);
		return;
	}
	syna_vpp_set_syncgen_enabled(crtc->dev->dev,
				     syna_crtc->syna_reg, enable);
}

static void syna_crtc_set_enabled(struct drm_crtc *crtc, bool enable)
{
	struct syna_drm_private *dev_priv = crtc->dev->dev_private;

	if (!dev_priv) {
		DRM_ERROR("%s %d dev priv is NULL!!\n", __func__, __LINE__);
		return;
	}

	if (enable) {
		syna_crtc_set_syncgen_enabled(crtc, enable);
		syna_crtc_set_plane_enabled(crtc, dev_priv->display_enabled);
		drm_crtc_vblank_on(crtc);
	} else {
		drm_crtc_vblank_off(crtc);
		syna_crtc_set_plane_enabled(crtc, enable);
		syna_crtc_set_syncgen_enabled(crtc, enable);
	}
}

static void syna_crtc_mode_set(struct drm_crtc *crtc,
			       struct drm_display_mode *adjusted_mode)
{
	/*
	 * ht   = horizontal total
	 * hbps = horizontal back porch start
	 * has  = horizontal active start
	 * hlbs = horizontal left border start
	 * hfps = horizontal front porch start
	 * hrbs = horizontal right border start
	 *
	 * vt   = vertical total
	 * vbps = vertical back porch start
	 * vas  = vertical active start
	 * vtbs = vertical top border start
	 * vfps = vertical front porch start
	 * vbbs = vertical bottom border start
	 */
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
	uint32_t ht = adjusted_mode->htotal;
	uint32_t hbps = adjusted_mode->hsync_end - adjusted_mode->hsync_start;
	uint32_t has = (adjusted_mode->htotal - adjusted_mode->hsync_start);
	uint32_t hlbs = has;
	uint32_t hfps = (hlbs + adjusted_mode->hdisplay);
	uint32_t hrbs = hfps;
	uint32_t vt = adjusted_mode->vtotal;
	uint32_t vbps = adjusted_mode->vsync_end - adjusted_mode->vsync_start;
	uint32_t vas = (adjusted_mode->vtotal - adjusted_mode->vsync_start);
	uint32_t vtbs = vas;
	uint32_t vfps = (vtbs + adjusted_mode->vdisplay);
	uint32_t vbbs = vfps;
	bool ok;

	if (!syna_crtc) {
		DRM_ERROR("%s %d crtc is NULL!!\n", __func__, __LINE__);
		return;
	}

	ok = syna_clocks_set(crtc, adjusted_mode);

	if (!ok) {
		dev_info(crtc->dev->dev, "%s failed\n", __func__);
		return;
	}
	syna_vpp_set_updates_enabled(crtc->dev->dev,
				     syna_crtc->syna_reg, false);
	syna_vpp_reset_planes(crtc->dev->dev, syna_crtc->syna_reg);
	syna_vpp_mode_set(crtc->dev->dev,
			  syna_crtc->syna_reg,
			  adjusted_mode->hdisplay,
			  adjusted_mode->vdisplay, hbps, ht, has, hlbs,
			  hfps, hrbs, vbps, vt, vas, vtbs, vfps, vbbs,
			  adjusted_mode->flags & DRM_MODE_FLAG_NHSYNC,
			  adjusted_mode->flags & DRM_MODE_FLAG_NVSYNC);
	syna_vpp_set_powerdwn_enabled(crtc->dev->dev,
				      syna_crtc->syna_reg, false);
	syna_vpp_set_updates_enabled(crtc->dev->dev,
				     syna_crtc->syna_reg, true);
}

static bool syna_crtc_helper_mode_fixup(struct drm_crtc *crtc,
					const struct drm_display_mode *mode,
					struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void syna_crtc_flip_complete(struct drm_crtc *crtc);

static void syna_crtc_helper_mode_set_nofb(struct drm_crtc *crtc)
{
	syna_crtc_mode_set(crtc, &crtc->state->adjusted_mode);
}

static void syna_crtc_helper_atomic_flush(struct drm_crtc *crtc,
					  struct drm_crtc_state *old_crtc_state)
{
	struct drm_crtc_state *new_crtc_state = crtc->state;

	if (!new_crtc_state->active || !old_crtc_state->active)
		return;

	if (crtc->state->event) {
		struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
		unsigned long flags;

		if (!syna_crtc) {
			DRM_ERROR("%s %d syna crtc is NULL!!\n",
				  __func__, __LINE__);
			return;
		}

		syna_crtc->flip_async = 0;//!!(new_crtc_state->pageflip_flags & DRM_MODE_PAGE_FLIP_ASYNC);

		if (syna_crtc->flip_async)
			WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		syna_crtc->flip_event = crtc->state->event;
		crtc->state->event = NULL;

		atomic_set(&syna_crtc->flip_status, SYNA_CRTC_FLIP_STATUS_DONE);
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

		if (syna_crtc->flip_async)
			syna_crtc_flip_complete(crtc);
	}
}

static void syna_crtc_helper_atomic_enable(struct drm_crtc *crtc,
					   struct drm_crtc_state *old_crtc_state
)
{
	syna_crtc_set_enabled(crtc, true);

	if (crtc->state->event) {
		struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
		unsigned long flags;

		if (!syna_crtc) {
			DRM_ERROR("%s %d syna crtc is NULL!!\n",
				  __func__, __LINE__);
			return;
		}
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		syna_crtc->flip_event = crtc->state->event;
		crtc->state->event = NULL;

		atomic_set(&syna_crtc->flip_status, SYNA_CRTC_FLIP_STATUS_DONE);
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
	}
}

static void syna_crtc_helper_atomic_disable(struct drm_crtc *crtc,
					    struct drm_crtc_state
					    *old_crtc_state)
{
	syna_crtc_set_enabled(crtc, false);

	if (crtc->state->event) {
		unsigned long flags;

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
	}
}

static void syna_crtc_destroy(struct drm_crtc *crtc)
{
	struct drm_device *dev;
	struct syna_drm_private *dev_priv;
	struct syna_crtc *syna_crtc;

	dev = crtc->dev;
	dev_priv = dev->dev_private;
	syna_crtc = to_syna_crtc(crtc);

	DRM_DEBUG_DRIVER("[CRTC:%d]\n", crtc->base.id);

	drm_crtc_cleanup(crtc);

	kfree(syna_crtc);
	dev_priv->crtc = NULL;
}

static void syna_crtc_flip_complete(struct drm_crtc *crtc)
{
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
	unsigned long flags;

	if (!syna_crtc) {
		DRM_ERROR("%s %d syna crtc is NULL!!\n", __func__, __LINE__);
		return;
	}
	spin_lock_irqsave(&crtc->dev->event_lock, flags);

	/* The flipping process has been completed so reset the flip state */
	atomic_set(&syna_crtc->flip_status, SYNA_CRTC_FLIP_STATUS_NONE);
	syna_crtc->flip_async = false;

#if !defined(SYNA_USE_ATOMIC)
	if (syna_crtc->flip_data) {
		dma_fence_put(syna_crtc->flip_data->wait_fence);
		kfree(syna_crtc->flip_data);
		syna_crtc->flip_data = NULL;
	}
#endif

	if (syna_crtc->flip_event) {
		drm_crtc_send_vblank_event(crtc, syna_crtc->flip_event);
		syna_crtc->flip_event = NULL;
	}

	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static const struct drm_crtc_helper_funcs syna_crtc_helper_funcs = {
	.mode_fixup = syna_crtc_helper_mode_fixup,
	.mode_set_nofb = syna_crtc_helper_mode_set_nofb,
	.atomic_flush = syna_crtc_helper_atomic_flush,
	.atomic_enable = syna_crtc_helper_atomic_enable,
	.atomic_disable = syna_crtc_helper_atomic_disable,
};

static const struct drm_crtc_funcs syna_crtc_funcs = {
	.destroy = syna_crtc_destroy,
	.reset = drm_atomic_helper_crtc_reset,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
};

struct drm_crtc *syna_crtc_create(struct drm_device *dev, uint32_t number,
				  struct drm_plane *primary_plane)
{
	struct syna_crtc *syna_crtc;
	int err;

	syna_crtc = kzalloc(sizeof(*syna_crtc), GFP_KERNEL);
	if (!syna_crtc) {
		err = -ENOMEM;
		goto err_exit;
	}

	init_waitqueue_head(&syna_crtc->flip_pending_wait_queue);
	atomic_set(&syna_crtc->flip_status, SYNA_CRTC_FLIP_STATUS_NONE);
	syna_crtc->number = number;

	err = drm_crtc_init_with_planes(dev, &syna_crtc->base, primary_plane,
					NULL, &syna_crtc_funcs, NULL);
	if (err) {
		DRM_ERROR("CRTC init with planes failed");
		goto err_iounmap_regs;
	}

	drm_crtc_helper_add(&syna_crtc->base, &syna_crtc_helper_funcs);

	DRM_DEBUG_DRIVER("[CRTC:%d]\n", syna_crtc->base.base.id);

	return &syna_crtc->base;

err_iounmap_regs:
	kfree(syna_crtc);
err_exit:
	return ERR_PTR(err);
}

void syna_crtc_set_vblank_enabled(struct drm_crtc *crtc, bool enable)
{
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);

	if (!syna_crtc) {
		DRM_ERROR("%s %d syna crtc is NULL!!\n", __func__, __LINE__);
		return;
	}
	syna_vpp_set_vblank_enabled(crtc->dev->dev,
				    syna_crtc->syna_reg, enable);
}

void syna_crtc_irq_handler(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
	bool handled;

	if (!syna_crtc) {
		DRM_ERROR("%s %d syna crtc is NULL!!\n", __func__, __LINE__);
		return;
	}
	handled = syna_vpp_check_and_clear_vblank(dev->dev,
						  syna_crtc->syna_reg);

	if (handled) {
		enum syna_crtc_flip_status status;

		drm_handle_vblank(dev, syna_crtc->number);

		status = atomic_read(&syna_crtc->flip_status);
		if (status == SYNA_CRTC_FLIP_STATUS_DONE) {
			if (!syna_crtc->flip_async)
				syna_crtc_flip_complete(crtc);
		}
	}
}
