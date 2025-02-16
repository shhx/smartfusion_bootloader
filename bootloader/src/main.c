#include <stdio.h>
#include <stdint.h>
#include "CMSIS/system_m2sxxx.h"
#include "drivers/mss_gpio/mss_gpio.h"
#include "drivers/mss_uart/mss_uart.h"

#define NVM_BASE_ADDRESS   0x60000000u
#define BOOLOADER_SIZE     0x8000U
#define APP_START_ADDR     (NVM_BASE_ADDRESS + BOOLOADER_SIZE)
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
    MSS_GPIO_init();
    for (int i = 0; i < 7; i++) {
        MSS_GPIO_config(i, MSS_GPIO_OUTPUT_MODE);
    }
    for (int j = 0; j < 5; j++) {
        for (int i = 0; i < 7; i++) {
            MSS_GPIO_set_outputs(1 << i);
            delay_ms(100);
        }
    }
    jump_to_app();
    // MSS_UART_init(&g_mss_uart0, MSS_UART_115200_BAUD,
    //               MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY);
    // SystemCoreClockUpdate();
    // MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*)"Hello, world!\r\n");
    // while(1);
    // SysTick_Config(SystemCoreClock / 1000);  // 1ms
    return 0;
}

static void delay_ms(uint32_t ms) {
    uint64_t end = tick + ms;
    while (tick < end);
}
