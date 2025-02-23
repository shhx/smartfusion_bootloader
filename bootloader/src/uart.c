#include "uart.h"
#include "ring-buffer.h"
#include "drivers/mss_uart/mss_uart.h"

#define BAUD_RATE MSS_UART_921600_BAUD
#define RING_BUFFER_SIZE (128)

static RingBuffer rb = {0U};
static uint8_t data_buffer[RING_BUFFER_SIZE] = {0U};

static void uart_rx_handler(mss_uart_instance_t *this_uart) {
    uint8_t rx_buff;
    size_t size = MSS_UART_get_rx(this_uart, &rx_buff, 1);
    if (size > 0) {
        ring_buffer_write(&rb, rx_buff);
    }
}

void uart_init() {
    ring_buffer_init(&rb, data_buffer, RING_BUFFER_SIZE);
    MSS_UART_init(&g_mss_uart0, BAUD_RATE,
                  MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY);
    MSS_UART_set_rx_handler(&g_mss_uart0, uart_rx_handler,
                            MSS_UART_FIFO_SINGLE_BYTE);
    MSS_UART_enable_irq(&g_mss_uart0, MSS_UART_RBF_IRQ);
}

void uart_deinit() {
    g_mss_uart0.hw_reg = UART0;
    g_mss_uart0.irqn = UART0_IRQn;
    /* reset UART0 */
    SYSREG->SOFT_RST_CR |= SYSREG_MMUART0_SOFTRESET_MASK;
    /* Clear any previously pended UART0 interrupt */
    NVIC_ClearPendingIRQ(UART0_IRQn);
    /* Take UART0 out of reset. */
    SYSREG->SOFT_RST_CR &= ~SYSREG_MMUART0_SOFTRESET_MASK;
}

void uart_write(const uint8_t *data, uint32_t len) {
    MSS_UART_polled_tx(&g_mss_uart0, data, len);
}

uint8_t uart_read(uint8_t *data, uint32_t len) {
    if(len == 0) {
        return 0;
    }
    for(size_t i = 0; i < len; i++) {
        if(!ring_buffer_read(&rb, &data[i])) {
            return i;
        }
    }
    return len;
}

uint8_t uart_receive_byte() {
    uint8_t byte = 0;
    (void)uart_read(&byte, 1);
    return byte;
}

bool uart_data_available() {
    return !ring_buffer_empty(&rb);
}
