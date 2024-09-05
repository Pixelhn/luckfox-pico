export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export LIBPCAP := $(CURRENT_DIR)/../libpcap
export LIB_OPENSSL := $(CURRENT_DIR)/../toolkits/openssl

BUILD_TARGET := ${CURRENT_DIR}/tcpdump-4.99.5.tar.xz
BUILD_TARGET_DIR := ${CURRENT_DIR}/tcpdump-4.99.5
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all: lib_openssl libpcap
	@test -f $(BUILD_TARGET_OUT)/bin/tcpdump || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS} \
			LDFLAGS='-L${LIBPCAP}/out/lib -L${LIB_OPENSSL}/out3/lib'\
			CPPFLAGS='-I${LIBPCAP}/out/include -I${LIB_OPENSSL}/out3/include'; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
	cp -rfp $(BUILD_TARGET_OUT)/bin/tcpdump  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin;

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}
	-$(MAKE) -C ${LIBPCAP} clean

lib_openssl:
	$(MAKE) -C ${LIB_OPENSSL}

libpcap:
	$(MAKE) -C ${LIBPCAP}