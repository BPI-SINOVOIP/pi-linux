// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include "synap_kernel.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define SYNAP_DRV_VERSION "2.2.0"

static int32_t __init synap_module_init(void)
{
    return synap_kernel_module_init();
}

static void __exit synap_module_exit(void)
{
    synap_kernel_module_exit();
}

module_init(synap_module_init)
module_exit(synap_module_exit)

MODULE_VERSION(SYNAP_DRV_VERSION);
MODULE_LICENSE("GPL v2");
