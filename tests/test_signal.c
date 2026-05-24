/* Agent 2 self-test: compile-time X-macro router dispatches correctly, no heap. */
#include "zcsr/signal.h"
#include <stdio.h>
#include <string.h>

static int         ok_calls = 0, cancel_calls = 0;
static const char* last_payload = 0;

static bool on_ok(const char* p)     { ok_calls++;     last_payload = p; return true; }
static bool on_cancel(const char* p) { cancel_calls++; last_payload = p; return true; }

#define UI_CONNS(X)            \
    X("btn.ok",     on_ok)     \
    X("btn.cancel", on_cancel)
ZCSR_DEFINE_ROUTER(ui, UI_CONNS)

int main(void) {
    int fails = 0;

    ui_emit("btn.ok", "P1");
    if (ok_calls != 1 || cancel_calls != 0) { printf("FAIL: ok dispatch\n"); fails++; }
    if (!last_payload || strcmp(last_payload, "P1") != 0) { printf("FAIL: payload passthrough\n"); fails++; }

    ui_emit("btn.cancel", "P2");
    if (cancel_calls != 1) { printf("FAIL: cancel dispatch\n"); fails++; }

    ui_emit("btn.unknown", "P3"); /* no matching connection -> nothing fires */
    if (ok_calls != 1 || cancel_calls != 1) { printf("FAIL: unknown signal fired a slot\n"); fails++; }

    if (fails == 0) printf("test_signal: PASS (compile-time router dispatch ok)\n");
    return fails ? 1 : 0;
}
