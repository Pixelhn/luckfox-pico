CURRENT_DIR := $(shell pwd)
export SHELL:=/bin/bash

BUILD_TARGET := ${CURRENT_DIR}/libnl-3.8.0.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/libnl-3.8.0
BUILD_TARGET_OUT := ${CURRENT_DIR}/out


all:
	@test -f $(BUILD_TARGET_OUT)/lib/libnl-3.so || ( \
		rm -rf $(BUILD_TARGET_DIR) ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p ${BUILD_TARGET_OUT}; \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS}; \
			make; \
			make install; \
		popd;)

clean:
	-rm -rf $(BUILD_TARGET_DIR) ${BUILD_TARGET_OUT}