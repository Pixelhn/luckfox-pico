
ifeq ($(SYSDRV_PARAM), )
    SYSDRV_PARAM:=../../../../Makefile.param
    include $(SYSDRV_PARAM)
endif

export LC_ALL=C
SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

# arch=arm default use thumb-2 compile
ifeq ($(SYSDRV_ARCH_TYPE),arm)
PKG_USE_THUMB2 ?= YES
endif

PKG_TARBALL_OPENSSL := openssl-1.1.1h.tar.gz
PKG_NAME_OPENSSL := openssl-1.1.1h
PKG_PATCHES_OPENSSL := openssl-1.1.1h-patches
ifeq ($(SYSDRV_ARCH_TYPE),arm)
PKG_OS_COMPILE_FLAG_OPENSSL := linux-armv4
else
PKG_OS_COMPILE_FLAG_OPENSSL := linux-aarch64
endif

PKG_DEP_ZLIB := $(CURRENT_DIR)/../zlib

PKG_BIN := out

PKG_CONF_OPTS += $(SYSDRV_OPTS) -Os \
				 -I $(PKG_DEP_ZLIB)/out/include \
				 -L $(PKG_DEP_ZLIB)/out/lib

ifeq ($(PKG_USE_THUMB2),YES)
PKG_CONF_OPTS += -mthumb -Wa,-mimplicit-it=thumb -mthumb-interwork
endif

PKG_TARBALL_OPENSSL3 := openssl-3.1.3.tar.gz
PKG_NAME_OPENSSL3 := openssl-3.1.3
PKG_BIN3 := out3
#SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf-

all:openssl3

openssl:
	@test -f $(CURRENT_DIR)/$(PKG_BIN)/lib/libcrypto.a || (\
	rm -fr $(CURRENT_DIR)/$(PKG_NAME_OPENSSL); \
	tar -xf $(PKG_TARBALL_OPENSSL); \
	mkdir -p $(CURRENT_DIR)/$(PKG_BIN); \
	pushd $(CURRENT_DIR)/$(PKG_NAME_OPENSSL)/; \
		cp -af $(CURRENT_DIR)/$(PKG_PATCHES_OPENSSL)/* ./; \
		$(SHELL) ./openssl-1.1.1h.patch.sh ; \
		CFLAGS="$(PKG_CONF_OPTS)" CPPFLAGS="$(PKG_CONF_OPTS)" \
		CXXFLAGS="$(PKG_CONF_OPTS)" LDFLAGS="$(PKG_CONF_OPTS)" \
			./Configure \
			$(PKG_OS_COMPILE_FLAG_OPENSSL) -latomic -lpthread threads shared  \
			no-rc5 enable-camellia enable-mdc2 no-tests no-fuzz-libfuzzer \
			no-fuzz-afl zlib \
			--prefix=$(CURRENT_DIR)/$(PKG_BIN) ;\
			sed -i -e "s#-march=[-a-z0-9] ##" -e "s#-mcpu=[-a-z0-9] ##g" $(CURRENT_DIR)/$(PKG_NAME_OPENSSL)/Makefile; \
			sed -i -e "s#-O[0-9s]#$(PKG_OPENSSL_CFLAGS)#" $(CURRENT_DIR)/$(PKG_NAME_OPENSSL)/Makefile; \
			sed -i -e "s# build_tests##" $(CURRENT_DIR)/$(PKG_NAME_OPENSSL)/Makefile; \
		make -j$(SYSDRV_JOBS) 1>/dev/null || exit -1; \
		make install > /dev/null; \
	popd; )
	cp -rfp $(CURRENT_DIR)/$(PKG_BIN)/lib/*.so*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/lib

openssl3:
	@test -f $(CURRENT_DIR)/$(PKG_BIN3)/lib/libcrypto.a || ( \
	rm -fr $(CURRENT_DIR)/$(PKG_NAME_OPENSSL3); \
	tar -xf $(PKG_TARBALL_OPENSSL3); \
	mkdir -p $(CURRENT_DIR)/$(PKG_BIN3); \
	pushd $(CURRENT_DIR)/$(PKG_NAME_OPENSSL3)/; \
			./Configure \
			$(PKG_OS_COMPILE_FLAG_OPENSSL) \
			--prefix=$(CURRENT_DIR)/$(PKG_BIN3) \
			--cross-compile-prefix=${SYSDRV_CROSS}-; \
			make -j$(SYSDRV_JOBS) || exit -1; \
			make install ; \
	popd; )
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
		cp -rfp $(CURRENT_DIR)/$(PKG_BIN3)/lib/*.so*  $(SYSDRV_DIR_OUT_ROOTFS)/usr/lib; \
	)

distclean:
	-rm -rf $(PKG_BIN) $(PKG_NAME_OPENSSL) $(PKG_BIN3) $(PKG_NAME_OPENSSL3)
	-$(MAKE) -C $(PKG_DEP_ZLIB) clean
