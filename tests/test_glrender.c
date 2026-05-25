#include "zcsr/gl_render.h"

#include <stdio.h>

int main(void) {
    static unsigned char renderer_buf[8 * 1024 * 1024];
    static const unsigned char rgba[16] = {
        255, 255, 255, 255, 255,   0,   0, 255,
          0, 255,   0, 255,   0,   0, 255, 255
    };

    if (zcsr_gl_create(0, renderer_buf, sizeof renderer_buf)) {
        printf("FAIL: null surface should not create a GL renderer\n");
        return 1;
    }
    if (zcsr_gl_create((zcsr_surface*)1, renderer_buf, 1)) {
        printf("FAIL: undersized caller buffer should not create a GL renderer\n");
        return 1;
    }

    /* NULL-safety: the public API is called every frame from game loops — must no-op on a NULL
     * renderer (and return invalid for value-returning calls), never deref. Runs headless. */
    zcsr_gl_destroy(0);
    zcsr_gl_set_vsync(0, true);
    zcsr_gl_begin(0);
    zcsr_gl_submit(0, 0);
    zcsr_gl_submit_batch(0, 0, 0);
    zcsr_gl_flush(0);
    zcsr_gl_set_target(0, 0);
    zcsr_gl_texture_filter(0, 0, true);
    zcsr_gl_texture_destroy(0, 0);
    zcsr_gl_draw_text(0, "x", 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    if (zcsr_gl_texture_create(0, rgba, 2, 2) != 0) {
        printf("FAIL: texture_create(NULL renderer) must return 0\n");
        return 1;
    }
    if (zcsr_gl_texture_upload(0, 1, rgba, 2, 2)) {
        printf("FAIL: texture_upload(NULL renderer) must return false\n");
        return 1;
    }

    zcsr_surface* surface = zcsr_surface_create("zcsr glrender smoke", (zcsr_rect){ 0, 0, 320, 240 });
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
    if (!zcsr_gl_texture_upload(renderer, tex, rgba, 2, 2)) {
        printf("FAIL: texture re-upload failed\n");
        zcsr_gl_destroy(renderer);
        zcsr_surface_destroy(surface);
        return 1;
    }

    zcsr_gl_texture_filter(renderer, tex, true);
    zcsr_gl_texture_filter(renderer, tex, false);
    zcsr_gl_sprite sprite = { tex, { 16.0f, 16.0f, 64.0f, 64.0f }, 0.25f, 1.0f, 1.0f, 1.0f, 1.0f,
                              ZCSR_MOD_MULTIPLY, 0.0f };
    zcsr_gl_begin(renderer);
    zcsr_gl_submit(renderer, &sprite);
    zcsr_gl_draw_text(renderer, "ZCSR", 4.0f, 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    zcsr_gl_flush(renderer);
    zcsr_gl_texture_destroy(renderer, tex);
    zcsr_gl_destroy(renderer);
    zcsr_surface_destroy(surface);
    printf("PASS: GL renderer smoke\n");
    return 0;
}
