#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <path to elf file>"
    exit 1
fi

file=$1
openocd \
    -c "set DEVICE M2S010" \
    -c "adapter_khz 6000" \
    -f board/microsemi-cortex-m3.cfg \
    -c "program ${file} verify reset exit"
