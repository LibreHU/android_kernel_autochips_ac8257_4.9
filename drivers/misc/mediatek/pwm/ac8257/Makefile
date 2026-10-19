ifeq (y, $(filter y, $(CONFIG_MACH_AC8257)))
PWM_PLAT_NAME = $(subst ",,ac8257)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pwm/ac8257/include
else
PWM_PLAT_NAME = $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pwm/$(PWM_PLAT_NAME)/include
endif

obj-$(CONFIG_MTK_PWM)	:= mtk_pwm_hal.o
