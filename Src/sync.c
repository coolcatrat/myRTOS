

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

void mutex_init(mutex_t *m){
    m->owner = NULL;
}
void mutex_lock(mutex_t *m){
    uint32_t sr = os_enter_critical();

    // if unlocked, assign current task as the owner and lock it.
    if(!m->owner){
        m->owner = current_tcb;
        os_exit_critical(sr);
        return;
    }
    // if locked, declare task waiting on mutex_t m and block it. 
    current_tcb->blocked_on = m;
    current_tcb->state = TASK_BLOCKED;
    os_yield_to_scheduler();        // request context switch
    os_exit_critical(sr);           
}
void mutex_unlock(mutex_t *m){
    uint32_t sr = os_enter_critical();

    // the task calling is not the owner, should not happen generally.
    if(current_tcb != m->owner){
        os_exit_critical(sr);
        return;
    }

    // unblock the next highest priority task waiting on the mutex
    tcb_t *waiter = highest_priority_waiter(m);

    // if a waiter exists, it is the next owner of the mutex.
    if(waiter != NULL){
        m->owner = waiter;
        waiter->blocked_on = NULL;
        waiter->state = TASK_READY;
        scheduler_ready_add(waiter);
        os_yield_to_scheduler();
        os_exit_critical(sr);
    }
    // if no waiter exists, mutex is free.
    else{
        m->owner = NULL;
        os_exit_critical(sr);
    }

}