/* Agent 5 — render tests: texture cache (init guards, path dedupe, bad-path, PNG decode) +
 * sprite-batch init guards. PNG decode uses an embedded 2x2 RGBA fixture (no external asset).
 * The 200-sprite blit timing is a GUI-host gate (integration/bench_render.c) — a valid batch needs
 * a real surface, so submit/flush aren't reachable on a headless box. */
#include "zcsr/texture.h"
#include "zcsr/sprite.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static int fails = 0;
#define CK(label, cond) do { if (!(cond)) { printf("FAIL: %s\n", label); fails++; } } while (0)

/* Minimal valid 2x2 RGBA PNG (red, green / blue, white); all chunk CRCs verified. */
static const unsigned char PNG_2x2[] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x08,0x06,0x00,0x00,0x00,0x72,0xb6,0x0d,
0x24,0x00,0x00,0x00,0x12,0x49,0x44,0x41,0x54,0x78,0xda,0x63,0xf8,0xcf,0xc0,0xf0,
0x1f,0x0c,0x81,0x34,0x18,0x00,0x00,0x49,0xc8,0x09,0xf7,0x03,0xd9,0x64,0xf1,0x00,
0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

static bool write_fixture(const char* path) {
    FILE* f = fopen(path, "wb");
    size_t n;
    if (!f) return false;
    n = fwrite(PNG_2x2, 1, sizeof PNG_2x2, f);
    fclose(f);
    return n == sizeof PNG_2x2;
}

static void test_cache(void) {
    static unsigned char buf[1 << 20]; /* 1 MiB: cache header + bump arena */
    CK("cache init null -> NULL", zcsr_texture_cache_init(NULL, sizeof buf) == NULL);
    CK("cache init tiny -> NULL", zcsr_texture_cache_init(buf, 8) == NULL);

    zcsr_texture_cache* c = zcsr_texture_cache_init(buf, sizeof buf);
    CK("cache init valid -> non-NULL", c != NULL);

    CK("load NULL path -> NULL", zcsr_texture_load(c, NULL) == NULL);
    CK("load bad path -> NULL",  zcsr_texture_load(c, "zcsr_no_such_file.png") == NULL);

    const char* p1 = "zcsr_test_render_a.png";
    const char* p2 = "zcsr_test_render_b.png";
    CK("fixture A written", write_fixture(p1));
    CK("fixture B written", write_fixture(p2));

    const zcsr_texture* t1 = zcsr_texture_load(c, p1);
    CK("load A -> non-NULL", t1 != NULL);
    if (t1) {
        CK("A width == 2",    t1->width == 2);
        CK("A height == 2",   t1->height == 2);
        CK("A rgba non-NULL", t1->rgba != NULL);
    }

    const zcsr_texture* t1b = zcsr_texture_load(c, p1);
    CK("dedupe: same path -> same pointer", t1 != NULL && t1 == t1b);

    const zcsr_texture* t2 = zcsr_texture_load(c, p2);
    CK("distinct path -> different pointer", t2 != NULL && t2 != t1);

    remove(p1);
    remove(p2);
}

static void test_batch_init(void) {
    static unsigned char buf[512 * 1024]; /* > sizeof(opaque batch); isolates the target check */
    int dummy;
    zcsr_surface* fake = (zcsr_surface*)&dummy; /* only stored by init; never dereferenced here */
    CK("batch init null buffer -> NULL", zcsr_sprite_batch_init(NULL, sizeof buf, fake) == NULL);
    CK("batch init tiny -> NULL",        zcsr_sprite_batch_init(buf, 8, fake) == NULL);
    CK("batch init null target -> NULL", zcsr_sprite_batch_init(buf, sizeof buf, NULL) == NULL);
    /* A usable batch requires a real surface (flush blits onto it) -> see the GUI bench. */
}

int main(void) {
    test_cache();
    test_batch_init();
    if (!fails) printf("test_render: PASS (texture cache dedupe/decode + batch init guards)\n");
    return fails ? 1 : 0;
}
