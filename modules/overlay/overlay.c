/* Agent 4 — Native overlay window. Renders ONLY text, one bitmap, up to 3 buttons.
 * GDI/X11/Cocoa only (no GL/DX/Vulkan). Target: single frame render < 1ms. */
#include "zcsr/overlay.h"

/* TODO(Agent 4 / Codex): implement zcsr_overlay
 *   - draw text + one bitmap + <=3 buttons onto the zcsr_surface (GDI/X11/Cocoa).
 *   - on_pointer: hover -> show item info; click -> emit button.click_signal, return bool.
 *   - keep render() under 1ms (measure in integration/bench). */

zcsr_overlay* zcsr_overlay_create(zcsr_surface* s)                                  { (void)s; return 0; }
void          zcsr_overlay_destroy(zcsr_overlay* o)                                 { (void)o; }
void          zcsr_overlay_set_text(zcsr_overlay* o, const char* t)                 { (void)o; (void)t; }
void          zcsr_overlay_set_bitmap(zcsr_overlay* o, const unsigned char* d, int w, int h) { (void)o; (void)d; (void)w; (void)h; }
void          zcsr_overlay_set_buttons(zcsr_overlay* o, const zcsr_button* b, int c){ (void)o; (void)b; (void)c; }
void          zcsr_overlay_render(zcsr_overlay* o)                                  { (void)o; }
bool          zcsr_overlay_on_pointer(zcsr_overlay* o, int x, int y, bool clk)      { (void)o; (void)x; (void)y; (void)clk; return false; }
