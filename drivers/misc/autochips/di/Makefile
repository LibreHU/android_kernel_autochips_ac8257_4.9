subdir-ccflags-y += -Wno-unused-result

SRC=$(srctree)/drivers/misc/autochips/di/
ccflags-y += -I$(srctree)/drivers/misc/autochips/avin/inc
ccflags-y += -I$(srctree)/drivers/misc/autochips/wch/include/
ccflags-y += -I$(srctree)/drivers/misc/autochips/libosal/inc
ccflags-y += -I$(srctree)/drivers/misc/atc/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/include
ccflags-y += -I$(SRC)/inc
ccflags-y += -I$(srctree)/drivers/misc/autochips/inc
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(srctree)/drivers/staging/android/ion
ccflags-y += -I$(srctree)/drivers/staging/android/ion/mtk
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sync/

ifeq ($(strip $(CONFIG_ATC_BUILD_GO)), y)
ccflags-y += -D DI_FPS_60
endif

pmx_files = $(patsubst $(SRC)/pmx/%.c,pmx/%.o,$(wildcard $(SRC)/pmx/*.c))
vdp_files = $(patsubst $(SRC)/vdp/%.c,vdp/%.o,$(wildcard $(SRC)/vdp/*.c))
di_files = $(patsubst $(SRC)/di/%.c,di/%.o,$(wildcard $(SRC)/di/*.c))

atcdi-objs := $(pmx_files) $(vdp_files) $(di_files)

obj-y += atcdi.o

