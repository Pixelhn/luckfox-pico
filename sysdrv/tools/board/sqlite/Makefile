export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/sqlite-autoconf-3530300.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/sqlite-autoconf-3530300
BUILD_TARGET_OUT := ${CURRENT_DIR}/out
INSTALL_TARGET := ${BUILD_TARGET_OUT}/bin/sqlite3

all:
	@test -f $(INSTALL_TARGET) || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS}; \
			make CC=${SYSDRV_CROSS}-gcc -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
		cp -rfp $(INSTALL_TARGET)  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin; \
	)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}