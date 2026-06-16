

#include<stdint.h>
#include "critical.h"
#include "rtos.h"
#include "sync.h"
#include "scheduler.h"
void sem_take(semaphore_t* sem){
    uint32_t sr = os_enter_critical();
    if(sem->count > 0){             // take and go 
        sem->count--;               
        os_exit_critical(sr);       
        return;
    }
    // no token available, block and register as waiting on this semaphore.
    current_tcb->state = TASK_BLOCKED;
    current_tcb->blocked_on = sem;
    os_exit_critical(sr);
    os_yield_to_scheduler();        
}

void sem_give(semaphore_t *sem){
    uint32_t sr = os_enter_critical();
    tcb_t *waiter = highest_priority_waiter(sem);

    if(waiter != NULL){
        // unblock the task that is waiting
        waiter->state = TASK_READY;
        waiter->blocked_on = NULL;
        scheduler_ready_add(waiter);
        os_exit_critical(sr);
        os_yield_to_scheduler();
    }else {
        // nobody is waiting
        if(sem->count < sem->max_count) sem->count++;
        os_exit_critical(sr);
    }
}

void sem_init(semaphore_t* sem, uint32_t initial, uint32_t maxCount){
    sem->count = initial;
    sem->max_count = maxCount;
}