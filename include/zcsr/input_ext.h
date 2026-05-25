#ifndef ZCSR_INPUT_EXT_H
#define ZCSR_INPUT_EXT_H
/* Engine-ext Agent 3 — mouse + gamepad input. ADDITIVE to zcsr/input.h (keyboard there is
 * unchanged). Implementer: modules/inputext. Pumped from the platform surface once per frame.
 * Backend: Win RawInput+XInput / Linux X11+evdev / macOS IOKit HID + GameController.
 * No runtime heap: state lives in a caller buffer. */
#include "platform.h"  /* zcsr_surface */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* ---- Mouse ---- */
typedef enum { ZCSR_MOUSE_LEFT = 1u, ZCSR_MOUSE_RIGHT = 2u, ZCSR_MOUSE_MIDDLE = 4u } zcsr_mouse_button;
typedef struct {
    int     x, y;     /* position in window (top-left origin) */
    int     dx, dy;   /* delta since last pump */
    int     wheel;    /* wheel ticks accumulated since last pump */
    uint8_t buttons;  /* OR of zcsr_mouse_button currently held */
} zcsr_mouse_state;

/* ---- Gamepad (up to 2, Xbox layout) ---- */
enum { ZCSR_PAD_MAX = 2 };
typedef enum {
    ZCSR_PAD_A, ZCSR_PAD_B, ZCSR_PAD_X, ZCSR_PAD_Y,
    ZCSR_PAD_LB, ZCSR_PAD_RB, ZCSR_PAD_BACK, ZCSR_PAD_START,
    ZCSR_PAD_LSTICK, ZCSR_PAD_RSTICK,
    ZCSR_PAD_DUP, ZCSR_PAD_DDOWN, ZCSR_PAD_DLEFT, ZCSR_PAD_DRIGHT,
    ZCSR_PAD_BUTTON_COUNT
} zcsr_pad_button;
typedef struct {
    bool     connected;
    uint16_t buttons;       /* bitmask: (1u << zcsr_pad_button) */
    float    lx, ly, rx, ry;/* sticks, -1..1 */
    float    lt, rt;        /* triggers, 0..1 */
} zcsr_pad_state;

typedef struct zcsr_input_ext zcsr_input_ext; /* opaque; mouse + gamepad state */

zcsr_input_ext*  zcsr_input_ext_init(void* buffer, size_t bytes);     /* NULL if buffer too small */
void             zcsr_input_ext_pump(zcsr_input_ext*, zcsr_surface*); /* call once per frame */
zcsr_mouse_state zcsr_input_ext_mouse(const zcsr_input_ext*);
zcsr_pad_state   zcsr_input_ext_pad(const zcsr_input_ext*, int index);/* index 0..ZCSR_PAD_MAX-1 */

#endif /* ZCSR_INPUT_EXT_H */
