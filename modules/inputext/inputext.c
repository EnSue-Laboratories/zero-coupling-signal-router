/* Engine-ext Agent 3 — mouse + gamepad input. PHASE-0 SKELETON STUB.
 * Implement against zcsr/input_ext.h: pump from the platform surface; backend Win RawInput+XInput /
 * Linux X11+evdev / macOS IOKit HID + GameController. Additive to zcsr/input.h (keyboard unchanged).
 * Includes ONLY shared contracts (zero-coupling). No runtime heap (state in caller buffer). */
#include "zcsr/input_ext.h"

zcsr_input_ext* zcsr_input_ext_init(void* buffer, size_t bytes) {
    (void)buffer; (void)bytes; return 0; /* TODO: place state in buffer */
}
void zcsr_input_ext_pump(zcsr_input_ext* in, zcsr_surface* window) { (void)in; (void)window; }

zcsr_mouse_state zcsr_input_ext_mouse(const zcsr_input_ext* in) {
    (void)in; zcsr_mouse_state m = { 0, 0, 0, 0, 0, 0 }; return m;
}
zcsr_pad_state zcsr_input_ext_pad(const zcsr_input_ext* in, int index) {
    (void)in; (void)index; zcsr_pad_state p = { false, 0, 0, 0, 0, 0, 0, 0 }; return p;
}
