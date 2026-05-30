/*
 * rtos.h
 *
 *  Created on: 27-May-2026
 *      Author: devvr
 */

#ifndef RTOS_H_
#define RTOS_H_

#include <stdint.h>

typedef struct {
    uint32_t *psp;
} tcb_t;

typedef void (*task_func_t)(void);

extern tcb_t tcbs[];
extern tcb_t *current_tcb;
//extern uint32_t task_a_stack[];
//extern uint32_t task_b_stack[];
extern volatile uint32_t task_a_counter;
extern volatile uint32_t task_b_counter;

void scheduler_start(void);

void task_a(void);
void task_b(void);
void task_init(tcb_t *tcb, task_func_t func, uint32_t *stack_base, uint32_t stack_size);
void rtos_init(void);

#endif /* RTOS_H_ */
