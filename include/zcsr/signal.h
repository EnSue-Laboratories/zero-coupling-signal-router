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

/* Dispatch `payload` to every connection whose tag matches `signal`. Pure table walk; no heap. */
void zcsr_emit(const zcsr_conn* table, size_t count, const char* signal, const char* payload);

/* ---- Compile-time router (X-macro) ---------------------------------------------------------
 * Declare connections ONCE as an X-macro list, then expand into a `static const` table plus a
 * typed `<name>_emit()`. The table is built at compile time — no dynamic registration, no heap.
 *
 *   bool on_ok(const char*);  bool on_cancel(const char*);
 *   #define MY_CONNS(X)  X("btn.ok", on_ok)  X("btn.cancel", on_cancel)
 *   ZCSR_DEFINE_ROUTER(ui, MY_CONNS)
 *   ...
 *   ui_emit("btn.ok", "payload");   // dispatches to on_ok
 *
 * Each slot is checked against the fixed `zcsr_slot_fn` signature via C11 `_Generic`: a slot
 * with the wrong signature has no matching association and is a hard COMPILE ERROR (not just a
 * warning — independent of `-Werror`). */
#define ZCSR_CONN(sig, slot) { (sig), _Generic((slot), zcsr_slot_fn: (slot)) },
#define ZCSR_DEFINE_ROUTER(name, LIST)                                                       \
    static const zcsr_conn name##_table[] = { LIST(ZCSR_CONN) };                              \
    static inline void name##_emit(const char* signal, const char* payload) {                \
        zcsr_emit(name##_table, sizeof(name##_table) / sizeof((name##_table)[0]),             \
                  signal, payload);                                                           \
    }

#endif /* ZCSR_SIGNAL_H */
