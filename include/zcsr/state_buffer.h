#ifndef ZCSR_STATE_BUFFER_H
#define ZCSR_STATE_BUFFER_H
// Agent 1 — read-only game state buffer (string/int/bool). Implementer: modules/core.
// Read and write sides are split so most modules only ever see read-only state.
#include "value.h"

namespace zcsr {

// Read-only view of shared state. Keys are NUL-terminated strings.
class IStateReader {
public:
    virtual ~IStateReader() = default;
    virtual Value get(const char* key) const = 0;
    virtual bool  has(const char* key) const = 0;
};

// Privileged write side (e.g., the HSM persists its current state here).
// Returns false when fixed capacity is exceeded — never grows the heap.
class IStateWriter {
public:
    virtual ~IStateWriter() = default;
    virtual bool set(const char* key, Value v) = 0;
};

} // namespace zcsr
#endif // ZCSR_STATE_BUFFER_H
