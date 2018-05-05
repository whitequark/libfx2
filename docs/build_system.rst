Build system reference
======================

*libfx2* provides a flexible build system based on GNU Make for convenient development.
It provides:

  * out-of-tree builds,
  * rebuilds after changes to application headers,
  * rebuilds (of both *libfx2* and application) after changes to *libfx2*,
  * minimal and readable configuration for most common cases,
  * a ``load`` target for building and uploading the design using :ref:`fx2tool <bootloader-tool>`.

To start using it, create a source file...

.. code-block:: c
  :caption: main.c

  #include <fx2regs.h>

  int main() {
    IOA = OEA = 1;
    while(1);
  }

\... and a Makefile, replacing ``.../libfx2`` with the path to the root of this repository...

.. code-block:: make
  :caption: Makefile

  LIBFX2 = .../libfx2/firmware/library
  include $(LIBFX2)/fx2rules.mk

\... and you're done! Running ``make`` will build a ``firmware.ihex``, and running ``make load`` will run it on any connected Cypress development kit.

Of course, as your project grows, so has your build system. The configuration values that may be set are as follows:

  * ``TARGET`` sets the base name of the output Intel HEX file. It is ``firmware`` if not specified.
  * ``SOURCES`` lists the ``.c`` or ``.asm`` source files to be built, without extension. It is ``main`` if not specified.
  * ``LIBRARIES`` lists the standard libraries to be linked in, without extension. It is ``fx2isrs`` by default, and can be any of ``fx2``, ``fx2isrs`` and ``fx2usb``.
  * ``VID``, ``PID`` set the USB VID:PID pair used to search for the development board. They are ``04B4:8613`` if not specified, which is the VID:PID pair of the Cypress development kit.
  * ``MODEL`` sets the sdcc_ code model, one of ``small``, ``medium``, ``large`` or ``huge``.
    The *libfx2* standard library as well as sdcc_ standard library are built for all code models. It is ``small`` if not specified.
  * ``CODE_SIZE``, ``XRAM_SIZE`` set the sizes of the corresponding sdcc_ segments. The ``CODE`` and ``XRAM`` segments must add up to at most ``0x4000``. They are ``0x3e00`` and ``0x0200`` if not specified.
  * ``CFLAGS`` appends arbitrary flags to every sdcc_ invocation.

An elaborate Makefile could look as follows:

.. code-block:: make
  :caption: Makefile

  VID      ?= 20b7
  PID      ?= 9db1

  TARGET    = glasgow
  SOURCES   = main leds fpga dac_ldo adc
  LIBRARIES = fx2 fx2usb fx2isrs
  MODEL     = medium
  CODE_SIZE = 0x3000
  XRAM_SIZE = 0x1000
  CFLAGS    = -DSYNCDELAYLEN=16

  LIBFX2    = ../vendor/libfx2/firmware/library
  include $(LIBFX2)/fx2rules.mk

.. _sdcc: http://sdcc.sourceforge.net
