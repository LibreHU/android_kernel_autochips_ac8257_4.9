# PMIC object
obj-y += mtk-soc-codec-6357.o mtk-auddrv-ana.o

ifneq ($(CONFIG_ARCH_MTK_PROJECT), "ac8257_evb")
ifneq ($(CONFIG_ARCH_MTK_PROJECT), "ac8257_jac")
# LibreHU: no TDA7729 on the Jancar UJC201 (absent from the stock kernel)
ifneq ($(CONFIG_JANCAR_SOLUTION),y)
ccflags-y +=	-DUSE_TDA7729
endif
endif
endif
