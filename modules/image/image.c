/* Engine-ext - image module.
 * Renderer-agnostic decode and offline CPU image processing. Includes only shared contracts
 * plus the vendored stb_image implementation; all allocations come from the caller buffer.
 */
#include "zcsr/image.h"

#include <limits.h>
#include <stdint.h>

typedef struct {
    unsigned char* base;
    size_t capacity;
    size_t offset;
} zcsr_image_arena;

static zcsr_image_arena* zcsr_image_active_arena;

static size_t zcsr_image_align_up(size_t value, size_t align) {
    size_t mask = align - 1u;
    return (value + mask) & ~mask;
}

static bool zcsr_image_size(int w, int h, size_t* out) {
    size_t sw;
    size_t sh;
    if (!out || w <= 0 || h <= 0) return false;
    sw = (size_t)w;
    sh = (size_t)h;
    if (sw > SIZE_MAX / sh || sw * sh > SIZE_MAX / 4u) return false;
    *out = sw * sh * 4u;
    return true;
}

static void* zcsr_image_alloc(size_t bytes) {
    size_t offset;
    if (!zcsr_image_active_arena || bytes == 0) return 0;
    offset = zcsr_image_align_up(zcsr_image_active_arena->offset, sizeof(void*));
    if (offset > zcsr_image_active_arena->capacity || bytes > zcsr_image_active_arena->capacity - offset) return 0;
    zcsr_image_active_arena->offset = offset + bytes;
    return zcsr_image_active_arena->base + offset;
}

static void zcsr_image_free(void* ptr) {
    (void)ptr;
}

static void* zcsr_image_realloc_sized(void* ptr, size_t old_size, size_t new_size) {
    unsigned char* dst = (unsigned char*)zcsr_image_alloc(new_size);
    unsigned char* src = (unsigned char*)ptr;
    size_t n = old_size < new_size ? old_size : new_size;
    if (!dst) return 0;
    for (size_t i = 0; src && i < n; ++i) dst[i] = src[i];
    return dst;
}

#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STB_IMAGE_STATIC
#define STBI_MALLOC(sz) zcsr_image_alloc(sz)
#define STBI_REALLOC(p, sz) zcsr_image_realloc_sized((p), 0, (sz))
#define STBI_REALLOC_SIZED(p, oldsz, newsz) zcsr_image_realloc_sized((p), (oldsz), (newsz))
#define STBI_FREE(p) zcsr_image_free(p)
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "../../third_party/stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static uint8_t zcsr_clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static uint8_t zcsr_mul_u8(uint8_t a, uint8_t b) {
    return (uint8_t)(((int)a * (int)b + 127) / 255);
}

static uint8_t zcsr_lerp_u8(uint8_t a, uint8_t b, uint8_t amount) {
    int t = (int)amount;
    return (uint8_t)(((int)a * (255 - t) + (int)b * t + 127) / 255);
}

static void zcsr_copy_rgba(uint8_t* dst, const uint8_t* src, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) dst[i] = src[i];
}

zcsr_image zcsr_image_decode(const void* encoded, size_t encoded_len, void* buffer, size_t bytes) {
    zcsr_image_arena arena;
    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char* pixels;
    size_t image_bytes;
    if (!encoded || encoded_len == 0 || encoded_len > (size_t)INT_MAX || !buffer || bytes == 0) {
        return (zcsr_image){ 0, 0, 0 };
    }
    arena = (zcsr_image_arena){ (unsigned char*)buffer, bytes, 0 };
    zcsr_image_active_arena = &arena;
    pixels = stbi_load_from_memory((const unsigned char*)encoded, (int)encoded_len, &w, &h, &channels, 4);
    zcsr_image_active_arena = 0;
    (void)channels;
    if (!pixels || !zcsr_image_size(w, h, &image_bytes)) return (zcsr_image){ 0, 0, 0 };
    (void)image_bytes;
    return (zcsr_image){ w, h, pixels };
}

bool zcsr_image_chroma_key(zcsr_image* img,
                           uint8_t key_r, uint8_t key_g, uint8_t key_b,
                           uint8_t tolerance, uint8_t softness) {
    size_t pixels;
    size_t bytes;
    if (!img || !img->rgba || !zcsr_image_size(img->w, img->h, &bytes)) return false;
    pixels = bytes / 4u;
    for (size_t i = 0; i < pixels; ++i) {
        uint8_t* p = &img->rgba[i * 4u];
        int dr = p[0] > key_r ? p[0] - key_r : key_r - p[0];
        int dg = p[1] > key_g ? p[1] - key_g : key_g - p[1];
        int db = p[2] > key_b ? p[2] - key_b : key_b - p[2];
        int dist = dr > dg ? dr : dg;
        if (db > dist) dist = db;
        if (dist <= (int)tolerance) {
            p[3] = 0;
        } else if (softness > 0 && dist < (int)tolerance + (int)softness) {
            int edge = dist - (int)tolerance;
            p[3] = (uint8_t)(((int)p[3] * edge + ((int)softness / 2)) / (int)softness);
        }
    }
    return true;
}

zcsr_image zcsr_image_make_variant(const zcsr_image* src, void* dst_buffer, size_t dst_bytes,
                                   zcsr_color_mode mode,
                                   uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca,
                                   uint8_t amount) {
    size_t bytes;
    uint8_t* dst = (uint8_t*)dst_buffer;
    if (!src || !src->rgba || !dst || !zcsr_image_size(src->w, src->h, &bytes) || dst_bytes < bytes) {
        return (zcsr_image){ 0, 0, 0 };
    }
    zcsr_copy_rgba(dst, src->rgba, bytes);
    for (size_t i = 0; i < bytes; i += 4u) {
        if (mode == ZCSR_MOD_MIX) {
            dst[i + 0u] = zcsr_lerp_u8(dst[i + 0u], cr, amount);
            dst[i + 1u] = zcsr_lerp_u8(dst[i + 1u], cg, amount);
            dst[i + 2u] = zcsr_lerp_u8(dst[i + 2u], cb, amount);
            dst[i + 3u] = zcsr_lerp_u8(dst[i + 3u], ca, amount);
        } else if (mode == ZCSR_MOD_ADD) {
            dst[i + 0u] = zcsr_clamp_u8((int)dst[i + 0u] + ((int)cr * (int)amount + 127) / 255);
            dst[i + 1u] = zcsr_clamp_u8((int)dst[i + 1u] + ((int)cg * (int)amount + 127) / 255);
            dst[i + 2u] = zcsr_clamp_u8((int)dst[i + 2u] + ((int)cb * (int)amount + 127) / 255);
            dst[i + 3u] = zcsr_clamp_u8((int)dst[i + 3u] + ((int)ca * (int)amount + 127) / 255);
        } else {
            dst[i + 0u] = zcsr_mul_u8(dst[i + 0u], cr);
            dst[i + 1u] = zcsr_mul_u8(dst[i + 1u], cg);
            dst[i + 2u] = zcsr_mul_u8(dst[i + 2u], cb);
            dst[i + 3u] = zcsr_mul_u8(dst[i + 3u], ca);
        }
    }
    return (zcsr_image){ src->w, src->h, dst };
}

zcsr_image zcsr_image_palette_remap(const zcsr_image* src, const uint8_t lut[256][4],
                                    void* dst_buffer, size_t dst_bytes) {
    size_t bytes;
    uint8_t* dst = (uint8_t*)dst_buffer;
    if (!src || !src->rgba || !lut || !dst || !zcsr_image_size(src->w, src->h, &bytes) || dst_bytes < bytes) {
        return (zcsr_image){ 0, 0, 0 };
    }
    for (size_t i = 0; i < bytes; i += 4u) {
        const uint8_t* color = lut[src->rgba[i]];
        dst[i + 0u] = color[0];
        dst[i + 1u] = color[1];
        dst[i + 2u] = color[2];
        dst[i + 3u] = color[3];
    }
    return (zcsr_image){ src->w, src->h, dst };
}
