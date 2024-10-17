#!/bin/bash
export PKG_CONFIG_PATH=/home/lhn/Git/rv1106/luckfox-pico/sysdrv/tools/board/toolkits/libexpat/out/lib/pkgconfig
cd dbus-1.14.10/

./configure --prefix=/home/lhn/Git/rv1106/luckfox-pico/sysdrv/tools/board/toolkits/dbus/out --host=arm-rockchip830-linux-uclibcgnueabihf \
	CFLAGS=-I/home/lhn/Git/rv1106/luckfox-pico/sysdrv/tools/board/toolkits/libexpat/out/include \
	LDFLAGS=-L/home/lhn/Git/rv1106/luckfox-pico/sysdrv/tools/board/toolkits/libexpat/out/lib \
	LIBS=-lexpat
