

#include "rtos.h"

typedef struct {
    volatile uint32_t count;    // 0 = unavailable, 1 = available
    uint32_t max_count;
} semaphore_t;

void sem_take(semaphore_t* sem);
void sem_give(semaphore_t* sem);
void sem_init(semaphore_t* sem, uint32_t initial, uint32_t maxCount);