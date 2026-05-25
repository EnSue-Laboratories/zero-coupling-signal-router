/* Engine-ext Agent 3 - universal input extension.
 * Additive mouse/gamepad state beside the existing keyboard module. Includes only contracts plus
 * native backend headers. State and queues live entirely in the caller buffer.
 */
#include "zcsr/input_ext.h"

#include <stdint.h>
#include <string.h>

struct zcsr_input_ext {
    zcsr_mouse_state mouse;
    zcsr_pad_state pads[ZCSR_PAD_MAX];
    zcsr_surface* surface;
    bool initialized;
#if defined(__linux__)
    int gamepad_fd[ZCSR_PAD_MAX];
#endif
};

#if defined(__linux__)
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <X11/Xlib.h>

#ifndef BTN_SOUTH
#define BTN_SOUTH 0x130
#endif
#ifndef BTN_EAST
#define BTN_EAST 0x131
#endif
#ifndef BTN_NORTH
#define BTN_NORTH 0x133
#endif
#ifndef BTN_WEST
#define BTN_WEST 0x134
#endif

static bool zcsr_bit_is_set(const unsigned long* bits, unsigned code) {
    return (bits[code / (8U * (unsigned)sizeof(unsigned long))] &
            (1UL << (code % (8U * (unsigned)sizeof(unsigned long))))) != 0;
}

static float zcsr_norm_axis(int value) {
    if (value < -32768) value = -32768;
    if (value > 32767) value = 32767;
    return value < 0 ? (float)value / 32768.0f : (float)value / 32767.0f;
}

static float zcsr_norm_trigger(int value) {
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return (float)value / 255.0f;
}

static void zcsr_set_button(zcsr_pad_state* pad, zcsr_pad_button button, int down) {
    uint16_t bit;
    if (!pad || button < 0 || button >= ZCSR_PAD_BUTTON_COUNT) return;
    bit = (uint16_t)(1u << (unsigned)button);
    if (down) pad->buttons = (uint16_t)(pad->buttons | bit);
    else pad->buttons = (uint16_t)(pad->buttons & (uint16_t)~bit);
}

static bool zcsr_try_open_pad(zcsr_input_ext* in, int slot, int index) {
    char path[32];
    int fd;
    unsigned long evbits[2] = { 0, 0 };
    unsigned long keybits[(KEY_MAX / (8 * (int)sizeof(unsigned long))) + 1] = { 0 };
    if (!in || slot < 0 || slot >= ZCSR_PAD_MAX) return false;
    (void)snprintf(path, sizeof path, "/dev/input/event%d", index);
    fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;
    if (ioctl(fd, EVIOCGBIT(0, (int)sizeof evbits), evbits) < 0 ||
        !zcsr_bit_is_set(evbits, EV_KEY) ||
        ioctl(fd, EVIOCGBIT(EV_KEY, (int)sizeof keybits), keybits) < 0 ||
        (!zcsr_bit_is_set(keybits, BTN_GAMEPAD) && !zcsr_bit_is_set(keybits, BTN_SOUTH) &&
         !zcsr_bit_is_set(keybits, BTN_EAST))) {
        close(fd);
        return false;
    }
    in->gamepad_fd[slot] = fd;
    in->pads[slot].connected = true;
    return true;
}

static void zcsr_scan_gamepads(zcsr_input_ext* in) {
    int slot = 0;
    if (!in) return;
    for (int i = 0; i < ZCSR_PAD_MAX; ++i) in->gamepad_fd[i] = -1;
    for (int event = 0; event < 32 && slot < ZCSR_PAD_MAX; ++event) {
        if (zcsr_try_open_pad(in, slot, event)) ++slot;
    }
}

static void zcsr_close_gamepads(zcsr_input_ext* in) {
    if (!in) return;
    for (int i = 0; i < ZCSR_PAD_MAX; ++i) {
        if (in->gamepad_fd[i] >= 0) close(in->gamepad_fd[i]);
        in->gamepad_fd[i] = -1;
        in->pads[i] = (zcsr_pad_state){ 0 };
    }
}

static void zcsr_apply_abs(zcsr_pad_state* pad, unsigned code, int value) {
    if (!pad) return;
    if (code == ABS_X) pad->lx = zcsr_norm_axis(value);
    else if (code == ABS_Y) pad->ly = zcsr_norm_axis(value);
    else if (code == ABS_RX) pad->rx = zcsr_norm_axis(value);
    else if (code == ABS_RY) pad->ry = zcsr_norm_axis(value);
    else if (code == ABS_Z) pad->lt = zcsr_norm_trigger(value);
    else if (code == ABS_RZ) pad->rt = zcsr_norm_trigger(value);
    else if (code == ABS_HAT0X) {
        zcsr_set_button(pad, ZCSR_PAD_DLEFT, value < 0);
        zcsr_set_button(pad, ZCSR_PAD_DRIGHT, value > 0);
    } else if (code == ABS_HAT0Y) {
        zcsr_set_button(pad, ZCSR_PAD_DUP, value < 0);
        zcsr_set_button(pad, ZCSR_PAD_DDOWN, value > 0);
    }
}

static void zcsr_apply_key(zcsr_pad_state* pad, unsigned code, int value) {
    int down = value != 0;
    if (!pad) return;
    if (code == BTN_SOUTH) zcsr_set_button(pad, ZCSR_PAD_A, down);
    else if (code == BTN_EAST) zcsr_set_button(pad, ZCSR_PAD_B, down);
    else if (code == BTN_WEST) zcsr_set_button(pad, ZCSR_PAD_X, down);
    else if (code == BTN_NORTH) zcsr_set_button(pad, ZCSR_PAD_Y, down);
    else if (code == BTN_TL) zcsr_set_button(pad, ZCSR_PAD_LB, down);
    else if (code == BTN_TR) zcsr_set_button(pad, ZCSR_PAD_RB, down);
    else if (code == BTN_SELECT) zcsr_set_button(pad, ZCSR_PAD_BACK, down);
    else if (code == BTN_START) zcsr_set_button(pad, ZCSR_PAD_START, down);
    else if (code == BTN_THUMBL) zcsr_set_button(pad, ZCSR_PAD_LSTICK, down);
    else if (code == BTN_THUMBR) zcsr_set_button(pad, ZCSR_PAD_RSTICK, down);
}

static void zcsr_pump_gamepads(zcsr_input_ext* in) {
    if (!in) return;
    for (int i = 0; i < ZCSR_PAD_MAX; ++i) {
        int fd = in->gamepad_fd[i];
        if (fd < 0) continue;
        for (;;) {
            struct input_event ev;
            ssize_t n = read(fd, &ev, sizeof ev);
            if (n == (ssize_t)sizeof ev) {
                if (ev.type == EV_ABS) zcsr_apply_abs(&in->pads[i], ev.code, ev.value);
                else if (ev.type == EV_KEY) zcsr_apply_key(&in->pads[i], ev.code, ev.value);
            } else {
                if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    close(fd);
                    in->gamepad_fd[i] = -1;
                    in->pads[i] = (zcsr_pad_state){ 0 };
                }
                break;
            }
        }
    }
}

static void zcsr_select_mouse_events(zcsr_surface* window) {
    Display* display = (Display*)zcsr_surface_native_display(window);
    Window xwindow = (Window)(uintptr_t)zcsr_surface_native_handle(window);
    XWindowAttributes attrs;
    long mask;
    if (!display || !xwindow || !XGetWindowAttributes(display, xwindow, &attrs)) return;
    mask = attrs.your_event_mask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
           KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;
    XSelectInput(display, xwindow, mask);
}

static void zcsr_pump_mouse(zcsr_input_ext* in, zcsr_surface* window) {
    Display* display = (Display*)zcsr_surface_native_display(window);
    Window xwindow = (Window)(uintptr_t)zcsr_surface_native_handle(window);
    XEvent ev;
    if (!in || !display || !xwindow) return;
    in->mouse.dx = 0;
    in->mouse.dy = 0;
    in->mouse.wheel = 0;
    while (XCheckTypedWindowEvent(display, xwindow, MotionNotify, &ev)) {
        int nx = ev.xmotion.x;
        int ny = ev.xmotion.y;
        in->mouse.dx += nx - in->mouse.x;
        in->mouse.dy += ny - in->mouse.y;
        in->mouse.x = nx;
        in->mouse.y = ny;
    }
    while (XCheckTypedWindowEvent(display, xwindow, ButtonPress, &ev)) {
        if (ev.xbutton.button == Button1) in->mouse.buttons |= ZCSR_MOUSE_LEFT;
        else if (ev.xbutton.button == Button2) in->mouse.buttons |= ZCSR_MOUSE_MIDDLE;
        else if (ev.xbutton.button == Button3) in->mouse.buttons |= ZCSR_MOUSE_RIGHT;
        else if (ev.xbutton.button == Button4) ++in->mouse.wheel;
        else if (ev.xbutton.button == Button5) --in->mouse.wheel;
        in->mouse.x = ev.xbutton.x;
        in->mouse.y = ev.xbutton.y;
    }
    while (XCheckTypedWindowEvent(display, xwindow, ButtonRelease, &ev)) {
        if (ev.xbutton.button == Button1) in->mouse.buttons &= (uint8_t)~ZCSR_MOUSE_LEFT;
        else if (ev.xbutton.button == Button2) in->mouse.buttons &= (uint8_t)~ZCSR_MOUSE_MIDDLE;
        else if (ev.xbutton.button == Button3) in->mouse.buttons &= (uint8_t)~ZCSR_MOUSE_RIGHT;
        in->mouse.x = ev.xbutton.x;
        in->mouse.y = ev.xbutton.y;
    }
}
#else
static void zcsr_scan_gamepads(zcsr_input_ext* in) { (void)in; }
static void zcsr_close_gamepads(zcsr_input_ext* in) { (void)in; }
static void zcsr_select_mouse_events(zcsr_surface* window) { (void)window; }
static void zcsr_pump_gamepads(zcsr_input_ext* in) { (void)in; }
static void zcsr_pump_mouse(zcsr_input_ext* in, zcsr_surface* window) {
    if (in) {
        in->mouse.dx = 0;
        in->mouse.dy = 0;
        in->mouse.wheel = 0;
    }
    (void)window;
}
#endif

zcsr_input_ext* zcsr_input_ext_init(void* buffer, size_t bytes) {
    zcsr_input_ext* in;
    if (!buffer || bytes < sizeof(zcsr_input_ext)) return 0;
    in = (zcsr_input_ext*)buffer;
    *in = (zcsr_input_ext){ 0 };
    zcsr_scan_gamepads(in);
    return in;
}

void zcsr_input_ext_shutdown(zcsr_input_ext* in) {
    if (!in) return;
    zcsr_close_gamepads(in);
    *in = (zcsr_input_ext){ 0 };
}

void zcsr_input_ext_pump(zcsr_input_ext* in, zcsr_surface* window) {
    if (!in) return;
    if (window && (!in->initialized || in->surface != window)) {
        zcsr_select_mouse_events(window);
        in->surface = window;
        in->initialized = true;
    }
    if (window) zcsr_pump_mouse(in, window);
    else {
        in->mouse.dx = 0;
        in->mouse.dy = 0;
        in->mouse.wheel = 0;
    }
    zcsr_pump_gamepads(in);
}

zcsr_mouse_state zcsr_input_ext_mouse(const zcsr_input_ext* in) {
    return in ? in->mouse : (zcsr_mouse_state){ 0, 0, 0, 0, 0, 0 };
}

zcsr_pad_state zcsr_input_ext_pad(const zcsr_input_ext* in, int index) {
    if (!in || index < 0 || index >= ZCSR_PAD_MAX) return (zcsr_pad_state){ 0 };
    return in->pads[index];
}
