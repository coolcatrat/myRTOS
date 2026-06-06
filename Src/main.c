/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */

#include <stdint.h>
#include "rtos.h"
#include "stm32f401xe.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU."
#endif


// Busy-wait helper for non-task code. Tasks should use osDelay instead.
void delay_ms(uint32_t ms) {
    uint32_t start = tick_count;
    while ((tick_count - start) < ms);
}

int main(void) {
    // rtos_init sets up PendSV, creates the idle task, starts SysTick,
    // and launches the first task. It does not return.
    rtos_init();

    while (1) {
        // never reached
    }
}