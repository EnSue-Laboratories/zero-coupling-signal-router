// Agent 2 — Compile-time, type-safe signal/slot. Zero runtime overhead. No STL/Qt/Boost.
// The Router is header-only/template (see contract in zcsr/signal.h); this TU exists so the
// module builds independently and can host any non-template helpers.
#include "zcsr/signal.h"

namespace zcsr {
// TODO(Agent 2 / Codex): provide `template <Conn... Cs> struct Router` with constexpr dispatch:
//   static void emit(const char* sig, const char* payload) {
//     // fold over Cs: if (matches(Cs.signal, sig)) Cs.slot(payload);  — no heap, no runtime registry
//   }
// Keep it in a header (modules/signal/include) so consumers get zero-overhead inlining.
} // namespace zcsr
