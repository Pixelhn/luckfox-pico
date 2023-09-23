export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)


export PKG_LIB_EXPAT := $(CURRENT_DIR)/../libexpat
export PKG_LIB_DBUS := $(CURRENT_DIR)/../dbus
export PKG_LIB_NL := $(CURRENT_DIR)/../libnl

all: lib_expat lib_dbus lib_nl


lib_expat:
	$(MAKE) -C ${PKG_LIB_EXPAT}

lib_dbus:
	$(MAKE) -C ${PKG_LIB_DBUS}

lib_nl:
	$(MAKE) -C ${PKG_LIB_NL}

clean:
	$(MAKE) -C ${PKG_LIB_EXPAT}	clean
	$(MAKE) -C ${PKG_LIB_DBUS} clean
	$(MAKE) -C ${PKG_LIB_NL} clean