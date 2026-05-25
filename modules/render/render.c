/* Agent 3 (Game Core) — Rendering & Resources: stb_image PNG load + cache, batch sprite renderer.
 * Includes ONLY shared contracts. */
#include "zcsr/texture.h"
#include "zcsr/sprite.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum {
    ZCSR_TEXTURE_CACHE_CAP = 32,
    ZCSR_TEXTURE_PATH_MAX = 191,
    ZCSR_SPRITE_BATCH_CAP = 10000
};

typedef struct {
    bool in_use;
    char path[ZCSR_TEXTURE_PATH_MAX + 1];
    zcsr_texture texture;
} zcsr_texture_entry;

struct zcsr_texture_cache {
    zcsr_texture_entry entries[ZCSR_TEXTURE_CACHE_CAP];
    unsigned char* arena;
    size_t capacity;
    size_t offset;
};

struct zcsr_sprite_batch {
    zcsr_surface* target;
    zcsr_sprite sprites[ZCSR_SPRITE_BATCH_CAP];
    size_t count;
};

static zcsr_texture_cache* zcsr_active_cache;

static size_t zcsr_align_up(size_t value, size_t align) {
    size_t mask = align - 1u;
    return (value + mask) & ~mask;
}

static void* zcsr_cache_alloc(zcsr_texture_cache* cache, size_t bytes, size_t align) {
    size_t offset;
    if (!cache || bytes == 0) return 0;
    if (align == 0) align = sizeof(void*);
    offset = zcsr_align_up(cache->offset, align);
    if (offset > cache->capacity || bytes > cache->capacity - offset) return 0;
    cache->offset = offset + bytes;
    return cache->arena + offset;
}

static void* zcsr_stbi_malloc(size_t bytes) {
    return zcsr_cache_alloc(zcsr_active_cache, bytes, sizeof(void*));
}

static void zcsr_stbi_free(void* ptr) {
    (void)ptr; /* Scratch allocations live in the caller-provided cache buffer. */
}

static void* zcsr_stbi_realloc_sized(void* ptr, size_t old_size, size_t new_size) {
    unsigned char* dst = (unsigned char*)zcsr_stbi_malloc(new_size);
    unsigned char* src = (unsigned char*)ptr;
    size_t n = old_size < new_size ? old_size : new_size;
    if (!dst) return 0;
    if (src) {
        for (size_t i = 0; i < n; ++i) dst[i] = src[i];
    }
    return dst;
}

#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MALLOC(sz) zcsr_stbi_malloc(sz)
#define STBI_REALLOC(p, sz) zcsr_stbi_realloc_sized((p), 0, (sz))
#define STBI_REALLOC_SIZED(p, oldsz, newsz) zcsr_stbi_realloc_sized((p), (oldsz), (newsz))
#define STBI_FREE(p) zcsr_stbi_free(p)
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "../../third_party/stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static bool zcsr_copy_path(char* dst, const char* src) {
    size_t i = 0;
    if (!dst || !src) return false;
    for (; i < ZCSR_TEXTURE_PATH_MAX && src[i] != '\0'; ++i) dst[i] = src[i];
    if (src[i] != '\0') return false;
    dst[i] = '\0';
    return true;
}

static bool zcsr_path_equal(const char* a, const char* b) {
    size_t i = 0;
    if (!a || !b) return false;
    while (a[i] == b[i]) {
        if (a[i] == '\0') return true;
        ++i;
    }
    return false;
}

static zcsr_texture_entry* zcsr_find_entry(zcsr_texture_cache* cache, const char* path) {
    if (!cache || !path) return 0;
    for (size_t i = 0; i < ZCSR_TEXTURE_CACHE_CAP; ++i) {
        if (cache->entries[i].in_use && zcsr_path_equal(cache->entries[i].path, path)) return &cache->entries[i];
    }
    return 0;
}

static zcsr_texture_entry* zcsr_free_entry(zcsr_texture_cache* cache) {
    if (!cache) return 0;
    for (size_t i = 0; i < ZCSR_TEXTURE_CACHE_CAP; ++i) {
        if (!cache->entries[i].in_use) return &cache->entries[i];
    }
    return 0;
}

static size_t zcsr_file_size(FILE* file) {
    long pos;
    long end;
    if (!file) return 0;
    pos = ftell(file);
    if (pos < 0) return 0;
    if (fseek(file, 0, SEEK_END) != 0) return 0;
    end = ftell(file);
    if (end < 0) return 0;
    if (fseek(file, pos, SEEK_SET) != 0) return 0;
    return (size_t)end;
}

static unsigned char* zcsr_read_file(zcsr_texture_cache* cache, const char* path, size_t* out_size) {
    FILE* file;
    size_t size;
    unsigned char* data;
    if (out_size) *out_size = 0;
    if (!cache || !path || !out_size) return 0;

    file = fopen(path, "rb");
    if (!file) return 0;
    size = zcsr_file_size(file);
    if (size == 0) {
        fclose(file);
        return 0;
    }

    data = (unsigned char*)zcsr_cache_alloc(cache, size, 1);
    if (!data) {
        fclose(file);
        return 0;
    }

    if (fread(data, 1, size, file) != size) {
        fclose(file);
        return 0;
    }

    fclose(file);
    *out_size = size;
    return data;
}

zcsr_texture_cache* zcsr_texture_cache_init(void* b, size_t n) {
    zcsr_texture_cache* cache;
    size_t header;
    if (!b || n < sizeof(zcsr_texture_cache) + 64u) return 0;
    cache = (zcsr_texture_cache*)b;
    *cache = (zcsr_texture_cache){ 0 };
    header = zcsr_align_up(sizeof(zcsr_texture_cache), sizeof(void*));
    cache->arena = ((unsigned char*)b) + header;
    cache->capacity = n - header;
    cache->offset = 0;
    return cache;
}

const zcsr_texture* zcsr_texture_load(zcsr_texture_cache* c, const char* path) {
    zcsr_texture_entry* entry;
    unsigned char* file_data;
    unsigned char* pixels;
    size_t file_size;
    int width = 0;
    int height = 0;
    int channels = 0;

    if (!c || !path) return 0;
    entry = zcsr_find_entry(c, path);
    if (entry) return &entry->texture;

    entry = zcsr_free_entry(c);
    if (!entry || !zcsr_copy_path(entry->path, path)) return 0;

    file_data = zcsr_read_file(c, path, &file_size);
    if (!file_data) return 0;

    zcsr_active_cache = c;
    pixels = stbi_load_from_memory(file_data, (int)file_size, &width, &height, &channels, 4);
    zcsr_active_cache = 0;
    if (!pixels || width <= 0 || height <= 0) return 0;

    entry->in_use = true;
    entry->texture.width = width;
    entry->texture.height = height;
    entry->texture.rgba = pixels;
    (void)channels;
    return &entry->texture;
}

zcsr_sprite_batch* zcsr_sprite_batch_init(void* b, size_t n, zcsr_surface* t) {
    zcsr_sprite_batch* batch;
    if (!b || n < sizeof(zcsr_sprite_batch) || !t) return 0;
    batch = (zcsr_sprite_batch*)b;
    *batch = (zcsr_sprite_batch){ 0 };
    batch->target = t;
    return batch;
}

void zcsr_sprite_batch_begin(zcsr_sprite_batch* s) {
    if (!s) return;
    s->count = 0;
}

void zcsr_sprite_batch_submit(zcsr_sprite_batch* s, const zcsr_sprite* sp) {
    if (!s || !sp || !sp->tex || !sp->tex->rgba) return;
    if (s->count >= ZCSR_SPRITE_BATCH_CAP) return;
    s->sprites[s->count++] = *sp;
}

#if defined(__linux__)
#include <X11/Xlib.h>

static void zcsr_sprite_batch_flush_x11(zcsr_sprite_batch* s, Display* display, Window window) {
    GC gc;
    if (!s || !display || !window) return;
    gc = XCreateGC(display, window, 0, 0);
    if (!gc) return;
    for (size_t i = 0; i < s->count; ++i) {
        const zcsr_sprite* sprite = &s->sprites[i];
        const uint8_t* px = sprite->tex->rgba;
        int x = (int)sprite->dst.x;
        int y = (int)sprite->dst.y;
        unsigned int w = sprite->dst.w > 0.0f ? (unsigned int)sprite->dst.w : (unsigned int)sprite->tex->width;
        unsigned int h = sprite->dst.h > 0.0f ? (unsigned int)sprite->dst.h : (unsigned int)sprite->tex->height;
        XSetForeground(display, gc, ((unsigned long)px[0] << 16) | ((unsigned long)px[1] << 8) | px[2]);
        XFillRectangle(display, window, gc, x, y, w, h);
    }
    XFlush(display);
    XFreeGC(display, gc);
}

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void zcsr_sprite_batch_flush_win32(zcsr_sprite_batch* s, HWND hwnd) {
    HDC dc;
    if (!s || !hwnd) return;
    dc = GetDC(hwnd);
    if (!dc) return;
    for (size_t i = 0; i < s->count; ++i) {
        const zcsr_sprite* sprite = &s->sprites[i];
        const uint8_t* px = sprite->tex->rgba;
        int x = (int)sprite->dst.x;
        int y = (int)sprite->dst.y;
        int w = sprite->dst.w > 0.0f ? (int)sprite->dst.w : sprite->tex->width;
        int h = sprite->dst.h > 0.0f ? (int)sprite->dst.h : sprite->tex->height;
        RECT rect = { x, y, x + w, y + h };
        HBRUSH brush = CreateSolidBrush(RGB(px[0], px[1], px[2]));
        if (brush) {
            FillRect(dc, &rect, brush);
            DeleteObject(brush);
        }
    }
    ReleaseDC(hwnd, dc);
}

#elif defined(__APPLE__)
#include "render_cocoa.h" /* same-module C-ABI shim over the Cocoa fill backend */

static void zcsr_sprite_batch_flush_cocoa(zcsr_sprite_batch* s, void* window) {
    if (!s || !window) return;
    zcsr_cocoa_render_begin(window);
    for (size_t i = 0; i < s->count; ++i) {
        const zcsr_sprite* sprite = &s->sprites[i];
        const uint8_t* px = sprite->tex->rgba;
        int x = (int)sprite->dst.x;
        int y = (int)sprite->dst.y;
        int w = sprite->dst.w > 0.0f ? (int)sprite->dst.w : sprite->tex->width;
        int h = sprite->dst.h > 0.0f ? (int)sprite->dst.h : sprite->tex->height;
        zcsr_cocoa_render_fill(window, x, y, w, h, px[0], px[1], px[2]);
    }
    zcsr_cocoa_render_end(window);
}
#endif

void zcsr_sprite_batch_flush(zcsr_sprite_batch* s) {
    void* handle;
    if (!s || !s->target) return;
    handle = zcsr_surface_native_handle(s->target);
    if (!handle) {
        s->count = 0;
        return;
    }
#if defined(__linux__)
    zcsr_sprite_batch_flush_x11(s, (Display*)zcsr_surface_native_display(s->target), (Window)(uintptr_t)handle);
#elif defined(_WIN32)
    zcsr_sprite_batch_flush_win32(s, (HWND)handle);
#elif defined(__APPLE__)
    zcsr_sprite_batch_flush_cocoa(s, handle);
#else
    (void)handle;
#endif
    s->count = 0;
}
