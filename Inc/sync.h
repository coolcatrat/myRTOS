

#include "rtos.h"

typedef struct {
    volatile uint32_t count;    // 0 = unavailable, 1 = available
    uint32_t max_count;
} semaphore_t;

void sem_take(semaphore_t* sem);
void sem_give(semaphore_t* sem);
void sem_init(semaphore_t* sem, uint32_t initial, uint32_t maxCount);


typedef struct {    
    tcb_t *owner;           // NULL when free, not NULL when occupied.
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);