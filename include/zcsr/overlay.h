#ifndef ZCSR_OVERLAY_H
#define ZCSR_OVERLAY_H
/* Agent 4 — Native overlay window. Borderless, transparent, always-on-top.
 * Renders ONLY: text, one bitmap image, up to 3 buttons. GDI/X11/Cocoa only. Target <1ms/frame.
 * Implementer: modules/overlay. */
#include "platform.h"
#include <stdbool.h>

typedef struct {
    const char* label;
    zcsr_rect   bounds;
    const char* click_signal; /* signal tag emitted on click */
} zcsr_button;

typedef struct zcsr_overlay zcsr_overlay; /* opaque */

zcsr_overlay* zcsr_overlay_create(zcsr_surface*);
void          zcsr_overlay_destroy(zcsr_overlay*);
void          zcsr_overlay_set_text(zcsr_overlay*, const char* utf8);
void          zcsr_overlay_set_bitmap(zcsr_overlay*, const unsigned char* rgba, int w, int h);
void          zcsr_overlay_set_buttons(zcsr_overlay*, const zcsr_button* buttons, int count); /* <=3 */
void          zcsr_overlay_render(zcsr_overlay*); /* single frame, target <1ms */
/* Hover shows item info; click returns bool only. Emits the button's click_signal on click. */
bool          zcsr_overlay_on_pointer(zcsr_overlay*, int x, int y, bool clicked);

#endif /* ZCSR_OVERLAY_H */
