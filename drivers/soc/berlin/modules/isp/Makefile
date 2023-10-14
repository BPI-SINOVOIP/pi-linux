obj-$(CONFIG_SYNA_ISP)	:=	isp.o

ISP_ABS_PATH := $(srctree)/drivers/soc/berlin/modules/isp

ifeq ($(CONFIG_ISP_VS680),y)
ISP_INC_PATH += -I$(ISP_ABS_PATH)/vs680
else ifeq ($(CONFIG_ISP_VS680_A0),y)
ISP_INC_PATH += -I$(ISP_ABS_PATH)/vs680a0
endif
isp-objs +=  dhub/configs/vs680/isp_dhub_cfg.o
ISP_INC_PATH += -I$(ISP_ABS_PATH)/fw_inc/vs680

HEADER_PATH +=  $(ISP_INC_PATH)    \
		-I$(srctree)/drivers/soc/berlin/modules/isp/dhub \
		-I$(srctree)/drivers/soc/berlin/modules/msgQ \
		-I$(srctree)/drivers/soc/berlin/modules/isp \

ccflags-y += $(HEADER_PATH)

isp-objs += 	isp_driver.o \
		drv_isp.o \
		dhub/drv_ispdhub.o \
		dhub/ispdhub_sema.o
