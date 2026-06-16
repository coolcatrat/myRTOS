/*
 * rtos.c
 *
 *  devvRTOS kernel core: scheduler (policy), context switch (mechanism),
 *  task creation, and the SysTick time base.
 *  Created on: 27-May-2026
 *      Author: devvr
 */

#include <stdint.h>
#include "rtos.h"
#include "config.h"
#include "allocator.h"
#include "scheduler.h"
#include "critical.h"
#include "stm32f401xe.h"


/* ---- Forward declarations (file-private helpers) ---- */
static void task_init(tcb_t *tcb, task_func_t func, uint32_t *stack_base, uint32_t stack_size);
static void idle_task(void);
static void task_exit_error(void);
static void systick_init(uint32_t reload);
void        scheduler(void);   // not static: PendSV asm reaches it as a global symbol


/* ---- Kernel state ---- */
/* exported (extern in rtos.h): */
volatile uint32_t tick_count;          // SysTick time base
tcb_t             tcbs[MAX_TASKS];      // the TCB table
tcb_t            *current_tcb;          // currently running task

/* private to this file: */
static uint32_t   num_tasks = 0;        // tasks created so far (append-only)


/* ============================================================ */
/*  Time base                                                   */
/* ============================================================ */

// Fires every tick. Wakes any task whose delay is up, then asks for a switch.
void SysTick_Handler(void) {
    tick_count++;

    for (int i = 0; i < num_tasks; i++) {
        tcb_t *cur = &tcbs[i];
        if (cur->state == TASK_BLOCKED && cur->wake_tick <= tick_count && cur->blocked_on == NULL){ // check if a task is ready
            cur->state = TASK_READY;
            scheduler_ready_add(cur);
        } 
    }

    os_yield_to_scheduler();          // request a context switch
}

// Block the calling task for `ticks` ticks, then give up the CPU now.
void osDelay(uint32_t ticks) {
    uint32_t old = os_enter_critical();
    current_tcb->wake_tick = tick_count + ticks;   // when to wake back up
    current_tcb->state     = TASK_BLOCKED;         // take self out of contention
    current_tcb->blocked_on = NULL;
    os_yield_to_scheduler();                       // yield immediately
    os_exit_critical(old);
}


/* ============================================================ */
/*  Scheduler — POLICY (swappable later)                        */
/* ============================================================ */

// Calls scheduler APIs
void scheduler(void) {
    if (current_tcb->state != TASK_BLOCKED)  {      // don't undo a self-block
        current_tcb->state = TASK_READY;
        scheduler_ready_add(current_tcb);           // if task is not blocked, add it back to queue
    }

    current_tcb = scheduler_next();         // call api to get next task
    current_tcb->state = TASK_RUNNING;
}


/* ============================================================ */
/*  Task creation                                               */
/* ============================================================ */

// Create a task. stack_words must be at least MIN_STACK_WORDS (one full frame).
// Returns the new task id, or -1 on failure (no slots / out of memory / stack too small).
int task_create(task_func_t func, uint8_t priority, uint32_t stack_words) {
    if (num_tasks >= MAX_TASKS)
        return -1;                  // task table full — raise MAX_TASKS

    if (stack_words < MIN_STACK_WORDS)
        return -1;                  // can't even fit the exception frame

    uint32_t *stack_base = kalloc(stack_words);
    if (stack_base == 0)
        return -1;                  // pool exhausted — nothing to allocate

    tcb_t *tcb    = &tcbs[num_tasks];
    tcb->state    = TASK_READY;
    tcb->priority = priority;
    tcb->queued   = 0;

    task_init(tcb, func, stack_base, stack_words);
    scheduler_ready_add(tcb);       // add task to queue
    num_tasks++;                    // commit only after full success
    return num_tasks - 1;           // task id
}


/* ============================================================ */
/*  Startup                                                     */
/* ============================================================ */

// Lowest-priority task: runs only when nothing else is READY.
static void idle_task(void) {
    while (1) { }
}

// Bring the kernel up: PendSV priority + idle task. Does NOT start ticking yet.
void rtos_init(void) {
    SCB->SHP[10] = 0xFF;                          // PendSV = lowest priority
    scheduler_init();
    NVIC_SetPriority(PendSV_IRQn,15);
    NVIC_SetPriority(SysTick_IRQn,15);
    task_create(idle_task, 0, MIN_STACK_WORDS);   // slot 0: always-READY fallback
}

// Enable the tick and launch the first task. Does not return.
void rtos_start(void) {
    current_tcb = scheduler_next();// start on idle; first tick switches to a real task
    current_tcb->state = TASK_RUNNING;
    systick_init(SYSTICK_RELOAD);  // tick on ONLY now — all tasks already exist
    __asm volatile("svc 0");       // launch
}

// Configure and enable SysTick. Kept private — only rtos_init calls it.
static void systick_init(uint32_t reload) {
    SysTick->LOAD = reload - 1;   // counter reloads from here
    SysTick->VAL  = 0;            // start the count at 0
    SysTick->CTRL = 0x7;          // enable counter + tick interrupt + processor clock
}


/* ============================================================ */
/*  Context switch — MECHANISM                                  */
/* ============================================================ */

// A task that returns lands here. Trap so the bug is obvious in the debugger.
static void task_exit_error(void) {
    while (1);
}

// Fabricate a fresh stack frame so the task looks like it was already running
// and got interrupted. The very first switch into it "restores" this fake frame.
static void task_init(tcb_t *tcb, task_func_t func, uint32_t *stack_base, uint32_t stack_size) {
    // Fill the whole 16-word frame (r0-r3, r12, r4-r11) with a marker so a wrong
    // restore is easy to spot in memory.
    for (int i = 4; i <= FULL_FRAME_WORDS; i++) {
        stack_base[stack_size - i] = 0xDEADBEEF;
    }

    stack_base[stack_size - 1] = INITIAL_XPSR;              // xPSR — Thumb bit set
    stack_base[stack_size - 2] = (uint32_t)func;            // pc  — where the task starts
    stack_base[stack_size - 3] = (uint32_t)task_exit_error; // lr  — where it goes if it returns

    tcb->psp = &stack_base[stack_size - FULL_FRAME_WORDS];  // top of the saved frame
}

// SVC handler: launches the very first task (current_tcb).
// Loads its fake frame and exception-returns into Thread mode on the PSP stack.
__attribute__((naked)) void SVC_Handler(void) {
    // 'naked' = compiler adds no prologue/epilogue; the asm below is the whole function.
    __asm volatile(
        "ldr r0, =current_tcb   \n" // r0 = &current_tcb
        "ldr r1, [r0]           \n" // r1 = current_tcb (the tcb pointer)
        "ldr r0, [r1]           \n" // r0 = current_tcb->psp (points at saved r4)
        "add r0, r0, #32        \n" // skip the 8 software-saved words (HW_FRAME_WORDS*4 = 32 bytes)
        "msr psp, r0            \n" // PSP now points at the hardware frame
        "ldr lr, =0xFFFFFFFD    \n" // EXC_RETURN_PSP: return to Thread mode, use PSP
        "isb                    \n" // flush the pipeline before the mode switch
        "bx lr                  \n" // exception-return -> the CPU pops the hardware frame
    );
}

// PendSV handler: the actual context switch (save current, pick next, restore next).
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile(
        // --- save current task's r4-r11 onto its stack ---
        "mrs r0, psp            \n"
        "stmdb r0!, {r4-r11}    \n" // r0 = new top of stack after pushing r4-r11

        "ldr r1, =current_tcb   \n" // r1 = &current_tcb
        "ldr r1, [r1]           \n" // r1 = current_tcb
        "str r0, [r1]           \n" // current_tcb->psp = r0   (save it)

        // --- pick the next task (in C, protecting lr/EXC_RETURN) ---
        "push {lr}              \n"
        "bl scheduler           \n"
        "pop {lr}               \n"

        // --- restore next task's r4-r11 from its stack ---
        "ldr r0, =current_tcb   \n" // r0 = &current_tcb
        "ldr r0, [r0]           \n" // r0 = current_tcb (now the new one)
        "ldr r1, [r0]           \n" // r1 = current_tcb->psp

        "ldmia r1!, {r4-r11}    \n" // pop r4-r11 back into the CPU
        "msr psp, r1            \n" // PSP points at the hardware frame
        "isb                    \n"
        "bx lr                  \n" // exception-return -> CPU pops the hardware frame
    );
}

/*
    SYNCHRONIZATION: Scan tcbs[] to find highest priority waiter on a lock
*/
tcb_t *highest_priority_waiter(void *sem)
{
    tcb_t *best = NULL;
    for (uint32_t i = 0; i < num_tasks; i++) {
        if (tcbs[i].state == TASK_BLOCKED && tcbs[i].blocked_on == sem) {
            if (best == NULL || tcbs[i].priority > best->priority) {
                best = &tcbs[i];
            }
        }
    }
    return best;
}
