#add kernel source code path as head file seach path
include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)

ifeq ($(TARGET_BUILD_VARIANT),eng)
obj-$(CONFIG_MTK_MMC) += dbg.o
endif
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
obj-$(CONFIG_MTK_MMC) += dbg.o
endif
obj-$(CONFIG_MTK_MMC) += sd.o mt_sd_misc.o
obj-$(CONFIG_MTK_MMC) += sdio_autok.o sdio_autok_proc.o ettagent.o

# kdump/kpainc interface move to lk 
ifeq ($(CONFIG_MTK_AEE_IPANIC),y)
obj-$(CONFIG_MTK_MMC) += mt_dump.o
endif

ifeq ($(CONFIG_MTK_EMMC_SUPPORT_OTP),y)
obj-$(CONFIG_MTK_MMC) += emmc_otp.o
endif
