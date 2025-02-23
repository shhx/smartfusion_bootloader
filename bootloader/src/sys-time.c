#include "sys-time.h"
#include "CMSIS/m2sxxx.h"
#include "CMSIS/system_m2sxxx.h"

static volatile uint64_t tick = 0;

__attribute__((__interrupt__)) void SysTick_Handler(void) {
    tick++; 
}

void sys_time_init(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);  // 1ms tick
}

void sys_time_deinit(void) {
    // Disable SysTick
    SysTick->CTRL = 0;
}

uint64_t sys_time_get_ticks(void) {
    return tick;
}

void sys_time_delay_ms(uint32_t ms) {
    uint64_t end = tick + ms;
    while (tick < end);
}
