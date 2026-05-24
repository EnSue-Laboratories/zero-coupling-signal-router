#ifndef ZCSR_SPRITE_H
#define ZCSR_SPRITE_H
/* Agent 3 (Game Core) — batch sprite renderer. Submit many sprites, draw them in one flush.
 * Target: render n*10000 (n>0) sprites within the frame budget; bench gate = 200 sprites <1ms.
 * Implementer: modules/render. Draws onto the platform surface. */
#include "texture.h"
#include "platform.h"
#include <stddef.h>

typedef struct { float x, y, w, h; } zcsr_rectf; /* destination rect, top-left origin */

typedef struct {
    const zcsr_texture* tex;
    zcsr_rectf          dst;
    /* (uv sub-rect / tint can be added later without breaking ABI) */
} zcsr_sprite;

typedef struct zcsr_sprite_batch zcsr_sprite_batch; /* opaque; fixed capacity in caller buffer */

zcsr_sprite_batch* zcsr_sprite_batch_init(void* buffer, size_t bytes, zcsr_surface* target);
void               zcsr_sprite_batch_begin(zcsr_sprite_batch*);
void               zcsr_sprite_batch_submit(zcsr_sprite_batch*, const zcsr_sprite*);
void               zcsr_sprite_batch_flush(zcsr_sprite_batch*); /* draw all submitted in one batch */

#endif /* ZCSR_SPRITE_H */
