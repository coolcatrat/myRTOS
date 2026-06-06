/*
 * allocator.c
 *
 *  Classic bump allocator.
 *  One fixed pool, a cursor that only moves forward, never frees.
 *  O(1), no fragmentation, predictable failure (returns 0 when full).
 *  This is the heap_1 model from FreeRTOS.
 */

#include <stdint.h>
#include "config.h"

/* ---- File-private state (invisible outside this file) ---- */

static uint32_t pool_used = 0;                                  // words handed out so far
static uint32_t memory_pool[POOL_SIZE_WORDS] __attribute__((aligned(8)));
//                                            ^ force the pool's base to an 8-byte boundary,
//                                              so an even offset always lands 8-aligned.

uint32_t *kalloc(uint32_t words) {
    words = (words + 1) & ~1u;                  // round request up to an even number of words
                                                // -> base stays 8-aligned, size stays even

    if (words + pool_used > POOL_SIZE_WORDS)    // not enough room left
        return 0;                               // caller must check for 0

    uint32_t *ptr = &memory_pool[pool_used];    // carve from the current cursor
    pool_used += words;                         // bump the cursor past it
    return ptr;
}