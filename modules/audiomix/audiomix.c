/* Engine-ext Agent 2 — multi-channel WAV mixer. PHASE-0 SKELETON STUB.
 * Implement against zcsr/audio_mix.h: dedicated audio thread + lock-free ring buffer, 16 channels,
 * backend Win WASAPI / Linux ALSA / macOS AudioUnit. Coexists with single-stream zcsr/audio.h.
 * Includes ONLY shared contracts (zero-coupling). No runtime heap (fixed pools). */
#include "zcsr/audio_mix.h"

zcsr_mixer* zcsr_mix_start(void) { return 0; }                 /* TODO: thread + device */
void        zcsr_mix_stop(zcsr_mixer* m) { (void)m; }
bool        zcsr_mix_register_wav(zcsr_mixer* m, const char* id, const char* wav_path) {
    (void)m; (void)id; (void)wav_path; return false;
}
int         zcsr_mix_play(zcsr_mixer* m, const char* id, float volume, bool loop) {
    (void)m; (void)id; (void)volume; (void)loop; return -1;
}
void        zcsr_mix_stop_channel(zcsr_mixer* m, int channel) { (void)m; (void)channel; }
bool        zcsr_mix_slot_play(const char* payload) { (void)payload; return false; }
bool        zcsr_mix_slot_stop(const char* payload) { (void)payload; return false; }
size_t      zcsr_mix_poll_finished(zcsr_mixer* m, int* out, size_t cap) {
    (void)m; (void)out; (void)cap; return 0;
}
