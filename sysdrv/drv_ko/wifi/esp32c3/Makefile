CURRENT_DIR := $(shell pwd)
export SHELL:=/bin/bash

TARGET := esp32_spi.ko

BUILD_TARGET := ${CURRENT_DIR}/esp32_spi.ko
BUILD_TARGET_DIR := ${CURRENT_DIR}/esp-hosted/esp_hosted_ng/host
BUILD_TARGET_OUT := ../../out

all:
	${MAKE} -C ${BUILD_TARGET_DIR} \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	KERNEL=$(KERNEL_DIR) \
	target=spi
	$(CROSS_COMPILE)strip ${BUILD_TARGET_DIR}/${TARGET} --strip-debug
	cp ${BUILD_TARGET_DIR}/${TARGET} ${BUILD_TARGET_OUT}


clean:
	${MAKE} -C ${BUILD_TARGET_DIR} clean \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	KERNEL=$(KERNEL_DIR) \
	target=spi