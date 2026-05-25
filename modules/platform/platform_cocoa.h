#ifndef ZCSR_PLATFORM_COCOA_H
#define ZCSR_PLATFORM_COCOA_H
/* Thin C-ABI shim over the Cocoa (NSWindow) backend. Implemented in platform_cocoa.m.
 * Pure C signatures so platform.c stays C and no Objective-C leaks across the boundary.
 * Same-module header (no cross-module/relative include) — zero-coupling preserved. */

/* Create a borderless, transparent, always-on-top NSWindow; returns a retained NSWindow* as void*,
 * or NULL on failure. */
void* zcsr_cocoa_window_create(const char* title, int x, int y, int w, int h);
void  zcsr_cocoa_window_destroy(void* ns_window);
/* Drain pending Cocoa events for the app (non-blocking). */
void  zcsr_cocoa_window_pump(void* ns_window);

#endif /* ZCSR_PLATFORM_COCOA_H */
