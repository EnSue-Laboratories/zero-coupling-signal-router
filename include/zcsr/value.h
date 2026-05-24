#ifndef ZCSR_VALUE_H
#define ZCSR_VALUE_H
// Zero-Coupling Signal Router — shared value type (Agent 1 data primitive).
// ONLY string / int / bool, per spec. No heap, no STL.
#include <cstdint>

namespace zcsr {

enum class ValueType : uint8_t { None = 0, Bool, Int, Str };

// A tagged value. For Str, `s` is a NUL-terminated pointer into caller/arena-owned
// storage; Value never owns or frees memory.
struct Value {
    ValueType   type = ValueType::None;
    bool        b    = false;
    int64_t     i    = 0;
    const char* s    = nullptr;

    static constexpr Value none()             { return Value{ValueType::None, false, 0, nullptr}; }
    static constexpr Value of(bool v)         { return Value{ValueType::Bool, v,    0, nullptr}; }
    static constexpr Value of(int64_t v)      { return Value{ValueType::Int,  false, v, nullptr}; }
    static constexpr Value of(const char* v)  { return Value{ValueType::Str,  false, 0, v};      }
};

} // namespace zcsr
#endif // ZCSR_VALUE_H
