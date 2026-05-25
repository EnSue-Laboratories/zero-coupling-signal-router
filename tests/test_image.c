#include "zcsr/image.h"

#include <stdio.h>

static int fails = 0;
#define CK(label, cond) do { if (!(cond)) { printf("FAIL: %s\n", label); fails++; } } while (0)

/* Minimal valid 2x2 RGBA PNG (red, green / blue, white); all chunk CRCs verified. */
static const unsigned char PNG_2x2[] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x06,0x00,0x00,0x00,0x72,0xb6,0x0d,
0x24,0x00,0x00,0x00,0x12,0x49,0x44,0x41,0x54,0x78,0xda,0x63,0xf8,0xcf,0xc0,0xf0,
0x1f,0x0c,0x81,0x34,0x18,0x00,0x00,0x49,0xc8,0x09,0xf7,0x03,0xd9,0x64,0xf1,0x00,
0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

static void test_guards(void) {
    static unsigned char buffer[64 * 1024];
    static const unsigned char encoded[8] = { 0x89, 'P', 'N', 'G', 0, 0, 0, 0 };
    static const unsigned char lut[256][4] = { { 0 } };
    zcsr_image bad;

    CK("decode(NULL) invalid", zcsr_image_decode(0, 0, buffer, sizeof buffer).w == 0);
    CK("decode(NULL buffer) invalid", zcsr_image_decode(encoded, sizeof encoded, 0, 0).rgba == 0);
    CK("chroma_key(NULL) false", !zcsr_image_chroma_key(0, 0, 255, 0, 16, 8));
    bad = (zcsr_image){ 4, 4, 0 };
    CK("chroma_key(NULL pixels) false", !zcsr_image_chroma_key(&bad, 0, 255, 0, 16, 8));
    CK("make_variant(NULL src) invalid",
       zcsr_image_make_variant(0, buffer, sizeof buffer, ZCSR_MOD_MIX, 255, 0, 0, 255, 128).h == 0);
    CK("palette_remap(NULL src) invalid", zcsr_image_palette_remap(0, lut, buffer, sizeof buffer).rgba == 0);
}

static void test_decode(void) {
    static unsigned char buffer[128 * 1024];
    zcsr_image img = zcsr_image_decode(PNG_2x2, sizeof PNG_2x2, buffer, sizeof buffer);
    CK("decode width", img.w == 2);
    CK("decode height", img.h == 2);
    CK("decode rgba", img.rgba != 0);
    if (img.rgba) {
        CK("pixel red r", img.rgba[0] == 255);
        CK("pixel green g", img.rgba[5] == 255);
        CK("pixel blue b", img.rgba[10] == 255);
        CK("pixel white a", img.rgba[15] == 255);
    }
}

static void test_processing(void) {
    unsigned char src_pixels[16] = {
        0, 255, 0, 255,     10, 250, 10, 200,
        100, 20, 0, 255,    255, 255, 255, 255
    };
    unsigned char variant_buf[16];
    unsigned char palette_buf[16];
    static const uint8_t lut[256][4] = {
        [100] = { 1, 2, 3, 4 },
        [255] = { 9, 8, 7, 6 }
    };
    zcsr_image img = { 2, 2, src_pixels };

    CK("chroma key valid", zcsr_image_chroma_key(&img, 0, 255, 0, 0, 20));
    CK("exact key alpha 0", img.rgba[3] == 0);
    CK("soft edge alpha reduced", img.rgba[7] > 0 && img.rgba[7] < 200);
    CK("non key unchanged alpha", img.rgba[11] == 255);

    zcsr_image mixed = zcsr_image_make_variant(&img, variant_buf, sizeof variant_buf,
                                               ZCSR_MOD_MIX, 255, 0, 0, 255, 128);
    CK("variant valid", mixed.rgba == variant_buf && mixed.w == 2 && mixed.h == 2);
    if (mixed.rgba) {
        CK("variant mix red", mixed.rgba[8] > img.rgba[8]);
        CK("variant mix green", mixed.rgba[9] < img.rgba[9]);
    }

    zcsr_image pal = zcsr_image_palette_remap(&img, lut, palette_buf, sizeof palette_buf);
    CK("palette valid", pal.rgba == palette_buf && pal.w == 2 && pal.h == 2);
    if (pal.rgba) {
        CK("palette red-index", pal.rgba[8] == 1 && pal.rgba[9] == 2 &&
                                pal.rgba[10] == 3 && pal.rgba[11] == 4);
        CK("palette white-index", pal.rgba[12] == 9 && pal.rgba[13] == 8 &&
                                  pal.rgba[14] == 7 && pal.rgba[15] == 6);
    }
}

/* Variant: MULTIPLY darkens by the color, ADD lightens and clamps (the modes a pure multiply
 * can't express — they back flash-red / freeze-blue). */
static void test_variant_modes(void) {
    unsigned char src[8] = { 100, 100, 100, 200,   50, 60, 70, 255 };
    unsigned char buf[8];
    zcsr_image img = { 2, 1, src };

    zcsr_image mul = zcsr_image_make_variant(&img, buf, sizeof buf,
                                             ZCSR_MOD_MULTIPLY, 128, 255, 0, 255, 0);
    CK("multiply valid", mul.rgba == buf && mul.w == 2 && mul.h == 1);
    if (mul.rgba) {
        CK("multiply r darkened", buf[0] > 40 && buf[0] < 100); /* 100*128/255 ~= 50 */
        CK("multiply g kept", buf[1] == 100);                   /* *255 ~= unchanged */
        CK("multiply b zeroed", buf[2] == 0);                   /* *0 */
    }

    zcsr_image add = zcsr_image_make_variant(&img, buf, sizeof buf,
                                             ZCSR_MOD_ADD, 255, 0, 0, 0, 255);
    CK("add valid", add.rgba == buf);
    if (add.rgba) {
        CK("add r clamps to 255", buf[0] == 255);
        CK("add g unchanged", buf[1] == 100);
    }
}

/* Undersized destination / decode buffers must fail closed (return invalid), never overrun. */
static void test_bounds(void) {
    unsigned char src[16] = { 0 };
    unsigned char small[8];               /* a 2x2 needs 16 bytes */
    static const uint8_t lut[256][4] = { { 0 } };
    zcsr_image img = { 2, 2, src };

    CK("variant undersized dst invalid",
       zcsr_image_make_variant(&img, small, sizeof small, ZCSR_MOD_MIX, 0, 0, 0, 0, 128).rgba == 0);
    CK("palette undersized dst invalid",
       zcsr_image_palette_remap(&img, lut, small, sizeof small).rgba == 0);
    CK("decode undersized buffer invalid",
       zcsr_image_decode(PNG_2x2, sizeof PNG_2x2, small, sizeof small).w == 0);
}

/* Chroma-key alpha must be monotonically non-decreasing as a pixel's distance from the key grows
 * (closer to the key = more transparent); keyed pixel fully transparent, far pixel fully opaque. */
static void test_chroma_monotonic(void) {
    unsigned char px[16 * 4];
    zcsr_image img = { 16, 1, px };
    int mono = 1;
    int i;
    for (i = 0; i < 16; ++i) {
        px[i * 4 + 0] = (unsigned char)(i * 16); /* distance from key red(0) = i*16 */
        px[i * 4 + 1] = 255;
        px[i * 4 + 2] = 0;
        px[i * 4 + 3] = 255;
    }
    CK("chroma sweep ok", zcsr_image_chroma_key(&img, 0, 255, 0, 0, 200));
    for (i = 1; i < 16; ++i) if (px[i * 4 + 3] < px[(i - 1) * 4 + 3]) mono = 0;
    CK("chroma alpha monotonic in distance", mono);
    CK("chroma key pixel transparent", px[3] == 0);
    CK("chroma far pixel opaque", px[15 * 4 + 3] == 255);
}

int main(void) {
    test_guards();
    test_decode();
    test_processing();
    test_variant_modes();
    test_bounds();
    test_chroma_monotonic();
    if (!fails) printf("PASS: image decode + processing\n");
    return fails ? 1 : 0;
}
