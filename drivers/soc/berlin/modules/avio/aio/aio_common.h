// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _AIO_COMMON_H_
#define _AIO_COMMON_H_
#include "drv_aio.h"

#define	CutTo(x, b)	((x) & (bSETMASK(b) - 1))

struct aio_handle {
	char name[64];
	void *aio;
};

int aio_read(struct aio_priv *aio, u32 offset);
void aio_write(struct aio_priv *aio, u32 offset, u32 val);
int aio_read_gbl(struct aio_priv *aio, u32 offset);
void aio_write_gbl(struct aio_priv *aio, u32 offset, u32 val);
struct aio_priv *hd_to_aio(void *hd);
struct aio_priv *get_aio(void);
#endif
