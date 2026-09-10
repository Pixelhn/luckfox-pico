APP_PARAM ?= ../Makefile.param
include $(APP_PARAM)
SHELL := /bin/bash
BR_DIR := $(abspath ../../../sysdrv/source/buildroot/buildroot-2026.02.2)
BR_SYSROOT := $(BR_DIR)/output/staging
PKG_CONFIG := $(BR_DIR)/output/host/bin/pkg-config
PKG_ENV := PKG_CONFIG_SYSROOT_DIR=$(BR_SYSROOT) PKG_CONFIG_LIBDIR=$(BR_SYSROOT)/usr/lib/pkgconfig:$(BR_SYSROOT)/usr/share/pkgconfig
CC := $(RK_APP_CROSS)-gcc
SOURCES := src/main.c src/protocol.c src/wifi.c
CFLAGS += -O2 -g -Wall -Wextra -Wformat=2 -Wno-misleading-indentation
.PHONY: all clean info test
ifeq ($(RK_ENABLE_BLUFI),y)
all: out/bin/blufi_service
	install -m 755 packaging/blufi-start out/bin/blufi-start
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT,$(RK_APP_OUTPUT),out)
else
all:
	@echo "blufi_service disabled for this board"
endif
out/bin/blufi_service: $(SOURCES) src/protocol.h src/wifi.h
	@$(PKG_ENV) $(PKG_CONFIG) --exists gio-2.0 openssl || { echo 'Build BlueZ/OpenSSL dependencies with ./build.sh firmware first'; exit 1; }
	mkdir -p out/bin
	$(CC) --sysroot=$(BR_SYSROOT) $(CFLAGS) $(SOURCES) -o $@ $$($(PKG_ENV) $(PKG_CONFIG) --cflags --libs gio-2.0 openssl) -Wl,-rpath-link,$(BR_SYSROOT)/usr/lib
clean:
	rm -rf out
info:
	@echo "RK_ENABLE_BLUFI=$(RK_ENABLE_BLUFI)"
