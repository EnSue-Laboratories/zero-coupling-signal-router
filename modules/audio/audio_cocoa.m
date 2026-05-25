/* Agent 2 — Cocoa (macOS) audio backend via NSSound. Objective-C, compiled only on Apple.
 * Implements the C-ABI shim in audio_cocoa.h. Includes only the same-module shim header and the
 * system Cocoa framework — no cross-module includes (zero-coupling preserved).
 *
 * Single-stream (one current sound), matching the Win32 PlaySound behavior. The audio module
 * calls these under its engine mutex (one command thread), so the static current-sound reference
 * is serialized. ARC-managed (CMake compiles the .m files with -fobjc-arc).
 *
 * NOTE: written without an Apple toolchain — NOT compiled here. Needs a Mac to build/verify. */
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include "audio_cocoa.h"

static NSSound* g_current_sound = nil; /* strong (ARC) */

void zcsr_cocoa_sound_play(const char* wav_path) {
    if (!wav_path) return;
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:wav_path];
        NSSound* sound = [[NSSound alloc] initWithContentsOfFile:path byReference:YES];
        if (!sound) return;
        if (g_current_sound) [g_current_sound stop];
        g_current_sound = sound; /* ARC releases the previous, retains the new */
        [g_current_sound play];
    }
}

void zcsr_cocoa_sound_stop(void) {
    @autoreleasepool {
        if (g_current_sound) {
            [g_current_sound stop];
            g_current_sound = nil;
        }
    }
}

#endif /* __APPLE__ */
