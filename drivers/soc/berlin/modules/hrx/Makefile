AVIO_ABS_PATH := $(srctree)/drivers/soc/berlin/modules/avio
AVIO_INC_PATH := -I$(AVIO_ABS_PATH)/include
ifeq ($(CONFIG_AVIO_VS680),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/vs680
endif
ifeq ($(CONFIG_AVIO_VS680_A0),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fw_inc/vs680a0
endif
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/common
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub/include
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)

MSGQ_INC_PATH := -I$(srctree)/drivers/soc/berlin/modules/msgQ
EXTERN_INC_PATH := $(MSGQ_INC_PATH)

EXTERN_INC_PATH += -I$(srctree)/drivers/soc/berlin/modules/shm
EXTERN_INC_PATH += -I$(srctree)/drivers/soc/berlin/modules/berlin_inc

HRX_ALL_OBJS = drv_hrx.o hrx_audio.o

hrx-objs += $(HRX_ALL_OBJS) \

HEADER_PATH +=  $(AVIO_INC_PATH)    \
                $(EXTERN_INC_PATH)  \

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror

obj-$(CONFIG_BERLIN_HRX) += hrx.o
