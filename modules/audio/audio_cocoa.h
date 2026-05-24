#ifndef ZCSR_AUDIO_COCOA_H
#define ZCSR_AUDIO_COCOA_H
/* Thin C-ABI shim over the Cocoa NSSound backend. Implemented in audio_cocoa.m.
 * Same-module header; pure C (no Objective-C, no cross-module include). Single-stream:
 * a new play replaces the current sound (matches the Win32 PlaySound behavior). */

void zcsr_cocoa_sound_play(const char* wav_path);
void zcsr_cocoa_sound_stop(void);

#endif /* ZCSR_AUDIO_COCOA_H */
