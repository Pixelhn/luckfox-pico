#!/bin/bash

make build/rkipc RK_APP_TYPE=RKIPC_RV1103 || exit



sudo cp build/rkipc ~/nfs/
sudo cp src/rkipc-300w.ini ~/nfs