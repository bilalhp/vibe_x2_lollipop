# drivers/dispsys/mt6735m
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/ \
             -I$(srctree)/drivers/misc/mediatek/videox/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/ \
             -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM)/

#driver
obj-y += ddp_ovl.o
obj-y += ddp_rdma.o

# EOF
