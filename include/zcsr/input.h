#ifndef ZCSR_INPUT_H
#define ZCSR_INPUT_H
/* Agent 1 (Game Core) — keyboard input (key down / key up). Implementer: modules/input.
 * Events are buffered in a fixed ring (no heap). Pumped from the platform window. */
#include "platform.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum { ZCSR_KEY_UP = 0, ZCSR_KEY_DOWN = 1 } zcsr_key_action;
typedef uint16_t zcsr_keycode; /* platform-independent code; backend maps OS scancodes -> these */

typedef struct {
    zcsr_keycode    key;
    zcsr_key_action action;
} zcsr_key_event;

typedef struct zcsr_input zcsr_input; /* opaque */

zcsr_input* zcsr_input_init(void* buffer, size_t bytes);
/* Drain OS key messages from the window into the queue (call once per frame). */
void        zcsr_input_pump(zcsr_input*, zcsr_surface* window);
/* Dequeue the next key event; false when the queue is empty. */
bool        zcsr_input_poll(zcsr_input*, zcsr_key_event* out);
/* Current held state of a key (down/up). */
bool        zcsr_input_is_down(const zcsr_input*, zcsr_keycode);

#endif /* ZCSR_INPUT_H */
