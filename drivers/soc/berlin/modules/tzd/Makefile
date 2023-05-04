
ccflags-$(CONFIG_REE_SYS_MUTEX)        += -DCONFIG_REE_SYS_MUTEX
ccflags-y        += -DCONFIG_TEE_SYS_MUTEX
ccflags-y        += -DCONFIG_REE_SYS_WAIT

obj-$(CONFIG_BERLIN_TZD) := tzd.o
tzd-objs = tz_driver.o tz_dev_logger.o tz_utils.o tz_shm.o
tzd-objs += tz/common/tz_comm.o
tzd-objs += tz/common/smc.o
tzd-objs  += tz/nw_api/tz_nw_comm.o
tzd-objs  += tz/nw_api/tz_nw_task_client.o
tzd-objs  += tz/nw_api/tz_nw_sys_callback.o
tzd-objs  += tz/nw_api/tz_nw_osal_linux.o
tzd-objs  += tz/client_api/tz_client_api_linux_kernel.o
tzd-objs  += tee/client_api/tee_client_api.o
tzd-objs  += tee/client_api/tee_client_util.o
tzd-objs  += tee/mgr/tee_ca_mgr_cmd.o
tzd-objs  += tee/mgr/tee_ca_sys_cmd.o
tzd-objs  += tee/ree/ree_sys_callback.o
tzd-objs  += tee/ree/ree_sys_callback_linux.o
tzd-objs  += tee/ree/ree_sys_callback_mutex_ext.o


ifeq ($(CONFIG_COMPAT), y)
tzd-objs += tz_compat.o
endif
ifeq ($(CONFIG_REE_SYS_MUTEX), y)
tzd-objs += ../../tee/ree/ree_sys_callback_mutex.o
endif
ifeq ($(CONFIG_REE_SYS_SEMAPHORE), y)
tzd-objs += ../../tee/ree/ree_sys_callback_semaphore.o
endif

EXTRA_CFLAGS += -I $(srctree)/$(src)/
EXTRA_CFLAGS += -I $(srctree)/$(src)/include/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tz/nw_api/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tz/common/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tz/client_api/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tee/common/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tee/client_api/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tee/internal_api/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tee/mgr/
EXTRA_CFLAGS += -I $(srctree)/$(src)/tee/ree/
EXTRA_CFLAGS += -DCONFIG_TEE
