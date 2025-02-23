#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NVM_BASE_ADDRESS   0x00000000u
#define BOOLOADER_SIZE     0x8000U
#define APP_START_ADDR     (NVM_BASE_ADDRESS + BOOLOADER_SIZE)
#define PROTOCOL_SYNC_BYTE   0xAA
#define MAX_DATA_LEN 256

typedef enum {
    BL_STATE_SYNC,
    BL_STATE_WAIT_UPDATE_REQ,
    BL_STATE_WAIT_FW_LEN,
    BL_STATE_WAIT_FW_DATA,
    BL_STATE_DONE,
    BL_STATE_FAIL,
    BL_STATE_NUM_STATES,
} BootloaderState;

typedef enum {
    CMD_GET_ID          = 0x01, // Get device ID
    CMD_GET_VERSION     = 0x02, // Get bootloader version
    CMD_UPDATE_REQ      = 0x03, // Request firmware update
    CMD_FW_LEN_REQ      = 0x04, // Request firmware length
    CMD_FW_LEN_RESP     = 0x05, // Response firmware length
    CMD_RESET           = 0x14, // Reset the device
    CMD_READ_MEM        = 0x15, // Read memory
    CMD_WRITE_MEM       = 0x16, // Write memory
    CMD_WRITE_DATA_RDY  = 0x17, // Ready for data
    CMD_FW_UPDATE_DONE  = 0x18, // Firmware update done
    CMD_RETX            = 0x90, // Retransmit last packet
    CMD_ACK             = 0x91, // Acknowledge
    CMD_NACK            = 0x92, // Not Acknowledge
} ProtocolCmd;

typedef struct __attribute__((packed)) {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[MAX_DATA_LEN];
    uint8_t checksum;
} Packet;

typedef struct {
    BootloaderState current_state;
    BootloaderState (*handler)(void);
} StateMachine;

void bl_state_machine_init();
void bl_state_machine_update();
bool bl_need_sync();
bool bl_is_done();

#endif // BOOTLOADER_H
