#ifndef ZCSR_HSM_H
#define ZCSR_HSM_H
// Agent 3 — Hierarchical State Machine. No global state. Transitions via signals only.
// Persists current state to the Core state buffer (via IStateWriter). Implementer: modules/hsm.
#include "signal.h"
#include "state_buffer.h"

namespace zcsr {

class IHsm {
public:
    virtual ~IHsm() = default;
    // Deliver a signal to drive a transition. Returns true if a transition occurred.
    virtual bool dispatch(const char* signal, const char* payload) = 0;
    // Current leaf-state name (NUL-terminated); also persisted to the state buffer.
    virtual const char* current() const = 0;
};

} // namespace zcsr
#endif // ZCSR_HSM_H
