#ifndef SIMPLE_SW_TIMER_H
#define SIMPLE_SW_TIMER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t wait_time;
    uint64_t end_time; 
    bool auto_reset;
    bool has_elapsed;
} SimpleTimer;

void simple_timer_init(SimpleTimer* timer, uint64_t wait_time, bool auto_reset);
bool simple_timer_has_elapsed(SimpleTimer* timer);
void simple_timer_reset(SimpleTimer* timer);

#endif // SIMPLE_SW_TIMER_H
