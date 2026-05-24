/* Agent 2 (Game Core) — audio tests: fixed WAV registration, signal-only PLAY/STOP slots, and
 * dedicated thread lifecycle. This stays device-neutral so it is stable on headless CI. */
#include "zcsr/audio.h"
#include "zcsr/signal.h"
#include <stdio.h>

static int fails = 0;
#define CK(label, cond) do { if (!(cond)) { printf("FAIL: %s\n", label); fails++; } } while (0)

/* 8-bit mono PCM WAV, 8000Hz, 4 samples. */
static const unsigned char WAV_MINIMAL[] = {
    'R','I','F','F', 40,0,0,0, 'W','A','V','E',
    'f','m','t',' ', 16,0,0,0, 1,0, 1,0, 0x40,0x1f,0,0,
    0x40,0x1f,0,0, 1,0, 8,0,
    'd','a','t','a', 4,0,0,0, 0x80,0x80,0x80,0x80
};

static int play_calls = 0;
static int stop_calls = 0;

static bool count_play(const char* payload) {
    play_calls++;
    return zcsr_audio_slot_play(payload);
}

static bool count_stop(const char* payload) {
    stop_calls++;
    return zcsr_audio_slot_stop(payload);
}

#define AUDIO_CONNS(X)                         \
    X(ZCSR_SIG_PLAY_SOUND, count_play)         \
    X(ZCSR_SIG_STOP_SOUND, count_stop)
ZCSR_DEFINE_ROUTER(audio_test, AUDIO_CONNS)

static bool write_fixture(const char* path, const unsigned char* data, size_t n) {
    FILE* f = fopen(path, "wb");
    size_t wrote;
    if (!f) return false;
    wrote = fwrite(data, 1, n, f);
    fclose(f);
    return wrote == n;
}

int main(void) {
    const char* good = "zcsr_test_audio.wav";
    const char* bad = "zcsr_test_audio_bad.wav";
    zcsr_audio* a;

    CK("fixture written", write_fixture(good, WAV_MINIMAL, sizeof WAV_MINIMAL));
    CK("bad fixture written", write_fixture(bad, (const unsigned char*)"not wav", 7));

    CK("play before start -> false", !zcsr_audio_slot_play("tick"));
    CK("stop before start -> false", !zcsr_audio_slot_stop("tick"));

    a = zcsr_audio_start();
    CK("start -> non-NULL", a != NULL);
    if (a) {
        CK("register null audio -> false", !zcsr_audio_register_wav(NULL, "tick", good));
        CK("register null id -> false", !zcsr_audio_register_wav(a, NULL, good));
        CK("register null path -> false", !zcsr_audio_register_wav(a, "tick", NULL));
        CK("register bad wav -> false", !zcsr_audio_register_wav(a, "bad", bad));
        CK("register valid wav -> true", zcsr_audio_register_wav(a, "tick", good));
        CK("reregister valid wav -> true", zcsr_audio_register_wav(a, "tick", good));

        CK("play unknown -> false", !zcsr_audio_slot_play("missing"));
        CK("stop unknown -> false", !zcsr_audio_slot_stop("missing"));
        CK("play registered -> true", zcsr_audio_slot_play("tick"));
        CK("stop registered -> true", zcsr_audio_slot_stop("tick"));

        audio_test_emit(ZCSR_SIG_PLAY_SOUND, "tick");
        audio_test_emit(ZCSR_SIG_STOP_SOUND, "tick");
        CK("router called play slot once", play_calls == 1);
        CK("router called stop slot once", stop_calls == 1);
        audio_test_emit("NO_MATCH", "tick");
        CK("unknown signal did not call slots", play_calls == 1 && stop_calls == 1);

        zcsr_audio_stop(a);
        CK("play after stop -> false", !zcsr_audio_slot_play("tick"));
    }

    remove(good);
    remove(bad);
    if (!fails) printf("test_audio: PASS (WAV registration + PLAY/STOP signal slots)\n");
    return fails ? 1 : 0;
}
