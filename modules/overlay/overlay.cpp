// Agent 4 — Native overlay window. Renders ONLY text, one bitmap, up to 3 buttons.
// GDI/X11/Cocoa only (no GL/DX/Vulkan). Target: single frame render < 1ms.
#include "zcsr/factories.h"

namespace zcsr {

// TODO(Agent 4 / Codex): implement IOverlay
//   - draw text + one bitmap + <=3 buttons onto the INativeSurface (GDI/X11/Cocoa).
//   - onPointer: hover -> show item info; click -> emit button.clickSignal, return bool.
//   - keep render() under 1ms (measure in integration/bench).
IOverlay* make_overlay() { return nullptr; }

} // namespace zcsr
