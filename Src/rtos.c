/*
 * rtos.c
 *
 *  Created on: 27-May-2026
 *      Author: devvr
 */

#include "rtos.h"
#include "stm32f401xe.h"
#define NUM_TASKS 2
#define STACK_SIZE 128 //words

static uint32_t task_a_stack[STACK_SIZE];
static uint32_t task_b_stack[STACK_SIZE];

tcb_t tcbs[NUM_TASKS];


// rtos.c

volatile uint32_t task_a_counter = 0;
volatile uint32_t task_b_counter = 0;

void task_a(void) {
    while (1) {
        task_a_counter++;

        // crude visible delay so the LED blink is perceptible
        for (volatile int i = 0; i < 1000000; i++);

        GPIOA->ODR ^= (1 << 5);   // toggle PA5 (onboard LED)
    }
}

void task_b(void) {
    while (1) {
        task_b_counter++;
    }
}
static void task_exit_error(void) {
    while (1);  // breakpoint here catches tasks that wrongly return
}
//│  xpsr    │ ┐
//│  pc      │ │
//│  lr      │ │
//│  r12     │ │ ← hardware pushed these (automatic)
//│  r3      │ │
//│  r2      │ │
//│  r1      │ │
//│  r0      │ ┘
//│  r11     │ ┐
//│  r10     │ │
//│  r9      │ │
//│  r8      │ │ ← your PendSV pushed these (manual)
//│  r7      │ │
//│  r6      │ │
//│  r5      │ │
//│  r4			stack top
void task_init(tcb_t *tcb, task_func_t func, uint32_t *stack_base, uint32_t stack_size) {
    // Fill r0-r3, r12 (hardware frame markers) and r4-r11 (software frame) with marker
    for (int i = 4; i <= 16; i++) {
        stack_base[stack_size - i] = 0xDEADBEEF;
    }

    stack_base[stack_size - 1] = 0x01000000;                  // xPSR (Thumb bit)
    stack_base[stack_size - 2] = (uint32_t)func;              // pc
    stack_base[stack_size - 3] = (uint32_t)task_exit_error;   // lr

    tcb->psp = &stack_base[stack_size - 16];
}

void rtos_init(void) {
    task_init(&tcbs[0], task_a, task_a_stack, STACK_SIZE);
    task_init(&tcbs[1], task_b, task_b_stack, STACK_SIZE);
}

void scheduler_start(void){
	__asm volatile(
			"ldr r0, =tcbs		\n"	// r0 contains address of tcb[0]
			"ldr r1, [r0]		\n" // r1 contains tcb[0].psp
			"ldr r2, [r1, #56]	\n" // load pc value into r2
			"add r1, r1, #64	\n"
			"msr psp, r1		\n"
			"mov r0, #2			\n"
			"msr control, r0	\n"
			"isb				\n"	// pipeline flush
			"bx r2				\n"	// jump to function
			:
			:
			:"r0","r1","r2"
	);

}
