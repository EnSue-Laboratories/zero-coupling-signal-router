#ifndef ZCSR_COLLISION_H
#define ZCSR_COLLISION_H
/* Agent 4 (Game Core) — AABB collision. Pure functions, no state, no heap.
 * Implementer: modules/gamelogic. */
#include <stdbool.h>

typedef struct { float x, y, w, h; } zcsr_aabb; /* x,y = top-left; w,h = size */

bool zcsr_aabb_intersect(zcsr_aabb a, zcsr_aabb b);
bool zcsr_aabb_contains_point(zcsr_aabb a, float px, float py);

#endif /* ZCSR_COLLISION_H */
