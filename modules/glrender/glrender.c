/* Engine-ext Agent 1 — GPU rendering (OpenGL 3.3). PHASE-0 SKELETON STUB.
 * Implement against zcsr/gl_render.h: vendor glad under third_party/glad (glad used ONLY here),
 * create the GL context via the platform native handle/display, batch sprites, textures, FBO,
 * built-in shaders, bitmap text. Includes ONLY shared contracts (zero-coupling). */
#include "zcsr/gl_render.h"

zcsr_gl_renderer* zcsr_gl_create(zcsr_surface* surface, void* buffer, size_t bytes) {
    (void)surface; (void)buffer; (void)bytes; return 0; /* TODO: GL3.3 context + state in buffer */
}
void zcsr_gl_destroy(zcsr_gl_renderer* r) { (void)r; }
void zcsr_gl_set_vsync(zcsr_gl_renderer* r, bool on) { (void)r; (void)on; }

zcsr_gl_texture zcsr_gl_texture_create(zcsr_gl_renderer* r, const uint8_t* rgba, int w, int h) {
    (void)r; (void)rgba; (void)w; (void)h; return 0;
}
void zcsr_gl_texture_filter(zcsr_gl_renderer* r, zcsr_gl_texture t, bool linear) { (void)r; (void)t; (void)linear; }
void zcsr_gl_texture_destroy(zcsr_gl_renderer* r, zcsr_gl_texture t) { (void)r; (void)t; }

void zcsr_gl_begin(zcsr_gl_renderer* r) { (void)r; }
void zcsr_gl_submit(zcsr_gl_renderer* r, const zcsr_gl_sprite* s) { (void)r; (void)s; }
void zcsr_gl_flush(zcsr_gl_renderer* r) { (void)r; }
void zcsr_gl_submit_batch(zcsr_gl_renderer* r, const zcsr_gl_sprite* sprites, size_t count) {
    (void)r; (void)sprites; (void)count;
}
void zcsr_gl_set_target(zcsr_gl_renderer* r, zcsr_gl_texture target_or_zero) { (void)r; (void)target_or_zero; }
void zcsr_gl_draw_text(zcsr_gl_renderer* r, const char* utf8, float x, float y, float scale,
                       float cr, float cg, float cb, float ca) {
    (void)r; (void)utf8; (void)x; (void)y; (void)scale; (void)cr; (void)cg; (void)cb; (void)ca;
}
