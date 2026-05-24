#ifndef ZCSR_PLATFORM_H
#define ZCSR_PLATFORM_H
// Agent 5 — Platform abstraction. Win32 / X11 / Cocoa, auto-selected at compile time.
// NO OpenGL/DirectX/Vulkan. Exactly one backend is selected by the gates below.
// Implementer: modules/platform.

namespace zcsr {

enum class Platform { Unknown, Windows, Linux, MacOS };

#if defined(_WIN32)
constexpr Platform kPlatform = Platform::Windows;
#elif defined(__APPLE__)
constexpr Platform kPlatform = Platform::MacOS;
#elif defined(__linux__)
constexpr Platform kPlatform = Platform::Linux;
#else
constexpr Platform kPlatform = Platform::Unknown;
#endif

struct Rect { int x, y, w, h; };

// A native top-level surface the Overlay draws onto (no engine subwindow).
// Borderless, transparent, always-on-top is requested at create().
class INativeSurface {
public:
    virtual ~INativeSurface() = default;
    virtual bool  create(const char* title, Rect bounds) = 0;
    virtual void  destroy() = 0;
    virtual void* nativeHandle() const = 0; // HWND / Window / NSWindow*
    virtual void  pumpEvents() = 0;
};

} // namespace zcsr
#endif // ZCSR_PLATFORM_H
