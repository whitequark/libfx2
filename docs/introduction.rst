Introduction
============

*libfx2* is a chip support package for Cypress EZ-USB FX2 series microcontrollers.

On the firmware side, it provides:

  * register definitions,
  * makefile templates,
  * common code for handling standard USB requests,
  * a bootloader implementing Cypress vendor commands.

On the software side, it provides:

  * :mod:`fx2`, a Python library for interacting with the bootloader,
  * :ref:`fx2tool <bootloader-tool>`, a tool for programming and debugging the chips.
