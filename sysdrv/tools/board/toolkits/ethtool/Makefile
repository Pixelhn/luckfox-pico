export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/ethtool-6.11.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/ethtool-6.11
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

export PKG_LIB_MNL := $(CURRENT_DIR)/../libmnl/

all: lib_mnl
	@test -f $(BUILD_TARGET_OUT)/sbin/ethtool || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			./configure --prefix=${BUILD_TARGET_OUT} --host=${SYSDRV_CROSS} \
			MNL_CFLAGS="-I${PKG_LIB_MNL}/out/include" MNL_LIBS="-L${PKG_LIB_MNL}/out/lib -lmnl"; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}

lib_mnl:
	$(MAKE) -C ${PKG_LIB_MNL}