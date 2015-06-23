
KERNEL_PATH=$(shell pwd)/X2_ROW_L_OpenSource/kernel-3.10/
DIST_PATH=dist/
IMAGE_NAME=boot_bilal.img

all: build package push flash
	@true

defconfig:
	cd ${KERNEL_PATH}; make x2eu_defconfig ARCH=arm CROSS_COMPILE=arm-eabi-

build:
	cd ${KERNEL_PATH}; make ARCH=arm CROSS_COMPILE=arm-eabi-

package:
	sed -i '/^bootsize/d' ${DIST_PATH}/bootimg.cfg
	cd ${DIST_PATH}; abootimg --create ${IMAGE_NAME} -f bootimg.cfg -k ${KERNEL_PATH}/arch/arm/boot/zImage-dtb -r initrd.img

push:
	adb push ${DIST_PATH}/${IMAGE_NAME} /sdcard/

flash:
	adb shell su -c 'dd if=/sdcard/${IMAGE_NAME} of=/dev/block/platform/mtk-msdc.0/by-name/boot'
	adb shell su -c 'reboot'

ramdisk_unpack:
	cd ${DIST_PATH}; abootimg -x ${IMAGE_NAME}
	cd ${DIST_PATH}; abootimg-unpack-initrd initrd.img

ramdisk_pack:
	cd ${DIST_PATH}; rm -f initrd.img
	cd ${DIST_PATH}; abootimg-pack-initrd

dist_clean:
	rm -rf ${DIST_PATH}/ramdisk/ zImage

clean: dist_clean
	cd ${KERNEL_PATH}; make ARCH=arm CROSS_COMPILE=arm-eabi- clean
