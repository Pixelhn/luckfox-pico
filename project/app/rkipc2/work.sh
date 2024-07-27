#!/bin/bash

make build/rkipc || exit



cp build/rkipc ~/nfs/
cp src/rv1103_ipc/rkipc-300w.ini ~/nfs