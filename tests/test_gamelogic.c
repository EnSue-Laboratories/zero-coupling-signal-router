/* Agent 5 — gamelogic tests: AABB collision + binary save/load (round-trip + reject paths). */
#include "zcsr/collision.h"
#include "zcsr/save.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int fails = 0;
#define CK(label, cond) do { if (!(cond)) { printf("FAIL: %s\n", label); fails++; } } while (0)

static void test_collision(void) {
    zcsr_aabb a = { 0, 0, 10, 10 };
    CK("overlap",                  zcsr_aabb_intersect(a, (zcsr_aabb){ 5, 5, 10, 10 }) == true);
    CK("separate",                 zcsr_aabb_intersect(a, (zcsr_aabb){ 20, 20, 5, 5 }) == false);
    CK("touching edge = no overlap", zcsr_aabb_intersect(a, (zcsr_aabb){ 10, 0, 5, 5 }) == false);
    CK("fully contained",          zcsr_aabb_intersect(a, (zcsr_aabb){ 2, 2, 3, 3 })  == true);
    CK("zero-size = false",        zcsr_aabb_intersect(a, (zcsr_aabb){ 5, 5, 0, 0 })  == false);
    CK("neg-size = false",         zcsr_aabb_intersect((zcsr_aabb){ 0, 0, -1, 5 }, a) == false);

    CK("point inside",             zcsr_aabb_contains_point(a, 5, 5) == true);
    CK("point outside",            zcsr_aabb_contains_point(a, 11, 5) == false);
    CK("point top-left inclusive", zcsr_aabb_contains_point(a, 0, 0) == true);
    CK("point bottom-right exclusive", zcsr_aabb_contains_point(a, 10, 10) == false);
}

static void test_save(void) {
    const char payload[] = "game-state: gold=276 pop=30/200 age=castle";
    const size_t ps = sizeof payload; /* includes trailing NUL */
    uint8_t buf[128];
    uint8_t loaded[128];

    size_t written = zcsr_save_write(payload, ps, buf, sizeof buf, 7);
    CK("write returns header+size", written == 20 + ps);

    size_t read = zcsr_save_read(buf, written, loaded, sizeof loaded, 7);
    CK("read returns payload size",   read == ps);
    CK("round-trip payload matches",  read == ps && memcmp(loaded, payload, ps) == 0);

    /* corrupt one payload byte -> checksum reject */
    uint8_t save25 = buf[25]; buf[25] ^= 0xFFu;
    CK("corrupt payload rejected", zcsr_save_read(buf, written, loaded, sizeof loaded, 7) == 0);
    buf[25] = save25;

    /* bad magic */
    uint8_t save0 = buf[0]; buf[0] ^= 0xFFu;
    CK("bad magic rejected", zcsr_save_read(buf, written, loaded, sizeof loaded, 7) == 0);
    buf[0] = save0;

    /* version mismatch */
    CK("version mismatch rejected", zcsr_save_read(buf, written, loaded, sizeof loaded, 8) == 0);

    /* reserved field != 0 */
    uint8_t save16 = buf[16]; buf[16] = 1u;
    CK("reserved!=0 rejected", zcsr_save_read(buf, written, loaded, sizeof loaded, 7) == 0);
    buf[16] = save16;

    /* truncated input (smaller than header) */
    CK("truncated input rejected", zcsr_save_read(buf, 10, loaded, sizeof loaded, 7) == 0);

    /* out buffer too small on write (room for header but not payload) */
    uint8_t small[20];
    CK("write too-small out -> 0", zcsr_save_write(payload, ps, small, sizeof small, 7) == 0);

    /* state_cap too small on read */
    CK("read state_cap too small -> 0", zcsr_save_read(buf, written, loaded, ps - 1, 7) == 0);
}

int main(void) {
    test_collision();
    test_save();
    if (!fails) printf("test_gamelogic: PASS (AABB + save/load round-trip & reject paths)\n");
    return fails ? 1 : 0;
}
