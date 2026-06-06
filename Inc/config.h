/*
 * config.h
 *
 *  Central place for kernel tunables and fixed hardware constants.
 *  Touch the "Tunables" section to resize/retune the kernel.
 *  Do NOT touch the "Architecture constants" section — those are
 *  facts about the Cortex-M4, not knobs.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/* ============================================================ */
/*  Tunables — safe to change                                   */
/* ============================================================ */

#define MAX_TASKS         16        // how many task slots the TCB table has
#define POOL_SIZE_WORDS   1024      // kalloc memory pool size, in words (4 bytes each)
#define MIN_STACK_WORDS   16        // smallest legal stack = one full frame, no headroom
#define SYSTICK_RELOAD    16000     // SysTick counts this many cycles per tick (~1ms @ 16MHz)

/* Budget note:
 * Every task stack comes out of the kalloc pool.
 * MAX_TASKS stacks must all fit inside POOL_SIZE_WORDS.
 * e.g. 16 tasks * 64-word stacks = 1024 words = the whole pool.
 * Oversubscribe and kalloc just returns 0 -> task_create returns -1. */


/* ============================================================ */
/*  Cortex-M4 architecture constants — do NOT change            */
/* ============================================================ */

/* Exception stack frame layout (no FPU context). */
#define HW_FRAME_WORDS    8         // r0-r3, r12, lr, pc, xPSR  (CPU auto-stacks these)
#define SW_FRAME_WORDS    8         // r4-r11                    (PendSV stacks these)
#define FULL_FRAME_WORDS  16        // the whole fabricated frame we build in task_init

/* Magic values the hardware expects. */
#define INITIAL_XPSR      0x01000000  // only the Thumb bit set — Cortex-M is Thumb-only
#define EXC_RETURN_PSP    0xFFFFFFFD  // "return to Thread mode, use the PSP stack"
                                      // (used as a literal inside the SVC asm)

#endif /* CONFIG_H_ */