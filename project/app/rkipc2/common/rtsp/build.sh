#!/bin/bash

PWD=$(pwd)
SDK_DIR=${PWD}/../../../../..
LIVE_DIR=${PWD}/live
OUT_DIR=${PWD}/out


mkdir -p ${OUT_DIR}
cd ${LIVE_DIR}

make C_COMPILER=arm-rockchip830-linux-uclibcgnueabihf-gcc \
    CPLUSPLUS_COMPILER=arm-rockchip830-linux-uclibcgnueabihf-g++ \
    LINK="arm-rockchip830-linux-uclibcgnueabihf-g++ -o " \
    LIBRARY_LINK="arm-rockchip830-linux-uclibcgnueabihf-ar cr " \
    INCLUDES="-Iinclude -I${LIVE_DIR}/UsageEnvironment/include -I${LIVE_DIR}/groupsock/include -I${LIVE_DIR}/liveMedia/include -I${LIVE_DIR}/BasicUsageEnvironment/include \
    -I${SDK_DIR}/sysdrv/tools/board/toolkits/openssl/out3/include \
    -DLOCALE_NOT_USED" \
    LDFLAGS="-L${SDK_DIR}/sysdrv/tools/board/toolkits/openssl/out3/lib " \
    -j8 || exit

make install PREFIX=${OUT_DIR}
