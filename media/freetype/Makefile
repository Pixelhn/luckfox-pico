ifeq ($(MEDIA_PARAM), )
    MEDIA_PARAM:=../Makefile.param
    include $(MEDIA_PARAM)
endif

export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export PKG_LIB_ZLIB := $(CURRENT_DIR)/../../../sysdrv/tools/board/toolkits/zlib/

PKG_CONFIG_PATH3=${PKG_LIB_ZLIB}/out/lib/pkgconfig

BUILD_TARGET := ${CURRENT_DIR}/freetype-2.13.2.tar.xz
BUILD_TARGET_DIR := ${CURRENT_DIR}/freetype-2.13.2
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all:
	@test -f $(BUILD_TARGET_OUT)/lib/libfreetype.so || ( \
	rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
	tar -xf ${BUILD_TARGET}; \
	mkdir -p $(BUILD_TARGET_OUT); \
	pushd $(BUILD_TARGET_DIR); \
		export PKG_CONFIG_PATH="${PKG_CONFIG_PATH3}"; \
		./configure --prefix=${BUILD_TARGET_OUT}  --host=${SYSDRV_CROSS} \
		--with-zlib=${PKG_LIB_ZLIB}/out --with-brotli=no --with-png=no --with-harfbuzz=no;  \
		make -j; \
		make install; \
	popd;)
	-rm -rf ${CURRENT_DIR}/out/share
	$(call MAROC_COPY_PKG_TO_MEDIA_OUTPUT, $(RK_MEDIA_OUTPUT), $(BUILD_TARGET_OUT))

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
