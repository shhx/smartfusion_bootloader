#ifndef INC_RING_BUFFER_H
#define INC_RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct RingBuffer {
    uint8_t* buffer;
    uint32_t mask;
    uint32_t read_index;
    uint32_t write_index;
} RingBuffer;

void ring_buffer_init(RingBuffer* rb, uint8_t* buffer, uint32_t size);
bool ring_buffer_empty(RingBuffer* rb);
bool ring_buffer_write(RingBuffer* rb, uint8_t byte);
bool ring_buffer_read(RingBuffer* rb, uint8_t* byte);

#endif  // INC_RING_BUFFER_H
