/* Agent 4 (Game Core) — Game Logic: AABB collision + binary save/load.
 * Includes ONLY shared contracts. TODO(Agent 4 / Codex): real implementation. */
#include "zcsr/collision.h"
#include "zcsr/save.h"

bool zcsr_aabb_intersect(zcsr_aabb a, zcsr_aabb b)                             { (void)a; (void)b; return false; }
bool zcsr_aabb_contains_point(zcsr_aabb a, float px, float py)                { (void)a; (void)px; (void)py; return false; }

size_t zcsr_save_write(const void* s, size_t ss, void* o, size_t oc, uint32_t v) { (void)s; (void)ss; (void)o; (void)oc; (void)v; return 0; }
size_t zcsr_save_read(const void* in, size_t is, void* s, size_t sc, uint32_t ev) { (void)in; (void)is; (void)s; (void)sc; (void)ev; return 0; }
