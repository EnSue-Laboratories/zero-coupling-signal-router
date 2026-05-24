#ifndef ZCSR_SIGNAL_H
#define ZCSR_SIGNAL_H
// Agent 2 — compile-time, type-safe signal/slot. Zero runtime overhead. No STL/Qt/Boost.
// Implementer: modules/signal.
//
// Spec signatures:   void signal(string)   /   bool slot(string)
//
// "Zero runtime overhead" = no dynamic registration, no heap, no runtime lookup table.
// Connections are known at compile time; the Router template resolves dispatch statically
// (e.g., a constexpr fold over the Conn list). See docs/ARCHITECTURE.md.

namespace zcsr {

// A slot consumes a signal's NUL-terminated string payload and returns true if it handled it.
using SlotFn   = bool (*)(const char* payload);
// Emitter signature required by the spec.
using SignalFn = void (*)(const char* payload);

// Compile-time binding of a signal tag (NUL-terminated literal) to a slot.
struct Conn {
    const char* signal;
    SlotFn      slot;
};

// Router contract (implemented by modules/signal):
//   template <Conn... Cs> struct Router { static void emit(const char* sig, const char* payload); };
// emit() must dispatch to every Conn whose signal tag matches `sig`, with no runtime allocation
// and no dynamic registry. Phase-0 declares the contract; Agent 2 provides the template impl.

} // namespace zcsr
#endif // ZCSR_SIGNAL_H
