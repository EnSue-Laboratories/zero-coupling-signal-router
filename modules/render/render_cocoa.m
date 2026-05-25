/* Agent 3 — Cocoa (macOS) sprite-fill backend. Objective-C, compiled only on Apple.
 * Implements the C-ABI shim in render_cocoa.h. Includes only the same-module shim header and the
 * system Cocoa framework — no cross-module includes (zero-coupling preserved).
 *
 * Like the X11/Win32 backends this draws each sprite as a solid rect (first-pixel color) to
 * establish the batch/surface path; a real RGBA blit (CGImage/IOSurface) is the follow-up.
 * Cocoa origin is BOTTOM-left, so zcsr top-left y is flipped against the view height.
 *
 * NOTE: written without an Apple toolchain — NOT compiled here. Needs a Mac to build/verify. */
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "render_cocoa.h"

void zcsr_cocoa_render_begin(void* ns_window) {
    if (!ns_window) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)ns_window;
        NSView* view = [window contentView];
        if (!view || ![view lockFocusIfCanDraw]) return;
        [[NSColor blackColor] setFill];
        NSRectFill([view bounds]);
    }
}

void zcsr_cocoa_render_fill(void* ns_window, int x, int y, int w, int h,
                            unsigned char r, unsigned char g, unsigned char b) {
    if (!ns_window) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)ns_window;
        NSView* view = [window contentView];
        if (!view) return;
        CGFloat H = [view bounds].size.height;
        [[NSColor colorWithSRGBRed:(CGFloat)r / 255.0 green:(CGFloat)g / 255.0
                              blue:(CGFloat)b / 255.0 alpha:1.0] setFill];
        /* zcsr (x,y) top-left -> Cocoa bottom-left */
        NSRectFill(NSMakeRect((CGFloat)x, H - (CGFloat)y - (CGFloat)h, (CGFloat)w, (CGFloat)h));
    }
}

void zcsr_cocoa_render_end(void* ns_window) {
    if (!ns_window) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)ns_window;
        NSView* view = [window contentView];
        if (view) [view unlockFocus];
        [window flushWindow];
    }
}

#endif /* __APPLE__ */
