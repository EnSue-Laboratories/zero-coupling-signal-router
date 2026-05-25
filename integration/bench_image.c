/* Offline image-processing throughput (Agent-4). The image kernels are one-time/load-time bakes
 * (not a per-frame budget), so this is informational, not a hard ctest gate: it reports ms and
 * megapixels/sec for chroma-key / variant / palette on a 512x512 image. Pure CPU — runs headless. */
#include "zcsr/image.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

enum { BENCH_W = 512, BENCH_H = 512, BENCH_ITERS = 50 };

static uint64_t zcsr_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void report(const char* name, uint64_t best_ns) {
    double ms = (double)best_ns / 1.0e6;
    double mpx = (double)(BENCH_W * BENCH_H) / ((double)best_ns / 1.0e3); /* px / us = Mpx/s */
    printf("  %-12s %7.3f ms  (%6.1f Mpx/s)\n", name, ms, mpx);
}

int main(void) {
    static unsigned char src[BENCH_W * BENCH_H * 4];
    static unsigned char dst[BENCH_W * BENCH_H * 4];
    static uint8_t lut[256][4];
    zcsr_image img = { BENCH_W, BENCH_H, src };
    uint64_t best, t0, d;
    int i, k;

    for (i = 0; i < BENCH_W * BENCH_H * 4; ++i) src[i] = (unsigned char)(i * 131u);
    for (i = 0; i < 256; ++i) {
        lut[i][0] = (uint8_t)i; lut[i][1] = (uint8_t)(255 - i);
        lut[i][2] = (uint8_t)(i / 2); lut[i][3] = 255;
    }

    printf("image bake bench (%dx%d, best of %d):\n", BENCH_W, BENCH_H, BENCH_ITERS);

    best = UINT64_MAX;
    for (k = 0; k < BENCH_ITERS; ++k) {
        t0 = zcsr_now_ns();
        zcsr_image_chroma_key(&img, 0, 255, 0, 16, 32);
        d = zcsr_now_ns() - t0; if (d < best) best = d;
    }
    report("chroma_key", best);

    best = UINT64_MAX;
    for (k = 0; k < BENCH_ITERS; ++k) {
        t0 = zcsr_now_ns();
        (void)zcsr_image_make_variant(&img, dst, sizeof dst, ZCSR_MOD_MIX, 64, 96, 255, 255, 128);
        d = zcsr_now_ns() - t0; if (d < best) best = d;
    }
    report("variant_mix", best);

    best = UINT64_MAX;
    for (k = 0; k < BENCH_ITERS; ++k) {
        t0 = zcsr_now_ns();
        (void)zcsr_image_palette_remap(&img, (const uint8_t (*)[4])lut, dst, sizeof dst);
        d = zcsr_now_ns() - t0; if (d < best) best = d;
    }
    report("palette_remap", best);

    printf("PASS: image bake bench\n");
    return 0;
}
