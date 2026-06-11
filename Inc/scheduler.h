

#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "rtos.h"                  /* for tcb_t struct*/

void   scheduler_init(void);           /* zero ready_list[] + bitmap */
void   scheduler_ready_add(tcb_t *task);  /* enqueue t into ready_list[t->priority], set bit */
tcb_t *scheduler_next(void);           /* CLZ the bitmap, dequeue head of top list, return task to run */

#endif