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
tcb_t *current_tcb;
static uint32_t current_index;
// rtos.c

volatile uint32_t task_a_counter = 0;
volatile uint32_t task_b_counter = 0;

void scheduler(void) {                       // POLICY — swappable later
    current_index = (current_index + 1) % NUM_TASKS;
    current_tcb   = &tcbs[current_index];
}

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
	SCB->SHP[10] = 0xFF;		// keep PendSV lowest priority ISR so it does not interrupt other ISR's
    task_init(&tcbs[0], task_a, task_a_stack, STACK_SIZE);
    task_init(&tcbs[1], task_b, task_b_stack, STACK_SIZE);
}

void scheduler_start(void){
	current_tcb = &tcbs[0];
	__asm volatile("svc 0");
}
__attribute__((naked)) void SVC_Handler(void){	//naked attribute tells the compiler to not run any extra instructions.
	// launch task a. load r4- r11 from fabricated stack into registers. then set LR: 0xFFFFFFFD
	// to set CPU to thread mode, and use PSP stack for the hardware pop.
	__asm volatile(
			"ldr r0, =current_tcb	\n"	// r0 has address of current_tcb
			"ldr r1, [r0]			\n"	// r1 has address of the tcb
			"ldr r0, [r1]			\n"	// r0 has the process stack pointer, currently pointing at r4
			"add r0, r0, #32		\n"	// make pointer point to hardware pop frame
			"msr psp, r0			\n" // move updated value to psp
			"ldr lr, =0xFFFFFFFD	\n"	// set LR
			"isb					\n"	// flush pipline
			"bx lr					\n"	// return
	);
}
__attribute__((naked)) void PendSV_Handler(void){
	__asm volatile(
			// save register r4-r11 into process stack
			// update current task's tcb
			// get next task tcb
			// get the psp for the new task
			// unload r4-r11 into cpu from the new process stack
			// EXEC_RETURN, set new psp
			"mrs r0, psp			\n"
			"stmdb r0!, {r4-r11}	\n"	// r0 stores the address of psp after pushing the registers

			"ldr r1, =current_tcb	\n"	// r1 contains address of pointer current_tcb
			"ldr r1, [r1]			\n"	// r1 contains address of current_tcb->psp
			"str r0, [r1]			\n"	// current_tcb->psp = r0

			"push {lr}				\n"
			"bl scheduler			\n"
			"pop {lr}				\n"

			"ldr r0, =current_tcb 	\n"	// r0 = &current_tcb
			"ldr r0, [r0] 			\n"	// r0 = current_tcb
			"ldr r1, [r0] 			\n"	// r1 = current_tcb->psp

			"ldmia r1!, {r4-r11}	\n"	// unload r4-r11 from stack into the cpu registers
			"msr psp, r1			\n"
			"isb					\n"
			"bx lr					\n"



	);
}

