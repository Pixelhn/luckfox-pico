SYSDRV_ARCH_TYPE=arm
SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf


export LC_ALL=C
SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

PKG_TARBALL_IPERF3 := iperf-3.15.tar.gz
PKG_NAME_IPERF3 := iperf-3.15

ifeq ($(SYSDRV_ARCH_TYPE),arm)
PKG_OS_COMPILE_FLAG_IPERF3 := linux-armv4
else
PKG_OS_COMPILE_FLAG_IPERF3 := linux-aarch64
endif

PKG_BIN := out


PKG_DEP_ZLIB := $(CURRENT_DIR)/../zlib
PKG_DEP_OPENSSL := $(CURRENT_DIR)/../openssl

PKG_CONF_OPTS += $(SYSDRV_OPTS) -Os \
				 -I $(PKG_DEP_OPENSSL)/out/include \
				 -L$(PKG_DEP_OPENSSL)/out/lib \
				 -I $(PKG_DEP_ZLIB)/out/include \
				 -L$(PKG_DEP_ZLIB)/out/lib \
				 -lz

all:
	@test -f $(CURRENT_DIR)/$(PKG_BIN)/bin/iperf3 ||(\
	tar -xf ${PKG_TARBALL_IPERF3}; \
	mkdir -p $(CURRENT_DIR)/$(PKG_BIN); \
	pushd $(CURRENT_DIR)/$(PKG_NAME_IPERF3)/; \
			CFLAGS="$(PKG_CONF_OPTS)" CPPFLAGS="$(PKG_CONF_OPTS)" \
			CXXFLAGS="$(PKG_CONF_OPTS)" LDFLAGS="$(PKG_CONF_OPTS)" \
			./configure \
			--host=$(SYSDRV_CROSS) \
			--prefix=$(CURRENT_DIR)/$(PKG_BIN) \
			CFLAGS=-static;\
		make -j$(SYSDRV_JOBS) || exit -1; \
		make install > /dev/null; \
		$(SYSDRV_CROSS)-strip $(CURRENT_DIR)/$(PKG_BIN)/bin/iperf3; \
	popd; )
	cp -rfp $(CURRENT_DIR)/$(PKG_BIN)/bin/iperf3  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin

distclean:
	-rm -rf $(PKG_BIN) $(PKG_NAME_IPERF3)