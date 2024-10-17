#!/bin/sh

ESP_RSETET_PIN=4
IF_NAME=espsta0
WIFI_CONF=/userdata/wpa_supplicant.conf
RTTRY_TIMES=3

step1_check_net_dev()
{
    i=0
    while [ ${i} -le ${RTTRY_TIMES} ]
    do
        IF_STATUS=$(cat /proc/net/dev | grep esp | awk '{printf $1}')
        echo ${IF_STATUS}
        if [ -n ${IF_STATUS} ]; then
            echo if up
            return 0
        fi
    
        sleep 1
        echo ${i}
        i=`expr $i + 1`
    done

    return 1
}


wait_wap()
{
    i=0
    while [ ${i} -le ${RTTRY_TIMES} ]
    do
        WPA_STATE=$(wpa_cli status | grep wpa_state)

        if [ "wpa_state=COMPLETED" = ${WPA_STATE} ]; then
                echo connected
                return 0
        fi

        sleep 1
        ((i++))
    done

    return 1
}

insmod cfg80211.ko

insmod esp32_spi.ko resetpin=${ESP_RSETET_PIN}

killall wpa_supplicant

if [ ! -f ${WIFI_CONF} ]; then
    echo no wifi conf file ${WIFI_CONF}
    exit 1
fi

sleep 2

ifconfig ${IF_NAME} up

wpa_supplicant -D nl80211 -i ${IF_NAME} -c ${WIFI_CONF}  &

wait_wap || exit

udhcpc -i  ${IF_NAME} -T 1 -A 0 -b -q