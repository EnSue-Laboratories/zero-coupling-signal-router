#ifndef ZCSR_RENDER_COCOA_H
#define ZCSR_RENDER_COCOA_H
/* Thin C-ABI shim over the Cocoa sprite-fill backend. Implemented in render_cocoa.m.
 * Same-module header; pure C signatures (no Objective-C, no cross-module include). */

/* begin: lockFocus + clear; fill: one solid rect (zcsr top-left coords); end: unlockFocus + flush.
 * Mirrors the X11/Win32 placeholder backends (solid rect per sprite). */
void zcsr_cocoa_render_begin(void* ns_window);
void zcsr_cocoa_render_fill(void* ns_window, int x, int y, int w, int h,
                            unsigned char r, unsigned char g, unsigned char b);
void zcsr_cocoa_render_end(void* ns_window);

#endif /* ZCSR_RENDER_COCOA_H */
