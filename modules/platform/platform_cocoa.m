/* Agent 5 — Cocoa (macOS) backend for the platform surface. Objective-C, compiled only on Apple.
 * Implements the C-ABI shim declared in platform_cocoa.h. Includes ONLY the same-module shim
 * header and the system Cocoa framework — no cross-module includes (zero-coupling preserved).
 *
 * NOTE: written on a Linux host without an Apple toolchain — NOT compiled/run here. Needs a Mac
 * (clang + Cocoa framework) to build and verify. */
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "platform_cocoa.h"

/* Ensure the process has an NSApplication so a window can appear from a plain C entry point. */
static void zcsr_cocoa_ensure_app(void) {
    static BOOL initialized = NO;
    if (initialized) return;
    initialized = YES;
    [NSApplication sharedApplication];
    /* Accessory: shows windows without a Dock icon / menu bar takeover — fits an overlay. */
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    [NSApp finishLaunching];
}

void* zcsr_cocoa_window_create(const char* title, int x, int y, int w, int h) {
    @autoreleasepool {
        zcsr_cocoa_ensure_app();

        NSRect frame = NSMakeRect((CGFloat)x, (CGFloat)y, (CGFloat)w, (CGFloat)h);
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:frame
                      styleMask:NSWindowStyleMaskBorderless
                        backing:NSBackingStoreBuffered
                          defer:NO];
        if (!window) return NULL;

        [window setTitle:[NSString stringWithUTF8String:(title ? title : "zcsr")]];
        [window setOpaque:NO];
        [window setBackgroundColor:[NSColor clearColor]];
        [window setLevel:NSStatusWindowLevel];   /* always-on-top */
        [window setIgnoresMouseEvents:NO];
        [window makeKeyAndOrderFront:nil];

        /* Returned as an owning reference; balanced by zcsr_cocoa_window_destroy. */
        return (void*)CFBridgingRetain(window);
    }
}

void zcsr_cocoa_window_destroy(void* ns_window) {
    if (!ns_window) return;
    @autoreleasepool {
        NSWindow* window = (NSWindow*)CFBridgingRelease(ns_window);
        [window orderOut:nil];
        [window close];
    }
}

void zcsr_cocoa_window_pump(void* ns_window) {
    (void)ns_window;
    @autoreleasepool {
        NSEvent* event;
        /* Non-blocking drain of the app event queue. */
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:[NSDate distantPast]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES]) != nil) {
            [NSApp sendEvent:event];
        }
    }
}

#endif /* __APPLE__ */
