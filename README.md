# Project Name

## Description
Brief description of the project.

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)

## Installation
How to install and set up the project.

```bash
# Install the ARM toolchain
sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi
sudo apt-get install gdb-arm-none-eabi

```bash
git clone https://github.com/yourusername/yourproject.git
cd yourproject
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=tools/toolchain-arm-none-eabi.cmake
# Add installation steps here
```
Runing openocd
```bash
openocd --command "set DEVICE M2S010" --file board/microsemi-cortex-m3.cfg
```

## Usage
How to use the project.

```bash
echo "add-auto-load-safe-path .gdbinit" >> ~/.config/gdb/gdbinit
```
https://openocd.org/doc/html/Flash-Programming.html
