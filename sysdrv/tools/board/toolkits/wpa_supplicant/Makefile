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

PKG_CONFIG_PATH2=${PKG_LIB_DBUS}/out/lib/pkgconfig:${PKG_LIB_NL}/out/lib/pkgconfig

all: lib_dbus lib_nl
	( \
		pushd $(BUILD_TARGET_DIR)/; \
		export PKG_CONFIG_PATH=${PKG_CONFIG_PATH2}; \
		echo ${PKG_CONFIG_PATH}; \
		${MAKE} -C ${BUILD_TARGET_DIR}; \
	popd;)

lib_dbus:lib_expat
	$(MAKE) -C ${PKG_LIB_DBUS}

lib_nl:
	$(MAKE) -C ${PKG_LIB_NL}

lib_expat:
	$(MAKE) -C ${PKG_LIB_EXPAT}

clean:
	$(MAKE) -C ${PKG_LIB_EXPAT}	clean
	$(MAKE) -C ${PKG_LIB_DBUS} clean
	$(MAKE) -C ${PKG_LIB_NL} clean