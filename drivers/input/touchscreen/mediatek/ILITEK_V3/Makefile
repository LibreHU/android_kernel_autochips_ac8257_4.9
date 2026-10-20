# ILITEK V3 TDDI touch driver, MediaTek TPD, I2C (from CartelProject/MT6765-4.9, drivers/input/touchscreen/mediatek/ili9881x)
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ILITEK_V3/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ILITEK_V3/firmware/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-y += ili9881x.o ili9881x_i2c.o ili9881x_mtk.o ili9881x_ic.o ili9881x_touch.o ili9881x_mp.o ili9881x_flash.o \
	ili9881x_node.o
