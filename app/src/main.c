#include <stdio.h>
#include <stdint.h>
#include "CMSIS/system_m2sxxx.h"
#include "drivers/mss_gpio/mss_gpio.h"
#include "drivers/mss_uart/mss_uart.h"

static volatile uint64_t tick = 0;
static void delay_ms(uint32_t ms);

__attribute__((__interrupt__)) void SysTick_Handler(void) {
    tick++;
}

int main() {
    MSS_GPIO_init();

    MSS_GPIO_config(MSS_GPIO_0, MSS_GPIO_OUTPUT_MODE);
    MSS_GPIO_config(MSS_GPIO_1, MSS_GPIO_OUTPUT_MODE);
    MSS_GPIO_config(MSS_GPIO_2, MSS_GPIO_OUTPUT_MODE);
    MSS_GPIO_config(MSS_GPIO_3, MSS_GPIO_OUTPUT_MODE);
    MSS_UART_init(&g_mss_uart0, MSS_UART_115200_BAUD,
                  MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY);
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);  // 1ms
    // MSS_GPIO_set_outputs(0b000000);
    /*
     * Infinite loop.
     */
    for (;;) {
        uint32_t gpio_pattern;
        /*
         * Decrement delay counter.
         */
        delay_ms(500);

        // MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*)"Holaa0\r\n");
        printf("Holaa0\r\n");
        gpio_pattern = MSS_GPIO_get_outputs();
        gpio_pattern ^= 0xFFFFFFFF;
        MSS_GPIO_set_outputs(gpio_pattern);
    }
}

static void delay_ms(uint32_t ms) {
    uint64_t end = tick + ms;
    while (tick < end);
}
