#include <stdio.h>
#include <stdint.h>
#include "CMSIS/m2sxxx.h"
#include "CMSIS/system_m2sxxx.h"
#include "drivers/mss_nvm/mss_nvm.h"
#include "led.h"
#include "uart.h"
#include "comms.h"
#include "bootloader.h"


static volatile uint64_t tick = 0;
static void delay_ms(uint32_t ms);

__attribute__((__interrupt__)) void SysTick_Handler(void) {
    tick++; 
}

void jump_to_app(void) {
    uint32_t *reset_vector_entry = (uint32_t *)(APP_START_ADDR + 4U);
    uint32_t *reset_vector = (uint32_t *)*reset_vector_entry;
    SCB->VTOR = APP_START_ADDR;
    void (*app_reset_handler)(void) = (void (*)(void))reset_vector;
    app_reset_handler();
}

int main() {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);  // 1ms
    uart_init();
    led_init();
    comms_init();
    for (int i = 0; i < 4; i++) {
        led_toggle(LED_SYNC);
        delay_ms(100);
    }
    // NVIC_SystemReset();
    while (1) {
        if (!bootloader_need_sync()) {
            // Only update comms when already synced
            comms_update();
        }
        bl_state_machine_update();
        if (bootloader_is_done()) {
            uart_deinit();
            jump_to_app();
        }
    }
}

static void delay_ms(uint32_t ms) {
    uint64_t end = tick + ms;
    while (tick < end);
}
