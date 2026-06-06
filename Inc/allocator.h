/*
 * allocator.h
 *
 *  Public contract for the kernel's static memory allocator.
 *  Callers only ever see kalloc — the pool and the bump logic are
 *  hidden inside allocator.c. Swap the method later (free-list, TLSF)
 *  without touching a single caller.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>

/* Allocate `words` 32-bit words from the kernel's static pool.
 * Returns an 8-byte-aligned pointer, or 0 if over budget.
 * No free — allocation is permanent. */
uint32_t *kalloc(uint32_t words);

#endif /* ALLOCATOR_H */