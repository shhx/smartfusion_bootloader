#include <stdio.h>
#include <stdint.h>
#include "CMSIS/m2sxxx.h"
#include "CMSIS/system_m2sxxx.h"
#include "drivers/mss_nvm/mss_nvm.h"
#include "led.h"
#include "uart.h"
#include "comms.h"
#include "bootloader.h"
#include "sys-time.h"

void jump_to_app(void) {
    uint32_t *reset_vector_entry = (uint32_t *)(APP_START_ADDR + 4U);
    uint32_t *reset_vector = (uint32_t *)*reset_vector_entry;
    SCB->VTOR = APP_START_ADDR;
    void (*app_reset_handler)(void) = (void (*)(void))reset_vector;
    app_reset_handler();
}

int main() {
    sys_time_init();
    uart_init();
    led_init();
    comms_init();
    bl_state_machine_init();
    for (int i = 0; i < 4; i++) {
        led_toggle(LED_SYNC);
        sys_time_delay_ms(100);
    }
    // NVIC_SystemReset();
    while (1) {
        if (!bl_need_sync()) {
            // Only update comms when are already synced
            comms_update();
        }
        bl_state_machine_update();
        if (bl_is_done()) {
            uart_deinit();
            sys_time_deinit();
            jump_to_app();
        }
    }
}
