/* Agent 1 — Cocoa (macOS) key-event pump. Objective-C, compiled only on Apple.
 * Implements the C-ABI shim in input_cocoa.h. Includes only the same-module shim header and the
 * system Cocoa framework — no cross-module includes (zero-coupling preserved).
 *
 * Keycodes are passed through as the native NSEvent.keyCode, matching the X11/Win32 backends
 * (the contract leaves cross-platform normalization to the caller).
 *
 * NOTE: written without an Apple toolchain — NOT compiled here. Needs a Mac to build/verify.
 * Event-pump ownership: zcsr_surface_pump_events (platform) drains all NON-key events and LEAVES
 * key down/up in the queue, which this function consumes — so a typical "surface pump then input
 * pump" loop delivers keys correctly. */
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "input_cocoa.h"

void zcsr_cocoa_input_pump(void* ns_window, zcsr_cocoa_key_cb cb, void* ctx, int* out_close) {
    if (out_close) *out_close = 0;
    @autoreleasepool {
        NSWindow* window = ns_window ? (__bridge NSWindow*)ns_window : nil;
        NSEvent* event;
        while ((event = [NSApp nextEventMatchingMask:(NSEventMaskKeyDown | NSEventMaskKeyUp)
                                           untilDate:[NSDate distantPast]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES]) != nil) {
            int is_down = ([event type] == NSEventTypeKeyDown) ? 1 : 0;
            if (is_down && [event isARepeat]) continue; /* edge semantics: ignore auto-repeat */
            if (cb) cb(ctx, (uint16_t)[event keyCode], is_down);
        }
        if (out_close && window && ![window isVisible]) *out_close = 1;
    }
}

#endif /* __APPLE__ */
