#!/bin/bash

make build/rkipc || exit



sudo cp build/rkipc ~/nfs/
sudo cp src/rv1103_ipc/rkipc-300w.ini ~/nfs