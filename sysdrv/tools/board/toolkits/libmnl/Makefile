export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/libmnl-1.0.5.tar.bz2
BUILD_TARGET_DIR := ${CURRENT_DIR}/libmnl-1.0.5
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/lib/libmnl.a || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS} --enable-static; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
