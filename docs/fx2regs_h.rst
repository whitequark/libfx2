fx2regs.h
=========

The ``fx2regs.h`` header contains register definitions for the Cypress FX2 series.

Renamed registers
-----------------

All definitions are (semi-)automatically generated from the datasheet information, and names match the datasheet, except where that would result in a conflict.

The following definitions are changed:

  * in the ``TMOD`` register, bits corresponding to the TIMER0 and TIMER1 peripherals are suffixed with ``_0`` and ``_1``,
  * in the ``PORTECFG`` register, bit ``INT6`` is renamed to ``INT6``,
  * in the ``EPnGPIFPFSTOP`` registers, bit ``FIFOnFLAG`` is renamed to ``FIFOFLAG``,
  * in the ``GPIFTRIG`` and ``GPIFIDLECS`` registers, bit ``GPIFDONE`` is renamed to ``GPIFIDLE``.

The following definitions are absent:

  * all ``EPn`` and ``PKTSn`` bit definitions.

All bit definitions that are a part of a two's-complement number (e.g. ``An`` and ``Dn`` address and data bits) are also absent.

Reference
---------

.. autodoxygenfile:: fx2regs.h
