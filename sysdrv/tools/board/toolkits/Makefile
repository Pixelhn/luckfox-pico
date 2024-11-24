ifeq ($(SYSDRV_PARAM), )
    SYSDRV_PARAM:=../../../Makefile.param
    include $(SYSDRV_PARAM)
endif

export LC_ALL=C
export SHELL:=/bin/bash

CURRENT_DIR := $(shell pwd)
PKG_BIN := out

PKG_CONF_OPTS += $(SYSDRV_CROSS_CFLAGS) -Os

ifeq ($(PKG_USE_THUMB2),YES)
PKG_CONF_OPTS += -mthumb -Wa,-mimplicit-it=thumb -mthumb-interwork
endif

PKG_DEP_OPENSSL := ./openssl
PKG_DEP_ZLIB := ./zlib
PKG_DEP_IPERF3 := ./iperf3
PKG_DEP_PICOCOM := ./picocom
PKG_DEP_WPA_SUPPLICANT := ./wpa_supplicant
PKG_DEP_OPENSSH := ./openssh
PKG_DEP_DROPBEAR := ./dropbear

all:dep_openssl tool_iperf3 tool_picocom tool_wpa_supplicant tool_hostapd tool_dropear tool_openssh

tool_io:
	rm -rf $(PKG_BIN)/io;
	mkdir -p $(CURRENT_DIR)/$(PKG_BIN)/usr/bin;
	$(SYSDRV_CROSS)-gcc $(PKG_CONF_OPTS) io.c -o $(PKG_BIN)/usr/bin/io
	$(SYSDRV_CROSS)-strip -s $(PKG_BIN)/usr/bin/io
	$(call MAROC_COPY_PKG_TO_SYSDRV_OUTPUT, $(SYSDRV_DIR_OUT_ROOTFS), $(CURRENT_DIR)/$(PKG_BIN))

tool_dropear:
	${MAKE} -C ${PKG_DEP_DROPBEAR}

#only use /usr/libexec/sftp-server
tool_openssh:
	$(MAKE) -C $(PKG_DEP_OPENSSH)

tool_wpa_supplicant:
	$(MAKE) -C $(PKG_DEP_WPA_SUPPLICANT)

tool_hostapd:
	$(MAKE) -C ./hostapd

tool_picocom:
	$(MAKE) -C $(PKG_DEP_PICOCOM)

tool_iperf3:
	$(MAKE) -C $(PKG_DEP_IPERF3)

dep_openssl:
	$(MAKE) -C $(PKG_DEP_OPENSSL)

dep_zlib:
	$(MAKE) -C $(PKG_DEP_ZLIB)

clean: distclean

distclean:
	-rm -rf $(CURRENT_DIR)/$(PKG_BIN)
	$(MAKE) -C $(PKG_DEP_IPERF3) distclean
	$(MAKE) -C $(PKG_DEP_OPENSSL) distclean
	$(MAKE) -C $(PKG_DEP_ZLIB) distclean
	$(MAKE) -C $(PKG_DEP_WPA_SUPPLICANT) distclean
	${MAKE} -C ${PKG_DEP_DROPBEAR} distclean
	$(MAKE) -C $(PKG_DEP_OPENSSH) distclean
	$(MAKE) -C $(PKG_DEP_PICOCOM) distclean
	$(MAKE) -C ./hostapd distclean