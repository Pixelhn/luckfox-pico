ifeq ($(MEDIA_PARAM), )
    MEDIA_PARAM:=../Makefile.param
    include $(MEDIA_PARAM)
endif

export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/libevdev-1.13.4.tar.xz
BUILD_TARGET_DIR := ${CURRENT_DIR}/libevdev-1.13.4
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/lib/libiconv.a || ( \
	rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
	tar -xf ${BUILD_TARGET}; \
	mkdir -p $(BUILD_TARGET_OUT); \
	pushd $(BUILD_TARGET_DIR); \
		./configure --prefix=${BUILD_TARGET_OUT}  --host=${SYSDRV_CROSS} --enable-static --disable-share; \
		make -j; \
		make install; \
	popd;)

distclean:
	-rm -rf ${BUILD_TARGET_OUT} ${BUILD_TARGET_OUT} ${BUILD_TARGET_DIR}
