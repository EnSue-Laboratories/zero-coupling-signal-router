/* Engine-ext — image module (Phase 0 STUB).
 * Loads/decodes encoded images to RGBA32 and runs offline CPU processing (chroma-key, variant,
 * palette) entirely within a caller-provided buffer (no global heap). SEPARATED from rendering:
 * this module only produces pixel data; glrender/render consume the RGBA32 it returns.
 *
 * Phase 0 = contract + buildable stub. The backend (multi-format decode via vendored stb_image
 * with STBI_MALLOC bump-allocating from the caller buffer + the CPU processing kernels) lands in
 * the implementation PR. Stubs are NULL-safe and fail closed so callers can integrate now. */
#include "zcsr/image.h"

zcsr_image zcsr_image_decode(const void* encoded, size_t encoded_len, void* buffer, size_t bytes) {
    (void)encoded; (void)encoded_len; (void)buffer; (void)bytes;
    return (zcsr_image){ 0, 0, 0 };
}

bool zcsr_image_chroma_key(zcsr_image* img,
                           uint8_t key_r, uint8_t key_g, uint8_t key_b,
                           uint8_t tolerance, uint8_t softness) {
    (void)key_r; (void)key_g; (void)key_b; (void)tolerance; (void)softness;
    if (!img || !img->rgba || img->w <= 0 || img->h <= 0) return false;
    return false; /* stub: no processing yet */
}

zcsr_image zcsr_image_make_variant(const zcsr_image* src, void* dst_buffer, size_t dst_bytes,
                                   zcsr_color_mode mode,
                                   uint8_t cr, uint8_t cg, uint8_t cb, uint8_t ca,
                                   uint8_t amount) {
    (void)src; (void)dst_buffer; (void)dst_bytes; (void)mode;
    (void)cr; (void)cg; (void)cb; (void)ca; (void)amount;
    return (zcsr_image){ 0, 0, 0 };
}

zcsr_image zcsr_image_palette_remap(const zcsr_image* src, const uint8_t lut[256][4],
                                    void* dst_buffer, size_t dst_bytes) {
    (void)src; (void)lut; (void)dst_buffer; (void)dst_bytes;
    return (zcsr_image){ 0, 0, 0 };
}
