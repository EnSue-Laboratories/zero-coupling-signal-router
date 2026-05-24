// Agent 6 — Integration & Validation. Owns the public wiring + demo + benchmark.
// Includes ONLY contracts (zcsr/factories.h) — never module sources. This is the seam
// that proves modules stay zero-coupled: integration depends on interfaces, not implementations.
#include "zcsr/factories.h"
#include <cstdio>

using namespace zcsr;

int main() {
    // Fixed, no-heap buffers (total well under the 256KB core budget).
    static unsigned char arena_mem[64 * 1024];
    static unsigned char state_mem[64 * 1024];

    IArena*         arena  = make_arena(arena_mem, sizeof arena_mem);
    IStateWriter*   writer = make_state_writer(state_mem, sizeof state_mem);
    IStateReader*   reader = make_state_reader(state_mem, sizeof state_mem);
    INativeSurface* surf   = make_native_surface();
    IOverlay*       ovl    = make_overlay();
    IHsm*           hsm    = make_hsm(writer);

    std::printf("zcsr demo — module wiring (Phase 0 skeleton)\n");
    std::printf("  Agent1 core.arena       : %s\n", arena ? "ready" : "TODO");
    std::printf("  Agent1 core.state       : %s\n", (reader && writer) ? "ready" : "TODO");
    std::printf("  Agent5 platform.surface : %s\n", surf ? "ready" : "TODO");
    std::printf("  Agent4 overlay          : %s\n", ovl ? "ready" : "TODO");
    std::printf("  Agent3 hsm              : %s\n", hsm ? "ready" : "TODO");
    std::printf("  Agent2 signal           : header-only Router (see zcsr/signal.h)\n");

    // TODO(Agent 6): once modules land, run the spec demo:
    //   surf->create("zcsr", {x,y,w,h}); ovl->init(surf);
    //   ovl->setText(...); ovl->setBitmap(...); ovl->setButtons(buttons<=3);
    //   loop: ovl->onPointer(x,y,clicked) -> emits click signal -> hsm->dispatch(...) -> writer->set(...)
    //   (Hover shows item info; click returns bool only.)
    return 0;
}
