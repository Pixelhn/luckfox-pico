#!/bin/sh

ESP_RSETET_PIN=4

cd /oem/usr/ko/
insmod cfg80211.ko
insmod esp32_spi.ko resetpin=${ESP_RSETET_PIN}
