#ifndef ZCSR_INPUT_COCOA_H
#define ZCSR_INPUT_COCOA_H
/* Thin C-ABI shim over Cocoa NSEvent key pumping. Implemented in input_cocoa.m.
 * Same-module header; pure C (no Objective-C, no cross-module include). */
#include <stdint.h>

/* Called once per key event drained from the Cocoa queue. keycode = native NSEvent.keyCode
 * (platform-native, same convention as the X11/Win32 backends). is_down: 1 = down, 0 = up. */
typedef void (*zcsr_cocoa_key_cb)(void* ctx, uint16_t keycode, int is_down);

/* Non-blocking: drain key-down/key-up events for the app, invoking cb per event.
 * Sets *out_close = 1 if the window is no longer visible (treated as a close request). */
void zcsr_cocoa_input_pump(void* ns_window, zcsr_cocoa_key_cb cb, void* ctx, int* out_close);

#endif /* ZCSR_INPUT_COCOA_H */
