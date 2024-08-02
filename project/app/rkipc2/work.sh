#!/bin/bash

# cd common/rtsp

# ./build.sh

# cd -

make RK_APP_TYPE=RKIPC_RV1106 build/rkipc



sudo cp build/rkipc ~/nfs

