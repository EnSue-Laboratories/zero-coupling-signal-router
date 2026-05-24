/* Agent 5 — input/clock tests: monotonic clock, fixed-step frame timer, input init/queue guards,
 * window-close flag. The OS-event-driven paths (real key events, real window close) require a
 * windowed host; this covers the deterministic, platform-independent surface that runs headless. */
#include "zcsr/clock.h"
#include "zcsr/input.h"
#include "zcsr/window_event.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static int fails = 0;
#define CK(label, cond) do { if (!(cond)) { printf("FAIL: %s\n", label); fails++; } } while (0)

/* Busy-wait at least `ns` nanoseconds using the monotonic clock under test. */
static void spin_ns(uint64_t ns) {
    uint64_t start = zcsr_clock_now_ns();
    while (zcsr_clock_now_ns() - start < ns) { /* spin */ }
}

static void test_clock(void) {
    uint64_t t0 = zcsr_clock_now_ns();
    CK("clock nonzero", t0 != 0);
    spin_ns(1000000ull); /* 1ms */
    uint64_t t1 = zcsr_clock_now_ns();
    CK("clock monotonic advances", t1 > t0);
}

static void test_frame_timer(void) {
    uint8_t buf[64];
    CK("init null buffer -> NULL", zcsr_frame_timer_init(NULL, sizeof buf, 60) == NULL);
    CK("init tiny buffer -> NULL", zcsr_frame_timer_init(buf, 1, 60) == NULL);
    CK("init fps=0 -> NULL",       zcsr_frame_timer_init(buf, sizeof buf, 0) == NULL);

    zcsr_frame_timer* t = zcsr_frame_timer_init(buf, sizeof buf, 60); /* ~16.67ms / frame */
    CK("init valid -> non-NULL",   t != NULL);
    CK("no tick before interval",  zcsr_frame_timer_tick(t) == false);

    spin_ns(20000000ull); /* 20ms > one 60FPS frame */
    CK("tick after interval",      zcsr_frame_timer_tick(t) == true);
    uint64_t d = zcsr_frame_timer_delta_ns(t);
    CK("delta >= frame interval",  d >= 16000000ull);
    CK("delta sane (<1s)",         d < 1000000000ull);
    CK("no immediate re-tick",     zcsr_frame_timer_tick(t) == false); /* baseline reset to now */

    CK("tick NULL -> false",       zcsr_frame_timer_tick(NULL) == false);
    CK("delta NULL -> 0",          zcsr_frame_timer_delta_ns(NULL) == 0);
}

static void test_input(void) {
    uint8_t buf[4096]; /* zcsr_input holds a 512-entry key table + 64-event ring */
    CK("input init null -> NULL", zcsr_input_init(NULL, sizeof buf) == NULL);
    CK("input init tiny -> NULL", zcsr_input_init(buf, 1) == NULL);

    zcsr_input* in = zcsr_input_init(buf, sizeof buf);
    CK("input init valid -> non-NULL", in != NULL);

    zcsr_key_event ev;
    CK("fresh queue empty -> poll false", zcsr_input_poll(in, &ev) == false);
    CK("fresh key not down",              zcsr_input_is_down(in, 65) == false);
    CK("poll NULL input -> false",        zcsr_input_poll(NULL, &ev) == false);
    CK("poll NULL out -> false",          zcsr_input_poll(in, NULL) == false);
    CK("is_down NULL -> false",           zcsr_input_is_down(NULL, 65) == false);
    CK("is_down out-of-range -> false",   zcsr_input_is_down(in, 60000) == false);
}

static void test_window_close(void) {
    int dummy;
    zcsr_surface* fake = (zcsr_surface*)&dummy; /* identity-compared only, never dereferenced */
    CK("close NULL surface -> false",    zcsr_window_close_requested(NULL) == false);
    CK("close unknown surface -> false", zcsr_window_close_requested(fake) == false);
}

int main(void) {
    test_clock();
    test_frame_timer();
    test_input();
    test_window_close();
    if (!fails) printf("test_input: PASS (clock + frame timer + input init/queue + close flag)\n");
    return fails ? 1 : 0;
}
