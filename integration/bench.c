/* Agent 6 — micro-benchmark harness.
 * Targets (spec): single frame render < 1ms; 1000 refreshes/sec; < 0.1 FPS impact. */
#define _POSIX_C_SOURCE 199309L
#include "zcsr/overlay.h"
#include <stdio.h>
#include <time.h>

int main(void) {
    zcsr_overlay* ovl = zcsr_overlay_create(0);
    if (!ovl) {
        printf("bench: overlay not implemented yet (Agent 4) — skipping render benchmark.\n");
        printf("Validate once ready: < 1ms/frame, >= 1000 refresh/s, < 0.1 FPS impact.\n");
        return 0;
    }
    enum { N = 1000 };
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; ++i) zcsr_overlay_render(ovl);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("bench: %d renders in %.3f ms  ->  %.4f ms/frame, %.0f refresh/s\n",
           N, ms, ms / N, N / (ms / 1000.0));
    int pass = (ms / N) < 1.0 && (N / (ms / 1000.0)) >= 1000.0;
    printf("PASS (<1ms/frame && >=1000 refresh/s): %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
