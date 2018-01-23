# libfx2

_libfx2_ is a chip support package for Cypress EZ-USB FX2 series microcontrollers.

On the firmware side, it provides:
  * register definitions
  * makefile templates
  * common code for handling standard USB requests
  * a bootloader implementing Cypress vendor commands

On the software side, it provides:
  * `fx2`, a Python library for interacting with the bootloader
  * `fx2tool`, a tool for programming and debugging the chips

## Prerequisites

_libfx2_ has the following dependencies:
  * [sdcc][] for building the firmware code
  * [Python 3.6+][python] and [Python libusb1 wrapper][python-libusb1] for interacting with the device

On a Debian system, these can be installed with:

    apt-get install sdcc python3 python3-libusb1

Then, install the Python components to `~/.local`:

    python3 software/setup.py develop --user

The last step is not required if you just want to upload example firmware.

[sdcc]: http://sdcc.sourceforge.net
[python]: https://www.python.org/
[python-libusb1]: https://pypi.python.org/pypi/libusb1

## Getting started

Build the support and example code with `make -C firmware`.

Run `fx2tool --help` and understand its capabilities.

Read configuration EEPROM with `fx2tool -S firmware/bootloader/bootloader.ihex read_eeprom 0 7`.

See the code for the [Blinky] example if you're looking for something minimal, or [bootloader] if you're looking for something that involves USB.

Load the Blinky example with `make -C firmware/blinky load` (by now you should know which pin it expects the LED to be on).

[blinky]: /firmware/blinky
[bootloader]: /firmware/bootloader

## Why not just use [fx2lib]?

I wrote all of the code in this repository from scratch using the USB specification and Cypress datasheets for two reasons: the fx2lib code quality is very low, and it is licensed under LGPL, which is absurd for chip support packages.

[fx2lib]: https://github.com/djmuhlestein/fx2lib

## License

[0-clause BSD](LICENSE-0BSD.txt)
