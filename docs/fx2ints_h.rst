fx2ints.h
=========

The ``fx2ints.h`` header contains interrupt handler declarations for the Cypress FX2 series.

Interrupt handler resolution
----------------------------

By including the ``fx2ints.h`` header, which declares every interrupt handler, your firmware requests the linker to locate a handler for every interrupt. If you provide a handler explicitly, this handler will be used; if you do not, the linker will go through the list of libraries, looking for the first one that includes a handler. The library ``fx2isrs.lib`` provides a default empty handler for every interrupt (as well as autovectoring jump tables) and should be linked last into every firmware.

Core interrupt handlers
-----------------------

To define a core interrupt handler, override the corresponding ``isr_`` function and provide the interrupt number in the ``__interrupt`` attribute, e.g. for the ``TF0`` interrupt:

.. code-block:: c

   void isr_TF0(void) __interrupt(_INT_TF0) {
     // TIMER0 has overflowed
   }

Interrupts with flags in the ``EXIF`` register need to be reset manually:

.. code-block:: c

   void isr_I2C(void) __interrupt(_INT_I2C) {
     // I2C is done or errored
     CLEAR_I2C_IRQ();
   }

Autovectored interrupt handlers
-------------------------------

To define an autovectored interrupt handler, override the corresponding ``isr_`` function and provide the ``__interrupt`` attribute without a number, e.g. for the ``SOF`` interrupt:

.. code-block:: c

   void isr_SOF(void) __interrupt {
     // Start of Frame packet has been received
     CLEAR_USB_IRQ();
     USBIRQ = _SOF;
   }

.. note::
   The order of clearing of the IRQ flags is important.

Reference
---------

.. autodoxygenfile:: fx2ints.h
