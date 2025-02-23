#ifndef COMMS_H
#define COMMS_H
#include <stdint.h>
#include <stdbool.h>
#include "bootloader.h"


void comms_init();
void comms_update();

bool comms_packet_available();
void comms_write(const Packet *packet);
void comms_read(Packet *packet);
Packet comms_create_cmd_packet(uint8_t cmd);
uint32_t big_endian_to_uint32(const uint8_t *bytes);
#endif // COMMS_H
