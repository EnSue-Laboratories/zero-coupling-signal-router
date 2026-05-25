#ifndef ZCSR_IMAGE_H
#define ZCSR_IMAGE_H
/* Engine-ext Agent — image loading, decoding, and offline CPU processing to 8-bit RGBA (RGBA32).
 *
 * SEPARATED from rendering (per project decision): this module ONLY produces pixel buffers.
 * The GPU path (glrender) and the software path (render) CONSUME the RGBA32 it produces through
 * their existing texture-create entry points (e.g. zcsr_gl_texture_create(rgba, w, h)). `image`
 * never includes a renderer; renderers never include `image`. Decouples decode/processing from
 * upload/draw so both render paths share one decoder.
 *
 * Implementer: modules/image (vendors stb_image, used ONLY here for multi-format decode).
 * No runtime heap: decode and all processing write into a CALLER-PROVIDED buffer; stb_image is
 * configured (STBI_MALLOC) to bump-allocate from that buffer, so there is no global heap growth.
 * The caller sizes the buffer to cover decode scratch + the packed RGBA output. */
#include "color.h"   /* zcsr_color_mode — shared with the glrender runtime path */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Decoded / processed image: tightly packed 8-bit RGBA, top-left origin, row stride = w*4.
 * Pixels live inside the caller-provided buffer (do NOT free). w==0 || h==0 || !rgba => invalid. */
typedef struct {
    int            w, h;
    uint8_t*       rgba; /* w*h*4 bytes, caller-buffer-owned */
} zcsr_image;

/* ---- Decode (multi-format) ---- */

/* Decode an encoded image from memory into RGBA32. Format is auto-detected; supported encodings
 * are PNG / JPEG / BMP / TGA / GIF / PSD / PIC. `buffer`/`bytes` is caller scratch+output (no
 * global heap) — the returned rgba points inside it. Returns {0,0,NULL} on decode failure or if
 * the buffer is too small to hold scratch + output. */
zcsr_image zcsr_image_decode(const void* encoded, size_t encoded_len, void* buffer, size_t bytes);

/* ---- Offline CPU processing (bake once, then upload as an ordinary static texture) ---- */

/* Color-key transparency (chroma-key / "green screen"). Texels within `tolerance` (per-channel
 * max distance, 0..255) of (key_r,key_g,key_b) become fully transparent (alpha=0); a `softness`
 * band (0..255) feathers alpha just outside the keyed range so edges are not hard-cut. Operates
 * IN PLACE on `img`'s buffer. Returns false on NULL / invalid image. */
bool zcsr_image_chroma_key(zcsr_image* img,
                           uint8_t key_r, uint8_t key_g, uint8_t key_b,
                           uint8_t tolerance, uint8_t softness);

/* Variant texture: copy `src` into `dst_buffer` and modulate every texel by ONE RGBA color
 * (full RGBA, no per-channel control), producing a recolored variant (e.g. team color).
 *   mode: MULTIPLY = texel*color, MIX = lerp(texel,color,amount), ADD = texel + color*amount.
 *   (cr,cg,cb,ca) 0..255 = modulation color; amount 0..255 = blend strength for MIX/ADD.
 * Returns the new image in dst_buffer, or {0,0,NULL} on failure / buffer too small. */
zcsr_image zcsr_image_make_variant(const zcsr_image* src, void* dst_buffer, size_t dst_bytes,
                                   zcsr_color_mode mode,
                                   uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca,
                                   uint8_t amount);

/* Palette remap: map each source texel to an RGBA32 entry via a 256-entry LUT, keyed by the
 * texel's RED channel (index/palette images) — for dynamic recolors (hair, faction palettes).
 * Runs on the CPU once; cache the result as a static texture. Writes the remapped image into
 * `dst_buffer`. Returns {0,0,NULL} on failure / buffer too small. */
zcsr_image zcsr_image_palette_remap(const zcsr_image* src, const uint8_t lut[256][4],
                                    void* dst_buffer, size_t dst_bytes);

#endif /* ZCSR_IMAGE_H */
