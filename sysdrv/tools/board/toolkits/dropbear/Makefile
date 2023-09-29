export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export PKG_LIB_ZLIB := $(CURRENT_DIR)/../zlib

BUILD_TARGET := ${CURRENT_DIR}/dropbear-2022.83.tar.bz2
BUILD_TARGET_DIR := ${CURRENT_DIR}/dropbear-2022.83
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all: lib_zlib
	@test -f $(BUILD_TARGET_OUT)/usr/local/sbin/sshd || ( \
		rm -rf ${BUILD_TARGET_DIR}/ ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure  --host=${SYSDRV_CROSS}  \
			--prefix=${BUILD_TARGET_OUT} \
			--with-zlib=${PKG_LIB_ZLIB}/out;  \
			make; \
			make install; \
			popd;)
#cp -rfp $(BUILD_TARGET_OUT)/*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin;

lib_zlib:
	$(MAKE) -C ${PKG_LIB_ZLIB}

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
	$(MAKE) -C ${PKG_LIB_OPENSSL}	clean