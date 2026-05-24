#ifndef ZCSR_ARENA_H
#define ZCSR_ARENA_H
// Agent 1 — stack-only memory manager (no runtime/heap allocation). Implementer: modules/core.
// CONTRACT ONLY. Modules include this header, never another module's source.
#include <cstddef>

namespace zcsr {

// Linear/stack allocator over a caller-provided fixed buffer. No malloc/new ever.
// `reset()` frees everything (LIFO stack semantics for scoped use).
class IArena {
public:
    virtual ~IArena() = default;
    virtual void*  alloc(size_t bytes, size_t align) = 0; // returns nullptr if exhausted
    virtual void   reset() = 0;
    virtual size_t used() const = 0;
    virtual size_t capacity() const = 0;
};

} // namespace zcsr
#endif // ZCSR_ARENA_H
