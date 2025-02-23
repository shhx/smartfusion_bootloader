#include <string.h>
#include "comms.h"
#include "uart.h"
#include "led.h"
#include "drivers/mss_nvm/mss_nvm.h"

#define PACKET_BUFFER_SIZE 4  // Number of packets in the buffer

typedef enum {
    STATE_RECEIVING_CMD,
    STATE_RECEIVING_LEN,
    STATE_RECEIVING_DATA,
    STATE_RECEIVING_CHECKSUM
} CommsState;

static uint8_t data_byte_count = 0;
static CommsState rx_state = STATE_RECEIVING_CMD;
static Packet last_tx_packet = {0}; // Last packet sent over comms
static Packet temp_packet = {0};  // Temporary packet for reading from UART
static Packet packet_ack = {0};
static Packet packet_retx = {0};

static Packet packet_buffer[PACKET_BUFFER_SIZE];
static uint32_t packet_read_index = 0;
static uint32_t packet_write_index = 0;
static uint32_t packet_buffer_mask = PACKET_BUFFER_SIZE - 1;

static uint8_t comms_receive_byte();
static uint8_t calculate_checksum(const Packet *packet);
static uint8_t crc8(const uint8_t *data, uint8_t len);

void comms_init() {
    packet_ack.cmd = CMD_ACK;
    packet_ack.len = 1;
    packet_ack.checksum = calculate_checksum(&packet_ack);
    packet_retx.cmd = CMD_RETX;
    packet_retx.len = 0;
    packet_retx.checksum = calculate_checksum(&packet_retx);
}

Packet comms_create_cmd_packet(uint8_t cmd) {
    Packet pkt = {0};
    pkt.cmd = cmd;
    pkt.len = 0;
    pkt.checksum = calculate_checksum(&pkt);
    return pkt;
}

void comms_update() {
    while (uart_data_available()) {
        switch (rx_state) {
            case STATE_RECEIVING_CMD:
                temp_packet.cmd = comms_receive_byte();
                rx_state = STATE_RECEIVING_LEN;
                break;
            case STATE_RECEIVING_LEN:
                temp_packet.len = comms_receive_byte();
                if (temp_packet.len > MAX_DATA_LEN) {
                    rx_state = STATE_RECEIVING_CMD;
                } else {
                    data_byte_count = 0;
                    rx_state = (temp_packet.len > 0) ? STATE_RECEIVING_DATA
                                                     : STATE_RECEIVING_CHECKSUM;
                }
                break;
            case STATE_RECEIVING_DATA:
                temp_packet.data[data_byte_count++] = comms_receive_byte();
                if (data_byte_count >= temp_packet.len) {
                    rx_state = STATE_RECEIVING_CHECKSUM;
                }
                break;
            case STATE_RECEIVING_CHECKSUM:
                temp_packet.checksum = comms_receive_byte();
                if (calculate_checksum(&temp_packet) != temp_packet.checksum) {
                    led_set(LED_ERROR, 1);
                    comms_write(&packet_retx);
                    rx_state = STATE_RECEIVING_CMD;
                    break;
                }
                if (temp_packet.cmd == CMD_RETX) {
                    comms_write(&last_tx_packet);
                    break;
                } 
                uint32_t next_wr_index =
                    (packet_write_index + 1) & packet_buffer_mask;
                if (next_wr_index != packet_read_index) {
                    led_toggle(LED_COMMS);
                    memcpy(&packet_buffer[packet_write_index], &temp_packet,
                        sizeof(Packet));
                    packet_write_index = next_wr_index;
                    packet_ack.data[0] = temp_packet.cmd;
                    packet_ack.checksum = calculate_checksum(&packet_ack);
                    comms_write(&packet_ack);
                } else {
                    led_set(LED_ERROR, 1);
                }
                rx_state = STATE_RECEIVING_CMD;
                break;
            default:
                rx_state = STATE_RECEIVING_CMD;
                break;
        }
    }
}

bool comms_packet_available() {
    return packet_read_index != packet_write_index;
}

static uint8_t comms_receive_byte() {
    uint8_t byte = 0;
    uart_read(&byte, 1);
    return byte;
}

void comms_write(const Packet *packet) {
    uart_write(&packet->cmd, 1);
    uart_write(&packet->len, 1);
    if (packet->len > 0) {
        uart_write(packet->data, packet->len);
    }
    uart_write(&packet->checksum, 1);
    // We could use a loop here to avoid string.h
    memcpy(&last_tx_packet, packet, sizeof(Packet));
}

void comms_read(Packet *packet) {
    // We could use a loop here
    memcpy(packet, &packet_buffer[packet_read_index], sizeof(Packet));
    packet_read_index = (packet_read_index + 1) & packet_buffer_mask;
}

uint32_t big_endian_to_uint32(const uint8_t *bytes) {
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

uint8_t calculate_checksum(const Packet *packet) {
    uint8_t crc = 0;
    crc ^= crc8((uint8_t *)&packet->cmd, 1);
    crc ^= crc8((uint8_t *)&packet->len, 1);
    crc ^= crc8(packet->data, packet->len);
    return crc;
}
