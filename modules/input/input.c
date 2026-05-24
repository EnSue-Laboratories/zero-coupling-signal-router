/* Agent 1 (Game Core) — Input & Window: keyboard events, window close, 60FPS timer.
 * Includes ONLY shared contracts. TODO(Agent 1 / Codex): real implementations. */
#include "zcsr/clock.h"
#include "zcsr/input.h"
#include "zcsr/window_event.h"

uint64_t          zcsr_clock_now_ns(void)                                      { return 0; }
zcsr_frame_timer* zcsr_frame_timer_init(void* b, size_t n, uint32_t fps)       { (void)b; (void)n; (void)fps; return 0; }
bool              zcsr_frame_timer_tick(zcsr_frame_timer* t)                   { (void)t; return false; }
uint64_t          zcsr_frame_timer_delta_ns(const zcsr_frame_timer* t)        { (void)t; return 0; }

zcsr_input* zcsr_input_init(void* b, size_t n)                                 { (void)b; (void)n; return 0; }
void        zcsr_input_pump(zcsr_input* i, zcsr_surface* w)                    { (void)i; (void)w; }
bool        zcsr_input_poll(zcsr_input* i, zcsr_key_event* o)                  { (void)i; (void)o; return false; }
bool        zcsr_input_is_down(const zcsr_input* i, zcsr_keycode k)            { (void)i; (void)k; return false; }

bool        zcsr_window_close_requested(zcsr_surface* s)                       { (void)s; return false; }
