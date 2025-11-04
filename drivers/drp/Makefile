# SPDX-License-Identifier: GPL-2.0
#
# Makefile for DRP-AI/DRP.
#
ccflags-y := -Werror

ifdef CONFIG_ARCH_R9A09G056
CFLAGS_lock_drp.o += -Wall -mno-outline-atomics
LDFLAGS_lock_drp.o += -lgcc
obj-$(CONFIG_DRP)	+= drp-core.o drp-if.o lock_drp.o
else
obj-$(CONFIG_DRP)	+= drp-core.o drp-if.o
endif
