/* Agent 5 — Platform abstraction. Win32 / X11 / Cocoa, gated on ZCSR_PLATFORM (zcsr/platform.h).
 * No OpenGL/DirectX/Vulkan. This stub keeps the module buildable on any host; the real backend
 * is compiled in per ZCSR_PLATFORM. */
#include "zcsr/platform.h"

/* TODO(Agent 5 / Codex): implement zcsr_surface per backend, gated on ZCSR_PLATFORM:
 *   Windows -> Win32 (CreateWindowExW, WS_EX_LAYERED|WS_EX_TOPMOST, borderless)
 *   Linux   -> X11   (override-redirect, _NET_WM_STATE_ABOVE, 32-bit ARGB visual)
 *   macOS   -> Cocoa (borderless NSWindow, NSStatusWindowLevel, transparent) */

zcsr_surface* zcsr_surface_create(const char* title, zcsr_rect bounds) { (void)title; (void)bounds; return 0; }
void          zcsr_surface_destroy(zcsr_surface* s)                    { (void)s; }
void*         zcsr_surface_native_handle(const zcsr_surface* s)        { (void)s; return 0; }
void          zcsr_surface_pump_events(zcsr_surface* s)                { (void)s; }
