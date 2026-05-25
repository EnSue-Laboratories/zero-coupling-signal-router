/* Engine-ext Agent 2 - multi-channel audio mixer.
 * Fixed pools only, one mixer thread, command/finished rings, no cross-module includes.
 * Linux attempts ALSA dynamically (no CMake find_package); if no device exists, the mixer
 * still runs its logic path so headless CI can validate threading/state.
 */
#include "zcsr/audio_mix.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

#if defined(__linux__)
#include <dlfcn.h>
typedef struct _snd_pcm snd_pcm_t;
typedef long snd_pcm_sframes_t;
typedef int (*zcsr_snd_pcm_open_fn)(snd_pcm_t**, const char*, int, int);
typedef int (*zcsr_snd_pcm_set_params_fn)(snd_pcm_t*, int, int, unsigned int, unsigned int, int, unsigned int);
typedef snd_pcm_sframes_t (*zcsr_snd_pcm_writei_fn)(snd_pcm_t*, const void*, unsigned long);
typedef int (*zcsr_snd_pcm_prepare_fn)(snd_pcm_t*);
typedef int (*zcsr_snd_pcm_close_fn)(snd_pcm_t*);
typedef union {
    void* obj;
    zcsr_snd_pcm_open_fn open_fn;
    zcsr_snd_pcm_set_params_fn set_params;
    zcsr_snd_pcm_writei_fn writei;
    zcsr_snd_pcm_prepare_fn prepare;
    zcsr_snd_pcm_close_fn close;
} zcsr_alsa_sym;
#endif

enum {
    ZCSR_MIX_POOL = 1,
    ZCSR_MIX_MAX_SOUNDS = 32,
    ZCSR_MIX_ID_CAP = 64,
    ZCSR_MIX_CMD_CAP = 128,
    ZCSR_MIX_FINISHED_CAP = 128,
    ZCSR_MIX_RATE = 44100,
    ZCSR_MIX_PERIOD_FRAMES = 512,
    ZCSR_MIX_SAMPLE_FRAMES = ZCSR_MIX_RATE * 20
};

enum {
    ZCSR_MIX_CH_FREE = 0,
    ZCSR_MIX_CH_RESERVED = 1,
    ZCSR_MIX_CH_ACTIVE = 2
};

typedef enum {
    ZCSR_MIX_CMD_PLAY = 1,
    ZCSR_MIX_CMD_STOP_CHANNEL = 2,
    ZCSR_MIX_CMD_STOP_ID = 3
} zcsr_mix_cmd_type;

typedef struct {
    bool used;
    char id[ZCSR_MIX_ID_CAP];
    uint32_t offset;
    uint32_t frames;
} zcsr_mix_sound;

typedef struct {
    atomic_int state;
    int sound;
    uint32_t frame;
    float volume;
    bool loop;
} zcsr_mix_channel;

typedef struct {
    zcsr_mix_cmd_type type;
    int channel;
    int sound;
    float volume;
    bool loop;
    char id[ZCSR_MIX_ID_CAP];
} zcsr_mix_cmd;

typedef struct {
#if defined(__linux__)
    void* lib;
    snd_pcm_t* pcm;
    zcsr_snd_pcm_writei_fn writei;
    zcsr_snd_pcm_prepare_fn prepare;
    zcsr_snd_pcm_close_fn close;
#endif
    bool opened;
} zcsr_mix_backend;

struct zcsr_mixer {
    bool in_use;
    atomic_bool running;
    bool thread_started;
    zcsr_mix_sound sounds[ZCSR_MIX_MAX_SOUNDS];
    zcsr_mix_channel channels[ZCSR_MIX_CHANNELS];
    int16_t samples[ZCSR_MIX_SAMPLE_FRAMES * 2];
    uint32_t sample_frames_used;
    zcsr_mix_cmd commands[ZCSR_MIX_CMD_CAP];
    atomic_uint cmd_head;
    atomic_uint cmd_tail;
    int finished[ZCSR_MIX_FINISHED_CAP];
    atomic_uint finished_head;
    atomic_uint finished_tail;
    int16_t mix_buffer[ZCSR_MIX_PERIOD_FRAMES * 2];
    zcsr_mix_backend backend;
#if defined(_WIN32)
    HANDLE thread;
#else
    pthread_t thread;
#endif
};

static zcsr_mixer g_mix_pool[ZCSR_MIX_POOL];
static zcsr_mixer* g_active_mixer;

static bool zcsr_mix_copy(char* dst, size_t cap, const char* src) {
    size_t len;
    if (!dst || !src || cap == 0) return false;
    len = strlen(src);
    if (len == 0 || len >= cap) return false;
    memcpy(dst, src, len + 1);
    return true;
}

static bool zcsr_mix_eq(const char* a, const char* b) {
    return a && b && strcmp(a, b) == 0;
}

static void zcsr_mix_sleep_period(void) {
#if defined(_WIN32)
    Sleep(12);
#else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 12000000L;
    nanosleep(&ts, 0);
#endif
}

static bool zcsr_mix_ring_push(zcsr_mixer* m, const zcsr_mix_cmd* cmd) {
    unsigned tail;
    unsigned next;
    unsigned head;
    if (!m || !cmd) return false;
    tail = atomic_load_explicit(&m->cmd_tail, memory_order_relaxed);
    next = (tail + 1U) % ZCSR_MIX_CMD_CAP;
    head = atomic_load_explicit(&m->cmd_head, memory_order_acquire);
    if (next == head) return false;
    m->commands[tail] = *cmd;
    atomic_store_explicit(&m->cmd_tail, next, memory_order_release);
    return true;
}

static bool zcsr_mix_ring_pop(zcsr_mixer* m, zcsr_mix_cmd* out) {
    unsigned head;
    unsigned tail;
    if (!m || !out) return false;
    head = atomic_load_explicit(&m->cmd_head, memory_order_relaxed);
    tail = atomic_load_explicit(&m->cmd_tail, memory_order_acquire);
    if (head == tail) return false;
    *out = m->commands[head];
    atomic_store_explicit(&m->cmd_head, (head + 1U) % ZCSR_MIX_CMD_CAP, memory_order_release);
    return true;
}

static void zcsr_mix_finished_push(zcsr_mixer* m, int channel) {
    unsigned tail;
    unsigned next;
    unsigned head;
    if (!m || channel < 0) return;
    tail = atomic_load_explicit(&m->finished_tail, memory_order_relaxed);
    next = (tail + 1U) % ZCSR_MIX_FINISHED_CAP;
    head = atomic_load_explicit(&m->finished_head, memory_order_acquire);
    if (next == head) return;
    m->finished[tail] = channel;
    atomic_store_explicit(&m->finished_tail, next, memory_order_release);
}

static int zcsr_mix_find_sound(const zcsr_mixer* m, const char* id) {
    if (!m || !id) return -1;
    for (int i = 0; i < ZCSR_MIX_MAX_SOUNDS; ++i) {
        if (m->sounds[i].used && zcsr_mix_eq(m->sounds[i].id, id)) return i;
    }
    return -1;
}

static int16_t zcsr_mix_read_i16le(FILE* f, bool* ok) {
    unsigned char b[2];
    if (!f || fread(b, 1, 2, f) != 2) {
        if (ok) *ok = false;
        return 0;
    }
    if (ok) *ok = true;
    return (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

static bool zcsr_mix_read_exact(FILE* f, unsigned char* out, size_t n) {
    return f && out && fread(out, 1, n, f) == n;
}

static bool zcsr_mix_read_u16(FILE* f, uint16_t* out) {
    unsigned char b[2];
    if (!zcsr_mix_read_exact(f, b, sizeof b)) return false;
    *out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
    return true;
}

static bool zcsr_mix_read_u32(FILE* f, uint32_t* out) {
    unsigned char b[4];
    if (!zcsr_mix_read_exact(f, b, sizeof b)) return false;
    *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return true;
}

static bool zcsr_mix_chunk_eq(const unsigned char id[4], const char* lit) {
    return id[0] == (unsigned char)lit[0] && id[1] == (unsigned char)lit[1] &&
           id[2] == (unsigned char)lit[2] && id[3] == (unsigned char)lit[3];
}

static bool zcsr_mix_parse_wav(FILE* f, uint16_t* channels, uint32_t* rate, uint32_t* data_size, long* data_at) {
    unsigned char riff[4];
    unsigned char wave[4];
    uint32_t riff_size;
    bool saw_fmt = false;
    bool saw_data = false;
    uint16_t format = 0;
    uint16_t bits = 0;
    if (!f || !channels || !rate || !data_size || !data_at) return false;
    if (!zcsr_mix_read_exact(f, riff, sizeof riff) || !zcsr_mix_read_u32(f, &riff_size) ||
        !zcsr_mix_read_exact(f, wave, sizeof wave) || !zcsr_mix_chunk_eq(riff, "RIFF") ||
        !zcsr_mix_chunk_eq(wave, "WAVE")) {
        return false;
    }
    (void)riff_size;
    for (;;) {
        unsigned char chunk[4];
        uint32_t size;
        long payload;
        if (!zcsr_mix_read_exact(f, chunk, sizeof chunk)) break;
        if (!zcsr_mix_read_u32(f, &size)) return false;
        payload = ftell(f);
        if (payload < 0) return false;
        if (zcsr_mix_chunk_eq(chunk, "fmt ")) {
            uint32_t byte_rate;
            uint16_t align;
            if (size < 16 || !zcsr_mix_read_u16(f, &format) || !zcsr_mix_read_u16(f, channels) ||
                !zcsr_mix_read_u32(f, rate) || !zcsr_mix_read_u32(f, &byte_rate) ||
                !zcsr_mix_read_u16(f, &align) || !zcsr_mix_read_u16(f, &bits)) {
                return false;
            }
            (void)byte_rate;
            (void)align;
            saw_fmt = true;
        } else if (zcsr_mix_chunk_eq(chunk, "data")) {
            *data_size = size;
            *data_at = payload;
            saw_data = true;
        }
        if (fseek(f, payload + (long)size + (long)(size & 1U), SEEK_SET) != 0) return false;
    }
    return saw_fmt && saw_data && format == 1 && bits == 16 && (*channels == 1 || *channels == 2) &&
           *rate == ZCSR_MIX_RATE && *data_size > 0;
}

static bool zcsr_mix_load_wav(zcsr_mixer* m, zcsr_mix_sound* s, const char* path) {
    FILE* f;
    uint16_t channels = 0;
    uint32_t rate = 0;
    uint32_t data_size = 0;
    long data_at = 0;
    uint32_t frames;
    uint32_t offset;
    if (!m || !s || !path) return false;
    f = fopen(path, "rb");
    if (!f) return false;
    if (!zcsr_mix_parse_wav(f, &channels, &rate, &data_size, &data_at)) {
        fclose(f);
        return false;
    }
    frames = data_size / ((uint32_t)channels * 2U);
    if (frames == 0 || frames > ZCSR_MIX_SAMPLE_FRAMES - m->sample_frames_used) {
        fclose(f);
        return false;
    }
    if (fseek(f, data_at, SEEK_SET) != 0) {
        fclose(f);
        return false;
    }
    offset = m->sample_frames_used;
    for (uint32_t i = 0; i < frames; ++i) {
        bool ok_l = false;
        bool ok_r = true;
        int16_t l = zcsr_mix_read_i16le(f, &ok_l);
        int16_t r = channels == 2 ? zcsr_mix_read_i16le(f, &ok_r) : l;
        if (!ok_l || !ok_r) {
            fclose(f);
            return false;
        }
        m->samples[(offset + i) * 2U] = l;
        m->samples[(offset + i) * 2U + 1U] = r;
    }
    fclose(f);
    s->offset = offset;
    s->frames = frames;
    m->sample_frames_used += frames;
    (void)rate;
    return true;
}

#if defined(__linux__)
static void* zcsr_mix_sym(void* lib, const char* name) {
    return lib ? dlsym(lib, name) : 0;
}

static zcsr_alsa_sym zcsr_mix_alsa_sym(void* lib, const char* name) {
    zcsr_alsa_sym s;
    s.obj = zcsr_mix_sym(lib, name);
    return s;
}

static void zcsr_mix_backend_close(zcsr_mix_backend* b) {
    if (!b) return;
    if (b->opened && b->pcm && b->close) b->close(b->pcm);
    if (b->lib) dlclose(b->lib);
    *b = (zcsr_mix_backend){ 0 };
}

static void zcsr_mix_backend_open(zcsr_mix_backend* b) {
    zcsr_snd_pcm_open_fn open_fn;
    zcsr_snd_pcm_set_params_fn set_params;
    if (!b) return;
    b->lib = dlopen("libasound.so.2", RTLD_LAZY | RTLD_LOCAL);
    if (!b->lib) return;
    open_fn = zcsr_mix_alsa_sym(b->lib, "snd_pcm_open").open_fn;
    set_params = zcsr_mix_alsa_sym(b->lib, "snd_pcm_set_params").set_params;
    b->writei = zcsr_mix_alsa_sym(b->lib, "snd_pcm_writei").writei;
    b->prepare = zcsr_mix_alsa_sym(b->lib, "snd_pcm_prepare").prepare;
    b->close = zcsr_mix_alsa_sym(b->lib, "snd_pcm_close").close;
    if (!open_fn || !set_params || !b->writei || !b->prepare || !b->close ||
        open_fn(&b->pcm, "default", 0, 0) < 0 ||
        set_params(b->pcm, 2, 3, 2U, ZCSR_MIX_RATE, 1, 25000U) < 0) {
        zcsr_mix_backend_close(b);
        return;
    }
    b->opened = true;
}

static void zcsr_mix_backend_write(zcsr_mix_backend* b, const int16_t* pcm, unsigned frames) {
    if (!b || !b->opened || !pcm) {
        zcsr_mix_sleep_period();
        return;
    }
    while (frames > 0) {
        snd_pcm_sframes_t n = b->writei(b->pcm, pcm, frames);
        if (n < 0) {
            (void)b->prepare(b->pcm);
            break;
        }
        if (n == 0) break;
        pcm += (unsigned)n * 2U;
        frames -= (unsigned)n;
    }
}
#else
static void zcsr_mix_backend_close(zcsr_mix_backend* b) { if (b) *b = (zcsr_mix_backend){ 0 }; }
static void zcsr_mix_backend_open(zcsr_mix_backend* b) { if (b) b->opened = false; }
static void zcsr_mix_backend_write(zcsr_mix_backend* b, const int16_t* pcm, unsigned frames) {
    (void)b; (void)pcm; (void)frames; zcsr_mix_sleep_period();
}
#endif

static int16_t zcsr_mix_clamp(int v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

static void zcsr_mix_apply_cmd(zcsr_mixer* m, const zcsr_mix_cmd* cmd) {
    if (!m || !cmd) return;
    if (cmd->type == ZCSR_MIX_CMD_PLAY && cmd->channel >= 0 && cmd->channel < ZCSR_MIX_CHANNELS) {
        zcsr_mix_channel* ch = &m->channels[cmd->channel];
        ch->sound = cmd->sound;
        ch->frame = 0;
        ch->volume = cmd->volume;
        ch->loop = cmd->loop;
        atomic_store_explicit(&ch->state, ZCSR_MIX_CH_ACTIVE, memory_order_release);
    } else if (cmd->type == ZCSR_MIX_CMD_STOP_CHANNEL && cmd->channel >= 0 && cmd->channel < ZCSR_MIX_CHANNELS) {
        atomic_store_explicit(&m->channels[cmd->channel].state, ZCSR_MIX_CH_FREE, memory_order_release);
        zcsr_mix_finished_push(m, cmd->channel);
    } else if (cmd->type == ZCSR_MIX_CMD_STOP_ID) {
        for (int i = 0; i < ZCSR_MIX_CHANNELS; ++i) {
            int state = atomic_load_explicit(&m->channels[i].state, memory_order_acquire);
            int sound = m->channels[i].sound;
            if (state == ZCSR_MIX_CH_ACTIVE && sound >= 0 && zcsr_mix_eq(m->sounds[sound].id, cmd->id)) {
                atomic_store_explicit(&m->channels[i].state, ZCSR_MIX_CH_FREE, memory_order_release);
                zcsr_mix_finished_push(m, i);
            }
        }
    }
}

static void zcsr_mix_drain_commands(zcsr_mixer* m) {
    zcsr_mix_cmd cmd;
    while (zcsr_mix_ring_pop(m, &cmd)) zcsr_mix_apply_cmd(m, &cmd);
}

static void zcsr_mix_render_period(zcsr_mixer* m) {
    for (unsigned frame = 0; frame < ZCSR_MIX_PERIOD_FRAMES; ++frame) {
        int left = 0;
        int right = 0;
        for (int c = 0; c < ZCSR_MIX_CHANNELS; ++c) {
            zcsr_mix_channel* ch = &m->channels[c];
            int state = atomic_load_explicit(&ch->state, memory_order_acquire);
            if (state != ZCSR_MIX_CH_ACTIVE || ch->sound < 0 || ch->sound >= ZCSR_MIX_MAX_SOUNDS) continue;
            zcsr_mix_sound* s = &m->sounds[ch->sound];
            if (!s->used || s->frames == 0) {
                atomic_store_explicit(&ch->state, ZCSR_MIX_CH_FREE, memory_order_release);
                continue;
            }
            if (ch->frame >= s->frames) {
                if (ch->loop) ch->frame = 0;
                else {
                    atomic_store_explicit(&ch->state, ZCSR_MIX_CH_FREE, memory_order_release);
                    zcsr_mix_finished_push(m, c);
                    continue;
                }
            }
            uint32_t sample = (s->offset + ch->frame) * 2U;
            left += (int)((float)m->samples[sample] * ch->volume);
            right += (int)((float)m->samples[sample + 1U] * ch->volume);
            ++ch->frame;
        }
        m->mix_buffer[frame * 2U] = zcsr_mix_clamp(left);
        m->mix_buffer[frame * 2U + 1U] = zcsr_mix_clamp(right);
    }
}

#if defined(_WIN32)
static DWORD WINAPI zcsr_mix_thread_main(LPVOID arg)
#else
static void* zcsr_mix_thread_main(void* arg)
#endif
{
    zcsr_mixer* m = (zcsr_mixer*)arg;
    zcsr_mix_backend_open(&m->backend);
    while (atomic_load_explicit(&m->running, memory_order_acquire)) {
        zcsr_mix_drain_commands(m);
        zcsr_mix_render_period(m);
        zcsr_mix_backend_write(&m->backend, m->mix_buffer, ZCSR_MIX_PERIOD_FRAMES);
    }
    zcsr_mix_drain_commands(m);
    zcsr_mix_backend_close(&m->backend);
#if defined(_WIN32)
    return 0;
#else
    return 0;
#endif
}

zcsr_mixer* zcsr_mix_start(void) {
    for (int i = 0; i < ZCSR_MIX_POOL; ++i) {
        zcsr_mixer* m = &g_mix_pool[i];
        if (m->in_use) continue;
        *m = (zcsr_mixer){ 0 };
        m->in_use = true;
        atomic_store(&m->running, true);
        for (int c = 0; c < ZCSR_MIX_CHANNELS; ++c) {
            atomic_store(&m->channels[c].state, ZCSR_MIX_CH_FREE);
            m->channels[c].sound = -1;
        }
#if defined(_WIN32)
        m->thread = CreateThread(0, 0, zcsr_mix_thread_main, m, 0, 0);
        m->thread_started = m->thread != 0;
#else
        m->thread_started = pthread_create(&m->thread, 0, zcsr_mix_thread_main, m) == 0;
#endif
        if (!m->thread_started) {
            *m = (zcsr_mixer){ 0 };
            return 0;
        }
        g_active_mixer = m;
        return m;
    }
    return 0;
}

void zcsr_mix_stop(zcsr_mixer* m) {
    if (!m || !m->in_use) return;
    atomic_store_explicit(&m->running, false, memory_order_release);
    if (m->thread_started) {
#if defined(_WIN32)
        WaitForSingleObject(m->thread, INFINITE);
        CloseHandle(m->thread);
#else
        pthread_join(m->thread, 0);
#endif
    }
    if (g_active_mixer == m) g_active_mixer = 0;
    *m = (zcsr_mixer){ 0 };
}

bool zcsr_mix_register_wav(zcsr_mixer* m, const char* id, const char* wav_path) {
    if (!m || !id || !wav_path || zcsr_mix_find_sound(m, id) >= 0) return false;
    for (int i = 0; i < ZCSR_MIX_MAX_SOUNDS; ++i) {
        zcsr_mix_sound candidate = { 0 };
        if (m->sounds[i].used) continue;
        if (!zcsr_mix_copy(candidate.id, sizeof candidate.id, id)) return false;
        if (!zcsr_mix_load_wav(m, &candidate, wav_path)) return false;
        candidate.used = true;
        m->sounds[i] = candidate;
        return true;
    }
    return false;
}

int zcsr_mix_play(zcsr_mixer* m, const char* id, float volume, bool loop) {
    int sound = zcsr_mix_find_sound(m, id);
    zcsr_mix_cmd cmd;
    if (!m || sound < 0) return -1;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    for (int c = 0; c < ZCSR_MIX_CHANNELS; ++c) {
        int expected = ZCSR_MIX_CH_FREE;
        if (!atomic_compare_exchange_strong(&m->channels[c].state, &expected, ZCSR_MIX_CH_RESERVED)) continue;
        cmd = (zcsr_mix_cmd){ ZCSR_MIX_CMD_PLAY, c, sound, volume, loop, { 0 } };
        if (!zcsr_mix_ring_push(m, &cmd)) {
            atomic_store(&m->channels[c].state, ZCSR_MIX_CH_FREE);
            return -1;
        }
        return c;
    }
    return -1;
}

void zcsr_mix_stop_channel(zcsr_mixer* m, int channel) {
    zcsr_mix_cmd cmd;
    if (!m || channel < 0 || channel >= ZCSR_MIX_CHANNELS) return;
    cmd = (zcsr_mix_cmd){ ZCSR_MIX_CMD_STOP_CHANNEL, channel, -1, 0.0f, false, { 0 } };
    (void)zcsr_mix_ring_push(m, &cmd);
}

bool zcsr_mix_slot_play(const char* payload) {
    return g_active_mixer && zcsr_mix_play(g_active_mixer, payload, 1.0f, false) >= 0;
}

bool zcsr_mix_slot_stop(const char* payload) {
    zcsr_mix_cmd cmd;
    if (!g_active_mixer || !zcsr_mix_copy(cmd.id, sizeof cmd.id, payload)) return false;
    cmd.type = ZCSR_MIX_CMD_STOP_ID;
    cmd.channel = -1;
    cmd.sound = -1;
    cmd.volume = 0.0f;
    cmd.loop = false;
    return zcsr_mix_ring_push(g_active_mixer, &cmd);
}

size_t zcsr_mix_poll_finished(zcsr_mixer* m, int* out, size_t cap) {
    size_t n = 0;
    if (!m || !out) return 0;
    while (n < cap) {
        unsigned head = atomic_load_explicit(&m->finished_head, memory_order_relaxed);
        unsigned tail = atomic_load_explicit(&m->finished_tail, memory_order_acquire);
        if (head == tail) break;
        out[n++] = m->finished[head];
        atomic_store_explicit(&m->finished_head, (head + 1U) % ZCSR_MIX_FINISHED_CAP, memory_order_release);
    }
    return n;
}
