#!/usr/bin/env python3
# Convert bin file to c array
# Usage: bin2c.py <input file> <output file> 
import sys

if len(sys.argv) != 3:
    print("Usage: bin2c.py <input file> <output file>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(input_file, "rb") as f:
    data = f.read()

with open(output_file, "w") as f:
    f.write("static const uint8_t data[] = {\n")
    for i in range(len(data)):
        f.write(f"0x{data[i]:02X}")
        if i != len(data) - 1:
            f.write(", ")
    f.write("};")
print(f"Converted {input_file} to {output_file}")
