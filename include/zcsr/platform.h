#ifndef ZCSR_PLATFORM_H
#define ZCSR_PLATFORM_H
/* Agent 5 — Platform abstraction. Win32 / X11 / Cocoa, auto-selected at compile time.
 * NO OpenGL/DirectX/Vulkan. Exactly one backend is selected by the gates below.
 * Implementer: modules/platform. */

typedef enum {
    ZCSR_PLATFORM_UNKNOWN = 0,
    ZCSR_PLATFORM_WINDOWS,
    ZCSR_PLATFORM_LINUX,
    ZCSR_PLATFORM_MACOS
} zcsr_platform;

#if defined(_WIN32)
#  define ZCSR_PLATFORM ZCSR_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#  define ZCSR_PLATFORM ZCSR_PLATFORM_MACOS
#elif defined(__linux__)
#  define ZCSR_PLATFORM ZCSR_PLATFORM_LINUX
#else
#  define ZCSR_PLATFORM ZCSR_PLATFORM_UNKNOWN
#endif

typedef struct { int x, y, w, h; } zcsr_rect;

typedef struct zcsr_surface zcsr_surface; /* opaque native top-level surface */

/* Borderless, transparent, always-on-top is requested at create(). NULL on failure. */
zcsr_surface* zcsr_surface_create(const char* title, zcsr_rect bounds);
void          zcsr_surface_destroy(zcsr_surface*);
void*         zcsr_surface_native_handle(const zcsr_surface*); /* HWND / Window / NSWindow* */
void          zcsr_surface_pump_events(zcsr_surface*);

#endif /* ZCSR_PLATFORM_H */
