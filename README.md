# SmartFusion2 Bootloader

## Description
Simple UART bootloader for the Microsemi SmartFusion2 SoC.

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)

## Installation
At the moment the only valid openocd version is the one included with Softconsole. It has the patches needed to use flaspro programmer.

```bash
# Install the ARM toolchain
sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi
sudo apt-get install gdb-arm-none-eabi
```
Building the bootloader
```bash
cd bootloader
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=../tools/toolchain-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
Runing openocd
```bash
openocd --command "set DEVICE M2S010" --file board/microsemi-cortex-m3.cfg
```
https://openocd.org/doc/html/Flash-Programming.html

## Debugging
It is necessary to use the .gdbinit file included in the project. This file sets the target device and some memory properties.

```bash
echo "add-auto-load-safe-path .gdbinit" >> ~/.config/gdb/gdbinit
```

## Usage
How to use the project:
1. Connect the device to the PC using a USB to UART adapter.
2. Boot the device in bootloader mode. (Just reset the device)
3. Start the flasher program with the corresponding arguments.
4. The program will flash the firmware and start the new firmware.


## TODO
- [ ] Add flash memory integrity check before jumping to the application. Use sha256 (hardware accelerated)
- [ ] Add a way to update the firmware from the application.
