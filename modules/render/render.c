/* Agent 3 (Game Core) — Rendering & Resources: stb_image PNG load + cache, batch sprite renderer.
 * Includes ONLY shared contracts. TODO(Agent 3 / Codex): stb_image (third_party/) + batch renderer. */
#include "zcsr/texture.h"
#include "zcsr/sprite.h"

zcsr_texture_cache* zcsr_texture_cache_init(void* b, size_t n)                 { (void)b; (void)n; return 0; }
const zcsr_texture* zcsr_texture_load(zcsr_texture_cache* c, const char* path) { (void)c; (void)path; return 0; }

zcsr_sprite_batch* zcsr_sprite_batch_init(void* b, size_t n, zcsr_surface* t)  { (void)b; (void)n; (void)t; return 0; }
void               zcsr_sprite_batch_begin(zcsr_sprite_batch* s)              { (void)s; }
void               zcsr_sprite_batch_submit(zcsr_sprite_batch* s, const zcsr_sprite* sp) { (void)s; (void)sp; }
void               zcsr_sprite_batch_flush(zcsr_sprite_batch* s)              { (void)s; }
