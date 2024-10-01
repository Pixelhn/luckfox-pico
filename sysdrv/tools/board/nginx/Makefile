export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/nginx-1.26.2.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/nginx-1.26.2
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

LIB_OPENSSL=../toolkits/openssl/
LIB_PRCE2=../prce2/
LIB_ZLIB=../toolkits/zlib/

all:lib_openssl lib_prce2 lib_zlib
	@test -f $(BUILD_TARGET_OUT)/bin/screen || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT}; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
# cp -rfp $(BUILD_TARGET_OUT)/bin/screen*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin;

lib_openssl:
	make -C ${LIB_OPENSSL}

lib_prce2:
	make -C ${LIB_PRCE2}

lib_zlib:
	make -C ${LIB_ZLIB}

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
