// Agent 6 — micro-benchmark harness.
// Targets (from spec): single frame render < 1ms; 1000 refreshes/sec; < 0.1 FPS impact.
// NOTE: modules stay STL-free; this validation harness may use std (it is a test tool, not shipped).
#include "zcsr/factories.h"
#include <cstdio>
#include <chrono>

using namespace zcsr;

int main() {
    IOverlay* ovl = make_overlay();
    if (!ovl) {
        std::printf("bench: overlay not implemented yet (Agent 4) — skipping render benchmark.\n");
        std::printf("Validate once ready: < 1ms/frame, >= 1000 refresh/s, < 0.1 FPS impact.\n");
        return 0;
    }
    constexpr int N = 1000;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) ovl->render();
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("bench: %d renders in %.3f ms  ->  %.4f ms/frame, %.0f refresh/s\n",
                N, ms, ms / N, N / (ms / 1000.0));
    const bool pass = (ms / N) < 1.0 && (N / (ms / 1000.0)) >= 1000.0;
    std::printf("PASS criteria (<1ms/frame && >=1000 refresh/s): %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
