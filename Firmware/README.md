# Firmware
This directory contains firmware for the RP2040 I2S Tester.

## Abstract
This firmware supports multiple PMOD breakouts for I2S CODECs which can be
selected at compile time by comments in the `main.h` file. Once compiled the
firmware can be installed in the RP2040 I2S Tester board either by copying the
UF2 file to the device over USB in bootloader mode, or via SWD using the RPI Pico
debugger.

## Prerequisites
Building this firmware requires that you have the RPi Pico C/C++ SDK installed and
properly configured.

## Building
After the prerequisites have been met, follow the following steps:
```
mkdir build
cd build
cmake ..
make
```

This will create the binary and .elf files needed after which you can flash the
binary into the RP2040 I2S Tester hardware either by copying the UF2 file to the
device over USB, or via SWD with the RPi Pico Debugging pod.

### CODECs
Choose the appropriate CODEC by uncommenting its CPP macro in the `main.h` file.

## Usage
Once the board is built and firmware installed, start the RP2040 I2S Tester board
by connecting it to your host computer or a USB-C power supply. The STATUS LED
will blink a short sequence to indicate which mode it's in:
* Mode 0 - short blink - sawtooth waveforms on L/R codec outputs.
* Mode 1 - two short blinks - sine waveforms on L/R codec outputs.
* Mode 2 - short + long blinks - pass-thru from ADC to DAC.

To select modes, press the USER button on the RP2040 I2S Tester board.


