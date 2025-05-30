export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/mbedtls-3.6.3.1.tar.bz2
BUILD_TARGET_DIR := ${CURRENT_DIR}/mbedtls-3.6.3.1
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/lib/libmbedtls.a || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			make DESTDIR=${BUILD_TARGET_OUT} CC=${SYSDRV_CROSS}-gcc lib install -j16 || exit -1; \
		popd;)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}