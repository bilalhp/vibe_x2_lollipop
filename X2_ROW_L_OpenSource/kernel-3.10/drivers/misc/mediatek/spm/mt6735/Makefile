include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y	+= mt_spm.o
obj-y	+= mt_spm_internal.o
ifneq ($(CONFIG_MTK_FPGA),y)
obj-$(CONFIG_PM)	+= mt_spm_fs.o
obj-y	+= mt_spm_sleep.o
obj-y	+= mt_spm_dpidle.o
obj-y	+= mt_spm_sodi.o
endif
#obj-y	+= mt_spm_mcdi.o
#obj-y	+= mt_spm_talking.o
