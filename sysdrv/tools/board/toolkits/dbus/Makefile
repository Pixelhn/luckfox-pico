CURRENT_DIR := $(shell pwd)
export SHELL:=/bin/bash

BUILD_TARGET := ${CURRENT_DIR}/dbus-1.14.10.tar.xz
BUILD_TARGET_DIR := ${CURRENT_DIR}/dbus-1.14.10
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

PKG_CONFIG_PATH=${PKG_LIB_EXPAT}/out/lib/pkgconfig

all:
	@test -f $(BUILD_TARGET_OUT)/lib/libdbus-1.so || ( \
		-rm -rf $(BUILD_TARGET_DIR) ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p ${BUILD_TARGET_OUT}; \
		pushd $(BUILD_TARGET_DIR)/; \
			export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS}; \
			make; \
			make install; \
		popd;)

clean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}