#ifndef ZCSR_AUDIO_MIX_H
#define ZCSR_AUDIO_MIX_H
/* Engine-ext Agent 2 — multi-channel WAV mixer. NEW module; COEXISTS with the single-stream
 * zcsr/audio.h (does not change/break it). 16 software channels, one dedicated audio thread,
 * lock-free ring buffer. Backend: Win WASAPI / Linux ALSA / macOS AudioUnit. Implementer:
 * modules/audiomix. Signals-friendly: audio_play / audio_finished. No runtime heap (fixed pools). */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum { ZCSR_MIX_CHANNELS = 16 };

typedef struct zcsr_mixer zcsr_mixer; /* opaque; owns the audio thread + device */

zcsr_mixer* zcsr_mix_start(void);       /* spawn audio thread + open output device. NULL on fail. */
void        zcsr_mix_stop(zcsr_mixer*); /* stop playback + join thread + close device. */

/* Register a WAV (PCM 16-bit 44100 Hz mono/stereo) under an id (no path in play calls/signals). */
bool        zcsr_mix_register_wav(zcsr_mixer*, const char* id, const char* wav_path);

/* Play a registered sound on a free channel. Returns channel 0..15, or -1 (no free channel /
 * unknown id). volume 0..1; loop repeats until stopped. Non-blocking (posts to the audio thread). */
int         zcsr_mix_play(zcsr_mixer*, const char* id, float volume, bool loop);
void        zcsr_mix_stop_channel(zcsr_mixer*, int channel);

/* Slots to connect to a zcsr/signal.h router (zcsr_slot_fn shape). payload = sound id.
 * Operate on the module-owned active mixer (like zcsr_audio_slot_*). */
bool        zcsr_mix_slot_play(const char* payload);   /* connect to "PLAY_SOUND" */
bool        zcsr_mix_slot_stop(const char* payload);   /* connect to "STOP_SOUND" (by id) */

/* Drain channels that finished since the last call (the audio thread marks them). Returns the
 * count written into out[] (capped at cap) — lets the caller emit audio_finished(channel). */
size_t      zcsr_mix_poll_finished(zcsr_mixer*, int* out, size_t cap);

#endif /* ZCSR_AUDIO_MIX_H */
