#ifndef ZCSR_SIGNAL_H
#define ZCSR_SIGNAL_H
/* Agent 2 — compile-time signal/slot. No heap, no dynamic registry. Implementer: modules/signal.
 *
 * Spec signatures:   void signal(string)   /   bool slot(string)
 *
 * In C, "compile-time / zero runtime overhead" means: connections live in a `static const`
 * table built at compile time (ideally via an X-macro), and dispatch is a fixed walk over that
 * table — no malloc, no runtime registration. (C lacks templates, so type-safety is enforced by
 * the fixed `zcsr_slot_fn` signature; see docs/ARCHITECTURE.md.) */
#include <stddef.h>
#include <stdbool.h>

typedef bool (*zcsr_slot_fn)(const char* payload);   /* slot: handles payload, true if handled */
typedef void (*zcsr_signal_fn)(const char* payload); /* emitter signature required by spec */

typedef struct {
    const char*  signal; /* compile-time signal tag (NUL-terminated literal) */
    zcsr_slot_fn slot;
} zcsr_conn;

/* Dispatch `payload` to every connection whose tag matches `signal`. Pure table walk; no heap.
 * Agent 2 also provides an X-macro (ZCSR_ROUTER) to declare the static const table cleanly. */
void zcsr_emit(const zcsr_conn* table, size_t count, const char* signal, const char* payload);

#endif /* ZCSR_SIGNAL_H */
