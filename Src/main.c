/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : devvRTOS demo — three counter tasks, watch them live
 ******************************************************************************
 */

#include <stdint.h>
#include "rtos.h"
#include "stm32f401xe.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU."
#endif


/*
Counter variables to track in debugger mode
a and b should climb equally, c should lag*/
volatile uint32_t counter_a = 0;
volatile uint32_t counter_b = 0;
volatile uint32_t counter_c = 0;


// Pure spinner. Just counts as fast as it gets scheduled.
static void task_a(void) {
    while (1) {
        counter_a++;
    }
}

// Pure spinner. Same as A — used to compare scheduling shares.
static void task_b(void) {
    while (1) {
        counter_b++;
    }
}

// Counts, then sleeps. osDelay blocks it, so its counter crawls
// compared to the spinners — this is the BLOCKED/READY path in action.
static void task_c(void) {
    while (1) {
        counter_c++;
    }
}


int main(void) {
    rtos_init();                    // kernel setup + idle task (slot 0)

    // task_create(function, priority, stack words)
    task_create(task_a, 1, 64);     // 64 words = 256 bytes, plenty for these
    task_create(task_b, 2, 64);
    task_create(task_c, 3, 64);

    rtos_start();                   // enable SysTick + launch — never returns

    while (1) { }                   // unreachable
}