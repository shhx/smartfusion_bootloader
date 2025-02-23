#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

void uart_init();
void uart_deinit();
void uart_write(const uint8_t *data, uint32_t len);
uint8_t uart_read(uint8_t *data, uint32_t len);
uint8_t uart_receive_byte();
bool uart_data_available();

#endif  // UART_H
