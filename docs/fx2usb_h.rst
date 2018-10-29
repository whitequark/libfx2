fx2usb.h
========

The ``fx2usb.h`` header contains USB support code for the Cypress FX2 series. When using this header, the ``fx2`` and ``fx2usb`` libraries must be linked in.

Callback resolution
-------------------

This header defines a number of callback functions, which requests the linker to locate a implementation for every of them. If you provide a callback explicitly, this callback will be used; if you do not, the linker will use the default one from the ``fx2usb`` library. Not all callbacks have default implementations.

Reference
---------

.. autodoxygenfile:: fx2usb.h
