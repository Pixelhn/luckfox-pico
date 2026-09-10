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
PKG_CONF_OPTS += $(SYSDRV_OPTS) -Os

all:
	@test -f $(CURRENT_DIR)/$(PKG_BIN)/bin/iperf3 ||(\
	tar -xf ${PKG_TARBALL_IPERF3}; \
	mkdir -p $(CURRENT_DIR)/$(PKG_BIN); \
	pushd $(CURRENT_DIR)/$(PKG_NAME_IPERF3)/; \
			CFLAGS="$(PKG_CONF_OPTS) -static" \
			CXXFLAGS="$(PKG_CONF_OPTS) -static" LDFLAGS="-static" \
			./configure \
			--host=$(SYSDRV_CROSS) \
			--prefix=$(CURRENT_DIR)/$(PKG_BIN) \
			--with-openssl=no;\
		make -j$(SYSDRV_JOBS) || exit -1; \
		make install > /dev/null; \
		$(SYSDRV_CROSS)-strip $(CURRENT_DIR)/$(PKG_BIN)/bin/iperf3; \
	popd; )
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
	cp -rfp $(CURRENT_DIR)/$(PKG_BIN)/bin/iperf3  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin \
	)

distclean:
	-rm -rf $(PKG_BIN) $(PKG_NAME_IPERF3)
