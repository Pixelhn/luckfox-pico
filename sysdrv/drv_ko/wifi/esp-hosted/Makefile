CURRENT_DIR := $(shell pwd)
export SHELL:=/bin/bash
export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf-

BUILD_TARGET_OUT := ../../out
KERNEL_DIR :=$(CURRENT_DIR)/../../../source/objs_kernel/


HOSTED_MCU_DRIVER_DIR=$(CURRENT_DIR)/esp-hosted-mcu/host/linux/eh_host_linux_kmod/driver
HOSTED_MCU_DRIVER_DIR_KO=${HOSTED_MCU_DRIVER_DIR}/esp32_spi.ko

hosted-mcu-spi-ko:
	${MAKE} -C ${HOSTED_MCU_DRIVER_DIR} \
		target=spi KERNEL="${KERNEL_DIR}" ARCH="${SYSDRV_ARCH_TYPE}" \
		CONFIG_TEST_RAW_TP=n CONFIG_BT_ENABLED=n \
		CROSS_COMPILE=$(SYSDRV_CROSS)
		cp ${HOSTED_MCU_DRIVER_DIR_KO} ${BUILD_TARGET_OUT}