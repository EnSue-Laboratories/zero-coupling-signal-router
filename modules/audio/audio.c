/* Agent 2 (Game Core) — Audio: WAV player in a dedicated thread; signals-only (PLAY/STOP).
 * Includes ONLY shared contracts. TODO(Agent 2 / Codex): real implementation. */
#include "zcsr/audio.h"

zcsr_audio* zcsr_audio_start(void)                                             { return 0; }
void        zcsr_audio_stop(zcsr_audio* a)                                     { (void)a; }
bool        zcsr_audio_register_wav(zcsr_audio* a, const char* id, const char* path) { (void)a; (void)id; (void)path; return false; }
bool        zcsr_audio_slot_play(const char* payload)                         { (void)payload; return false; }
bool        zcsr_audio_slot_stop(const char* payload)                         { (void)payload; return false; }
