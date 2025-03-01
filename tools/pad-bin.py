import argparse
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-f", "--file", required=True)
parser.add_argument("-s", "--size", required=True)
args = parser.parse_args()

BOOTLOADER_FILE = args.file
BOOTLOADER_SIZE = int(args.size, 16)

with open(BOOTLOADER_FILE, "rb") as f:
    raw_file = f.read()

bytes_to_pad = BOOTLOADER_SIZE - len(raw_file)
padding = bytes([0xff for _ in range(bytes_to_pad)])

with open("output.bin", "wb") as f:
    f.write(raw_file + padding)
