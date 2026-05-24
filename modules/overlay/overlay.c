/* Agent 4 — Native overlay window. Renders ONLY text, one bitmap, up to 3 buttons.
 * GDI/X11/Cocoa only (no GL/DX/Vulkan). Target: single frame render < 1ms. */
#include "zcsr/overlay.h"

#include <stddef.h>
#include <stdint.h>

enum { ZCSR_OVERLAY_POOL = 4, ZCSR_OVERLAY_TEXT_MAX = 255, ZCSR_OVERLAY_BUTTON_MAX = 3 };

struct zcsr_overlay {
    bool in_use;
    zcsr_surface* surface;
    char text[ZCSR_OVERLAY_TEXT_MAX + 1];
    const unsigned char* bitmap;
    int bitmap_w;
    int bitmap_h;
    zcsr_button buttons[ZCSR_OVERLAY_BUTTON_MAX];
    int button_count;
    int hover_index;
};

static zcsr_overlay zcsr_overlays[ZCSR_OVERLAY_POOL];

static zcsr_overlay* zcsr_overlay_acquire(void) {
    for (size_t i = 0; i < ZCSR_OVERLAY_POOL; ++i) {
        if (!zcsr_overlays[i].in_use) {
            zcsr_overlays[i] = (zcsr_overlay){ 0 };
            zcsr_overlays[i].in_use = true;
            zcsr_overlays[i].hover_index = -1;
            return &zcsr_overlays[i];
        }
    }
    return 0;
}

static void zcsr_copy_text(char* dst, size_t cap, const char* src) {
    size_t i = 0;
    if (!dst || cap == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    for (; i + 1u < cap && src[i] != '\0'; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static bool zcsr_rect_contains(zcsr_rect r, int x, int y) {
    return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
}

static size_t zcsr_overlay_text_len(const char* text);

zcsr_overlay* zcsr_overlay_create(zcsr_surface* s) {
    zcsr_overlay* overlay = zcsr_overlay_acquire();
    if (!overlay) return 0;
    overlay->surface = s;
    return overlay;
}

void zcsr_overlay_destroy(zcsr_overlay* o) {
    if (!o) return;
    *o = (zcsr_overlay){ 0 };
}

void zcsr_overlay_set_text(zcsr_overlay* o, const char* t) {
    if (!o || !o->in_use) return;
    zcsr_copy_text(o->text, sizeof o->text, t);
}

void zcsr_overlay_set_bitmap(zcsr_overlay* o, const unsigned char* d, int w, int h) {
    if (!o || !o->in_use) return;
    o->bitmap = (d && w > 0 && h > 0) ? d : 0;
    o->bitmap_w = o->bitmap ? w : 0;
    o->bitmap_h = o->bitmap ? h : 0;
}

void zcsr_overlay_set_buttons(zcsr_overlay* o, const zcsr_button* b, int c) {
    if (!o || !o->in_use) return;
    if (!b || c <= 0) {
        o->button_count = 0;
        return;
    }
    if (c > ZCSR_OVERLAY_BUTTON_MAX) c = ZCSR_OVERLAY_BUTTON_MAX;
    for (int i = 0; i < c; ++i) o->buttons[i] = b[i];
    o->button_count = c;
    if (o->hover_index >= c) o->hover_index = -1;
}

#if defined(__linux__)
#include <X11/Xlib.h>

static void zcsr_overlay_draw_x11(zcsr_overlay* o, Window window) {
    Display* display = XOpenDisplay(0);
    XWindowAttributes attrs;
    GC gc;

    if (!display) return;
    if (!XGetWindowAttributes(display, window, &attrs)) {
        XCloseDisplay(display);
        return;
    }

    gc = XCreateGC(display, window, 0, 0);
    if (!gc) {
        XCloseDisplay(display);
        return;
    }

    XSetForeground(display, gc, 0x202020u);
    XFillRectangle(display, window, gc, 0, 0, (unsigned int)attrs.width, (unsigned int)attrs.height);
    XSetForeground(display, gc, 0xF5F5F5u);
    XDrawString(display, window, gc, 12, 24, o->text, (int)zcsr_overlay_text_len(o->text));

    if (o->bitmap) {
        int max_w = o->bitmap_w > 64 ? 64 : o->bitmap_w;
        int max_h = o->bitmap_h > 64 ? 64 : o->bitmap_h;
        for (int y = 0; y < max_h; ++y) {
            for (int x = 0; x < max_w; ++x) {
                const unsigned char* px = o->bitmap + ((y * o->bitmap_w + x) * 4);
                if (px[3] == 0) continue;
                XSetForeground(display, gc, ((unsigned long)px[0] << 16) | ((unsigned long)px[1] << 8) | px[2]);
                XDrawPoint(display, window, gc, 12 + x, 36 + y);
            }
        }
    }

    for (int i = 0; i < o->button_count; ++i) {
        zcsr_button* button = &o->buttons[i];
        XSetForeground(display, gc, i == o->hover_index ? 0x5FAF92u : 0x3F7F6Au);
        XFillRectangle(display, window, gc, button->bounds.x, button->bounds.y,
                       (unsigned int)button->bounds.w, (unsigned int)button->bounds.h);
        XSetForeground(display, gc, 0xFFFFFFu);
        if (button->label) {
            XDrawString(display, window, gc, button->bounds.x + 6, button->bounds.y + 18,
                        button->label, (int)zcsr_overlay_text_len(button->label));
        }
    }

    XFlush(display);
    XFreeGC(display, gc);
    XCloseDisplay(display);
}

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void zcsr_overlay_draw_win32(zcsr_overlay* o, HWND hwnd) {
    HDC dc = GetDC(hwnd);
    RECT rect;
    HBRUSH bg;
    HBRUSH button_brush;
    if (!dc) return;
    GetClientRect(hwnd, &rect);
    bg = CreateSolidBrush(RGB(32, 32, 32));
    FillRect(dc, &rect, bg);
    DeleteObject(bg);
    SetTextColor(dc, RGB(245, 245, 245));
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, 12, 10, o->text, (int)zcsr_overlay_text_len(o->text));
    if (o->bitmap) {
        int max_w = o->bitmap_w > 64 ? 64 : o->bitmap_w;
        int max_h = o->bitmap_h > 64 ? 64 : o->bitmap_h;
        for (int y = 0; y < max_h; ++y) {
            for (int x = 0; x < max_w; ++x) {
                const unsigned char* px = o->bitmap + ((y * o->bitmap_w + x) * 4);
                if (px[3] != 0) SetPixel(dc, 12 + x, 36 + y, RGB(px[0], px[1], px[2]));
            }
        }
    }
    button_brush = CreateSolidBrush(RGB(63, 127, 106));
    for (int i = 0; i < o->button_count; ++i) {
        RECT br = { o->buttons[i].bounds.x, o->buttons[i].bounds.y,
                    o->buttons[i].bounds.x + o->buttons[i].bounds.w,
                    o->buttons[i].bounds.y + o->buttons[i].bounds.h };
        FillRect(dc, &br, button_brush);
        if (o->buttons[i].label) {
            TextOutA(dc, o->buttons[i].bounds.x + 6, o->buttons[i].bounds.y + 5,
                     o->buttons[i].label, (int)zcsr_overlay_text_len(o->buttons[i].label));
        }
    }
    DeleteObject(button_brush);
    ReleaseDC(hwnd, dc);
}
#endif

static size_t zcsr_overlay_text_len(const char* text) {
    size_t n = 0;
    if (!text) return 0;
    while (text[n] != '\0') ++n;
    return n;
}

void zcsr_overlay_render(zcsr_overlay* o) {
    void* handle;
    if (!o || !o->in_use || !o->surface) return;
    handle = zcsr_surface_native_handle(o->surface);
    if (!handle) return;
#if defined(__linux__)
    zcsr_overlay_draw_x11(o, (Window)(uintptr_t)handle);
#elif defined(_WIN32)
    zcsr_overlay_draw_win32(o, (HWND)handle);
#else
    (void)handle;
#endif
}

bool zcsr_overlay_on_pointer(zcsr_overlay* o, int x, int y, bool clk) {
    if (!o || !o->in_use) return false;
    o->hover_index = -1;
    for (int i = 0; i < o->button_count; ++i) {
        if (zcsr_rect_contains(o->buttons[i].bounds, x, y)) {
            o->hover_index = i;
            return clk;
        }
    }
    return false;
}
