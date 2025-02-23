import os
import time
from argparse import ArgumentParser
from ctypes import Structure, c_uint8
from enum import IntEnum
from logging import basicConfig, getLogger

from serial import Serial
from tqdm import tqdm

MAX_DATA_LEN = 255
FW_ADDR_LEN = 4
SYNC_BYTES = b'\xDE\xAD\xBE\xEF'

logger = getLogger(__name__)

class Packet(Structure):
    _fields_ = [
        ("cmd", c_uint8),
        ("len", c_uint8),
        ("data", c_uint8 * MAX_DATA_LEN),
        ("checksum", c_uint8),
    ]

    def __str__(self):
        str = f"{self.__class__.__name__}({{}})"
        fields = []
        fields.append(f"cmd=0x{self.cmd:02X}")
        fields.append(f"len={self.len}")
        fields.append(f"data={self.data[:self.len]}")
        fields.append(f"checksum={self.checksum}")
        return str.format(", ".join(fields))

    def __repr__(self):
        return str(self)

class BootloaderException(Exception):
    pass

class ProtocolCmd(IntEnum):
    GET_ID          = 0x01 # Get device ID
    GET_VERSION     = 0x02 # Get bootloader version
    UPDATE_REQ      = 0x03 # Request firmware update
    FW_LEN_REQ      = 0x04 # Request firmware length
    FW_LEN_RESP     = 0x05 # Response firmware length
    RESET           = 0x14 # Reset the device
    READ_MEM        = 0x15 # Read memory
    WRITE_MEM       = 0x16 # Write memory
    WRITE_DATA_RDY  = 0x17 # Ready for data
    FW_UPDATE_DONE  = 0x18 # Firmware update done
    RETX            = 0x90 # Retransmit last packet
    ACK             = 0x91 # Acknowledge
    NACK            = 0x92 # Not Acknowledge

class BootloaderFlasher:
    def __init__(self, serial_port: str, baud_rate: int):
        self.serial_port = serial_port
        self.serial = Serial(serial_port, baud_rate, timeout=0.1)
        logger.info(f"Opened serial port {serial_port} at {baud_rate} baud")
        self.serial.reset_input_buffer()
        self.serial.set_output_flow_control(False)
        self.serial.set_input_flow_control(False)

    def send_sync(self):
        self.serial.write(SYNC_BYTES)
        logger.debug("Sent sync")

    def send_packet(self, packet: Packet):
        packet.checksum = self._checksum(packet)
        data = bytes([packet.cmd, packet.len, *packet.data[:packet.len], packet.checksum])
        self.serial.write(data)

    def send_fw_data(self, addr: int, data: bytes):
        if len(data) > MAX_DATA_LEN - 4:
            raise ValueError(f"Data length {len(data)} exceeds maximum {MAX_DATA_LEN - 4}")
        try:
            packet = self.receive_packet()
        except TimeoutError:
            raise BootloaderException("Timeout waiting for bootloader ready for data")
        if packet.cmd != ProtocolCmd.WRITE_DATA_RDY:
            raise BootloaderException("Bootloader not ready for data")
        packet = Packet()
        packet.cmd = ProtocolCmd.WRITE_MEM
        packet.len = len(data) + FW_ADDR_LEN
        packet.data[:FW_ADDR_LEN] = addr.to_bytes(FW_ADDR_LEN, byteorder='big')
        packet.data[FW_ADDR_LEN:packet.len] = data
        logger.debug("Writing %d bytes to 0x%08X", len(data), addr)
        self.send_packet(packet)
        self.wait_ack(packet)

    def send_request(self, cmd: ProtocolCmd, data=None):
        packet = Packet()
        packet.cmd = cmd
        if data is not None:
            packet.len = len(data)
            packet.data[:packet.len] = data
        else:
            packet.len = 1
        self.send_packet(packet)
        self.wait_ack(packet)

    def wait_ack(self, packet: Packet, timeout=1):
        resp = self.receive_packet(timeout)
        while resp.cmd == ProtocolCmd.RETX:
            logger.warning("Retransmitting packet %s", packet)
            self.send_packet(packet)
            resp = self.receive_packet(timeout)
        if resp.cmd == ProtocolCmd.NACK:
            raise BootloaderException("NACK received")
        if resp.cmd != ProtocolCmd.ACK:
            raise ValueError(f"Expected ACK, got 0x{resp.cmd:X}")
        if resp.data[0] != packet.cmd:
            if resp.data[0] in list(ProtocolCmd):
                raise ValueError(f"Expected ACK for CMD:{packet.cmd}, got CMD:{ProtocolCmd(resp.data[0])}")
            raise ValueError(f"Expected ACK for CMD:{packet.cmd}, got CMD:0x{resp.data[0]:X}")
        return

    def receive_packet(self, timeout: float = 1) -> Packet:
        packet = Packet()
        t0 = time.time()
        while self.serial.in_waiting < 2:
            if time.time() - t0 > timeout:
                raise TimeoutError
        packet.cmd = ProtocolCmd(self.serial.read(1)[0])
        packet.len = self.serial.read(1)[0]
        while self.serial.in_waiting < packet.len + 1:
            if time.time() - t0 > timeout:
                raise TimeoutError
        data = self.serial.read(packet.len)
        for i in range(packet.len):
            packet.data[i] = int.from_bytes(data[i:i+1], byteorder='little')
        packet.checksum = self.serial.read(1)[0]
        logger.debug(f"Received packet: {packet}")
        return packet

    def request_version(self):
        response = self._request_insist(ProtocolCmd.GET_VERSION)
        logger.info("Requested version")
        return response.data[0]

    def request_update(self):
        response = self._request_insist(ProtocolCmd.UPDATE_REQ)
        if response.cmd != ProtocolCmd.FW_LEN_REQ:
            raise ValueError(f"Expected FW_LEN_REQ, got {response.cmd}")
        logger.info("Requested firmware update")

    def send_fw_length(self, fw_len_bytes):
        data = fw_len_bytes.to_bytes(4, byteorder='big')
        self.send_request(ProtocolCmd.FW_LEN_RESP, data)
        logger.info(f"Sent firmware length: {fw_len_bytes}")

    def _request_insist(self, cmd: ProtocolCmd) -> Packet:
        self.send_request(cmd)
        response = self.receive_packet()
        while response.cmd == ProtocolCmd.RETX:
            logger.warning("Retransmitting")
            self.send_request(cmd)
            response = self.receive_packet()
        return response

    def close(self):
        self.serial.close()

    def _checksum(self, packet: Packet) -> int:
        checksum = 0
        checksum ^= self._crc8([packet.cmd])
        checksum ^= self._crc8([packet.len])
        checksum ^= self._crc8(packet.data[:packet.len])
        return checksum

    def _crc8(self, data: list[int]) -> int:
        crc = 0
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 0x80:
                    crc = (crc << 1) ^ 0x07
                else:
                    crc <<= 1
        return crc

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-f", "--file", help="Firmware file to flash", required=True)
    parser.add_argument("-p", "--port", help="Serial port", required=True)
    parser.add_argument("-b", "--baud", help="Baud rate", type=int, default=921600)
    parser.add_argument("-v", "--verbose", help="Verbose output", action="store_true")
    args = parser.parse_args()
    if args.verbose:
        basicConfig(level="DEBUG", format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    else:
        basicConfig(level="INFO", format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    protocol = BootloaderFlasher(args.port, args.baud)
    t0 = time.time()
    protocol.send_sync()
    # version = protocol.request_version()
    # logger.info(f"Version: 0x{version:02X}")
    protocol.request_update()
    fw_len_bytes = os.path.getsize(args.file)
    protocol.send_fw_length(fw_len_bytes)
    ADDR_START = 0x0 + 0x8000
    curr_addr = ADDR_START
    bar = tqdm(total=fw_len_bytes, unit='B', unit_scale=True, ascii=True)
    chunk_size = MAX_DATA_LEN - FW_ADDR_LEN
    with open(args.file, "rb") as f:
        data = f.read(chunk_size)
        while data:
            addr = curr_addr.to_bytes(FW_ADDR_LEN, byteorder='big')
            protocol.send_fw_data(curr_addr, data)
            bar.update(len(data))
            curr_addr += len(data)
            data = f.read(chunk_size)
    bar.close()
    done = protocol.receive_packet()
    if done.cmd != ProtocolCmd.FW_UPDATE_DONE:
        raise ValueError(f"Expected FW_UPDATE_DONE, got {done.cmd}")
    update_time = time.time() - t0
    logger.info("Firmware update done in %ds", update_time)
    protocol.close()
