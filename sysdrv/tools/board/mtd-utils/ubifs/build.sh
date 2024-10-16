
mkfs_ubi.sh src dst 31457280 oem ubifs

mkfs_ubi.sh src dst 31457280 oem squashfs

mkfs.ubifs -x lzo -e 126976 -m 2048 -c 247 -d src -F -v -o dst/.ubi_cfg/oem_2KB_128KB_30MB.ubifs

mksquashfs src dts/.ubi_cfg/oem_30MB.squashfs -noappend -processors 9 -comp xz

ubinize -o dst/oem_2KB_128KB_30MB.ubi -m 2048 -p 0x20000 -v dst/.ubi_cfg/oem_2KB_128KB_30MB_ubinize.cfg


# squashfs
[ubifs]
mode=ubi
vol_id=0
vol_type=static
vol_name=oem
vol_alignment=1
vol_flags=autoresize
image=dts/.ubi_cfg/oem_30MB.squashfs

# ubifs
[ubifs]
mode=ubi
vol_id=0
vol_type=dynamic
vol_name=oem
vol_alignment=1
vol_flags=autoresize
image=dts/.ubi_cfg/oem_2KB_128KB_30MB.ubifs

