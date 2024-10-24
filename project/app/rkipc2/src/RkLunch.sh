#!/bin/sh

rcS()
{
	for i in /oem/usr/etc/init.d/S??* ;do

		# Ignore dangling symlinks (if any).
		[ ! -f "$i" ] && continue

		case "$i" in
			*.sh)
				# Source shell script for speed.
				(
					trap - INT QUIT TSTP
					set start
					. $i
				)
				;;
			*)
				# No sh extension, so fork subprocess.
				$i start
				;;
		esac
	done
}

check_linker()
{
        [ ! -L "$2" ] && ln -sf $1 $2
}

post_chk()
{
	#TODO: ensure /userdata mount done
	cnt=0
	while [ $cnt -lt 30 ];
	do
		cnt=$(( cnt + 1 ))
		if mount | grep -w userdata; then
			break
		fi
		sleep .1
	done


	# if /data/rkipc not exist, cp /usr/share
	rkipc_ini=/userdata/rkipc.ini
	default_rkipc_ini=/oem/usr/share/rkipc-300w.ini

	if [ ! -f "/oem/usr/share/rkipc.ini" ]; then
		lsmod | grep sc3336
		if [ $? -eq 0 ] ;then
			default_rkipc_ini=/oem/usr/share/rkipc-300w.ini
		fi
	fi

	if [ ! -f $rkipc_ini ]; then
		cp -fa $default_rkipc_ini $rkipc_ini
	fi

	if [ ! -f "/userdata/image.bmp" ]; then
		cp -fa /oem/usr/share/image.bmp /userdata/
	fi

	if [ -d "/oem/usr/share/iqfiles" ];then
		rkipc -a /oem/usr/share/iqfiles &
	fi
}

rcS

ulimit -c unlimited
echo "/mnt/nfs/1103/core-%p-%e" > /proc/sys/kernel/core_pattern
# echo 0 > /sys/devices/platform/rkcif-mipi-lvds/is_use_dummybuf

echo 1 > /proc/sys/vm/overcommit_memory

post_chk &
