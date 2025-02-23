#include "simple-sw-timer.h"
#include "sys-time.h"

void simple_timer_init(SimpleTimer* timer, uint64_t wait_time, bool auto_reset) {
    timer->end_time = sys_time_get_ticks() + wait_time;
    timer->wait_time = wait_time;
    timer->auto_reset = auto_reset;
    timer->has_elapsed = false;
}

bool simple_timer_has_elapsed(SimpleTimer* timer) {
    uint64_t current_time = sys_time_get_ticks();
    bool elapsed = current_time >= timer->end_time;
    if (elapsed) {
        if (timer->auto_reset) {
            uint64_t overtime = current_time - timer->end_time;
            timer->end_time = current_time + timer->wait_time - overtime; 
        }
        timer->has_elapsed = true;
    }
    return elapsed;
}

void simple_timer_reset(SimpleTimer* timer) {
    timer->end_time = sys_time_get_ticks() + timer->wait_time;
    timer->has_elapsed = false;
}
