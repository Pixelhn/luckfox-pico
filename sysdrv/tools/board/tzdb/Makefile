export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export LIB_NCURSES := $(CURRENT_DIR)/../ncurses

BUILD_TARGET := ${CURRENT_DIR}/tzdb-2024a.tar.lz
BUILD_TARGET_DIR := ${CURRENT_DIR}/tzdb-2024a
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/usr/share/zoneinfo/Asia/Shanghai || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			make -j$(SYSDRV_JOBS); \
			make install DESTDIR=$(BUILD_TARGET_OUT); \
		popd;)
	cp -rfp $(BUILD_TARGET_OUT)/usr/share/zoneinfo  $(SYSDRV_DIR_OUT_ROOTFS)/usr/share;

#CC=${SYSDRV_CROSS}-gcc CFLAGS=-DHAVE_STRFTIME_L=0

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
