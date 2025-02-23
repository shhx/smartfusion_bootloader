#ifndef LED_H
#define LED_H

#include <stdint.h>

typedef enum {
    LED_ERROR = 0,
    LED_COMMS = 2,
    LED_FW_WRITE = 4,
    LED_DONE = 5,
    LED_SYNC = 6,
} LedIndex;

void led_init();
void led_set_many(uint8_t value);
void led_set(uint8_t index, uint8_t value);
void led_toggle(uint8_t index);

#endif /* LED_H */
