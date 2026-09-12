CURRENT_DIR := $(shell pwd)
export SHELL:=/bin/bash
export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf-

BUILD_TARGET_OUT := ../../out
KERNEL_DIR :=$(CURRENT_DIR)/../../../source/objs_kernel/


HOSTED_MCU_DRIVER_DIR=$(CURRENT_DIR)/esp-hosted-mcu/host/linux/eh_host_linux_kmod/driver
HOSTED_MCU_DRIVER_DIR_KO=${HOSTED_MCU_DRIVER_DIR}/esp32_spi.ko

HOSTED_MCU_DIR := $(CURRENT_DIR)/esp-hosted-mcu
WIFI_STA_C_APP_DIR := $(HOSTED_MCU_DIR)/examples/wifi/sta/linux_802_3_host/c_app
WIFI_STA_C_APP_BUILD := $(WIFI_STA_C_APP_DIR)/build
HOSTED_TOOLCHAIN_BIN := $(abspath $(CURRENT_DIR)/../../../..)/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin
WIFI_STA_C_APP_BIN := $(WIFI_STA_C_APP_BUILD)/main/wifi_sta_host_c_app

all: hosted-mcu-spi-ko hosted-mcu-spi-app

hosted-mcu-spi-app:
	@test -f $(HOSTED_MCU_DIR)/tools/vendor/esp-idf-kconfig/esp_idf_kconfig/__init__.py || \
		(echo "Missing esp-idf-kconfig submodule; run git submodule update --init $(HOSTED_MCU_DIR)/tools/vendor/esp-idf-kconfig"; exit 1)
	rm -rf $(WIFI_STA_C_APP_BUILD)
	PATH="$(HOSTED_TOOLCHAIN_BIN)":$$PATH EH_PATH="$(HOSTED_MCU_DIR)" cmake -S "$(WIFI_STA_C_APP_DIR)" -B "$(WIFI_STA_C_APP_BUILD)" \
		-DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=arm \
		-DCMAKE_C_COMPILER=$(SYSDRV_CROSS)gcc -DCMAKE_CXX_COMPILER=$(SYSDRV_CROSS)g++ \
		-DCMAKE_AR=$(HOSTED_TOOLCHAIN_BIN)/$(SYSDRV_CROSS)ar -DCMAKE_RANLIB=$(HOSTED_TOOLCHAIN_BIN)/$(SYSDRV_CROSS)ranlib \
		-DCMAKE_STRIP=$(HOSTED_TOOLCHAIN_BIN)/$(SYSDRV_CROSS)strip -DCMAKE_C_FLAGS=-DEH_HOST_LINUX -DCMAKE_BUILD_TYPE=Release
	PATH="$(HOSTED_TOOLCHAIN_BIN)":$$PATH cmake --build "$(WIFI_STA_C_APP_BUILD)" -j$$(nproc)
	@file "$(WIFI_STA_C_APP_BIN)"
	cp $(WIFI_STA_C_APP_BIN) ${BUILD_TARGET_OUT}

hosted-mcu-spi-ko:
	${MAKE} -C ${HOSTED_MCU_DRIVER_DIR} \
		target=spi KERNEL="${KERNEL_DIR}" ARCH="${SYSDRV_ARCH_TYPE}" \
		CONFIG_TEST_RAW_TP=n CONFIG_BT_ENABLED=n \
		CROSS_COMPILE=$(SYSDRV_CROSS)
		cp ${HOSTED_MCU_DRIVER_DIR_KO} ${BUILD_TARGET_OUT}
		cp $(CURRENT_DIR)/insmod_esp_hosted_mcu_spi.sh ${BUILD_TARGET_OUT}
