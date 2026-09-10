/* SPDX-License-Identifier: MIT */
#pragma once
#include <gio/gio.h>
#include <stdint.h>
struct wifi_job {
    int operation; /* 0=status, 1=scan, 2=connect */
    uint8_t ssid[32], password[64]; size_t ssid_len, password_len;
    const char *socket_path, *config_path;
    int result; gint phase;
    char detail[96];
    uint8_t response[4096]; size_t response_len;
};
void wifi_work(GTask *, gpointer, gpointer, GCancellable *);
