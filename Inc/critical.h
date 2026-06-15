#include <stdint.h>
#include "config.h"
#include "cmsis_gcc.h"

static inline uint32_t os_enter_critical(void){
    uint32_t old_basepri = __get_BASEPRI();
    __set_BASEPRI(MAX_SYSCALL_INTERRUPT_PRIORITY);
    __DSB();
    __ISB();
    return old_basepri;
}
static inline void os_exit_critical(uint32_t old_basepri){
    __set_BASEPRI(old_basepri);
    __DSB();
    __ISB();
}