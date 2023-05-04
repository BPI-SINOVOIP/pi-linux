# SPDX-License-Identifier: GPL-2.0-only
ifneq ($(filter y,$(CONFIG_MACH_BERLIN_BG2) $(CONFIG_MACH_BERLIN_BG2CD) $(CONFIG_MACH_BERLIN_BG2Q)),)
obj-y += berlin2-avpll.o berlin2-pll.o berlin2-div.o
endif

obj-$(CONFIG_SYNA_AS370_PLL)	+= as370-pll.o
obj-$(CONFIG_SYNA_VS680_PLL)	+= vs680-pll.o
obj-$(CONFIG_SYNA_CLK_BASE)	+= syna-clk.o
syna-clk-y := clk.o gate.o
obj-$(CONFIG_SYNA_AS370_CLK)	+= as370-clks.o
obj-$(CONFIG_SYNA_AS470_CLK)	+= as470-clks.o
obj-$(CONFIG_SYNA_VS640_CLK)	+= vs640-clks.o
obj-$(CONFIG_SYNA_VS680_CLK)	+= vs680-clks.o
obj-$(CONFIG_MACH_BERLIN_BG2)	+= bg2.o
obj-$(CONFIG_MACH_BERLIN_BG2CD)	+= bg2.o
obj-$(CONFIG_MACH_BERLIN_BG2Q)	+= bg2q.o
