#!/bin/bash

ubinize -o system.ubi -m 2048 -p 0x20000 -v system.cfg || exit

cp system.ubi ./out/system_a.img
cp system.ubi ./out/system_b.img

../../output/out/sysdrv_out/pc/mkenvimage -s 0x40000 -p 0x0 -o out/env.img env.txt