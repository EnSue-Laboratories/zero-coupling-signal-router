/* Agent 5 — Platform abstraction. Win32 / X11 / Cocoa, gated on ZCSR_PLATFORM (zcsr/platform.h).
 * No OpenGL/DirectX/Vulkan. */
#include "zcsr/platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct zcsr_surface {
    bool in_use;
#if defined(_WIN32)
    void* hwnd;
#elif defined(__linux__)
    void* display;
    unsigned long window;
#elif defined(__APPLE__)
    void* ns_window;
#else
    void* handle;
#endif
};

enum { ZCSR_SURFACE_POOL = 4 };
static zcsr_surface zcsr_surfaces[ZCSR_SURFACE_POOL];

static zcsr_surface* zcsr_surface_acquire(void) {
    for (size_t i = 0; i < ZCSR_SURFACE_POOL; ++i) {
        if (!zcsr_surfaces[i].in_use) {
            zcsr_surfaces[i].in_use = true;
            return &zcsr_surfaces[i];
        }
    }
    return 0;
}

static void zcsr_surface_release(zcsr_surface* s) {
    if (!s) return;
    *s = (zcsr_surface){ 0 };
}

#if defined(__linux__)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static void zcsr_set_atom(Display* display, Window window, const char* property, const char* atom_name) {
    Atom prop = XInternAtom(display, property, False);
    Atom atom = XInternAtom(display, atom_name, False);
    XChangeProperty(display, window, prop, XA_ATOM, 32, PropModeReplace, (unsigned char*)&atom, 1);
}

zcsr_surface* zcsr_surface_create(const char* title, zcsr_rect bounds) {
    Display* display = XOpenDisplay(0);
    int screen;
    Window root;
    XVisualInfo visual_info;
    Visual* visual;
    int depth;
    Colormap colormap;
    XSetWindowAttributes attrs;
    unsigned long mask;
    Window window;
    zcsr_surface* surface;

    if (!display) return 0;
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    if (XMatchVisualInfo(display, screen, 32, TrueColor, &visual_info)) {
        visual = visual_info.visual;
        depth = visual_info.depth;
        colormap = XCreateColormap(display, root, visual, AllocNone);
    } else {
        visual = DefaultVisual(display, screen);
        depth = DefaultDepth(display, screen);
        colormap = DefaultColormap(display, screen);
    }

    attrs.override_redirect = True;
    attrs.colormap = colormap;
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    mask = CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel;

    if (bounds.w <= 0) bounds.w = 320;
    if (bounds.h <= 0) bounds.h = 120;

    window = XCreateWindow(display, root, bounds.x, bounds.y, (unsigned int)bounds.w, (unsigned int)bounds.h,
                           0, depth, InputOutput, visual, mask, &attrs);
    if (!window) {
        XCloseDisplay(display);
        return 0;
    }

    XStoreName(display, window, title ? title : "zcsr");
    zcsr_set_atom(display, window, "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");
    zcsr_set_atom(display, window, "_NET_WM_STATE", "_NET_WM_STATE_ABOVE");
    XMapRaised(display, window);
    XFlush(display);

    surface = zcsr_surface_acquire();
    if (!surface) {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        return 0;
    }

    surface->display = display;
    surface->window = window;
    return surface;
}

void zcsr_surface_destroy(zcsr_surface* s) {
    if (!s || !s->in_use) return;
    if (s->display && s->window) {
        XDestroyWindow((Display*)s->display, (Window)s->window);
        XCloseDisplay((Display*)s->display);
    }
    zcsr_surface_release(s);
}

void* zcsr_surface_native_handle(const zcsr_surface* s) {
    return (s && s->in_use) ? (void*)(uintptr_t)s->window : 0;
}

void* zcsr_surface_native_display(const zcsr_surface* s) {
    return (s && s->in_use) ? s->display : 0; /* the X11 Display*, reusable by renderers */
}

void zcsr_surface_pump_events(zcsr_surface* s) {
    Display* display;
    if (!s || !s->in_use || !s->display) return;
    display = (Display*)s->display;
    while (XPending(display) > 0) {
        XEvent event;
        XNextEvent(display, &event);
    }
}

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>

static LRESULT CALLBACK zcsr_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CLOSE) {
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static ATOM zcsr_window_class(void) {
    static ATOM atom = 0;
    if (!atom) {
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = zcsr_wndproc;
        wc.hInstance = GetModuleHandleA(0);
        wc.lpszClassName = "zcsr_overlay_surface";
        atom = RegisterClassA(&wc);
    }
    return atom;
}

zcsr_surface* zcsr_surface_create(const char* title, zcsr_rect bounds) {
    HWND hwnd;
    zcsr_surface* surface;
    if (!zcsr_window_class()) return 0;
    if (bounds.w <= 0) bounds.w = 320;
    if (bounds.h <= 0) bounds.h = 120;
    hwnd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                           "zcsr_overlay_surface", title ? title : "zcsr", WS_POPUP,
                           bounds.x, bounds.y, bounds.w, bounds.h,
                           0, 0, GetModuleHandleA(0), 0);
    if (!hwnd) return 0;
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    surface = zcsr_surface_acquire();
    if (!surface) {
        DestroyWindow(hwnd);
        return 0;
    }
    surface->hwnd = hwnd;
    return surface;
}

void zcsr_surface_destroy(zcsr_surface* s) {
    if (!s || !s->in_use) return;
    if (s->hwnd) DestroyWindow((HWND)s->hwnd);
    zcsr_surface_release(s);
}

void* zcsr_surface_native_handle(const zcsr_surface* s) {
    return (s && s->in_use) ? s->hwnd : 0;
}

void* zcsr_surface_native_display(const zcsr_surface* s) {
    (void)s; return 0; /* Win32 has no separate display connection; the HWND suffices */
}

void zcsr_surface_pump_events(zcsr_surface* s) {
    MSG msg;
    (void)s;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

#elif defined(__APPLE__)
/* Cocoa lives in platform_cocoa.m (Objective-C). This C side keeps the shared pool/struct and
 * delegates the NSWindow work through a thin C-ABI shim — no Objective-C leaks into the contract. */
#include "platform_cocoa.h"

zcsr_surface* zcsr_surface_create(const char* title, zcsr_rect bounds) {
    void* window;
    zcsr_surface* surface;
    if (bounds.w <= 0) bounds.w = 320;
    if (bounds.h <= 0) bounds.h = 120;
    window = zcsr_cocoa_window_create(title ? title : "zcsr", bounds.x, bounds.y, bounds.w, bounds.h);
    if (!window) return 0;
    surface = zcsr_surface_acquire();
    if (!surface) {
        zcsr_cocoa_window_destroy(window);
        return 0;
    }
    surface->ns_window = window;
    return surface;
}

void zcsr_surface_destroy(zcsr_surface* s) {
    if (!s || !s->in_use) return;
    if (s->ns_window) zcsr_cocoa_window_destroy(s->ns_window);
    zcsr_surface_release(s);
}

void* zcsr_surface_native_handle(const zcsr_surface* s) {
    return (s && s->in_use) ? s->ns_window : 0; /* the NSWindow* */
}

void* zcsr_surface_native_display(const zcsr_surface* s) {
    (void)s; return 0; /* Cocoa has no separate display connection; the NSWindow* suffices */
}

void zcsr_surface_pump_events(zcsr_surface* s) {
    if (s && s->in_use && s->ns_window) zcsr_cocoa_window_pump(s->ns_window);
}

#else
zcsr_surface* zcsr_surface_create(const char* title, zcsr_rect bounds) { (void)title; (void)bounds; return 0; }
void          zcsr_surface_destroy(zcsr_surface* s)                    { zcsr_surface_release(s); }
void*         zcsr_surface_native_handle(const zcsr_surface* s)        { (void)s; return 0; }
void*         zcsr_surface_native_display(const zcsr_surface* s)       { (void)s; return 0; }
void          zcsr_surface_pump_events(zcsr_surface* s)                { (void)s; }
#endif
