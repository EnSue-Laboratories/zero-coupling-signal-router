// Agent 3 — Hierarchical State Machine. No global state. Transitions via signals only.
// Persists current state into the Core state buffer (IStateWriter).
#include "zcsr/factories.h"

namespace zcsr {

// TODO(Agent 3 / Codex): implement IHsm
//   - hierarchical states, no global/static mutable state (instance-owned).
//   - dispatch(signal, payload): resolve transition for the active state hierarchy.
//   - on transition: persistTo->set("hsm.state", Value::of(current())).
IHsm* make_hsm(IStateWriter* /*persistTo*/) { return nullptr; }

} // namespace zcsr
