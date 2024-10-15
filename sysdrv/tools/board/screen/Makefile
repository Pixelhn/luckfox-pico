export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export LIB_NCURSES := $(CURRENT_DIR)/../ncurses

BUILD_TARGET := ${CURRENT_DIR}/screen-4.9.1.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/screen-4.9.1
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all: lib_ncurses
	@test -f $(BUILD_TARGET_OUT)/bin/screen || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} \
			LDFLAGS=-L${LIB_NCURSES}/out/lib \
			CPPFLAGS=-I${LIB_NCURSES}/out/include \
			LIBS=-lncurses; \
			make CC=${SYSDRV_CROSS}-gcc -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
		cp -rfp $(BUILD_TARGET_OUT)/bin/screen*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin; \
	)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}


lib_ncurses:
	$(MAKE) -C ${LIB_NCURSES}