#include "zcsr/image.h"

#include <stdio.h>

/* Phase-0 coverage: the contract must fail closed on NULL / invalid / undersized inputs and never
 * deref. These guards hold for both the stub and the real backend, so the test stays valid as the
 * decode + processing kernels land (Agent-4 then adds positive decode/chroma-key/palette cases). */
int main(void) {
    static unsigned char buffer[64 * 1024];
    static const unsigned char encoded[8] = { 0x89, 'P', 'N', 'G', 0, 0, 0, 0 };
    static const unsigned char lut[256][4] = { { 0 } };
    zcsr_image bad;

    /* decode: NULL inputs / zero length / undersized buffer must yield an invalid image. */
    if (zcsr_image_decode(0, 0, buffer, sizeof buffer).w != 0) {
        printf("FAIL: decode(NULL) should be invalid\n");
        return 1;
    }
    if (zcsr_image_decode(encoded, sizeof encoded, 0, 0).rgba != 0) {
        printf("FAIL: decode(NULL buffer) should be invalid\n");
        return 1;
    }

    /* chroma_key: NULL image / NULL pixels / non-positive dims must return false. */
    if (zcsr_image_chroma_key(0, 0, 255, 0, 16, 8)) {
        printf("FAIL: chroma_key(NULL) should be false\n");
        return 1;
    }
    bad = (zcsr_image){ 4, 4, 0 };
    if (zcsr_image_chroma_key(&bad, 0, 255, 0, 16, 8)) {
        printf("FAIL: chroma_key(NULL pixels) should be false\n");
        return 1;
    }

    /* make_variant / palette_remap: NULL src / NULL dst must yield an invalid image. */
    if (zcsr_image_make_variant(0, buffer, sizeof buffer, ZCSR_MOD_MIX, 255, 0, 0, 255, 128).h != 0) {
        printf("FAIL: make_variant(NULL src) should be invalid\n");
        return 1;
    }
    if (zcsr_image_palette_remap(0, lut, buffer, sizeof buffer).rgba != 0) {
        printf("FAIL: palette_remap(NULL src) should be invalid\n");
        return 1;
    }

    printf("PASS: image contract guards\n");
    return 0;
}
