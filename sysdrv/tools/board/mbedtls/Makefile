export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/mbedtls-4.1.0.tar.bz2
BUILD_TARGET_DIR := ${CURRENT_DIR}/mbedtls-4.1.0
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

all: mbedtls4

mbedtls3:
	@test -f $(BUILD_TARGET_OUT)/lib/libmbedtls.a || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			make DESTDIR=${BUILD_TARGET_OUT} CC=${SYSDRV_CROSS}-gcc lib install -j16 || exit -1; \
		popd;)

mbedtls4:
	@test -f $(BUILD_TARGET_OUT)/lib/libmbedtls.a || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			mkdir build && cd build; \
			export CC=${SYSDRV_CROSS}-gcc; \
			cmake $(BUILD_TARGET_DIR) -DENABLE_TESTING=Off -DCMAKE_INSTALL_PREFIX=${BUILD_TARGET_OUT}; \
			make -C $(BUILD_TARGET_DIR)/build -j8 && make -C $(BUILD_TARGET_DIR)/build install; \
		popd;)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}