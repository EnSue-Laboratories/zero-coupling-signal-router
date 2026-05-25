#ifndef ZCSR_GL_RENDER_H
#define ZCSR_GL_RENDER_H
/* Engine-ext Agent 1 — GPU rendering (OpenGL 3.3 Core). OPTIONAL renderer, beside the existing
 * software/overlay path (which stays unchanged). Implementer: modules/glrender (vendors glad;
 * glad used ONLY inside that module). Includes only shared contracts. No runtime heap: renderer
 * state lives in a caller buffer; GPU memory is the driver's. */
#include "platform.h"  /* zcsr_surface */
#include "sprite.h"    /* zcsr_rectf */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct zcsr_gl_renderer zcsr_gl_renderer; /* opaque; instance-owned */
typedef uint32_t zcsr_gl_texture;                 /* GPU texture handle; 0 = invalid */

/* Create a GL 3.3 Core context on the surface (Win32 wgl / X11 GLX / Cocoa NSOpenGL|CGL) and the
 * renderer state inside the caller buffer. NULL on failure / buffer too small. */
zcsr_gl_renderer* zcsr_gl_create(zcsr_surface* surface, void* buffer, size_t bytes);
void              zcsr_gl_destroy(zcsr_gl_renderer*);
void              zcsr_gl_set_vsync(zcsr_gl_renderer*, bool on);

/* Upload 8-bit RGBA pixels to a GPU texture. `linear` = linear filter (else nearest). */
zcsr_gl_texture   zcsr_gl_texture_create(zcsr_gl_renderer*, const uint8_t* rgba, int w, int h);
void              zcsr_gl_texture_filter(zcsr_gl_renderer*, zcsr_gl_texture, bool linear);
void              zcsr_gl_texture_destroy(zcsr_gl_renderer*, zcsr_gl_texture);

/* One drawable sprite for the batch: GPU texture + dest rect (top-left origin) + rotation about
 * the rect center (radians) + color modulation (0..1). */
typedef struct {
    zcsr_gl_texture tex;
    zcsr_rectf      dst;
    float           rotation;
    float           r, g, b, a;
} zcsr_gl_sprite;

/* Batched draw. Target throughput: 200..10000 sprites within frame budget.
 * begin() clears the batch; submit() appends one; flush() draws all in one pass.
 * submit_batch() is the signal-friendly entry (render_submit_batch maps here). */
void zcsr_gl_begin(zcsr_gl_renderer*);
void zcsr_gl_submit(zcsr_gl_renderer*, const zcsr_gl_sprite*);
void zcsr_gl_flush(zcsr_gl_renderer*);
void zcsr_gl_submit_batch(zcsr_gl_renderer*, const zcsr_gl_sprite* sprites, size_t count);

/* Offscreen render-to-texture: set the draw target (0 = back buffer / window). */
void zcsr_gl_set_target(zcsr_gl_renderer*, zcsr_gl_texture target_or_zero);

/* Built-in bitmap-font text (scores / UI). Top-left origin, color 0..1. No external font loading. */
void zcsr_gl_draw_text(zcsr_gl_renderer*, const char* utf8, float x, float y, float scale,
                       float r, float g, float b, float a);

#endif /* ZCSR_GL_RENDER_H */
