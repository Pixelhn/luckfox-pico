export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export LIB_NCURSES := $(CURRENT_DIR)/../ncurses

BUILD_TARGET := ${CURRENT_DIR}/libpcap-1.10.5.tar.xz
BUILD_TARGET_DIR := ${CURRENT_DIR}/libpcap-1.10.5
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all: 
	@test -f $(BUILD_TARGET_OUT)/lib/libpcap.so || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS}; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
	cp -rfp $(BUILD_TARGET_OUT)/lib/libpcap.so*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/lib;

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
