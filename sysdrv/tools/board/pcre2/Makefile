export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/pcre2-10.44.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/pcre2-10.44
BUILD_TARGET_OUT := ${CURRENT_DIR}/out
INSTALL_TARGET := $(BUILD_TARGET_OUT)/lib/libpcre2-8.so.0.13.0

all:
	@test -f $(INSTALL_TARGET) || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS}; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
		cp -rfp $(BUILD_TARGET_OUT)/lib/libpcre2-8.so*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/lib; \
	)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
