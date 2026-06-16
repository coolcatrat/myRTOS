/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : devvRTOS demo — button-signalled LED blinker + free counter
 ******************************************************************************
 */

#include <stdint.h>
#include "rtos.h"
#include "stm32f401xe.h"
#include "sync.h"

/* Free-running counter — climbs ONLY while led_blinker is blocked on the
 semaphore. If it keeps climbing between button presses, the blinker is
 truly asleep (good). If it stalls, the blinker is wrongly spinning. */

volatile uint32_t count = 0;
volatile uint32_t led_count = 0;
volatile uint32_t handler_count = 0;
semaphore_t button_sem;


/* ---- board setup --------------------*/
void led_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;     // LED LD2 is on PA5
    GPIOA->MODER &= ~GPIO_MODER_MODER5;
    GPIOA->MODER |=  GPIO_MODER_MODER5_0;     // 01 = output
}

void button_init(void)
{
    /* clocks: GPIOC for the pin, SYSCFG to route the EXTI line */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;     // button B1 is on PC13
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    GPIOC->MODER &= ~GPIO_MODER_MODER13;

    /* route EXTI line 13 to port C */
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13;
    SYSCFG->EXTICR[3] |=  SYSCFG_EXTICR4_EXTI13_PC;

    /* configure EXTI line 13 */
    EXTI->IMR  |= EXTI_IMR_MR13;             // unmask the interrupt
    EXTI->FTSR |= EXTI_FTSR_TR13;            // trigger on falling edge (press)

    /* set up handler */
    NVIC_SetPriority(EXTI15_10_IRQn, 15);
    NVIC_EnableIRQ(EXTI15_10_IRQn);          // enable LAST — sem_init must run first
}


/* ---- interrupt handler ---------------------------------------------------*/

/* PC13 -> EXTI line 13 -> this is the real vector-table name for lines 10..15 */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR13) {          // was it line 13 that fired?
        EXTI->PR = EXTI_PR_PR13;            // clear flag FIRST (write 1) or it re-fires forever
        handler_count++;                    // counter to see handler trigger
        sem_give(&button_sem);              // signal the blinker (top-half: just ring the bell)
    }
}


/* ---- tasks ---------------------------------------------------------------*/

void led_blinker(void)
{

    while (1) {
        sem_take(&button_sem);              // sleep here until the ISR gives
        GPIOA->ODR ^= (1U << 5);            // woke up -> toggle the green LED
        led_count++;
    }
}

void counter(void)
{
    while (1) {
        count++;                        //  counts only when blinker is blocked
    }
}


/* ---- main ----------------------------------------------------------------*/

int main(void)
{
    rtos_init();                            // kernel setup + idle task (slot 0)

    sem_init(&button_sem, 0, 1);  
    /* task_create(function, priority, stack words) — higher number = higher priority */
    task_create(led_blinker, 15, 64);       // top priority: wakes and preempts counter on press
    task_create(counter,     14, 64);       // runs only while blinker is blocked

    led_init();
    button_init();                          // enables the button IRQ (after sem_init — order matters)

    rtos_start();                           // enable SysTick + launch — never returns
}
