
#include<stdint.h>
#include "config.h"
#include "scheduler.h"

typedef struct {
    tcb_t* head;
    tcb_t* tail;
} ready_queue_t;

static ready_queue_t ready_list[NUM_PRIORITIES];
static uint32_t ready_bitmap;

void scheduler_init(void){
    for(int i = 0; i < NUM_PRIORITIES;i++){
        ready_list[i].head = 0;
        ready_list[i].tail = 0;
    }
    ready_bitmap = 0;
}

/* enqueue a task into the ready list of its 
respective priority */
static void task_enqueue(tcb_t *task){
    if(task->queued == 1)
        return;

    ready_queue_t *q = &ready_list[task->priority];  // queue to be enqueued in
    task->queued = 1;
    task->next = 0;
    
    if(!q->head && !q->tail){   // empty list
        q->head = task;
        q->tail = task;
    }else{                      // push at tail
        q->tail->next = task;
        q->tail = task;    
    }
}

/* dequeue a task from the ready list of the 
input priority level */
static tcb_t* task_dequeue(uint8_t priority){
    ready_queue_t *q = &ready_list[priority];
    tcb_t* task = q->head;
    
    if(!task)   return 0;           // list is empty (safety catch, should not happen)
    q->head = task->next;
    if(!q->head)   q->tail = 0;     // if dequeued task was the last element in queue
    
    task->next = 0;
    task->queued = 0;
    return task;
}

void scheduler_remove_task(tcb_t *task){
    ready_queue_t *q = &ready_list[task->priority];
    if(q->head == NULL) return;                 // safety check, caller guarantees membership

    // case 1: task is the head
    if(q->head == task){
        q->head = task->next;                   // NULL if it was the only node
        if(q->tail == task) q->tail = NULL;     // was sole node, list now empty
        if(q->head == NULL)
            ready_bitmap &= ~(1u << task->priority);  // update bitmap
        task->next   = NULL;
        task->queued = 0;
        return;
    }

    // case 2: task is after head — find its predecessor
    tcb_t *prev = q->head;
    while(prev->next && prev->next != task)
        prev = prev->next;

    if(prev->next == task){                     // found
        prev->next = task->next;                // unconditional: NULL if task was tail
        if(q->tail == task) q->tail = prev;     // task was tail → prev is new tail
        task->next   = NULL;
        task->queued = 0;
    }
}

/* api to add task to ready list*/
void scheduler_ready_add(tcb_t *task){
    task_enqueue(task);
    ready_bitmap |= (1u << task->priority);
}

/* api to get the tcb of the next ready 
task of highest priority available */
tcb_t* scheduler_next(void){
    uint8_t priority = 31 - __builtin_clz(ready_bitmap);
    tcb_t* task = task_dequeue(priority);

    // if level becomes empty, clear the bit
    if(ready_list[priority].head == 0)
        ready_bitmap &= ~(1u << priority);      // clear the priority bit
    
    return task;
}



void scheduler_reprioritize(tcb_t* task, uint8_t new_priority){
    // remove task from ready list. 
    scheduler_remove_task(task);
    task->priority = new_priority;
    scheduler_ready_add(task);
}
