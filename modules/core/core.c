/* Agent 1 — Core Foundation (stack-only memory manager + read-only state buffer).
 * Constraints: no runtime allocation, string/int/bool only, zero deps, footprint < 256KB.
 * Includes ONLY shared contracts — never another module's source. */
#include "zcsr/arena.h"
#include "zcsr/state_buffer.h"

#include <stdint.h>

struct zcsr_arena {
    unsigned char* data;
    size_t capacity;
    size_t used;
};

typedef struct {
    const char* key;
    zcsr_value value;
} zcsr_state_entry;

struct zcsr_state {
    zcsr_state_entry* entries;
    size_t capacity;
    size_t count;
    char* strings;
    size_t strings_capacity;
    size_t strings_used;
};

static uintptr_t zcsr_align_up_ptr(uintptr_t value, size_t align) {
    if (align <= 1) return value;
    uintptr_t rem = value % (uintptr_t)align;
    if (rem == 0) return value;
    return value + ((uintptr_t)align - rem);
}

static size_t zcsr_strlen(const char* s) {
    size_t n = 0;
    if (!s) return 0;
    while (s[n] != '\0') ++n;
    return n;
}

static bool zcsr_streq(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

static char* zcsr_state_store_string(zcsr_state* s, const char* text) {
    size_t len;
    char* dst;
    if (!s || !text) return 0;
    len = zcsr_strlen(text) + 1u;
    if (len == 0 || len > s->strings_capacity - s->strings_used) return 0;
    dst = s->strings + s->strings_used;
    for (size_t i = 0; i < len; ++i) dst[i] = text[i];
    s->strings_used += len;
    return dst;
}

static void zcsr_copy_string(char* dst, const char* src, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) dst[i] = src[i];
}

static zcsr_state_entry* zcsr_state_find(zcsr_state* s, const char* key) {
    if (!s || !key) return 0;
    for (size_t i = 0; i < s->count; ++i) {
        if (zcsr_streq(s->entries[i].key, key)) return &s->entries[i];
    }
    return 0;
}

static const zcsr_state_entry* zcsr_state_find_const(const zcsr_state* s, const char* key) {
    if (!s || !key) return 0;
    for (size_t i = 0; i < s->count; ++i) {
        if (zcsr_streq(s->entries[i].key, key)) return &s->entries[i];
    }
    return 0;
}

zcsr_arena* zcsr_arena_init(void* buffer, size_t bytes) {
    uintptr_t raw = (uintptr_t)buffer;
    uintptr_t header_at = zcsr_align_up_ptr(raw, sizeof(void*));
    uintptr_t data_at = zcsr_align_up_ptr(header_at + sizeof(zcsr_arena), sizeof(void*));
    zcsr_arena* arena;

    if (!buffer || data_at < raw || data_at > raw + bytes) return 0;
    arena = (zcsr_arena*)header_at;
    arena->data = (unsigned char*)data_at;
    arena->capacity = (raw + bytes) - data_at;
    arena->used = 0;
    return arena;
}

void* zcsr_arena_alloc(zcsr_arena* a, size_t bytes, size_t align) {
    uintptr_t aligned_ptr;
    size_t offset;
    if (!a || bytes == 0) return 0;
    if (align == 0) align = 1;
    aligned_ptr = zcsr_align_up_ptr((uintptr_t)(a->data + a->used), align);
    if (aligned_ptr < (uintptr_t)a->data) return 0;
    offset = (size_t)(aligned_ptr - (uintptr_t)a->data);
    if (offset > a->capacity) return 0;
    if (bytes > a->capacity - offset) return 0;
    a->used = offset + bytes;
    return (void*)aligned_ptr;
}

void zcsr_arena_reset(zcsr_arena* a) {
    if (a) a->used = 0;
}

size_t zcsr_arena_used(const zcsr_arena* a) {
    return a ? a->used : 0;
}

size_t zcsr_arena_capacity(const zcsr_arena* a) {
    return a ? a->capacity : 0;
}

zcsr_state* zcsr_state_init(void* buffer, size_t bytes) {
    uintptr_t raw = (uintptr_t)buffer;
    uintptr_t header_at = zcsr_align_up_ptr(raw, sizeof(void*));
    uintptr_t entries_at;
    uintptr_t strings_at;
    size_t remaining;
    size_t max_entries;
    zcsr_state* state;

    if (!buffer || header_at + sizeof(zcsr_state) < header_at || header_at + sizeof(zcsr_state) > raw + bytes) {
        return 0;
    }

    entries_at = zcsr_align_up_ptr(header_at + sizeof(zcsr_state), sizeof(void*));
    if (entries_at > raw + bytes) return 0;

    remaining = (raw + bytes) - entries_at;
    max_entries = remaining / (sizeof(zcsr_state_entry) + 32u);
    if (max_entries > 128u) max_entries = 128u;
    if (max_entries == 0) return 0;

    strings_at = zcsr_align_up_ptr(entries_at + max_entries * sizeof(zcsr_state_entry), sizeof(void*));
    if (strings_at > raw + bytes) return 0;

    state = (zcsr_state*)header_at;
    state->entries = (zcsr_state_entry*)entries_at;
    state->capacity = max_entries;
    state->count = 0;
    state->strings = (char*)strings_at;
    state->strings_capacity = (raw + bytes) - strings_at;
    state->strings_used = 0;

    for (size_t i = 0; i < max_entries; ++i) {
        state->entries[i].key = 0;
        state->entries[i].value = zcsr_none();
    }

    return state;
}

zcsr_value zcsr_state_get(const zcsr_state* s, const char* key) {
    const zcsr_state_entry* entry = zcsr_state_find_const(s, key);
    return entry ? entry->value : zcsr_none();
}

bool zcsr_state_has(const zcsr_state* s, const char* key) {
    return zcsr_state_find_const(s, key) != 0;
}

bool zcsr_state_set(zcsr_state* s, const char* key, zcsr_value v) {
    zcsr_state_entry* entry;
    zcsr_value stored = v;
    size_t checkpoint;

    if (!s || !key || v.type == ZCSR_NONE) return false;

    entry = zcsr_state_find(s, key);
    if (entry) {
        if (v.type == ZCSR_STR) {
            const char* text = v.s ? v.s : "";
            size_t bytes = zcsr_strlen(text) + 1u;
            if (entry->value.type == ZCSR_STR && entry->value.s && bytes <= zcsr_strlen(entry->value.s) + 1u) {
                zcsr_copy_string((char*)entry->value.s, text, bytes);
                stored.s = entry->value.s;
            } else {
                stored.s = zcsr_state_store_string(s, text);
                if (!stored.s) return false;
            }
        }
        entry->value = stored;
        return true;
    }

    if (s->count >= s->capacity) return false;
    checkpoint = s->strings_used;
    entry = &s->entries[s->count];
    entry->key = zcsr_state_store_string(s, key);
    if (!entry->key) {
        s->strings_used = checkpoint;
        return false;
    }
    if (v.type == ZCSR_STR) {
        stored.s = zcsr_state_store_string(s, v.s ? v.s : "");
        if (!stored.s) {
            entry->key = 0;
            s->strings_used = checkpoint;
            return false;
        }
    }
    entry->value = stored;
    s->count += 1u;
    return true;
}
