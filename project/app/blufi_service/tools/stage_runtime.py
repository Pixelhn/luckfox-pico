#!/usr/bin/env python3
"""Stage already-built BluFi/BlueZ binaries and their non-libc dependencies.

Usage: stage_runtime.py --dest /path/to/nfs-share/blufi-test
Build first through ./build.sh app and ./build.sh firmware.
Atomic replacements avoid overwriting an executable currently mapped on the board.
"""
import argparse
from pathlib import Path
import os
import re
import shutil
import subprocess

sdk = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dest', type=Path, required=True)
args = parser.parse_args()
target = sdk / 'sysdrv/source/buildroot/buildroot-2026.02.2/output/target'
app = sdk / 'project/app/blufi_service/out/bin/blufi_service'
files = [(target / p, p) for p in (
    'usr/bin/dbus-daemon', 'usr/bin/dbus-uuidgen', 'usr/bin/bluetoothctl',
    'usr/bin/hciattach', 'usr/bin/hciconfig', 'usr/bin/btmon',
    'usr/bin/gdbus', 'usr/libexec/bluetooth/bluetoothd')]
files.append((app, 'usr/bin/blufi_service'))
skip = {'libc.so.0', 'libm.so.0', 'libpthread.so.0', 'libdl.so.0', 'librt.so.0', 'ld-uClibc.so.0'}
seen = set()
while files:
    src, rel = files.pop()
    if rel in seen:
        continue
    if not src.exists():
        raise SystemExit(f'Missing build artifact: {src}')
    seen.add(rel)
    dst = args.dest / rel
    dst.parent.mkdir(parents=True, exist_ok=True)
    tmp = dst.with_name(dst.name + '.new')
    shutil.copy2(src, tmp, follow_symlinks=True)
    os.replace(tmp, dst)
    dynamic = subprocess.check_output(['readelf', '-d', str(src)], text=True)
    for lib in re.findall(r'Shared library: \[(.*?)\]', dynamic):
        if lib in skip:
            continue
        for base in ('usr/lib', 'lib'):
            dep = target / base / lib
            if dep.exists():
                files.append((dep, base + '/' + lib))
                break
        else:
            raise SystemExit(f'Unresolved target dependency: {lib}')
packaging = sdk / 'project/app/blufi_service/packaging'
for name in ('system.conf', 'main.conf', 'test-start', 'S99blufi-test'):
    shutil.copy2(packaging / name, args.dest / name)
overlay = sdk / 'project/cfg/BoardConfig_IPC/overlay/overlay-luckfox-blufi'
shutil.copytree(overlay, args.dest / 'startup-overlay', dirs_exist_ok=True)
print(f'Staged {len(seen)} ELF files in {args.dest}; no credentials copied.')
