SYSDRV_ARCH_TYPE=arm
SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf


export LC_ALL=C
SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

PKG_TARBALL_PICOCOM := picocom-3.1.zip
PKG_NAME_PICOCOM := picocom-3.1

ifeq ($(SYSDRV_ARCH_TYPE),arm)
PKG_OS_COMPILE_FLAG_IPERF3 := linux-armv4
else
PKG_OS_COMPILE_FLAG_IPERF3 := linux-aarch64
endif

PKG_BIN := out


all:
	@test -f $(CURRENT_DIR)/$(PKG_BIN)/picocom ||(\
	rm -rf $(PKG_BIN) ${PKG_NAME_PICOCOM}; \
	unzip ${PKG_TARBALL_PICOCOM}; \
	mkdir -p $(CURRENT_DIR)/$(PKG_BIN); \
	pushd $(CURRENT_DIR)/$(PKG_NAME_PICOCOM)/; \
		make CC=${SYSDRV_CROSS}-gcc; \
		cp $(CURRENT_DIR)/$(PKG_NAME_PICOCOM)/picocom $(CURRENT_DIR)/$(PKG_BIN); \
		$(SYSDRV_CROSS)-strip $(CURRENT_DIR)/$(PKG_NAME_PICOCOM)/picocom; \
	popd; )
	cp -rfp $(CURRENT_DIR)/$(PKG_BIN)/picocom  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin

distclean:
	-rm -rf $(PKG_BIN) ${PKG_NAME_PICOCOM}