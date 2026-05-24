#ifndef ZCSR_CLOCK_H
#define ZCSR_CLOCK_H
/* Agent 1 (Game Core) — high-precision clock + fixed-step frame pacing (~60 FPS).
 * Implementer: modules/input. No heap (timer placed in caller buffer). */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Monotonic high-precision time in nanoseconds. */
uint64_t zcsr_clock_now_ns(void);

typedef struct zcsr_frame_timer zcsr_frame_timer; /* opaque */

/* Init a fixed-step pacer in the caller buffer. NULL if buffer too small / fps == 0. */
zcsr_frame_timer* zcsr_frame_timer_init(void* buffer, size_t bytes, uint32_t target_fps);
/* Advance the pacer; returns true each time a full frame interval (1/target_fps) has elapsed. */
bool              zcsr_frame_timer_tick(zcsr_frame_timer*);
/* Nanoseconds elapsed since the previous tick that returned true (frame delta). */
uint64_t          zcsr_frame_timer_delta_ns(const zcsr_frame_timer*);

#endif /* ZCSR_CLOCK_H */
