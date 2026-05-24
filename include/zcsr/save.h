#ifndef ZCSR_SAVE_H
#define ZCSR_SAVE_H
/* Agent 4 (Game Core) — binary save/load of game state. No heap, no file I/O in the contract
 * (caller persists the bytes). Writes a small header (magic + version + length + checksum) so
 * loads can be validated. Implementer: modules/gamelogic. */
#include <stddef.h>
#include <stdint.h>

/* Serialize `state` (state_size bytes) into `out` (out_cap bytes) with header + checksum.
 * Returns total bytes written, or 0 on failure (e.g., out_cap too small). */
size_t zcsr_save_write(const void* state, size_t state_size,
                       void* out, size_t out_cap, uint32_t version);

/* Validate `in` (in_size bytes) and copy the payload into `state` (state_cap bytes).
 * Returns the payload size on success, or 0 if magic/version/checksum/length is invalid. */
size_t zcsr_save_read(const void* in, size_t in_size,
                      void* state, size_t state_cap, uint32_t expect_version);

#endif /* ZCSR_SAVE_H */
