#ifndef ZCSR_AUDIO_H
#define ZCSR_AUDIO_H
/* Agent 2 (Game Core) — WAV audio player. Runs in ONE dedicated background thread.
 * Communicates ONLY via signals: "PLAY_SOUND" / "STOP_SOUND" (payload = registered sound id).
 * Implementer: modules/audio. Wires into the zcsr/signal.h router. */
#include "signal.h"
#include <stdbool.h>

/* Signal tags this module responds to (wire these into a router: PLAY/STOP -> slots below). */
#define ZCSR_SIG_PLAY_SOUND "PLAY_SOUND"
#define ZCSR_SIG_STOP_SOUND "STOP_SOUND"

typedef struct zcsr_audio zcsr_audio; /* opaque; owns the background thread */

zcsr_audio* zcsr_audio_start(void);          /* spawns the dedicated audio thread */
void        zcsr_audio_stop(zcsr_audio*);    /* stops playback + joins the thread */
/* Register a WAV file under an id so signals can reference it by id (no path in the signal). */
bool        zcsr_audio_register_wav(zcsr_audio*, const char* id, const char* wav_path);

/* Slots to connect to the router (zcsr_slot_fn). payload = sound id. Non-blocking: they post a
 * message to the audio thread (the engine instance is module-owned, like integration glue). */
bool        zcsr_audio_slot_play(const char* payload); /* connect to "PLAY_SOUND" */
bool        zcsr_audio_slot_stop(const char* payload); /* connect to "STOP_SOUND" */

#endif /* ZCSR_AUDIO_H */
