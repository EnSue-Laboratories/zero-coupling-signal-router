#ifndef ZCSR_FACTORIES_H
#define ZCSR_FACTORIES_H
// Module factory contracts. Each module implements ITS factory in its own TU.
// Integration/demo obtains interface instances through these — WITHOUT including any
// module source. This is the seam that keeps modules zero-coupled.
//
// Memory: callers pass fixed buffers; modules never allocate from the heap.
#include "arena.h"
#include "state_buffer.h"
#include "hsm.h"
#include "platform.h"
#include "overlay.h"

namespace zcsr {

// Agent 1 — Core. Construct over caller-owned buffers (placement only, no heap).
IArena*       make_arena(void* buffer, size_t bytes);
IStateReader* make_state_reader(void* buffer, size_t bytes);
IStateWriter* make_state_writer(void* buffer, size_t bytes);

// Agent 5 — Platform. Native surface for the compile-time-selected backend.
INativeSurface* make_native_surface();

// Agent 4 — Overlay renderer.
IOverlay* make_overlay();

// Agent 3 — HSM that persists its current state into `persistTo`.
IHsm* make_hsm(IStateWriter* persistTo);

} // namespace zcsr
#endif // ZCSR_FACTORIES_H
