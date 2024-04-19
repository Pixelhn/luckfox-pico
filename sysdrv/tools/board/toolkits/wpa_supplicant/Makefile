export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

export PKG_LIB_EXPAT := $(CURRENT_DIR)/../libexpat
export PKG_LIB_DBUS := $(CURRENT_DIR)/../dbus
export PKG_LIB_NL := $(CURRENT_DIR)/../libnl
export PKG_LIB_OPENSSL := $(CURRENT_DIR)/../openssl

BUILD_TARGET := ${CURRENT_DIR}/wpa_supplicant-2.10.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/wpa_supplicant-2.10/wpa_supplicant/
BUILD_TARGET_OUT := ${CURRENT_DIR}/out

PKG_CONFIG_PATH3=${PKG_LIB_DBUS}/out/lib/pkgconfig:${PKG_LIB_NL}/out/lib/pkgconfig:${PKG_LIB_OPENSSL}/out3/lib/pkgconfig

all: lib_nl lib_openssl
	@test -f $(BUILD_TARGET_OUT)/wpa_supplicant || ( \
		rm -rf ${CURRENT_DIR}/wpa_supplicant-2.10 ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		cp defconfig ${BUILD_TARGET_DIR}/.config; \
		pushd $(BUILD_TARGET_DIR)/; \
		export PKG_CONFIG_PATH="${PKG_CONFIG_PATH3}"; \
		echo "LIBS += -L${PKG_LIB_NL}/out/lib" >> .config; \
		echo "LIBS += -L${PKG_LIB_OPENSSL}/out3/lib" >> .config; \
		echo "CFLAGS += -I${PKG_LIB_OPENSSL}/out3/include" >> .config; \
		echo "CFLAGS += -I${PKG_LIB_NL}/out/include" >> .config; \
		echo "CC=${SYSDRV_CROSS}-gcc" >> .config; \
		${MAKE} -C ${BUILD_TARGET_DIR} PKG_CONFIG_PATH=${PKG_CONFIG_PATH3} -j$(SYSDRV_JOBS); \
		cp ${BUILD_TARGET_DIR}/wpa_supplicant ${BUILD_TARGET_DIR}/wpa_cli ${BUILD_TARGET_OUT}; \
		${SYSDRV_CROSS}-strip ${BUILD_TARGET_OUT}/*; \
	popd;)
	cp -rfp $(BUILD_TARGET_OUT)/*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin;

lib_dbus:lib_expat
	$(MAKE) -C ${PKG_LIB_DBUS}

lib_nl:
	$(MAKE) -C ${PKG_LIB_NL}

lib_expat:
	$(MAKE) -C ${PKG_LIB_EXPAT}

lib_openssl:
	$(MAKE) -C ${PKG_LIB_OPENSSL}

distclean:
	-rm -rf ${CURRENT_DIR}/wpa_supplicant-2.10 ${BUILD_TARGET_OUT}
	$(MAKE) -C ${PKG_LIB_EXPAT}	clean
	$(MAKE) -C ${PKG_LIB_DBUS} clean
	$(MAKE) -C ${PKG_LIB_NL} clean