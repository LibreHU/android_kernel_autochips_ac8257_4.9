subdir-ccflags-y += 

obj-$(CONFIG_MTK_CMDQ) += mtk-cmdq-helper.o
obj-$(CONFIG_MTK_INFRACFG) += mtk-infracfg.o
obj-$(CONFIG_MTK_PMIC_WRAP) += mtk-pmic-wrap.o
obj-$(CONFIG_MTK_SCPSYS) += mtk-scpsys.o mtk-scpsys-bringup.o
obj-$(CONFIG_MTK_SCPSYS_MT8163) += mtk-scpsys-mt8163.o

# LibreHU 4.19: headers and APIs the 4.9 vendor drivers got implicitly (include/linux/librehu_compat_4_9.h)
subdir-ccflags-y += -include $(srctree)/include/linux/librehu_compat_4_9.h -Wno-macro-redefined
