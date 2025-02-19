#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "CMSIS/system_m2sxxx.h"
#include "drivers/mss_gpio/mss_gpio.h"
#include "drivers/mss_nvm/mss_nvm.h"
#include "uart.h"
#include "comms.h"
#include "protocol.h"
#include "data.h"

#define NVM_BASE_ADDRESS   0x00000000u
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

void uart_print(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t *)buffer);
}


int main() {
    MSS_GPIO_init();
    MSS_GPIO_config( MSS_GPIO_0 , MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config( MSS_GPIO_1 , MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config( MSS_GPIO_2 , MSS_GPIO_OUTPUT_MODE );
    MSS_GPIO_config( MSS_GPIO_3 , MSS_GPIO_OUTPUT_MODE );
    MSS_UART_init(&g_mss_uart0, MSS_UART_115200_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY);
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000); // 1ms
    MSS_GPIO_set_outputs(0xAA);

    size_t chunk_size = 256;
    uint8_t outs = 0xFF;
    for (size_t i = 0; i < sizeof(data); i+=chunk_size) {
        uint32_t write_size = chunk_size < sizeof(data) - i ? chunk_size : sizeof(data) - i;
        int nvm_status = NVM_write((uint32_t)(NVM_BASE_ADDRESS + BOOLOADER_SIZE + i), data + i, write_size, NVM_DO_NOT_LOCK_PAGE);
        if (nvm_status == NVM_SUCCESS) {
        } else {
        }
        MSS_GPIO_set_outputs(outs);
        outs = ~outs;
    }
    delay_ms(200);
    jump_to_app();
    __asm__("BKPT #0");
    while(1);
}

static void delay_ms(uint32_t ms) {
    uint64_t end = tick + ms;
    while (tick < end);
}
