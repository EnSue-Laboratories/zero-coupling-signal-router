/* Agent 4 — Cocoa (macOS) overlay draw backend. Objective-C, compiled only on Apple.
 * Implements the C-ABI shim in overlay_cocoa.h. Includes only the same-module shim header
 * (which pulls in the zcsr/overlay.h contract) and the system Cocoa framework.
 *
 * Cocoa's content-view origin is BOTTOM-left (y up); the zcsr layout uses TOP-left (y down,
 * like X11/Win32), so y is flipped against the view height below.
 *
 * NOTE: written without an Apple toolchain — NOT compiled here. Needs a Mac to build/verify;
 * exact pixel offsets may want tuning on-device. Uses immediate lockFocus drawing to mirror the
 * X11/Win32 backends; a custom NSView+drawRect is the longer-term "proper" path (see PR notes). */
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "overlay_cocoa.h"

static NSColor* zcsr_rgb(unsigned char r, unsigned char g, unsigned char b) {
    return [NSColor colorWithSRGBRed:(CGFloat)r / 255.0 green:(CGFloat)g / 255.0
                                blue:(CGFloat)b / 255.0 alpha:1.0];
}

void zcsr_cocoa_overlay_draw(void* ns_window, const char* text,
                             const unsigned char* rgba, int bitmap_w, int bitmap_h,
                             const zcsr_button* buttons, int button_count, int hover_index) {
    if (!ns_window) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)ns_window;
        NSView* view = [window contentView];
        if (!view) return;
        NSRect bounds = [view bounds];
        CGFloat H = bounds.size.height;

        if (![view lockFocusIfCanDraw]) return;

        /* background */
        [zcsr_rgb(32, 32, 32) setFill];
        NSRectFill(bounds);

        /* text (top-left in zcsr coords -> near top in Cocoa) */
        if (text && text[0]) {
            NSDictionary* attrs = @{
                NSForegroundColorAttributeName : zcsr_rgb(245, 245, 245),
                NSFontAttributeName : [NSFont systemFontOfSize:13.0]
            };
            NSString* s = [NSString stringWithUTF8String:text];
            [s drawAtPoint:NSMakePoint(12.0, H - 24.0) withAttributes:attrs];
        }

        /* bitmap: capped to 64x64, top-left at zcsr (12,36) */
        if (rgba && bitmap_w > 0 && bitmap_h > 0) {
            int max_w = bitmap_w > 64 ? 64 : bitmap_w;
            int max_h = bitmap_h > 64 ? 64 : bitmap_h;
            for (int y = 0; y < max_h; ++y) {
                for (int x = 0; x < max_w; ++x) {
                    const unsigned char* px = rgba + ((y * bitmap_w + x) * 4);
                    if (px[3] == 0) continue;
                    [zcsr_rgb(px[0], px[1], px[2]) setFill];
                    /* 1x1 px; zcsr y=(36+y) from top -> Cocoa y=H-(36+y)-1 */
                    NSRectFill(NSMakeRect(12.0 + x, H - (36.0 + y) - 1.0, 1.0, 1.0));
                }
            }
        }

        /* buttons */
        for (int i = 0; i < button_count; ++i) {
            const zcsr_button* btn = &buttons[i];
            CGFloat bx = (CGFloat)btn->bounds.x;
            CGFloat bw = (CGFloat)btn->bounds.w;
            CGFloat bh = (CGFloat)btn->bounds.h;
            CGFloat by = H - (CGFloat)btn->bounds.y - bh; /* top-left -> Cocoa bottom-left */
            NSColor* fill = (i == hover_index) ? zcsr_rgb(95, 175, 146) : zcsr_rgb(63, 127, 106);
            [fill setFill];
            NSRectFill(NSMakeRect(bx, by, bw, bh));
            if (btn->label) {
                NSDictionary* attrs = @{
                    NSForegroundColorAttributeName : [NSColor whiteColor],
                    NSFontAttributeName : [NSFont systemFontOfSize:12.0]
                };
                NSString* label = [NSString stringWithUTF8String:btn->label];
                [label drawAtPoint:NSMakePoint(bx + 6.0, by + bh - 18.0) withAttributes:attrs];
            }
        }

        [view unlockFocus];
        [window flushWindow];
    }
}

#endif /* __APPLE__ */
