subdir-ccflags-y += -Werror

ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/

obj-$(CONFIG_COMMON_CLK_MEDIATEK) += clk-mtk.o clk-pll.o clk-gate.o clk-mux.o clk-apmixed.o
obj-$(CONFIG_RESET_CONTROLLER) += reset.o
obj-$(CONFIG_COMMON_CLK_AC8257) += clk-ac8257.o clk-ac8257-pg.o clk-mtk-v1.o ac8257_clkmgr.o
obj-y += clk-bringup.o

ifeq ($(CONFIG_DEBUG_FS), y)
obj-$(CONFIG_COMMON_CLK_MEDIATEK) += clkdbg.o clkchk.o
obj-$(CONFIG_COMMON_CLK_AC8257) += clkdbg_ac8257.o clkchk-ac8257.o
endif
