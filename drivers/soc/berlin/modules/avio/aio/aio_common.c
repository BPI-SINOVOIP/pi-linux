// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#include "aio_common.h"
#include "aio_hal.h"

struct aio_priv *hd_to_aio(void *hd)
{
	struct aio_handle *handle = hd;

	return handle->aio;
}

int aio_read(struct aio_priv *aio, u32 offset)
{
	if (unlikely(!aio->pbase)) {
		pr_err("%s, aio->pbase NULL\n", __func__);
		return 0;
	}

	if (unlikely(offset > resource_size(&aio->base_res))) {
		pr_err("%s, aio address %x out of range %llx",
			__func__, offset, resource_size(&aio->base_res));
		return 0;
	}
	return readl_relaxed(aio->pbase + offset);
}

void aio_write(struct aio_priv *aio, u32 offset, u32 val)
{
	if (unlikely(!aio->pbase)) {
		pr_err("%s, aio->pbase NULL\n", __func__);
		return;
	}

	if (unlikely(offset > resource_size(&aio->base_res))) {
		pr_err("%s, aio address %x out of range %llx",
			__func__, offset, resource_size(&aio->base_res));
		return;
	}
	writel_relaxed(val, aio->pbase + offset);
}

void *open_aio(const char *name)
{
	struct aio_handle *handle;
	struct aio_priv *aio = get_aio();

	if (!aio) {
		pr_err("aio get fail\n");
		return NULL;
	}

	handle = kzalloc(sizeof(struct aio_handle), GFP_KERNEL);

	if (handle == NULL)
		return ERR_PTR(-ENOMEM);

	snprintf(handle->name, sizeof(handle->name), "%s", name);
	handle->aio = (void *)aio;

	return handle;
}
EXPORT_SYMBOL(open_aio);

int close_aio(void *hd)
{
	struct aio_handle *handle = hd;

	handle->aio = NULL;
	kfree(handle);
	return 0;
}
EXPORT_SYMBOL(close_aio);

static struct clk *get_clk(struct aio_priv *aio, u32 clk_idx)
{
	struct clk *l_clk;

	if (clk_idx == AIO_APLL_0)
		l_clk = aio->a_clk[AIO_CLK_0];
	else if (clk_idx == AIO_APLL_1)
		l_clk = aio->a_clk[AIO_CLK_1];
	else
		l_clk = NULL;

	return l_clk;
}

int aio_clk_enable(void *hd, u32 clk_idx, bool en)
{
	struct aio_handle *handle = hd;
	struct aio_priv *aio = hd_to_aio(hd);
	int ret;
	struct clk *l_clk;

	pr_info("%s: %s(%d, %d)\n",
		handle->name, __func__, clk_idx, en);

	l_clk = get_clk(aio, clk_idx);
	if (!l_clk) {
		pr_err("clk(%d) not support %s\n", clk_idx, __func__);
		return -EINVAL;
	}

	if (en) {
		ret = clk_prepare_enable(l_clk);
	} else {
		clk_disable_unprepare(l_clk);
		ret = 0;
	}

	if (ret) {
		pr_err("aio_clk %s error\n", en ? "enable" : "disable");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(aio_clk_enable);

int aio_set_clk_rate(void *hd, u32 clk_idx, unsigned long rate)
{
	struct aio_handle *handle = hd;
	struct aio_priv *aio = hd_to_aio(hd);
	int ret;
	struct clk *l_clk;

	pr_info("%s: %s(%d, %lu)\n",
		handle->name, __func__, clk_idx, rate);

	l_clk = get_clk(aio, clk_idx);
	if (!l_clk) {
		pr_err("clk(%d) not support %s\n", clk_idx, __func__);
		return -EINVAL;
	}

	ret = clk_set_rate(l_clk, rate);
	if (ret) {
		pr_err(" %d clk_set_rate error\n", clk_idx);
		return -EFAULT;
	}
	return 0;
}
EXPORT_SYMBOL(aio_set_clk_rate);

unsigned long aio_get_clk_rate(void *hd, u32 clk_idx)
{
	struct aio_handle *handle = hd;
	struct aio_priv *aio = hd_to_aio(hd);
	struct clk *l_clk;
	unsigned long rate;

	pr_info("%s: %s(%d)\n", handle->name, __func__, clk_idx);

	l_clk = get_clk(aio, clk_idx);
	if (!l_clk) {
		pr_err("clk(%d) not support %s\n", clk_idx, __func__);
		return -EINVAL;
	}

	rate = clk_get_rate(l_clk);
	pr_info("%s: clk:%d, rate:%lu\n", __func__, clk_idx, rate);
	return rate;
}
EXPORT_SYMBOL(aio_get_clk_rate);

__weak int aio_misc_enable_audio_timer(void *hd, bool en)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_enable_audio_timer(void *hd, bool en)
{
	return aio_misc_enable_audio_timer(hd, en);
}
EXPORT_SYMBOL(aio_enable_audio_timer);


__weak int aio_misc_get_audio_timer(void *hd, u32 *val)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_get_audio_timer(void *hd, u32 *val)
{
	return aio_misc_get_audio_timer(hd, val);
}
EXPORT_SYMBOL(aio_get_audio_timer);

__weak int aio_misc_enable_sampinfo(void *hd, u32 idx, bool en)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}
int aio_enable_sampinfo(void *hd, u32 idx, bool en)
{
	return aio_misc_enable_sampinfo(hd, idx, en);
}
EXPORT_SYMBOL(aio_enable_sampinfo);

__weak int aio_misc_set_sampinfo_req(void *hd, u32 idx, bool en)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_set_sampinfo_req(void *hd, u32 idx, bool en)
{
	return aio_misc_set_sampinfo_req(hd, idx, en);
}
EXPORT_SYMBOL(aio_set_sampinfo_req);

__weak int aio_misc_get_audio_counter(void *hd, u32 idx, u32 *c)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_get_audio_counter(void *hd, u32 idx, u32 *c)
{
	return aio_misc_get_audio_counter(hd, idx, c);
}
EXPORT_SYMBOL(aio_get_audio_counter);

__weak int aio_misc_get_audio_timestamp(void *hd, u32 idx, u32 *t)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_get_audio_timestamp(void *hd, u32 idx, u32 *t)
{
	return aio_misc_get_audio_timestamp(hd, idx, t);
}
EXPORT_SYMBOL(aio_get_audio_timestamp);

__weak int aio_misc_sw_rst(void *hd, u32 option, u32 val)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_sw_rst(void *hd, u32 option, u32 val)
{
	return aio_misc_sw_rst(hd, option, val);
}
EXPORT_SYMBOL(aio_sw_rst);

__weak int aio_misc_set_loopback_clk_gate(void *hd, u32 idx, u32 en)
{
	pr_info("weak %s called\n", __func__);
	return 0;
}

int aio_set_loopback_clk_gate(void *hd, u32 idx, u32 en)
{
	return aio_misc_set_loopback_clk_gate(hd, idx, en);
}
EXPORT_SYMBOL(aio_set_loopback_clk_gate);
