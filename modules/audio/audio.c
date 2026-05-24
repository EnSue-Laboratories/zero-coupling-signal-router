/* Agent 2 (Game Core) — Audio: WAV command thread; signals-only (PLAY/STOP).
 * Includes ONLY shared contracts from the project. Platform/thread headers are local backend
 * details, and no cross-module includes are used. */
#include "zcsr/audio.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#else
#include <pthread.h>
#endif

enum {
    ZCSR_AUDIO_POOL = 2,
    ZCSR_AUDIO_MAX_SOUNDS = 16,
    ZCSR_AUDIO_CMD_CAP = 32,
    ZCSR_AUDIO_ID_CAP = 64,
    ZCSR_AUDIO_PATH_CAP = 260
};

typedef enum {
    ZCSR_AUDIO_CMD_PLAY = 1,
    ZCSR_AUDIO_CMD_STOP = 2
} zcsr_audio_cmd_type;

typedef struct {
    bool used;
    char id[ZCSR_AUDIO_ID_CAP];
    char path[ZCSR_AUDIO_PATH_CAP];
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t sample_rate;
    uint32_t data_size;
} zcsr_audio_sound;

typedef struct {
    zcsr_audio_cmd_type type;
    char id[ZCSR_AUDIO_ID_CAP];
} zcsr_audio_cmd;

struct zcsr_audio {
    bool in_use;
    bool running;
    bool thread_started;
    zcsr_audio_sound sounds[ZCSR_AUDIO_MAX_SOUNDS];
    zcsr_audio_cmd queue[ZCSR_AUDIO_CMD_CAP];
    unsigned head;
    unsigned tail;
    char current_id[ZCSR_AUDIO_ID_CAP];
#if defined(_WIN32)
    HANDLE thread;
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE cond;
#else
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
};

static zcsr_audio g_audio_pool[ZCSR_AUDIO_POOL];
static zcsr_audio* g_active_audio;

static bool zcsr_str_copy(char* dst, size_t cap, const char* src) {
    size_t len;
    if (!dst || cap == 0 || !src) return false;
    len = strlen(src);
    if (len == 0 || len >= cap) return false;
    memcpy(dst, src, len + 1);
    return true;
}

static bool zcsr_str_eq(const char* a, const char* b) {
    return a && b && strcmp(a, b) == 0;
}

static bool zcsr_read_exact(FILE* f, unsigned char* out, size_t n) {
    return f && out && fread(out, 1, n, f) == n;
}

static bool zcsr_read_u16le(FILE* f, uint16_t* out) {
    unsigned char b[2];
    if (!zcsr_read_exact(f, b, sizeof b)) return false;
    *out = (uint16_t)(b[0] | ((uint16_t)b[1] << 8));
    return true;
}

static bool zcsr_read_u32le(FILE* f, uint32_t* out) {
    unsigned char b[4];
    if (!zcsr_read_exact(f, b, sizeof b)) return false;
    *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
    return true;
}

static bool zcsr_skip_bytes(FILE* f, uint32_t n) {
    while (n > 0) {
        long step = n > 0x7fffffffU ? 0x7fffffffL : (long)n;
        if (fseek(f, step, SEEK_CUR) != 0) return false;
        n -= (uint32_t)step;
    }
    return true;
}

static bool zcsr_chunk_id_eq(const unsigned char id[4], const char* lit) {
    return id && lit && id[0] == (unsigned char)lit[0] && id[1] == (unsigned char)lit[1] &&
           id[2] == (unsigned char)lit[2] && id[3] == (unsigned char)lit[3];
}

static bool zcsr_parse_wav(const char* path, zcsr_audio_sound* out) {
    FILE* f;
    unsigned char riff[4];
    unsigned char wave[4];
    uint32_t riff_size;
    bool saw_fmt = false;
    bool saw_data = false;
    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint16_t bits = 0;
    uint32_t sample_rate = 0;
    uint32_t data_size = 0;

    if (!path || !out) return false;
    f = fopen(path, "rb");
    if (!f) return false;

    if (!zcsr_read_exact(f, riff, sizeof riff) || !zcsr_read_u32le(f, &riff_size) ||
        !zcsr_read_exact(f, wave, sizeof wave) || !zcsr_chunk_id_eq(riff, "RIFF") ||
        !zcsr_chunk_id_eq(wave, "WAVE")) {
        fclose(f);
        return false;
    }
    (void)riff_size;

    for (;;) {
        unsigned char chunk[4];
        uint32_t chunk_size;
        long payload_start;
        if (!zcsr_read_exact(f, chunk, sizeof chunk)) break;
        if (!zcsr_read_u32le(f, &chunk_size)) {
            fclose(f);
            return false;
        }
        payload_start = ftell(f);
        if (payload_start < 0) {
            fclose(f);
            return false;
        }

        if (zcsr_chunk_id_eq(chunk, "fmt ")) {
            uint32_t byte_rate;
            uint16_t block_align;
            if (chunk_size < 16 || !zcsr_read_u16le(f, &audio_format) ||
                !zcsr_read_u16le(f, &channels) || !zcsr_read_u32le(f, &sample_rate) ||
                !zcsr_read_u32le(f, &byte_rate) || !zcsr_read_u16le(f, &block_align) ||
                !zcsr_read_u16le(f, &bits)) {
                fclose(f);
                return false;
            }
            (void)byte_rate;
            (void)block_align;
            saw_fmt = true;
        } else if (zcsr_chunk_id_eq(chunk, "data")) {
            data_size = chunk_size;
            saw_data = true;
        }

        if (fseek(f, payload_start, SEEK_SET) != 0 ||
            !zcsr_skip_bytes(f, chunk_size + (chunk_size & 1U))) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    if (!saw_fmt || !saw_data) return false;
    if (audio_format != 1 || (channels != 1 && channels != 2) ||
        (bits != 8 && bits != 16) || sample_rate == 0 || data_size == 0) {
        return false;
    }

    out->channels = channels;
    out->bits_per_sample = bits;
    out->sample_rate = sample_rate;
    out->data_size = data_size;
    return true;
}

static void zcsr_audio_lock(zcsr_audio* a) {
#if defined(_WIN32)
    EnterCriticalSection(&a->mutex);
#else
    pthread_mutex_lock(&a->mutex);
#endif
}

static void zcsr_audio_unlock(zcsr_audio* a) {
#if defined(_WIN32)
    LeaveCriticalSection(&a->mutex);
#else
    pthread_mutex_unlock(&a->mutex);
#endif
}

static void zcsr_audio_signal(zcsr_audio* a) {
#if defined(_WIN32)
    WakeConditionVariable(&a->cond);
#else
    pthread_cond_signal(&a->cond);
#endif
}

static void zcsr_audio_wait(zcsr_audio* a) {
#if defined(_WIN32)
    SleepConditionVariableCS(&a->cond, &a->mutex, INFINITE);
#else
    pthread_cond_wait(&a->cond, &a->mutex);
#endif
}

static bool zcsr_audio_queue_empty(const zcsr_audio* a) {
    return a->head == a->tail;
}

static bool zcsr_audio_queue_push_locked(zcsr_audio* a, zcsr_audio_cmd_type type, const char* id) {
    unsigned next;
    zcsr_audio_cmd* cmd;
    if (!a || !a->running || !id) return false;
    next = (a->tail + 1U) % ZCSR_AUDIO_CMD_CAP;
    if (next == a->head) return false;
    cmd = &a->queue[a->tail];
    cmd->type = type;
    if (!zcsr_str_copy(cmd->id, sizeof cmd->id, id)) return false;
    a->tail = next;
    zcsr_audio_signal(a);
    return true;
}

static bool zcsr_audio_queue_pop_locked(zcsr_audio* a, zcsr_audio_cmd* out) {
    if (!a || !out || zcsr_audio_queue_empty(a)) return false;
    *out = a->queue[a->head];
    a->head = (a->head + 1U) % ZCSR_AUDIO_CMD_CAP;
    return true;
}

static int zcsr_audio_find_sound_locked(const zcsr_audio* a, const char* id) {
    int i;
    if (!a || !id) return -1;
    for (i = 0; i < ZCSR_AUDIO_MAX_SOUNDS; ++i) {
        if (a->sounds[i].used && zcsr_str_eq(a->sounds[i].id, id)) return i;
    }
    return -1;
}

static void zcsr_audio_backend_play(const char* path) {
#if defined(_WIN32)
    if (path) PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC);
#else
    (void)path; /* No dependency-free POSIX desktop audio backend; command state remains testable. */
#endif
}

static void zcsr_audio_backend_stop(void) {
#if defined(_WIN32)
    PlaySoundA(NULL, NULL, 0);
#endif
}

static void zcsr_audio_apply_command(zcsr_audio* a, const zcsr_audio_cmd* cmd) {
    int idx;
    if (!a || !cmd) return;
    if (cmd->type == ZCSR_AUDIO_CMD_PLAY) {
        idx = zcsr_audio_find_sound_locked(a, cmd->id);
        if (idx < 0) return;
        zcsr_str_copy(a->current_id, sizeof a->current_id, cmd->id);
        zcsr_audio_backend_play(a->sounds[idx].path);
    } else if (cmd->type == ZCSR_AUDIO_CMD_STOP && zcsr_str_eq(a->current_id, cmd->id)) {
        a->current_id[0] = '\0';
        zcsr_audio_backend_stop();
    }
}

#if defined(_WIN32)
static DWORD WINAPI zcsr_audio_thread_main(LPVOID arg)
#else
static void* zcsr_audio_thread_main(void* arg)
#endif
{
    zcsr_audio* a = (zcsr_audio*)arg;
    for (;;) {
        zcsr_audio_cmd cmd;
        zcsr_audio_lock(a);
        while (zcsr_audio_queue_empty(a) && a->running) {
            zcsr_audio_wait(a);
        }
        if (!a->running && zcsr_audio_queue_empty(a)) {
            zcsr_audio_unlock(a);
            break;
        }
        if (zcsr_audio_queue_pop_locked(a, &cmd)) {
            zcsr_audio_apply_command(a, &cmd);
        }
        zcsr_audio_unlock(a);
    }
#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}

static void zcsr_audio_clear(zcsr_audio* a) {
    if (a) memset(a, 0, sizeof *a);
}

zcsr_audio* zcsr_audio_start(void) {
    int i;
    for (i = 0; i < ZCSR_AUDIO_POOL; ++i) {
        zcsr_audio* a = &g_audio_pool[i];
        if (a->in_use) continue;
        zcsr_audio_clear(a);
        a->in_use = true;
        a->running = true;
#if defined(_WIN32)
        InitializeCriticalSection(&a->mutex);
        InitializeConditionVariable(&a->cond);
        a->thread = CreateThread(NULL, 0, zcsr_audio_thread_main, a, 0, NULL);
        if (!a->thread) {
            DeleteCriticalSection(&a->mutex);
            zcsr_audio_clear(a);
            return NULL;
        }
#else
        if (pthread_mutex_init(&a->mutex, NULL) != 0) {
            zcsr_audio_clear(a);
            return NULL;
        }
        if (pthread_cond_init(&a->cond, NULL) != 0) {
            pthread_mutex_destroy(&a->mutex);
            zcsr_audio_clear(a);
            return NULL;
        }
        if (pthread_create(&a->thread, NULL, zcsr_audio_thread_main, a) != 0) {
            pthread_cond_destroy(&a->cond);
            pthread_mutex_destroy(&a->mutex);
            zcsr_audio_clear(a);
            return NULL;
        }
#endif
        a->thread_started = true;
        if (!g_active_audio) g_active_audio = a;
        return a;
    }
    return NULL;
}

void zcsr_audio_stop(zcsr_audio* a) {
    if (!a || !a->in_use) return;
    zcsr_audio_lock(a);
    a->running = false;
    zcsr_audio_signal(a);
    zcsr_audio_unlock(a);

    if (a->thread_started) {
#if defined(_WIN32)
        zcsr_audio_backend_stop();
        WaitForSingleObject(a->thread, INFINITE);
        CloseHandle(a->thread);
        DeleteCriticalSection(&a->mutex);
#else
        pthread_join(a->thread, NULL);
        pthread_cond_destroy(&a->cond);
        pthread_mutex_destroy(&a->mutex);
#endif
    }
    if (g_active_audio == a) g_active_audio = NULL;
    zcsr_audio_clear(a);
}

bool zcsr_audio_register_wav(zcsr_audio* a, const char* id, const char* path) {
    zcsr_audio_sound parsed;
    int i;
    int free_slot = -1;
    if (!a || !a->in_use || !id || !path) return false;
    memset(&parsed, 0, sizeof parsed);
    if (!zcsr_parse_wav(path, &parsed)) return false;

    zcsr_audio_lock(a);
    i = zcsr_audio_find_sound_locked(a, id);
    if (i < 0) {
        for (i = 0; i < ZCSR_AUDIO_MAX_SOUNDS; ++i) {
            if (!a->sounds[i].used) {
                free_slot = i;
                break;
            }
        }
        i = free_slot;
    }
    if (i < 0 ||
        !zcsr_str_copy(parsed.id, sizeof parsed.id, id) ||
        !zcsr_str_copy(parsed.path, sizeof parsed.path, path)) {
        zcsr_audio_unlock(a);
        return false;
    }
    parsed.used = true;
    a->sounds[i] = parsed;
    zcsr_audio_unlock(a);
    return true;
}

bool zcsr_audio_slot_play(const char* payload) {
    zcsr_audio* a = g_active_audio;
    bool ok = false;
    if (!a || !payload) return false;
    zcsr_audio_lock(a);
    if (zcsr_audio_find_sound_locked(a, payload) >= 0) {
        ok = zcsr_audio_queue_push_locked(a, ZCSR_AUDIO_CMD_PLAY, payload);
    }
    zcsr_audio_unlock(a);
    return ok;
}

bool zcsr_audio_slot_stop(const char* payload) {
    zcsr_audio* a = g_active_audio;
    bool ok = false;
    if (!a || !payload) return false;
    zcsr_audio_lock(a);
    if (zcsr_audio_find_sound_locked(a, payload) >= 0) {
        ok = zcsr_audio_queue_push_locked(a, ZCSR_AUDIO_CMD_STOP, payload);
    }
    zcsr_audio_unlock(a);
    return ok;
}
