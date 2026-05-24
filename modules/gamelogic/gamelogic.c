/* Agent 4 (Game Core) — Game Logic: AABB collision + binary save/load.
 * Includes ONLY shared contracts. */
#include "zcsr/collision.h"
#include "zcsr/save.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    ZCSR_SAVE_MAGIC = 0x5253435Au, /* "ZCSR" little-endian */
    ZCSR_SAVE_HEADER_SIZE = 20
};

static uint32_t zcsr_read_u32_le(const uint8_t* p) {
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void zcsr_write_u32_le(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static uint32_t zcsr_checksum32(const uint8_t* data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        hash ^= (uint32_t)data[i];
        hash *= 16777619u;
    }
    return hash;
}

static void zcsr_copy_bytes(uint8_t* dst, const uint8_t* src, size_t n) {
    for (size_t i = 0; i < n; ++i) dst[i] = src[i];
}

static bool zcsr_size_to_u32(size_t value, uint32_t* out) {
    if (!out || value > 0xffffffffu) return false;
    *out = (uint32_t)value;
    return true;
}

bool zcsr_aabb_intersect(zcsr_aabb a, zcsr_aabb b) {
    if (a.w <= 0.0f || a.h <= 0.0f || b.w <= 0.0f || b.h <= 0.0f) return false;
    return a.x < (b.x + b.w) &&
           (a.x + a.w) > b.x &&
           a.y < (b.y + b.h) &&
           (a.y + a.h) > b.y;
}

bool zcsr_aabb_contains_point(zcsr_aabb a, float px, float py) {
    if (a.w <= 0.0f || a.h <= 0.0f) return false;
    return px >= a.x && px < (a.x + a.w) &&
           py >= a.y && py < (a.y + a.h);
}

size_t zcsr_save_write(const void* state, size_t state_size,
                       void* out, size_t out_cap, uint32_t version) {
    uint8_t* bytes = (uint8_t*)out;
    const uint8_t* payload = (const uint8_t*)state;
    uint32_t payload_size;
    uint32_t checksum;

    if (!bytes || (!payload && state_size > 0)) return 0;
    if (!zcsr_size_to_u32(state_size, &payload_size)) return 0;
    if (out_cap < ZCSR_SAVE_HEADER_SIZE || out_cap - ZCSR_SAVE_HEADER_SIZE < state_size) return 0;

    checksum = zcsr_checksum32(payload, state_size);
    zcsr_write_u32_le(bytes + 0, ZCSR_SAVE_MAGIC);
    zcsr_write_u32_le(bytes + 4, version);
    zcsr_write_u32_le(bytes + 8, payload_size);
    zcsr_write_u32_le(bytes + 12, checksum);
    zcsr_write_u32_le(bytes + 16, 0u);
    zcsr_copy_bytes(bytes + ZCSR_SAVE_HEADER_SIZE, payload, state_size);

    return ZCSR_SAVE_HEADER_SIZE + state_size;
}

size_t zcsr_save_read(const void* in, size_t in_size,
                      void* state, size_t state_cap, uint32_t expect_version) {
    const uint8_t* bytes = (const uint8_t*)in;
    uint8_t* out = (uint8_t*)state;
    uint32_t version;
    uint32_t payload_size;
    uint32_t checksum;
    const uint8_t* payload;

    if (!bytes || !out || in_size < ZCSR_SAVE_HEADER_SIZE) return 0;
    if (zcsr_read_u32_le(bytes + 0) != ZCSR_SAVE_MAGIC) return 0;

    version = zcsr_read_u32_le(bytes + 4);
    if (version != expect_version) return 0;

    payload_size = zcsr_read_u32_le(bytes + 8);
    checksum = zcsr_read_u32_le(bytes + 12);
    if (zcsr_read_u32_le(bytes + 16) != 0u) return 0;
    if ((size_t)payload_size > state_cap) return 0;
    if (in_size - ZCSR_SAVE_HEADER_SIZE < (size_t)payload_size) return 0;

    payload = bytes + ZCSR_SAVE_HEADER_SIZE;
    if (zcsr_checksum32(payload, (size_t)payload_size) != checksum) return 0;

    zcsr_copy_bytes(out, payload, (size_t)payload_size);
    return (size_t)payload_size;
}
