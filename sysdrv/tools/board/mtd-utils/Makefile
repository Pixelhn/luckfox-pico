
ifeq ($(SYSDRV_PARAM), )
    SYSDRV_PARAM:=../../../Makefile.param
    include $(SYSDRV_PARAM)
endif

export LC_ALL=C
SHELL:=/bin/bash

CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/mtd-utils-2.2.1.tar.bz2
BUILD_TARGET_DIR := ${CURRENT_DIR}/mtd-utils-2.2.1
BUILD_TARGET_OUT := ${CURRENT_DIR}/out
INSTALL_TARGET := ${BUILD_TARGET_OUT}/sbin/ubinfo

all:
	@test -f $(INSTALL_TARGET) || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --host=$(SYSDRV_CROSS) --target=$(SYSDRV_CROSS) CFLAGS="$(SYSDRV_CROSS_CFLAGS)" \
				LDFLAGS="$(SYSDRV_CROSS_CFLAGS)" --prefix=$(BUILD_TARGET_OUT) \
				--disable-static --enable-shared \
				--disable-tests --disable-install-tests \
				--without-jffs --without-ubifs \
				--without-xattr ; \
			make -j$(SYSDRV_JOBS) || exit -1; \
			make install; \
		popd;)
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
		pushd $(BUILD_TARGET_OUT)/sbin/; \
		cp -vfa	\
			flashcp flash_erase flash_eraseall flash_lock flash_unlock \
			nanddump nandwrite \
			ubiattach ubiblock ubidetach ubiformat ubimkvol ubirename ubirmvol ubirsvol ubinfo \
			$(SYSDRV_DIR_OUT_ROOTFS)/usr/bin; \
		popd; \
	)

clean: distclean

distclean:
	-rm -rf $(PKG_NAME) $(PKG_BIN)

