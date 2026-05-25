#ifndef ZCSR_COLOR_H
#define ZCSR_COLOR_H
/* Engine-ext — shared color-modulation type for the GPU runtime path (glrender) and the
 * offline image-processing path (image). Full-RGBA modulation only: a single RGBA color is
 * applied per `mode`; there is NO per-channel control surface (by design).
 *
 * MULTIPLY is value 0 so a zero-initialized modulation reproduces the legacy `texel * color`
 * behavior — existing sprites are unaffected. MIX/ADD are needed because a pure multiply can
 * only darken; "flash red" / "freeze blue" must lighten/tint toward a color. */

typedef enum {
    ZCSR_MOD_MULTIPLY = 0, /* texel * color                  (default; legacy behavior)        */
    ZCSR_MOD_MIX,          /* lerp(texel, color, amount)      (tint toward color: flash/freeze) */
    ZCSR_MOD_ADD           /* texel + color * amount          (additive flash / glow)           */
} zcsr_color_mode;

#endif /* ZCSR_COLOR_H */
