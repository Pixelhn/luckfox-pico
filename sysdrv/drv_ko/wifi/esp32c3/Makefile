CURRENT_DIR := $(shell pwd)
export SHELL:=/bin/bash
export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf-

TARGET := esp32_spi.ko

BUILD_TARGET := ${CURRENT_DIR}/esp32_spi.ko
BUILD_TARGET_DIR := ${CURRENT_DIR}/esp-hosted/esp_hosted_ng/host
BUILD_TARGET_OUT := ../../out
KERNEL_DIR :=$(CURRENT_DIR)/../../../source/kernel/

all:
	${MAKE} -C ${BUILD_TARGET_DIR} \
	CROSS_COMPILE=$(SYSDRV_CROSS) \
	KERNEL=$(KERNEL_DIR) \
	target=spi ARCH=${SYSDRV_ARCH_TYPE}
	$(CROSS_COMPILE)strip ${BUILD_TARGET_DIR}/${TARGET} --strip-debug
	cp ${BUILD_TARGET_DIR}/${TARGET} ${BUILD_TARGET_OUT}


clean:
	${MAKE} -C ${BUILD_TARGET_DIR} clean \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	KERNEL=$(KERNEL_DIR) \
	target=spi