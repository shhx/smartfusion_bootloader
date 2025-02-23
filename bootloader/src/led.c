#include "led.h"
#include "drivers/mss_gpio/mss_gpio.h"

#define LED_COUNT 7

void led_init() {
    MSS_GPIO_init();
    for(int i = 0; i < LED_COUNT; i++) {
        MSS_GPIO_config(i, MSS_GPIO_OUTPUT_MODE);
    }
}

void led_set_many(uint8_t value) {
    MSS_GPIO_set_outputs(value);
}

void led_set(uint8_t index, uint8_t value) {
    // Active low
    MSS_GPIO_set_output(index, !value);
}

void led_toggle(uint8_t index) {
    int state = MSS_GPIO_get_outputs();
    state ^= (1 << index);
    MSS_GPIO_set_outputs(state);
}
