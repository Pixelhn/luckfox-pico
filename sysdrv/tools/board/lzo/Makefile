export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/lzo-2.10.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/lzo-2.10
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/lib/liblzo2.a || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --host=${SYSDRV_CROSS} --prefix=${BUILD_TARGET_OUT};\
			make;\
			make install;\
		popd;)

distclean:
	-rm -rf ${BUILD_TARGET_OUT} ${BUILD_TARGET_OUT}