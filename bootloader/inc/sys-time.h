#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <stdint.h>

void sys_time_init(void);
void sys_time_deinit(void);
uint64_t sys_time_get_ticks(void);
void sys_time_delay_ms(uint32_t ms);

#endif // SYS_TIME_H
