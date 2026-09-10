# Luckfox Ultra W BluFi service

Linux BLE provisioning for the stock EspBlufi Android client, using BlueZ GATT,
BluFi 1.3 (security v1) and the existing wpa_supplicant control socket.

## Build

The Ultra W board enables `RK_ENABLE_BLUFI=y`. Use the SDK entry points:

```sh
./build.sh firmware   # first build: BlueZ, GLib, OpenSSL and sysroot
./build.sh app
./build.sh firmware   # include the application and startup overlay
```

Buildroot's authoritative defconfig is
`sysdrv/tools/board/buildroot/rv1106_defconfig`: the SDK copies it into the
Buildroot source tree during firmware packaging. The BlueZ 5.79 uClibc patch
is kept in `sysdrv/tools/board/buildroot/bluez579_patch/`.

Outputs: `out/bin/blufi_service`, installed through the app/OEM workflow.
The Ultra W overlay installs `/etc/init.d/S99blufi`; D-Bus and BlueZ retain
their own Buildroot startup scripts. The service uses `/userdata/wpa_supplicant.conf`
and `/var/run/wpa_supplicant/wlan0`. Keep `update_config=1` in that config.

## NFS-assisted board test

Stage the **already-built** files on the host:

```sh
python3 project/app/blufi_service/tools/stage_runtime.py --dest /path/to/nfs-share/blufi-test
```

In the commands below, the mounted directory is represented by `/path/to/mounted-share/blufi-test`.
**Copy to local eMMC before running anything that switches Wi-Fi.** Running ELF
binaries or libraries from the hard-mounted NFS share can stall the service and
shell when wlan0 disconnects, even if reconnecting to the same access point.

```sh
mkdir -p /userdata/blufi-test
cp -a /path/to/mounted-share/blufi-test/. /userdata/blufi-test/
# First install the managed startup overlay (see Startup ownership below).
sync
/userdata/blufi-test/test-start > /tmp/blufi-service.log 2>&1 &
```

`test-start` only adjusts its child environment, reuses an existing bus/BlueZ,
and initializes AIC UART at 1500000 baud if hci0 is absent. Do not export an NFS
`LD_LIBRARY_PATH` into the interactive shell. Do not run two hciattach processes.
Test D-Bus policy permits only root clients; it is not a replacement for the
normal production system bus configuration.

Hold Recovery-key for 3 seconds, or pass `--pair-now` for an immediate 180-second
window. In EspBlufi: connect to **BLUFI_Luckfox**, get version if necessary,
**negotiate security**, request the Wi-Fi list, select STA mode, then submit a
2.4GHz WPA2-Personal SSID/password. Credential data without the negotiated
AES/CRC mode is rejected. An App message saying configuration was *sent* does
not prove network connectivity; check the subsequent device status.

Read-only verification (never print the configuration contents):

```sh
tail -n 30 /tmp/blufi-service.log
wpa_cli -i wlan0 status | grep -E '^(wpa_state|id|ip_address)='
ls -l /userdata/wpa_supplicant.conf
cat /sys/class/leds/work/trigger
```

Successful provisioning logs `Wi-Fi operation 2: complete`, obtains an IPv4
address and writes a 0600 config. The previous file is retained as
`wpa_supplicant.conf.blufi-last-good`, also 0600. DHCP continues in the background
for lease renewal, owned by `/var/run/blufi-udhcpc.pid`. An existing independent
wlan0 network manager must be disabled/coordinated before deployment; eth0 DHCP
is left alone. Do not call the legacy `wifi_start.sh` from this service.

## Button and LED

Recovery-key is GPIO4_C0, active low, KEY_0 (11), 30ms debounce. The service locates
the `gpio-keys` evdev device by name and capability, ignoring KEY_3/KEY_4. A short
press does nothing; one long press opens one window; holding through startup
requires release before a new press. It does not implement factory reset.

| State | Work LED (GPIO3_C6) |
|---|---|
| Idle | Preserve original trigger/brightness |
| Waiting/session | 500ms on / 500ms off |
| Connecting/DHCP/commit | 100ms on / 100ms off |
| Success | Solid 3 seconds, close window, restore original trigger |
| Failure | Three double flashes, then return to waiting if time remains |

The shipped board exposes `/sys/class/leds/work`, max brightness 255. The service
sets trigger `none` while owning the LED and restores the previous value on
normal exit. It does not directly request the GPIO. A process killed with SIGKILL
cannot run cleanup; restore the baseline trigger before restarting after a crash.

## Protocol reference and limits

- ESP-IDF **v5.4.2**: `blufi_int.h`, `blufi_prf.c`, `blufi_protocol.c`,
  `examples/bluetooth/blufi/main/blufi_security.c`.
- EspBlufiForAndroid **b98ac52aed274414993b040e09255fed5a5fd268**:
  `BlufiClientImpl.java`, `BlufiCRC.java`.
- Service FFFF, phone-write FF01, device-notify FF02, Bluetooth base UUID.
- Protocol version **1.3**, fixed Android v1 1024-bit DH group, MD5-derived
  AES-128-CFB key; CRC covers sequence/length/plain payload before encryption.
- This is legacy interoperability, **not authenticated device ownership**.
  No claim of active MITM protection. Security v2 / BluFi 1.4 is not implemented.
- Bound reassembly (4096 bytes), conservative MTU=23 outgoing fragmentation,
  sequence validation, encrypted credential enforcement, session cleanup.
- Scope: WPA2-Personal, 1–32 byte SSID, 8–63 byte passphrase. No enterprise,
  WPA3-only, open-network, 64-hex-PSK input or SoftAP provisioning.
- A status query while connecting returns CONNECTING or NO_IP. Success means
  IP and durable config commit, not Internet access.
- One session/network operation at a time. Android was exercised on hardware;
  iOS, deliberate power-cut testing, 50-cycle stress and a second-phone race
  remain separate acceptance work. BlueZ restart currently requires service
  restart; no supervision/automatic D-Bus re-registration is claimed.

## Local protocol tests

```sh
gcc -Wall -Wextra -fsanitize=address,undefined -g \
  project/app/blufi_service/tests/test_protocol.c \
  project/app/blufi_service/src/protocol.c -lcrypto -o /tmp/blufi-protocol-test
ASAN_OPTIONS=detect_leaks=0 /tmp/blufi-protocol-test
```

LeakSanitizer is disabled only because this execution environment uses ptrace;
AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. Tests cover the
independent CRC check value, replay, fragment boundaries/type mismatch, oversize
messages, ACK, AES decryption, and rejection of plaintext credentials.

## Hardware acceptance on 2026-09-10

Android tests with the user passed discovery, scanning, security negotiation,
correct credentials, wrong-password recovery, Recovery-key entry and LED state
changes. A correct final transaction completed in about 6.7 seconds, persisted
network id=2 and a private last-good backup, and left a renewing DHCP daemon.
After a normal reboot the board recovered id=2 / <DHCP 地址> and started GATT
in idle mode automatically.

The board now runs the production layout: `/oem/usr/bin/blufi_service`, system
libraries under `/usr/lib`, and `S30dbus-daemon`, `S40bluetoothd`, `S99blufi`.
The writable filesystem was updated directly; no image was flashed. The old test
init entry is disabled and the old runtime is retained only as a rollback backup
under `/userdata/blufi-migration/test-runtime.backup`. It is not used at boot.
See the production migration record below.

## Startup ownership (cleanup on 2026-09-10)

The Ultra-W overlay now owns the sequence:

- `S21appinit`: load AIC modules, restore a validated Ethernet MAC, guard optional RkLunch hooks.
- `S40network`: Ethernet only, using the system interfaces file.
- `S41wifi` / `/usr/bin/luckfox-wifi`: one wlan0 supplicant using the saved userdata
  configuration, one persistent DHCP daemon. BluFi reuses `/var/run/blufi-udhcpc.pid`
  when changing networks. Repeated `start` calls preserve a live connection and
  must not interfere with an active provisioning transaction.
- `S50crond`: create the volatile cron directory before starting the daemon.
- `S95userdata-app`: start `/userdata/app.sh` once per boot, independently of BLE.
  It waits up to 30 seconds for a default route in the background. An optional
  `/userdata/app-stop.sh` owns application-specific shutdown.
- `S99blufi` (production) or `S99blufi-test` (local test): BLE provisioning only.
  Test shutdown stops only dependencies that this test runtime started.
- `rcK`: stop scripts in descending order and wait for the BluFi transaction
  before stopping Wi-Fi.

On the existing board, `/userdata/net.sh -> wifi.sh` is retained. `wifi.sh` now
only executes `/usr/bin/luckfox-wifi "${1:-start}"`; `eth.sh` delegates to
`ifup eth0`. Neither starts the user application. The private app.sh remains
on the board; only its NFS mount line was replaced by `/usr/bin/luckfox-nfs-mount`.
That helper reads device-only `/userdata/luckfox-nfs.conf` and mounts the optional
development share asynchronously; use `retry=0` in the private mount options.
With no configuration file it does nothing;
a failed mount no longer prevents local NTP/UPS application startup. NFS-backed
executables must still never be used during Wi-Fi changes.

`stage_runtime.py` includes these scripts under `startup-overlay/`. For an old
board, back up its init/userdata scripts first, install the overlay's `usr/bin/`
helpers and `S21appinit`, `S41wifi`, `S50crond`, `S95userdata-app`, `rcK` into their
rootfs paths, and migrate its private net/app scripts as above. Use only
`S99blufi-test` for the local runtime, or `S99blufi` for production. Do not copy
both init entries. This migration deliberately does not replace Wi-Fi credentials
or private application arguments. The current board's pre-migration files are
in `/userdata/startup-cleanup/backup/startup.tar` (root-only).

Read-only status: `luckfox-wifi status`. `start` is idempotent, not a health
supervisor. Before an intentional `luckfox-wifi stop`, stop the active BluFi init
entry first. No automatic crash recovery is claimed.

Three normal reboots passed during cleanup. Final boot restored <DHCP 地址>,
kept the saved configuration checksum unchanged, and ran one supplicant,
one DHCP client per interface, one BLE stack/service, cron and the existing
local user applications. Advertising registration was also checked. The eight
overlay files match both generated rootfs and Buildroot target. Existing ext4
error history (count 4) and unrelated peripheral probe warnings remain recorded
in the implementation document; this is not a claim of a warning-free kernel boot.

## Private deployment configuration

Do not add real server addresses, login names, host paths or credentials to this
repository. `/path/to/nfs-share` and `<DHCP 地址>` above are placeholders; standard
device runtime paths such as `/userdata` are part of the software interface.

Create `/userdata/luckfox-nfs.conf` **only on the board**, owned by root and mode
0600. It contains three literal `KEY=value` lines (no shell quoting or expansion):
`NFS_SOURCE` (server and export), `NFS_TARGET` (absolute mount directory), and
`NFS_OPTIONS` (mount options, including `retry=0` to avoid mount retries).
No real configuration or machine-specific defaults ship in the overlay or runtime
package. Run `luckfox-nfs-mount --check` to validate it without mounting or printing
its contents. A missing file disables the optional NFS mount.

## Production directory migration

The writable board filesystem was migrated directly after backing up replaced
files under `/userdata/blufi-migration/backup`. Runtime binaries/libraries were
copied from the locally verified package to their final `/usr` locations; the
BluFi executable and launcher live in `/oem/usr/bin`. Existing identical `/lib`
runtime components, including the C loader, were retained. Replaced libraries
were installed using temporary files and rename, preserving already mapped code.

The generated firmware supplies the standard system D-Bus configuration and
Bluetooth policy, BlueZ configuration, the D-Bus activation helper and system init
scripts. The board's missing `dbus` system user was created; activation-helper
ownership/mode matches Buildroot (`root:dbus`, 4750). The test-only root D-Bus
configuration and test `LD_LIBRARY_PATH` are no longer used.

After stopping the test stack, its init symlink was moved into the backup and
only the production entries were enabled. A normal reboot restored Wi-Fi with an
unchanged saved-config checksum, started GATT and the existing local applications,
and showed no process mappings from either the old test directory or migration
backup. The old directory is now `/userdata/blufi-migration/test-runtime.backup`;
it is an inactive rollback copy. Real NFS settings remain solely in the private
userdata configuration, not in this repository or the transferred config archive.

Log: `/var/log/blufi-service.log`; service control:
`/etc/init.d/S99blufi {start|stop|restart}`. Restart BluFi after restarting BlueZ.

The migration check also exposed an HCI teardown race in `S99blufi restart`.
The stop path now waits for both the owned hciattach process and old hci0 node
to disappear before starting a new controller. Live restart was verified after
this fix. The service is left idle with the normal LED trigger restored.
