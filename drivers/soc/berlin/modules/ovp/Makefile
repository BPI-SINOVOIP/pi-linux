###############################################################################
#
# Common ovp kernel module objects
#
###############################################################################
BERLIN_MOD_PATH := $(srctree)/drivers/soc/berlin/modules

OVP_COMMON_API_OBJS := drv_ovp.o tee_ca_ovp.o

###############################################################################

OVP_API_OBJS := $(OVP_COMMON_API_OBJS)

###############################################################################
# TZ include files
###############################################################################

TZ_INC_PATH += -I$(BERLIN_MOD_PATH)/tzd/tee/client_api \
		-I$(BERLIN_MOD_PATH)/tzd/tee/common \
		-I$(BERLIN_MOD_PATH)/tzd/include

###############################################################################
###############################################################################
#
# Common MsgQ kernel module objects
#
###############################################################################
MSGQ_INC_PATH := -I$(BERLIN_MOD_PATH)/msgQ

EXTERN_INC_PATH := $(MSGQ_INC_PATH) $(TZ_INC_PATH)

###############################################################################
#
# Common macro/defines
#
###############################################################################

###############################################################################
#Final part of the Makefile
###############################################################################

OVP_ALL_OBJS := $(OVP_API_OBJS)

ovp-objs += $(OVP_ALL_OBJS)

HEADER_PATH += $(EXTERN_INC_PATH)  \

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror

obj-$(CONFIG_BERLIN_OVP) += ovp.o
