#ifndef ZCSR_TEXTURE_H
#define ZCSR_TEXTURE_H
/* Agent 3 (Game Core) — PNG loading (stb_image, single-header) + resource cache (dedupe).
 * Implementer: modules/render. Cache lives in a caller buffer; stb_image is configured to
 * allocate from that buffer (STBI_MALLOC), so no global heap growth. */
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int            width;
    int            height;
    const uint8_t* rgba; /* 8-bit RGBA, cache-owned (do not free) */
} zcsr_texture;

typedef struct zcsr_texture_cache zcsr_texture_cache; /* opaque */

zcsr_texture_cache*  zcsr_texture_cache_init(void* buffer, size_t bytes);
/* Load a PNG by path. Same path returns the SAME cached texture (no duplicate load). NULL on fail. */
const zcsr_texture*  zcsr_texture_load(zcsr_texture_cache*, const char* png_path);

#endif /* ZCSR_TEXTURE_H */
