#ifndef ZCSR_OVERLAY_COCOA_H
#define ZCSR_OVERLAY_COCOA_H
/* Thin C-ABI shim over the Cocoa overlay draw backend. Implemented in overlay_cocoa.m.
 * Same-module header; includes only the shared contract for the zcsr_button type. */
#include "zcsr/overlay.h"

/* Draw one overlay frame onto the NSWindow content view: dark background, text, an optional
 * RGBA bitmap (top-left), and up to 3 buttons (highlighting hover_index). */
void zcsr_cocoa_overlay_draw(void* ns_window, const char* text,
                             const unsigned char* rgba, int bitmap_w, int bitmap_h,
                             const zcsr_button* buttons, int button_count, int hover_index);

#endif /* ZCSR_OVERLAY_COCOA_H */
