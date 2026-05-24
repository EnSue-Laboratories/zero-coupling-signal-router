#ifndef ZCSR_OVERLAY_H
#define ZCSR_OVERLAY_H
// Agent 4 — Native overlay window. Borderless, transparent, always-on-top.
// Renders ONLY: text, one bitmap image, up to 3 buttons. GDI/X11/Cocoa only. Target <1ms/frame.
// Implementer: modules/overlay.
#include "platform.h"
#include "signal.h"

namespace zcsr {

struct Button {
    const char* label;
    Rect        bounds;
    const char* clickSignal; // signal tag emitted on click
};

class IOverlay {
public:
    virtual ~IOverlay() = default;
    virtual bool init(INativeSurface* surface) = 0;
    virtual void setText(const char* utf8) = 0;
    virtual void setBitmap(const unsigned char* rgba, int w, int h) = 0;
    virtual void setButtons(const Button* buttons, int count) = 0; // count <= 3
    virtual void render() = 0;                                     // single frame, target <1ms
    // Hover shows item info; click returns bool only. Emits the button's clickSignal on click.
    virtual bool onPointer(int x, int y, bool clicked) = 0;
};

} // namespace zcsr
#endif // ZCSR_OVERLAY_H
