export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/nginx-1.26.2.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/nginx-1.26.2
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/bin/screen || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT}; \
			make -j$(SYSDRV_JOBS) CC=${SYSDRV_CROSS}-gcc; \
			make install CC=${SYSDRV_CROSS}-gcc; \
		popd;)
# cp -rfp $(BUILD_TARGET_OUT)/bin/screen*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin;

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
