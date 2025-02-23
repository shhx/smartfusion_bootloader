#include "drivers/mss_nvm/mss_nvm.h"
#include "bootloader.h"
#include "comms.h"
#include "uart.h"
#include "led.h"
#include "simple-sw-timer.h"

#define DEFAULT_TIMEOUT 2000 // ms
#define FW_MAX_SIZE 0x100000
#define SYNC_LEN 4

static uint8_t sync_seq[SYNC_LEN] = {0};
static BootloaderState bl_state = BL_STATE_SYNC;
const static uint8_t SYNC_BYTES[SYNC_LEN] = {0xDE, 0xAD, 0xBE, 0xEF};
static uint32_t fw_len = 0;
static uint32_t fw_bytes_written = 0;
static SimpleTimer timeout_timer = {0};

static bool bl_check_sync(uint8_t new_byte);
static BootloaderState bl_wait_sync(void);
static BootloaderState bl_wait_update_req(void);
static BootloaderState bl_wait_fw_len(void);
static BootloaderState bl_wait_fw_data(void);
static BootloaderState bl_done(void);
static BootloaderState bl_fail(void);

static StateMachine state_table[] = {
    {BL_STATE_SYNC, bl_wait_sync},
    {BL_STATE_WAIT_UPDATE_REQ, bl_wait_update_req},
    {BL_STATE_WAIT_FW_LEN, bl_wait_fw_len},
    {BL_STATE_WAIT_FW_DATA, bl_wait_fw_data},
    {BL_STATE_DONE, bl_done},
    {BL_STATE_FAIL, bl_fail},
};

void bl_state_machine_init() {
    bl_state = BL_STATE_SYNC;
    fw_len = 0;
    fw_bytes_written = 0;
    simple_timer_init(&timeout_timer, DEFAULT_TIMEOUT, false);
}

void bl_state_machine_update() {
    if (bl_state >= BL_STATE_NUM_STATES) {
        return;
    }
    bl_state = state_table[bl_state].handler();
}

static bool did_timeout() {
    return simple_timer_has_elapsed(&timeout_timer);
}

BootloaderState bl_wait_sync(void) {
    if (!bl_check_sync(uart_receive_byte())) {
        if (did_timeout()) {
            return BL_STATE_FAIL;
        }
        return BL_STATE_SYNC;
    }
    simple_timer_reset(&timeout_timer);
    led_set(LED_SYNC, 1);
    return BL_STATE_WAIT_UPDATE_REQ;
}

BootloaderState bl_wait_update_req(void) {
    if(comms_packet_available()) {
        Packet pkt;
        comms_read(&pkt);
        if (pkt.cmd == CMD_UPDATE_REQ) {
            Packet req = comms_create_cmd_packet(CMD_FW_LEN_REQ);
            comms_write(&req);
            simple_timer_reset(&timeout_timer);
            return BL_STATE_WAIT_FW_LEN;
        }
    }
    if (did_timeout()) {
        return BL_STATE_FAIL;
    }
    return BL_STATE_WAIT_UPDATE_REQ;
}

BootloaderState bl_wait_fw_len(void) {
    if(comms_packet_available()) {
        Packet pkt;
        comms_read(&pkt);
        if (pkt.cmd == CMD_FW_LEN_RESP) {
            fw_len = big_endian_to_uint32(pkt.data);
            if (fw_len > FW_MAX_SIZE) {
                led_set(LED_ERROR, 1);
                return BL_STATE_FAIL;
            }
            // Signal host that we are ready for data
            Packet rdy = comms_create_cmd_packet(CMD_WRITE_DATA_RDY);
            comms_write(&rdy);
            simple_timer_reset(&timeout_timer);
            return BL_STATE_WAIT_FW_DATA;
        }
    }
    if (did_timeout()) {
        return BL_STATE_FAIL;
    }
    return BL_STATE_WAIT_FW_LEN;
}

BootloaderState bl_wait_fw_data(void) {
    if(comms_packet_available()) {
        Packet pkt;
        comms_read(&pkt);
        if (pkt.cmd == CMD_WRITE_MEM) {
            uint32_t addr = big_endian_to_uint32(pkt.data);
            const uint8_t *data = pkt.data + sizeof(addr);
            uint32_t len = pkt.len - sizeof(addr);
            if(addr < APP_START_ADDR || addr + len > APP_START_ADDR + fw_len) {
                led_set(LED_ERROR, 1);
                return BL_STATE_FAIL;
            }
            nvm_status_t status = NVM_write(addr, data, len, NVM_DO_NOT_LOCK_PAGE);
            if (status != NVM_SUCCESS) {
                led_set(LED_ERROR, 1);
                return BL_STATE_FAIL;
            }
            fw_bytes_written += len;
            led_toggle(LED_FW_WRITE);
            if (fw_bytes_written >= fw_len) {
                Packet done = comms_create_cmd_packet(CMD_FW_UPDATE_DONE);
                comms_write(&done);
                return BL_STATE_DONE;
            }
            Packet rdy = comms_create_cmd_packet(CMD_WRITE_DATA_RDY);
            comms_write(&rdy);
            simple_timer_reset(&timeout_timer);
            return BL_STATE_WAIT_FW_DATA;
        }
    }
    if (did_timeout()) {
        return BL_STATE_FAIL;
    }
    return BL_STATE_WAIT_FW_DATA;
}

BootloaderState bl_done(void) {
    return BL_STATE_DONE;
}

BootloaderState bl_fail(void) {
    Packet pkt = comms_create_cmd_packet(CMD_NACK);
    comms_write(&pkt);
    return BL_STATE_DONE;
}

bool bl_check_sync(uint8_t new_byte) {
    for (int i = 0; i < SYNC_LEN - 1; i++) {
        sync_seq[i] = sync_seq[i + 1];
    }
    sync_seq[SYNC_LEN - 1] = new_byte;
    bool synced = true;
    for (int i = 0; i < SYNC_LEN; i++) {
        if (sync_seq[i] != SYNC_BYTES[i]) {
            synced = false;
            break;
        }
    }
    return synced;
}

bool bl_need_sync() {
    return bl_state == BL_STATE_SYNC;
}

bool bl_is_done() {
    return bl_state == BL_STATE_DONE;
}
