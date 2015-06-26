
KERNEL_PATH=$(shell pwd)/X2_ROW_L_OpenSource/kernel-3.10/
DIST_PATH=dist/
IMAGE_NAME=boot.img

.PHONY: all defconfig build package push flash ramdisk_unpack ramdisk_pack dist_clean clean

all: dist_clean build package push flash 
	@true

defconfig:
	cd ${KERNEL_PATH}; make x2eu_bilal_defconfig ARCH=arm CROSS_COMPILE=arm-eabi-

saveconfig:
	cd ${KERNEL_PATH}; cp .config arch/arm/configs/x2eu_bilal_defconfig

build:
	cd ${KERNEL_PATH}; make ARCH=arm CROSS_COMPILE=arm-eabi-

package: dist_clean ${DIST_PATH}/initrd.img
	sed -i '/^bootsize/d' ${DIST_PATH}/bootimg.cfg
	cd ${DIST_PATH}; abootimg --create ${IMAGE_NAME} -f bootimg.cfg -k ${KERNEL_PATH}/arch/arm/boot/zImage-dtb -r initrd.img

flash:
	adb push ${DIST_PATH}/${IMAGE_NAME} /sdcard/
	adb shell su -c 'dd if=/sdcard/${IMAGE_NAME} of=/dev/block/platform/mtk-msdc.0/by-name/boot'
	adb reboot

ramdisk_unpack:
	cd ${DIST_PATH}; abootimg -x ${IMAGE_NAME}
	cd ${DIST_PATH}; abootimg-unpack-initrd initrd.img

ramdisk_pack ${DIST_PATH}/initrd.img:
	cd ${DIST_PATH}; rm -f initrd.img
	cd ${DIST_PATH}; abootimg-pack-initrd

dist_clean:
	cd ${DIST_PATH}; rm -f initrd.img

clean: dist_clean
	cd ${KERNEL_PATH}; make ARCH=arm CROSS_COMPILE=arm-eabi- mrproper
