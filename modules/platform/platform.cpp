// Agent 5 — Platform abstraction. Win32 / X11 / Cocoa, selected at compile time (zcsr/platform.h).
// No OpenGL/DirectX/Vulkan. This stub keeps the module buildable on any host; the real backend
// is gated on kPlatform.
#include "zcsr/factories.h"

namespace zcsr {

// TODO(Agent 5 / Codex): implement INativeSurface per backend, gated by kPlatform:
//   Windows -> Win32 (CreateWindowEx, WS_EX_LAYERED|WS_EX_TOPMOST, borderless)
//   Linux   -> X11   (override-redirect, _NET_WM_STATE_ABOVE, ARGB visual)
//   macOS   -> Cocoa (NSWindow borderless, level NSStatusWindowLevel, transparent)
INativeSurface* make_native_surface() { return nullptr; }

} // namespace zcsr
