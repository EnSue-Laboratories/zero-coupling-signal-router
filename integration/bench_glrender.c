/* GPU render bench — 200..10000 sprite gate for the optional OpenGL renderer.
 * Needs a real windowed host. On headless boxes it skips with exit 0.
 */
#include "zcsr/gl_render.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define MAX_SPRITES 10000
#define ITERS 240

static uint64_t zcsr_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void zcsr_fill_sprites(zcsr_gl_sprite* sprites, size_t count, zcsr_gl_texture tex) {
    for (size_t i = 0; i < count; ++i) {
        float x = (float)((i * 17u) % 760u);
        float y = (float)((i * 29u) % 560u);
        float t = (float)(i % 255u) / 255.0f;
        sprites[i] = (zcsr_gl_sprite){
            tex,
            { x, y, 8.0f, 8.0f },
            (float)(i % 64u) * 0.03f,
            0.45f + 0.55f * t,
            0.75f,
            1.0f - 0.35f * t,
            1.0f
        };
    }
}

static double zcsr_bench_count(zcsr_gl_renderer* renderer, const zcsr_gl_sprite* sprites, size_t count) {
    uint64_t best = UINT64_MAX;
    for (int i = 0; i < ITERS; ++i) {
        uint64_t t0 = zcsr_now_ns();
        zcsr_gl_submit_batch(renderer, sprites, count);
        uint64_t dt = zcsr_now_ns() - t0;
        if (dt < best) best = dt;
    }
    return (double)best / 1e6;
}

int main(void) {
    static unsigned char renderer_buf[8 * 1024 * 1024];
    static zcsr_gl_sprite sprites[MAX_SPRITES];
    static const unsigned char rgba[16] = {
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255
    };

    zcsr_surface* surface = zcsr_surface_create("zcsr glrender bench", (zcsr_rect){ 0, 0, 800, 600 });
    if (!surface) {
        printf("SKIP: no window surface (headless / no DISPLAY)\n");
        return 0;
    }

    zcsr_gl_renderer* renderer = zcsr_gl_create(surface, renderer_buf, sizeof renderer_buf);
    if (!renderer) {
        printf("SKIP: GL renderer unavailable on this surface/backend\n");
        zcsr_surface_destroy(surface);
        return 0;
    }

    zcsr_gl_set_vsync(renderer, false);
    zcsr_gl_texture tex = zcsr_gl_texture_create(renderer, rgba, 2, 2);
    if (!tex) {
        printf("FAIL: texture upload failed\n");
        zcsr_gl_destroy(renderer);
        zcsr_surface_destroy(surface);
        return 1;
    }

    zcsr_fill_sprites(sprites, MAX_SPRITES, tex);
    double ms200 = zcsr_bench_count(renderer, sprites, 200);
    double ms1000 = zcsr_bench_count(renderer, sprites, 1000);
    double ms10000 = zcsr_bench_count(renderer, sprites, 10000);
    printf("glrender bench: 200=%.4f ms, 1000=%.4f ms, 10000=%.4f ms\n", ms200, ms1000, ms10000);

    zcsr_gl_texture_destroy(renderer, tex);
    zcsr_gl_destroy(renderer);
    zcsr_surface_destroy(surface);
    return ms200 < 1.0 && ms10000 < 1.0 ? 0 : 1;
}
