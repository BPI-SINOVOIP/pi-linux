dtb-$(CONFIG_BOARD_BANANAPI_M4BERRY) += sun50i-h618-bananapi-m4berry.dtb
dtb-$(CONFIG_BOARD_BANANAPI_M4ZERO) += sun50i-h618-bananapi-m4zero.dtb

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb

DTC_FLAGS_board += -@
DTC_FLAGS ?= -a 262144

subdir-$(CONFIG_BOARD_BANANAPI_M4BERRY) += overlay/bpi-m4berry
subdir-$(CONFIG_BOARD_BANANAPI_M4ZERO) += overlay/bpi-m4zero
