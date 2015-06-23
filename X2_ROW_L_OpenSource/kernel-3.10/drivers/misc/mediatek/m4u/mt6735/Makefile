include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -O0 -gdwarf-2
ccflags-y += -I$(srctree)/drivers/staging/android/ion

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735/
obj-y += mt6735/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735m/
obj-y += mt6735m/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6753/
obj-y += mt6753/
endif

obj-y += m4u.o m4u_mva.o m4u_pgtable.o m4u_hw.o m4u_debug.o

