/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define BF_LIMIT 4096
struct bf {
    uint8_t rx, tx, mode, key[16], frag_type, frag_flags;
    bool keyed;
    size_t used, total, dh_len;
    uint8_t buf[BF_LIMIT];
    void (*emit)(void *, const uint8_t *, size_t);
    void (*message)(void *, uint8_t, const uint8_t *, size_t);
    void *user;
};
void bf_init(struct bf *, void *, void (*)(void *, const uint8_t *, size_t), void (*)(void *, uint8_t, const uint8_t *, size_t));
void bf_clear(struct bf *);
int bf_receive(struct bf *, const uint8_t *, size_t);
int bf_send(struct bf *, uint8_t, const uint8_t *, size_t);
uint16_t bf_crc(const uint8_t *, size_t);
int bf_negotiate(struct bf *, const uint8_t *, size_t);
