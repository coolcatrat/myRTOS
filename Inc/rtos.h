/*
 * rtos.h
 *
 *  Public API + shared types for the devvRTOS kernel.
 *  Created on: 27-May-2026
 *      Author: devvr
 */

#ifndef RTOS_H_
#define RTOS_H_

#include <stdint.h>
#include "config.h"
#include "allocator.h"

/* ---- Task control block ---- */
typedef struct {
    uint32_t *psp;          // saved process stack pointer for this task
    uint32_t  wake_tick;    // tick at which a BLOCKED task becomes READY (osDelay)
    uint8_t   state;        // holds a task_state_t, narrowed to 1 byte to keep the TCB small
    uint8_t   priority;     // static priority
    // to be added later: whatever semaphore/queue it's waiting on
} tcb_t;

/* ---- Task states ---- */
typedef enum {
    TASK_READY   = 0,
    TASK_RUNNING = 1,
    TASK_BLOCKED = 2,
} task_state_t;

/* A task is just a function that takes nothing and never returns. */
typedef void (*task_func_t)(void);

/* ---- Kernel state shared with asm / main ---- */
extern tcb_t          tcbs[];        // the TCB table (exported mainly for debugger visibility)
extern tcb_t         *current_tcb;   // running task — referenced by SVC/PendSV asm
extern volatile uint32_t tick_count; // SysTick time base — read by main's delay_ms

/* ---- Public API ---- */
void SysTick_Handler(void);
void osDelay(uint32_t ticks);
void rtos_init(void);
void rtos_start(void);

int  task_create(task_func_t func, uint8_t priority, uint32_t stack_words);

#endif /* RTOS_H_ */