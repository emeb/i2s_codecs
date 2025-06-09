# i2s_codecs
This repository is a collection of hardware and software for evaluating I2S codecs
for use in digital audio projects. I've been using codecs for personal and commercial
applications for more than a decade now and I've seen a lot of them come and go. Recently
several of my go-to codecs went EOL and I had to dive back into the distributors sites to
find a new one to use for some upcoming projects. The first one I tried had some very
odd behavior so I picked a few more, built some breakout boards and a little I2S signal
generator for comparing them all on even footing. The designs here are the result of
that effort.

## Hardware
There are two subdirectories here with KiCAD hardware designs that I used:
* `codec_pmods` - these are simple breakout boards for a number of different codecs,
some of which are currently available, some are obsolete. They are small 2-layer
boards intended to provide minimal support for the codec devices with little to
no attention to audio signal conditioning. The I2S interface is through a standard
12-pin (8 signals + 4 power & ground) PMOD connector that also provides 3 GPIO pins
that support I2C or bitbang configuration protocols that some codecs require for
startup.
* `rp2040_i2s_tester` - this is a KiCAD design for an RP2040 MCU that can drive the
12-pin PMOD with appropriate I2S, I2C or bitbang as well as 3.3V power. It uses a
USB-C connector for power and bootloading but also provides a Qwiic I2C connector,
a blinky LED and a user-defined button for mode selection.

## Software
The directory `Firmware` contains a Raspberry Pi SDK project with basic firmware
for the `rp2040_i2s_tester` board.

## Going Further
This is bare-bones stuff but can support more complex stuff in the future. I may
investigate USB Audio class drivers for interfacing the codecs to a host. There are
also a wide range of other codecs I may add to the list of breakouts. Time will tell.
