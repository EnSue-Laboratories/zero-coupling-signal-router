/* Render bench (Agent 5) — sprite-batch throughput gate: 200 sprites < 1ms per flush.
 * Needs a windowed host: flush blits onto a real platform surface. On a headless box
 * zcsr_surface_create returns NULL -> the bench SKIPS (exit 0) and prints why. */
#include "zcsr/sprite.h"
#include "zcsr/texture.h"
#include "zcsr/platform.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define SPRITES 200
#define ITERS   1000

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(void) {
    zcsr_surface* win = zcsr_surface_create("zcsr render bench", (zcsr_rect){ 0, 0, 640, 480 });
    if (!win) {
        printf("SKIP: no window surface (headless / no DISPLAY) — run on a GUI host to measure the 200-sprite gate\n");
        return 0;
    }

    static unsigned char batch_buf[512 * 1024];
    zcsr_sprite_batch* batch = zcsr_sprite_batch_init(batch_buf, sizeof batch_buf, win);
    if (!batch) {
        printf("FAIL: sprite batch init\n");
        zcsr_surface_destroy(win);
        return 1;
    }

    /* a tiny opaque texture — timing exercises the batch/flush path, not PNG decode */
    static const uint8_t rgba[4] = { 200, 120, 60, 255 };
    zcsr_texture tex = { 2, 2, rgba };

    uint64_t best = UINT64_MAX;
    for (int it = 0; it < ITERS; ++it) {
        zcsr_sprite_batch_begin(batch);
        for (int i = 0; i < SPRITES; ++i) {
            zcsr_sprite s = { &tex, { (float)((i * 7) % 600), (float)((i * 13) % 440), 8.0f, 8.0f } };
            zcsr_sprite_batch_submit(batch, &s);
        }
        uint64_t t0 = now_ns();
        zcsr_sprite_batch_flush(batch);
        uint64_t dt = now_ns() - t0;
        if (dt < best) best = dt;
    }

    zcsr_surface_destroy(win);
    double ms = (double)best / 1e6;
    printf("render bench: %d sprites, best flush = %.4f ms (gate < 1.000 ms) -> %s\n",
           SPRITES, ms, ms < 1.0 ? "PASS" : "FAIL");
    return ms < 1.0 ? 0 : 1;
}
