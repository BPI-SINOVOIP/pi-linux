# SPDX-License-Identifier: GPL-2.0
#
# Makefile for DRP-AI/DRP.
#
ccflags-y := -Werror

ifdef CONFIG_ARCH_R9A09G056
CFLAGS_lock_drpai.o += -Wall -mno-outline-atomics
LDFLAGS_lock_drpai.o += -lgcc
obj-$(CONFIG_DRPAI)	+= drpai-core.o drpai-if.o lock_drpai.o
else
obj-$(CONFIG_DRPAI)	+= drpai-core.o drpai-if.o
endif
