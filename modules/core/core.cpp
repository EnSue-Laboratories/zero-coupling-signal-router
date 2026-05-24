// Agent 1 — Core Foundation (stack-only memory manager + read-only state buffer).
// Constraints: no runtime allocation, string/int/bool only, single-header-friendly,
// zero external deps, total footprint < 256KB.
// Includes ONLY shared contracts — never another module's source.
#include "zcsr/factories.h"

namespace zcsr {

// TODO(Agent 1 / Codex): implement
//   - IArena: linear/stack allocator over the caller buffer.
//   - IStateReader / IStateWriter: fixed-capacity key->Value map (string/int/bool), no heap.
IArena*       make_arena(void*, size_t)        { return nullptr; }
IStateReader* make_state_reader(void*, size_t) { return nullptr; }
IStateWriter* make_state_writer(void*, size_t) { return nullptr; }

} // namespace zcsr
