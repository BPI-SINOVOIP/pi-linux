INC_PATH = -I$(srctree)/drivers/soc/berlin/modules/tzd/tee/client_api \
           -I$(srctree)/drivers/soc/berlin/modules/tzd/tee/common \
           -I$(srctree)/drivers/soc/berlin/modules/tzd/include \
           -I$(srctree)/drivers/soc/berlin/modules/tzd \
           -I$(srctree)/drivers/soc/berlin/modules/tzd/tz/common
ccflags-y += -Werror
ccflags-y += $(INC_PATH)
obj-$(CONFIG_BERLIN_BM) := amp_bm.o
amp_bm-objs += bm.o ptm_wrapper.o
