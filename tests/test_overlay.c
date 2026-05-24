#include "zcsr/overlay.h"

static int fail_if(int condition) {
    return condition ? 1 : 0;
}

int main(void) {
    static const unsigned char rgba[4 * 4 * 4] = {
        255, 0, 0, 255,   0, 255, 0, 255,   0, 0, 255, 255,   255, 255, 255, 255,
        255, 0, 0, 128,   0, 255, 0, 128,   0, 0, 255, 128,   255, 255, 255, 128,
        255, 0, 0, 64,    0, 255, 0, 64,    0, 0, 255, 64,    255, 255, 255, 64,
        255, 0, 0, 0,     0, 255, 0, 0,     0, 0, 255, 0,     255, 255, 255, 0,
    };
    zcsr_button buttons[4] = {
        { "A", { 10, 10, 40, 20 }, "a" },
        { "B", { 60, 10, 40, 20 }, "b" },
        { "C", { 110, 10, 40, 20 }, "c" },
        { "D", { 160, 10, 40, 20 }, "d" },
    };
    zcsr_overlay* overlay = zcsr_overlay_create(0);

    if (fail_if(!overlay)) return 1;
    zcsr_overlay_set_text(overlay, "Potion");
    zcsr_overlay_set_bitmap(overlay, rgba, 4, 4);
    zcsr_overlay_set_buttons(overlay, buttons, 4);
    zcsr_overlay_render(overlay);

    if (fail_if(!zcsr_overlay_on_pointer(overlay, 12, 12, true))) return 2;
    if (fail_if(zcsr_overlay_on_pointer(overlay, 160, 12, true))) return 3;
    if (fail_if(zcsr_overlay_on_pointer(overlay, 220, 80, true))) return 4;
    if (fail_if(zcsr_overlay_on_pointer(overlay, 12, 12, false))) return 5;

    zcsr_overlay_destroy(overlay);
    return 0;
}
